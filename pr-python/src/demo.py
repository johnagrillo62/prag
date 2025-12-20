# file: ast_reified_demo.py

import ast
from typing import Optional, List, Tuple, get_type_hints, Union, Type, Any

# ----------------------
# Example source code
# ----------------------
source_code = """
from typing import Optional, List, Tuple

class Athlete:
    athlete: int
    team1: Optional[int]
    scores: List[int]
    records: Optional[List[Tuple[int, str]]]
"""

# ----------------------
# Step 1: Parse AST (check syntax without execution)
# ----------------------
parsed_ast = ast.parse(source_code)
print("AST parsed successfully. No syntax errors.")

# Optionally inspect class and field nodes
for node in parsed_ast.body:
    if isinstance(node, ast.ClassDef):
        print(f"Found class: {node.name}")
        for stmt in node.body:
            if isinstance(stmt, ast.AnnAssign) and isinstance(stmt.target, ast.Name):
                print(f"  Field: {stmt.target.id}, annotation: {ast.dump(stmt.annotation)}")

# ----------------------
# Step 2: Define classes to allow get_type_hints evaluation
# ----------------------
class Athlete:
    athlete: int
    team1: Optional[int]
    scores: List[int]
    records: Optional[List[Tuple[int, str]]]

# ----------------------
# Step 3: Extract reified types with get_type_hints
# ----------------------
hints = get_type_hints(Athlete)
print("\nReified types from get_type_hints:")
for name, typ in hints.items():
    print(f"  {name}: {typ}")

# ----------------------
# Step 4: Recursive function to walk nested types
# ----------------------
def describe_type(typ: Type[Any], indent: int = 0):
    from typing import get_origin, get_args
    prefix = "  " * indent
    origin = get_origin(typ)
    args = get_args(typ)

    if origin is Union and type(None) in args:
        non_none = [a for a in args if a is not type(None)][0]
        print(f"{prefix}Optional[")
        describe_type(non_none, indent + 1)
        print(f"{prefix}]")
    elif origin:
        print(f"{prefix}{origin.__name__}[")
        for a in args:
            describe_type(a, indent + 1)
        print(f"{prefix}]")
    else:
        print(f"{prefix}{typ.__name__}")

print("\nNested reified type structure:")
for name, typ in hints.items():
    print(f"{name}:")
    describe_type(typ)


    
