# bhw.py
import sys
import importlib.util
from pathlib import Path

from langs.utils import build_meta
from langs.cpp import generate_cpp_struct
from langs.cpp import cpp_registry

def load_module_from_file(module_name: str, file_path: str):
    """Dynamically load a Python module from a file path."""
    spec = importlib.util.spec_from_file_location(module_name, file_path)
    if spec is None:
        raise ImportError(f"Cannot load module from {file_path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)  # type: ignore
    return module

def main():
    if len(sys.argv) < 2:
        print("Usage: python bhw.py <path_to_classes_file.py>")
        sys.exit(1)

    classes_file = Path(sys.argv[1])
    if not classes_file.exists():
        print(f"File not found: {classes_file}")
        sys.exit(1)

    module_name = classes_file.stem
    module = load_module_from_file(module_name, str(classes_file))

    # Iterate over all attributes in the module
    for name in dir(module):
        cls = getattr(module, name)
        # Only process dataclasses with _table_name attribute
        if hasattr(cls, "_table_name"):
            meta = build_meta(cls)
            cpp_code = generate_cpp_struct(meta, ns=["my", "ns"])
            file_name = cpp_registry["naming"]["file_name"](cls._class_name)  + ".h"
            with open(file_name, "w") as f:
                f.write(cpp_code)
            print(f"Generated C++ struct for {cls.__name__} -> {file_name}")

if __name__ == "__main__":
    main()
