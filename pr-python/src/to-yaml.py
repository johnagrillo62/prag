import ast
import inspect
from typing import get_type_hints, Optional, List, Tuple
import yaml
from dataclasses import dataclass, field

# ----------------------
# Example class
# ----------------------
@dataclass
class Athlete:
    athlete: int = 0
    team1: Optional[int] = None
    scores: List[int] = field(default_factory=list)
    records: Optional[List[Tuple[int, str]]] = None

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
# Helper to convert dataclass to dict
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
# 3. Generate type-safe YAML from class instance
# ----------------------
athlete_instance = Athlete(athlete=123, team1=5, scores=[10, 20], records=[(1, "A")])
yaml_str = yaml.safe_dump(dataclass_to_dict(athlete_instance), sort_keys=False)
print("\nYAML output:")
print(yaml_str)
