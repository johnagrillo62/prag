import ast
from typing import get_type_hints, Optional, List, Tuple, Union
from dataclasses import dataclass, field
import yaml
import sys
import pathlib

# ----------------------
# Convert AST annotation to Python type
# ----------------------
def annotation_to_type(node):
    if isinstance(node, ast.Name):
        return eval(node.id)
    elif isinstance(node, ast.Subscript):
        value_id = node.value.id if isinstance(node.value, ast.Name) else None
        if value_id == "Optional":
            return Optional[annotation_to_type(node.slice)]
        elif value_id == "List":
            return List[annotation_to_type(node.slice)]
        elif value_id == "Tuple":
            if isinstance(node.slice, ast.Tuple):
                return Tuple[tuple(annotation_to_type(e) for e in node.slice.elts)]
            else:
                return Tuple[annotation_to_type(node.slice)]
        else:
            return object
    elif isinstance(node, ast.Constant):
        return type(node.value)
    elif hasattr(ast, "Index") and isinstance(node, ast.Index):
        return annotation_to_type(node.value)
    return object

# ----------------------
# Helper to convert dataclass-like instance dict to YAML
# ----------------------
def dataclass_to_dict(obj):
    if isinstance(obj, list):
        return [dataclass_to_dict(x) for x in obj]
    if isinstance(obj, tuple):
        return tuple(dataclass_to_dict(x) for x in obj)
    if isinstance(obj, dict):
        return {k: dataclass_to_dict(v) for k, v in obj.items()}
    return obj

# ----------------------
# Recursive type checker
# ----------------------
def check_type(value, expected_type):
    origin = getattr(expected_type, "__origin__", None)
    args = getattr(expected_type, "__args__", None)
    if origin is None:
        return isinstance(value, expected_type)
    elif origin in (list, List):
        if not isinstance(value, list):
            return False
        return all(check_type(v, args[0]) for v in value)
    elif origin in (tuple, Tuple):
        if not isinstance(value, tuple):
            return False
        return all(check_type(v, t) for v, t in zip(value, args))
    elif origin is Union:  # Optional[T] becomes Union[T, NoneType]
        return any(check_type(value, a) for a in args)
    return True

def type_check_instance(instance_dict, class_types):
    errors = []
    for field_name, field_type in class_types.items():
        value = instance_dict.get(field_name, None)
        if value is not None and not check_type(value, field_type):
            errors.append(f"Type error in field '{field_name}': expected {field_type}, got {type(value)}")
    return errors

# ----------------------
# Main
# ----------------------
if len(sys.argv) < 2:
    print("Usage: python py-ast-typecheck.py <input_file.py>")
    sys.exit(1)

input_file = pathlib.Path(sys.argv[1]).resolve()
with open(input_file, "r") as f:
    source = f.read()

# 1. Parse the file
tree = ast.parse(source)
print("AST parsed successfully. No syntax errors.\n")

# 2. Extract classes and fields
classes = {}
for node in tree.body:
    if isinstance(node, ast.ClassDef):
        field_types = {}
        for stmt in node.body:
            if isinstance(stmt, ast.AnnAssign):
                field_name = stmt.target.id
                field_types[field_name] = annotation_to_type(stmt.annotation)
        classes[node.name] = field_types
        print(f"Found class: {node.name}")
        for fname, ftype in field_types.items():
            print(f"  Field: {fname}, type: {ftype}")

# 3. Extract instances (simple top-level assigns)
instances = {}
for node in tree.body:
    if isinstance(node, ast.Assign):
        for target in node.targets:
            if isinstance(target, ast.Name):
                instances[target.id] = ast.literal_eval(node.value)

# 4. Type check instances
print("\nType errors found:")
for name, instance_dict in instances.items():
    class_name = type(instance_dict).__name__  # crude, assume instance dict mimics dataclass
    # pick first class to type-check
    class_types = list(classes.values())[0]
    errors = type_check_instance(instance_dict, class_types)
    if errors:
        print(f"Instance '{name}':")
        for e in errors:
            print(f"  {e}")

# 5. Convert to YAML
print("\nYAML output:")
for name, instance_dict in instances.items():
    yaml_str = yaml.safe_dump(dataclass_to_dict(instance_dict), sort_keys=False)
    print(f"# Instance: {name}")
    print(yaml_str)


    


from dataclasses import dataclass, field
from typing import Optional, List, Tuple

@dataclass
class Record:
    event: int = 0
    mark: str = ""

@dataclass
class Athlete:
    athlete: int = 0
    team1: Optional[int] = None
    scores: List[int] = field(default_factory=list)
    records: Optional[List[Tuple[int, str]]] = None
    nested: Optional[Record] = None

    

# Instances
r = Record(event=42, mark="100")  # mark should be str -> type error
a = Athlete(athlete=123, team1=5, scores=[10, 20], records=[(1, "A")], nested=r)

