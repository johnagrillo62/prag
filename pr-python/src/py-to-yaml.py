import ast
from typing import get_type_hints, Optional, List, Tuple
from dataclasses import dataclass, field
import yaml
import sys
import importlib.util
import pathlib

# ----------------------
# Helper to convert dataclass instance to dict
# ----------------------
def dataclass_to_dict(obj):
    if isinstance(obj, list):
        return [dataclass_to_dict(x) for x in obj]
    if isinstance(obj, tuple):
        return tuple(dataclass_to_dict(x) for x in obj)
    if hasattr(obj, "__dataclass_fields__"):
        return {k: dataclass_to_dict(getattr(obj, k)) for k in obj.__dataclass_fields__}
    return obj

# ----------------------
# Helper to type check an instance against type hints
# ----------------------
def type_check_instance(obj):
    errors = []
    hints = get_type_hints(obj.__class__)
    for name, expected_type in hints.items():
        value = getattr(obj, name, None)
        if value is not None:
            try:
                if not check_type(value, expected_type):
                    errors.append(f"Type error on field '{name}': expected {expected_type}, got {type(value)}")
            except TypeError:
                # for complex generics, we skip strict isinstance
                pass
    return errors

def check_type(value, expected_type):
    """Simple recursive check for nested generics"""
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
    elif origin is Optional:
        non_none_type = args[0]
        return value is None or check_type(value, non_none_type)
    return True

# ----------------------
# Main entry point
# ----------------------
if len(sys.argv) < 2:
    print("Usage: python3 py-to-yaml.py <input_file.py>")
    sys.exit(1)

input_file = sys.argv[1]
input_path = pathlib.Path(input_file).resolve()

# ----------------------
# 1. Parse the Python file using AST
# ----------------------
with open(input_path, "r") as f:
    source = f.read()

tree = ast.parse(source)
print("AST parsed successfully. No syntax errors.\n")

# ----------------------
# 2. Inspect classes and fields
# ----------------------
for node in tree.body:
    if isinstance(node, ast.ClassDef):
        print(f"Found class: {node.name}")
        for stmt in node.body:
            if isinstance(stmt, ast.AnnAssign):
                field_name = stmt.target.id
                annotation = stmt.annotation
                print(f"  Field: {field_name}, annotation: {ast.dump(annotation)}")

# ----------------------
# 3. Dynamically import the module
# ----------------------
module_name = input_path.stem
spec = importlib.util.spec_from_file_location(module_name, str(input_path))
module = importlib.util.module_from_spec(spec)
sys.modules[module_name] = module
spec.loader.exec_module(module)

# ----------------------
# 4. Create an instance and type check
# ----------------------
# Example: assume first class is Athlete
cls = getattr(module, "Athlete")
instance = cls(athlete="", team1=5, scores=[10, 20], records=[(1, "A"), (2, "B")])

errors = type_check_instance(instance)
if errors:
    print("\nType errors found:")
    for e in errors:
        print(f"  {e}")
else:
    print("\nNo type errors.")

# ----------------------
# 5. Generate YAML
# ----------------------
yaml_str = yaml.safe_dump(dataclass_to_dict(instance), sort_keys=False)
print("\nYAML output:")
print(yaml_str)

