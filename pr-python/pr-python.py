#!/usr/bin/env python3
"""
Python to Prag JSON AST Parser
"""

import ast
import sys
import json
import re
from typing import Any, Dict, List

def parse_generic_args(args_str: str) -> List[str]:
    """Parse generic arguments, handling nested brackets."""
    args = []
    current = ""
    depth = 0
    
    for char in args_str:
        if char == '[':
            depth += 1
            current += char
        elif char == ']':
            depth -= 1
            current += char
        elif char == ',' and depth == 0:
            if current.strip():
                args.append(current.strip())
            current = ""
        else:
            current += char
    
    if current.strip():
        args.append(current.strip())
    
    return args


def python_type_to_reified(type_str: str) -> Dict[str, Any]:
    """Convert Python type string to reified type object."""
    type_str = type_str.strip()
    
    # Handle generic types
    match = re.match(r'(\w+)\[(.*)\]', type_str)
    if match:
        container = match.group(1)
        args_str = match.group(2)
        
        # Parse arguments - handle nested brackets
        args = parse_generic_args(args_str)
        args_types = [python_type_to_reified(arg.strip()) for arg in args]
        
        # Map container names to reified types
        container_map = {
            "Optional": "Option",
            "List": "Vec",
            "Dict": "Map",
            "Set": "Set",
            "Tuple": "Tuple"
        }
        
        mapped_name = container_map.get(container, container)
        
        return {
            "kind": "generic",
            "name": mapped_name,
            "args": args_types
        }
    
    # Map primitive types to reified types
    type_map = {
        "int": "i64",
        "float": "f64",
        "str": "String",
        "bool": "bool",
    }
    
    reified_name = type_map.get(type_str, type_str)
    
    return {
        "kind": "primitive",
        "name": reified_name
    }


def annotation_to_type_string(annotation: ast.expr) -> str:
    """Convert AST annotation to type string."""
    if isinstance(annotation, ast.Name):
        return annotation.id
    
    if isinstance(annotation, ast.Subscript):
        base = annotation.value
        if isinstance(base, ast.Name):
            base_name = base.id
        else:
            base_name = "Generic"
        
        if isinstance(annotation.slice, ast.Tuple):
            args = [annotation_to_type_string(arg) for arg in annotation.slice.elts]
        else:
            args = [annotation_to_type_string(annotation.slice)]
        
        if len(args) == 1:
            return f"{base_name}[{args[0]}]"
        else:
            return f"{base_name}[{', '.join(args)}]"
    
    if isinstance(annotation, ast.Constant):
        return str(annotation.value)
    
    return "Any"


def build_struct(class_def: ast.ClassDef) -> Dict[str, Any]:
    """Build a Prag struct from a class definition."""
    fields = []
    
    for node in class_def.body:
        if isinstance(node, ast.AnnAssign) and isinstance(node.target, ast.Name):
            field_name = node.target.id
            src_type_string = annotation_to_type_string(node.annotation)
            reified_type = python_type_to_reified(src_type_string)
            
            # For Python, don't include srcTypeString - let C++ walker use reifiedType mapping
            fields.append({
                "name": field_name,
                "type": reified_type
            })
    
    return {
        "type": "Struct",
        "name": class_def.name,
        "fields": fields
    }


def parse_python_to_pragjon(source_code: str) -> Dict[str, Any]:
    """Parse Python source to Prag AST JSON."""
    try:
        tree = ast.parse(source_code)
    except SyntaxError as e:
        print(f"Syntax error: {e}", file=sys.stderr)
        sys.exit(1)
    
    items = []
    for node in tree.body:
        if isinstance(node, ast.ClassDef):
            items.append(build_struct(node))
    
    return {
        "type": "Module",
        "srcLanguage": "python",
        "items": items
    }


def format_type(type_obj: Any) -> str:
    """Format a type object to string."""
    if isinstance(type_obj, str):
        return type_obj
    
    if not isinstance(type_obj, dict):
        return str(type_obj)
    
    kind = type_obj.get("kind", "unknown")
    name = type_obj.get("name", "unknown")
    args = type_obj.get("args", [])
    
    if kind == "primitive":
        return name
    elif kind == "generic":
        if args:
            arg_strs = [format_type(arg) for arg in args]
            return f"{name}[{', '.join(arg_strs)}]"
        return name
    else:
        return "unknown"


def display_pragjon_ast(data: Dict[str, Any]) -> str:
    """Display Prag AST in text format."""
    lines = []
    
    if data.get("type") == "Module":
        for item in data.get("items", []):
            if item.get("type") == "Struct":
                lines.append(f"Struct: {item.get('name', '')}")
                lines.append(f"  Namespace:")
                lines.append(f"  Attributes:")
                lines.append(f"  Members:")
                
                for field in item.get("fields", []):
                    type_str = format_type(field.get("type", {}))
                    lines.append(f"    Field name: {field.get('name', '')}")
                    lines.append(f"          type: {type_str}")
    
    return "\n".join(lines)


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage: python_prag_parser.py INPUT_FILE [OPTIONS]", file=sys.stderr)
        sys.exit(1)
    
    if sys.argv[1] in ("--help", "-h"):
        print("Python to Prag AST Parser")
        sys.exit(0)
    
    input_file = sys.argv[1]
    output_file = None
    output_format = "json"
    
    i = 2
    while i < len(sys.argv):
        if sys.argv[i] in ("-o", "--output"):
            output_file = sys.argv[i + 1]
            i += 2
        elif sys.argv[i] == "--out-ast":
            output_format = "ast"
            i += 1
        else:
            i += 1
    
    try:
        with open(input_file, 'r') as f:
            source_code = f.read()
    except FileNotFoundError:
        print(f"Error: File '{input_file}' not found", file=sys.stderr)
        sys.exit(1)
    
    pragjon = parse_python_to_pragjon(source_code)
    
    if output_format == "ast":
        output = display_pragjon_ast(pragjon)
    else:
        output = json.dumps(pragjon, indent=2)
    
    if output_file:
        with open(output_file, 'w') as f:
            f.write(output + "\n")
    else:
        print(output)


if __name__ == "__main__":
    main()


