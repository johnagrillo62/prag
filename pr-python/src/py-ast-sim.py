import ast
from typing import Optional, List, Tuple, ForwardRef, Union, get_args, get_origin
import yaml
import sys

# ----------------------
# Resolve AST annotation nodes to Python types or ForwardRefs
# ----------------------
def annotation_to_type(node, defined_classes=None):
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
        elif value_id == 'Tuple':
            if isinstance(slice_node, ast.Tuple):
                return Tuple[tuple(annotation_to_type(e, defined_classes) for e in slice_node.elts)]
            else:
                return Tuple[annotation_to_type(slice_node, defined_classes)]
    elif isinstance(node, ast.Constant):
        return type(node.value)
    return str(node)

# ----------------------
# Type checking functions
# ----------------------
def check_type(value, expected_type, defined_classes=None):
    """Recursively check if value matches expected type"""
    defined_classes = defined_classes or {}
    origin = get_origin(expected_type)
    args = get_args(expected_type)
    
    # Handle Optional[T] (Union[T, None])
    if origin is Union:
        return any(check_type(value, arg, defined_classes) for arg in args)
    
    # Handle List[T]
    if origin is list:
        if not isinstance(value, list):
            return False
        if args:
            return all(check_type(item, args[0], defined_classes) for item in value)
        return True
    
    # Handle Tuple[T1, T2, ...]
    if origin is tuple:
        if not isinstance(value, tuple):
            return False
        if args:
            if len(value) != len(args):
                return False
            return all(check_type(v, t, defined_classes) for v, t in zip(value, args))
        return True
    
    # Handle ForwardRef (custom class instances)
    if isinstance(expected_type, ForwardRef):
        ref_name = expected_type.__forward_arg__
        # Check if it's a dict representing the class
        if isinstance(value, dict):
            ref_fields = defined_classes.get(ref_name, {})
            return check_instance(value, ref_fields, defined_classes)
        return False
    
    # Handle basic types
    if expected_type is type(None):
        return value is None
    
    return isinstance(value, expected_type)

def check_instance(instance_dict, field_types, defined_classes):
    """Check if instance dict matches class field types"""
    errors = []
    for field_name, expected_type in field_types.items():
        if field_name in instance_dict:
            value = instance_dict[field_name]
            if not check_type(value, expected_type, defined_classes):
                errors.append(f"  Field '{field_name}': expected {expected_type}, got {type(value).__name__} = {value!r}")
    return errors

def extract_instance_values(node):
    """Extract values from ast.Call node (like Record(...))"""
    if isinstance(node, ast.Call):
        result = {}
        # Handle keyword arguments
        for keyword in node.keywords:
            arg_name = keyword.arg
            try:
                arg_value = ast.literal_eval(keyword.value)
            except:
                arg_value = None
            result[arg_name] = arg_value
        return result
    return None

# ----------------------
# Recursively generate a YAML skeleton from type hints
# ----------------------
def type_to_yaml(typ, defined_classes=None):
    defined_classes = defined_classes or {}
    origin = get_origin(typ)
    args = get_args(typ)

    # Handle Optional[T] - unwrap to T
    if origin is Union:
        non_none_args = [a for a in args if a is not type(None)]
        if non_none_args:
            return type_to_yaml(non_none_args[0], defined_classes)
        return None
    
    # Handle List[T]
    if origin is list:
        if args:
            return [type_to_yaml(args[0], defined_classes)]
        return []
    
    # Handle Tuple[T1, T2, ...]
    if origin is tuple:
        if args:
            return tuple(type_to_yaml(a, defined_classes) for a in args)
        return ()
    
    # Handle ForwardRef (class references)
    if isinstance(typ, ForwardRef):
        ref_name = typ.__forward_arg__
        ref_fields = defined_classes.get(ref_name)
        if ref_fields:
            return {k: type_to_yaml(v, defined_classes) for k, v in ref_fields.items()}
        return {}
    
    # Handle primitive types
    if typ in (int, float):
        return 0
    elif typ is str:
        return ""
    elif typ is bool:
        return False
    elif isinstance(typ, type):
        return {}
    
    return None

# ----------------------
# Parse classes and instances from AST
# ----------------------
def parse_classes_and_instances(file_path):
    defined_classes = {}
    with open(file_path) as f:
        source = f.read()
    tree = ast.parse(source)
    print("AST parsed successfully. No syntax errors.\n")

    classes = {}
    instances = {}

    # First pass: register class names
    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            defined_classes[node.name] = {}

    # Second pass: find classes and their fields
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

    # Third pass: find instance assignments
    for node in tree.body:
        if isinstance(node, ast.Assign):
            if len(node.targets) == 1:
                target = node.targets[0]
                if isinstance(target, ast.Name):
                    instance_name = target.id
                    instance_values = extract_instance_values(node.value)
                    if instance_values is not None:
                        instances[instance_name] = (node.value, instance_values)

    return classes, instances

# ----------------------
# Main
# ----------------------
if len(sys.argv) < 2:
    print("Usage: python3 py-ast-static.py <input_file.py>")
    sys.exit(1)

file_path = sys.argv[1]
classes, instances = parse_classes_and_instances(file_path)

# ----------------------
# Type check instances
# ----------------------
print("\n" + "="*60)
print("TYPE CHECKING INSTANCES")
print("="*60)

for instance_name, (call_node, instance_dict) in instances.items():
    # Determine which class this is an instance of
    if isinstance(call_node, ast.Call) and isinstance(call_node.func, ast.Name):
        class_name = call_node.func.id
        if class_name in classes:
            print(f"\nChecking instance '{instance_name}' of class '{class_name}':")
            field_types = classes[class_name]
            errors = check_instance(instance_dict, field_types, classes)
            if errors:
                print("  ❌ TYPE ERRORS:")
                for error in errors:
                    print(error)
            else:
                print("  ✅ All types correct")

# ----------------------
# Generate YAML skeletons recursively
# ----------------------
print("\n" + "="*60)
print("YAML SKELETONS")
print("="*60)

for cls_name, fields in classes.items():
    skeleton = {k: type_to_yaml(v, classes) for k, v in fields.items()}
    print(f"\nYAML skeleton for {cls_name}:")
    print(yaml.safe_dump(skeleton, sort_keys=False, default_flow_style=False, indent=4))
    

