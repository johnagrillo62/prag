import ast
from typing import get_type_hints, Optional, List, Tuple, Union, _GenericAlias
from dataclasses import dataclass, field
import sys
import pathlib

# ----------------------
# Convert AST annotation to actual Python type
# ----------------------
def annotation_to_type(node):
    if isinstance(node, ast.Name):
        # basic type like int, str
        return eval(node.id)
    elif isinstance(node, ast.Subscript):
        base = annotation_to_type(node.value)
        if isinstance(base, type):
            # e.g., Optional[int] -> typing.Optional[int]
            if getattr(base, "__origin__", None) is Union:
                return base
            # support Optional[X]
            if base is Optional:
                return Optional[annotation_to_type(node.slice)]
            if base in (list, List):
                return List[annotation_to_type(node.slice)]
            if base in (tuple, Tuple):
                if isinstance(node.slice, ast.Tuple):
                    return Tuple[tuple(annotation_to_type(elt) for elt in node.slice.elts)]
                else:
                    return Tuple[annotation_to_type(node.slice)]
        return base
    elif isinstance(node, ast.Attribute):
        # for things like typing.List
        return eval(f"{node.value.id}.{node.attr}")
    elif isinstance(node, ast.Tuple):
        return tuple(annotation_to_type(elt) for elt in node.elts)
    else:
        return object  # fallback

# ----------------------
# AST parse a Python file
# ----------------------
def parse_classes(file_path):
    file_path = pathlib.Path(file_path)
    with open(file_path, "r") as f:
        source = f.read()
    tree = ast.parse(source)

    classes = {}
    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            class_name = node.name
            fields = {}
            for stmt in node.body:
                if isinstance(stmt, ast.AnnAssign):
                    field_name = stmt.target.id
                    field_type = annotation_to_type(stmt.annotation)
                    fields[field_name] = field_type
            classes[class_name] = fields
    return classes

# ----------------------
# Example: type-check an instance
# ----------------------
def type_check_instance(instance, field_types):
    errors = []
    for field_name, expected_type in field_types.items():
        value = getattr(instance, field_name, None)
        if value is not None and not check_type(value, expected_type):
            errors.append(f"Type error in field '{field_name}': expected {expected_type}, got {type(value)}")
    return errors

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
    elif origin is Union:
        return any(check_type(value, arg) for arg in args)
    return True

# ----------------------
# Main
# ----------------------
if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 py-ast.py <python_file>")
        sys.exit(1)

    classes = parse_classes(sys.argv[1])
    print("AST parsed successfully. No syntax errors.\n")

    for cls_name, fields in classes.items():
        print(f"Found class: {cls_name}")
        for fname, ftype in fields.items():
            print(f"  Field: {fname}, type: {ftype}")

    # ----------------------
    # Example instance type checking
    # ----------------------
    from input import Athlete, Record  # replace with your real file
    a = Athlete(athlete="", team1=5, scores=[10, 20], records=[(1, "A")])
    errors = type_check_instance(a, classes["Athlete"])
    if errors:
        print("\nType errors found:")
        for e in errors:
            print(f"  {e}")
    else:
        print("\nNo type errors.")
