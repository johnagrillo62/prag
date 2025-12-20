import ast
import inspect
from dataclasses import dataclass, field, fields, is_dataclass
from typing import Optional, List, Tuple, Union, get_type_hints, get_origin, get_args
import yaml

# ----------------------
# Example nested dataclasses
# ----------------------
@dataclass
class Record:
    event: int
    mark: str

@dataclass
class Athlete:
    athlete: int = 0
    team1: Optional[int] = None
    scores: List[int] = field(default_factory=list)
    records: Optional[List[Tuple[int, str]]] = None
    nested: Optional[Record] = None  # nested dataclass

# ----------------------
# 1. AST parsing to inspect class
# ----------------------
source = inspect.getsource(Athlete)
tree = ast.parse(source)

for node in tree.body:
    if isinstance(node, ast.ClassDef):
        print(f"Found class: {node.name}")
        for stmt in node.body:
            if isinstance(stmt, ast.AnnAssign):
                field_name = stmt.target.id
                annotation = stmt.annotation
                print(f"  Field: {field_name}, annotation: {ast.dump(annotation)}")

# ----------------------
# 2. Reified types with get_type_hints
# ----------------------
hints = get_type_hints(Athlete)
print("\nReified types from get_type_hints:")
for name, typ in hints.items():
    print(f"  {name}: {typ}")

# ----------------------
# 3. Recursive type checker
# ----------------------

# ----------------------
# 3. Recursive type checker (fixed)
# ----------------------
def check_type(value, expected_type):
    origin = get_origin(expected_type)
    args = get_args(expected_type)
    
    # Optional[T]
    if origin is Union and type(None) in args:
        non_none_type = [a for a in args if a is not type(None)][0]
        if value is None:
            return True
        return check_type(value, non_none_type)
    
    # List[T]
    if origin in (list, List):
        if not isinstance(value, list):
            return False
        elem_type = args[0]
        return all(check_type(v, elem_type) for v in value)
    
    # Tuple[T1, T2, ...]
    if origin in (tuple, Tuple):
        if not isinstance(value, tuple) or len(value) != len(args):
            return False
        return all(check_type(v, t) for v, t in zip(value, args))
    
    # Nested dataclass
    if is_dataclass(expected_type):
        if not is_dataclass(value):
            return False
        nested_errors = type_check_instance(value)  # <- fixed
        return len(nested_errors) == 0
    
    # Regular type
    return isinstance(value, expected_type)

def type_check_instance(instance):
    cls = type(instance)
    hints = get_type_hints(cls)
    errors = []
    
    for f in fields(instance):
        name = f.name
        value = getattr(instance, name)
        expected_type = hints[name]
        if not check_type(value, expected_type):
            errors.append(f"Type error on field '{name}': expected {expected_type}, got {type(value)}")
    
    return errors

# ----------------------
# 4. Helper to convert dataclass to dict
# ----------------------
def dataclass_to_dict(obj):
    if isinstance(obj, list):
        return [dataclass_to_dict(x) for x in obj]
    if isinstance(obj, tuple):
        return tuple(dataclass_to_dict(x) for x in obj)
    if is_dataclass(obj):
        return {k: dataclass_to_dict(getattr(obj, k)) for k in obj.__dataclass_fields__}
    return obj

# ----------------------
# 5. Demo
# ----------------------
athlete_instance = Athlete(
    athlete="",
    team1=5,
    scores=[10, 20],
    records=[(1, "A"), (2, "B")],
    nested=Record(event=42, mark="X")
)

# Type check
errors = type_check_instance(athlete_instance)
if errors:
    print("\nType errors found:")
    for e in errors:
        print(" ", e)
else:
    print("\nAll types valid!")

# YAML output
yaml_str = yaml.safe_dump(dataclass_to_dict(athlete_instance), sort_keys=False)
print("\nYAML output:")
print(yaml_str)


