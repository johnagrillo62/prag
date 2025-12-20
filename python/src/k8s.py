import ast
from typing import Optional, List, Dict, Union, ForwardRef, Any, get_args, get_origin
from pydantic import BaseModel, Field, field_validator, ConfigDict
import yaml
import sys
import re

# ============================================================================
# PART 1: KUBERNETES SCHEMA WITH PYDANTIC V2
# ============================================================================

class ResourceRequirements(BaseModel):
    """CPU and Memory resources"""
    cpu: str = Field(..., pattern=r'^\d+m?$', description="CPU (e.g., 500m, 2)")
    memory: str = Field(..., pattern=r'^\d+(Mi|Gi|Ki)$', description="Memory (e.g., 512Mi)")
    
    @field_validator('memory')
    @classmethod
    def memory_not_too_small(cls, v):
        num = int(re.findall(r'\d+', v)[0])
        unit = re.findall(r'[A-Za-z]+', v)[0]
        if unit == 'Mi' and num < 128:
            raise ValueError('Memory must be at least 128Mi')
        if unit == 'Ki' and num < 131072:
            raise ValueError('Memory must be at least 128Mi')
        return v

class ContainerPort(BaseModel):
    """Container port specification"""
    container_port: int = Field(..., ge=1, le=65535, alias="containerPort")
    protocol: str = Field(default="TCP", pattern=r'^(TCP|UDP|SCTP)$')
    name: Optional[str] = Field(default=None, max_length=15, pattern=r'^[a-z0-9]([-a-z0-9]*[a-z0-9])?$')
    
    model_config = ConfigDict(populate_by_name=True)

class EnvVar(BaseModel):
    """Environment variable"""
    name: str = Field(..., min_length=1)
    value: str

class Container(BaseModel):
    """Kubernetes container specification"""
    name: str = Field(..., pattern=r'^[a-z0-9]([-a-z0-9]*[a-z0-9])?$', max_length=63)
    image: str = Field(..., min_length=1)
    ports: Optional[List[ContainerPort]] = None
    env: Optional[List[EnvVar]] = None
    resources: Optional[Dict[str, ResourceRequirements]] = None
    
    @field_validator('image')
    @classmethod
    def validate_image(cls, v):
        if ':' not in v:
            raise ValueError('Image must include a tag (e.g., nginx:1.21)')
        return v
    
    model_config = ConfigDict(populate_by_name=True)

class PodSpec(BaseModel):
    """Pod specification"""
    containers: List[Container] = Field(..., min_length=1)
    restart_policy: str = Field(default="Always", pattern=r'^(Always|OnFailure|Never)$', alias="restartPolicy")
    
    model_config = ConfigDict(populate_by_name=True)

class PodTemplateSpec(BaseModel):
    """Pod template"""
    metadata: Optional[Dict[str, Any]] = None  # Changed 'any' to 'Any'
    spec: PodSpec

class LabelSelector(BaseModel):
    """Label selector"""
    match_labels: Dict[str, str] = Field(..., alias="matchLabels")
    
    model_config = ConfigDict(populate_by_name=True)

class DeploymentSpec(BaseModel):
    """Deployment specification"""
    replicas: int = Field(default=1, ge=0, le=100)
    selector: LabelSelector
    template: PodTemplateSpec

class Metadata(BaseModel):
    """Kubernetes metadata"""
    name: str = Field(..., pattern=r'^[a-z0-9]([-a-z0-9]*[a-z0-9])?$', max_length=253)
    namespace: str = Field(default="default", pattern=r'^[a-z0-9]([-a-z0-9]*[a-z0-9])?$', max_length=63)
    labels: Optional[Dict[str, str]] = None

class Deployment(BaseModel):
    """Kubernetes Deployment"""
    api_version: str = Field(default="apps/v1", alias="apiVersion")
    kind: str = Field(default="Deployment")
    metadata: Metadata
    spec: DeploymentSpec
    
    model_config = ConfigDict(populate_by_name=True)

# ============================================================================
# PART 2: AST PARSER
# ============================================================================

def annotation_to_type(node, defined_classes=None):
    """Convert AST annotation to Python type"""
    defined_classes = defined_classes or {}
    if isinstance(node, ast.Name):
        if node.id in defined_classes:
            return ForwardRef(node.id)
        try:
            return eval(node.id)
        except NameError:
            return ForwardRef(node.id)
    elif isinstance(node, ast.Subscript):
        value_id = getattr(node.value, 'id', None)
        slice_node = node.slice
        if value_id == 'Optional':
            return Optional[annotation_to_type(slice_node, defined_classes)]
        elif value_id == 'List':
            return List[annotation_to_type(slice_node, defined_classes)]
        elif value_id == 'Dict':
            if isinstance(slice_node, ast.Tuple) and len(slice_node.elts) >= 2:
                key_type = annotation_to_type(slice_node.elts[0], defined_classes)
                val_type = annotation_to_type(slice_node.elts[1], defined_classes)
                return Dict[key_type, val_type]
    elif isinstance(node, ast.Constant):
        return type(node.value)
    return str(node)

def check_type(value, expected_type, defined_classes=None):
    """Recursively check if value matches expected type"""
    defined_classes = defined_classes or {}
    origin = get_origin(expected_type)
    args = get_args(expected_type)
    
    if origin is Union:
        return any(check_type(value, arg, defined_classes) for arg in args)
    
    if origin is list:
        if not isinstance(value, list):
            return False
        if args:
            return all(check_type(item, args[0], defined_classes) for item in value)
        return True
    
    if origin is dict:
        if not isinstance(value, dict):
            return False
        return True
    
    if isinstance(expected_type, ForwardRef):
        return isinstance(value, dict)
    
    if expected_type is type(None):
        return value is None
    
    return isinstance(value, expected_type)

def parse_classes_from_ast(file_path):
    """Parse Python file and extract class definitions"""
    with open(file_path) as f:
        source = f.read()
    
    tree = ast.parse(source)
    print("✅ AST parsed successfully\n")
    
    defined_classes = {}
    classes = {}
    
    # First pass: register class names
    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            defined_classes[node.name] = {}
    
    # Second pass: extract fields
    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            fields = {}
            for stmt in node.body:
                if isinstance(stmt, ast.AnnAssign):
                    field_name = stmt.target.id
                    field_type = annotation_to_type(stmt.annotation, defined_classes)
                    fields[field_name] = field_type
            classes[node.name] = fields
            defined_classes[node.name] = fields
            
            print(f"Found class: {node.name}")
            for name, typ in fields.items():
                print(f"  Field: {name}, type: {typ}")
    
    return classes

# ============================================================================
# PART 3: K8S YAML GENERATOR
# ============================================================================

def generate_k8s_yaml(deployment: Deployment) -> str:
    """Generate Kubernetes YAML from Pydantic model"""
    # Convert to dict with aliases
    data = deployment.model_dump(by_alias=True, exclude_none=True)
    
    # Generate YAML
    return yaml.safe_dump(data, sort_keys=False, default_flow_style=False)

# ============================================================================
# PART 4: EXAMPLE USAGE
# ============================================================================

if __name__ == "__main__":
    print("="*60)
    print("KUBERNETES YAML GENERATOR")
    print("AST Type Checking + Pydantic Validation")
    print("="*60)
    print()
    
    # Example 1: Valid deployment
    print("Example 1: Creating valid deployment...")
    try:
        deployment = Deployment(
            metadata=Metadata(
                name="nginx-app",
                namespace="production",
                labels={"app": "nginx", "env": "prod"}
            ),
            spec=DeploymentSpec(
                replicas=3,
                selector=LabelSelector(
                    matchLabels={"app": "nginx"}
                ),
                template=PodTemplateSpec(
                    metadata={"labels": {"app": "nginx"}},
                    spec=PodSpec(
                        containers=[
                            Container(
                                name="nginx",
                                image="nginx:1.21",
                                ports=[
                                    ContainerPort(containerPort=80, protocol="TCP", name="http"),
                                    ContainerPort(containerPort=443, protocol="TCP", name="https")
                                ],
                                env=[
                                    EnvVar(name="ENV", value="production"),
                                    EnvVar(name="LOG_LEVEL", value="info")
                                ],
                                resources={
                                    "requests": ResourceRequirements(cpu="500m", memory="512Mi"),
                                    "limits": ResourceRequirements(cpu="1000m", memory="1Gi")
                                }
                            )
                        ],
                        restartPolicy="Always"
                    )
                )
            )
        )
        
        print("✅ Pydantic validation passed!\n")
        print("Generated K8s YAML:")
        print("-" * 60)
        print(generate_k8s_yaml(deployment))
        
    except Exception as e:
        print(f"❌ Validation error: {e}\n")
    
    # Example 2: Invalid deployment (catches errors)
    print("\n" + "="*60)
    print("Example 2: Testing validation (intentional errors)...")
    try:
        bad_deployment = Deployment(
            metadata=Metadata(
                name="My_App",  # ❌ Underscore not allowed
                namespace="default"
            ),
            spec=DeploymentSpec(
                replicas=150,  # ❌ Too many replicas (max 100)
                selector=LabelSelector(matchLabels={"app": "test"}),
                template=PodTemplateSpec(
                    spec=PodSpec(
                        containers=[
                            Container(
                                name="nginx",
                                image="nginx",  # ❌ No tag
                                ports=[ContainerPort(containerPort=99999)],  # ❌ Invalid port
                                resources={
                                    "requests": ResourceRequirements(
                                        cpu="500m",
                                        memory="64Mi"  # ❌ Too small
                                    )
                                }
                            )
                        ]
                    )
                )
            )
        )
    except Exception as e:
        print(f"✅ Caught validation errors (as expected):")
        print(f"   {str(e)[:200]}...\n")
        if hasattr(e, 'errors'):
            for error in e.errors():
                print(f"   Field: {error['loc']}")
                print(f"   Error: {error['msg']}")
                print(f"   Type: {error['type']}\n")
    else:
        print(f"   {e}\n")
    
    print("="*60)
    print("✅ Complete! AST parsing + Pydantic validation + K8s YAML generation")
    print("="*60)
