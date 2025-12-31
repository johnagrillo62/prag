# file: cpp.py
import re
from dataclasses import dataclass, field, fields as dataclass_fields
from typing import Optional, List, Dict, Set, Tuple, get_type_hints, Type, Union, get_origin, get_args
from datetime import datetime

# ----------------------
# Meta definitions
# ----------------------
@dataclass
class MetaType:
    type_obj: Type
    args: list = field(default_factory=list)
    is_optional: bool = False

@dataclass
class MetaField:
    name: str
    type: MetaType
    csv_column: Optional[str] = None
    table_column: Optional[str] = None

@dataclass
class MetaStruct:
    name: str
    fields: list[MetaField] = field(default_factory=list)
    table_name: Optional[str] = None

# ----------------------
# Table decorator
# ----------------------
def table_meta(table_name: str, class_name: str):
    def decorator(cls):
        cls._table_name = table_name
        cls._class_name = class_name
        return cls
    return decorator

# ----------------------
# Field helper
# ----------------------
def field_meta(*, csv_column=None, table_column=None, **kwargs):
    return field(metadata={"csv_column": csv_column, "table_column": table_column}, **kwargs)

# ----------------------
# Naming conventions
# ----------------------
def to_pascal_case(name: str) -> str:
    return "".join(word.capitalize() for word in re.split(r"[_\s]", name) if word)

def to_snake_case(name: str) -> str:
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
    return s2.lower()

# ----------------------
# Build meta recursively
# ----------------------
def build_meta(cls: Type) -> MetaStruct:
    hints = get_type_hints(cls, include_extras=True)
    fields_list = []
    table_name = getattr(cls, "_table_name", None)

    def wrap_type(typ):
        origin = get_origin(typ)
        args = get_args(typ)

        # Optional
        if origin is Union and type(None) in args:
            non_none = [a for a in args if a is not type(None)][0]
            inner = wrap_type(non_none)
            inner.is_optional = True
            return inner

        # Containers
        if origin is None:
            return MetaType(type_obj=typ, args=[])
        else:
            return MetaType(type_obj=typ, args=[wrap_type(a) for a in args])

    for name, typ in hints.items():
        f = getattr(cls, "__dataclass_fields__", {}).get(name)
        csv_col = f.metadata.get("csv_column") if f else None
        table_col = f.metadata.get("table_column") if f else None
        fields_list.append(MetaField(name=name, type=wrap_type(typ), csv_column=csv_col, table_column=table_col))

    return MetaStruct(name=cls.__name__, fields=fields_list, table_name=table_name)

# ----------------------
# Inline lookup tables
# ----------------------
cpp_registry = {
    "primitives": {
        str: "std::string",
        int: "int",
        float: "double",
        bool: "bool",
    },
    "complex": {
        list: "std::vector<{}>",
        dict: "std::map<{}, {}>",
        set: "std::set<{}>",
        tuple: "std::tuple<{}>",
        Optional: "std::optional<{}>"
    },
    "standard": {
        datetime: "std::chrono::system_clock::time_point"
    },
    "naming": {
        "class_name": to_pascal_case,
        "member_name": to_snake_case,
        "file_name": to_snake_case,
    }
}

# ----------------------
# Recursive type resolver
# ----------------------
def cpp_type(typ: MetaType) -> str:
    origin = get_origin(typ.type_obj)
    args = typ.args

    if typ.is_optional:
        inner = MetaType(typ.type_obj, typ.args)
        inner.is_optional = False
        return cpp_registry["complex"][Optional].format(cpp_type(inner))

    if origin in (list, List) and args:
        return cpp_registry["complex"][list].format(cpp_type(args[0]))
    if origin in (dict, Dict) and args:
        return cpp_registry["complex"][dict].format(cpp_type(args[0]), cpp_type(args[1]))
    if origin in (set, Set) and args:
        return cpp_registry["complex"][set].format(cpp_type(args[0]))
    if origin in (tuple, Tuple) and args:
        inner = ", ".join(cpp_type(a) for a in args)
        return cpp_registry["complex"][tuple].format(inner)

    if typ.type_obj in cpp_registry["primitives"]:
        return cpp_registry["primitives"][typ.type_obj]
    if typ.type_obj in cpp_registry["standard"]:
        return cpp_registry["standard"][typ.type_obj]
    elif hasattr(typ.type_obj, "__annotations__"):
        return typ.type_obj.__name__
    else:
        return str(typ.type_obj)

# ----------------------
# Generate C++ struct with meta tuples
# ----------------------
def generate_cpp_struct(meta: MetaStruct, ns: List[str] = []) -> str:
    indent = "    "
    lines = []

    # includes
    includes = set()
    for f in meta.fields:
        t = f.type.type_obj
        if t is str: includes.add("<string>")
        if t is int: includes.add("<cstdint>")
        if t is datetime: includes.add("<chrono>")
        if get_origin(f.type.type_obj) in (list, List): includes.add("<vector>")
        if get_origin(f.type.type_obj) in (dict, Dict): includes.add("<map>")
        if get_origin(f.type.type_obj) in (set, Set): includes.add("<set>")
        if get_origin(f.type.type_obj) in (tuple, Tuple): includes.add("<tuple>")
        if f.type.is_optional: includes.add("<optional>")
    lines.extend(f"#include {inc}" for inc in sorted(includes))
    lines.append("")

    # namespace
    if ns:
        lines.append("\n".join(f"namespace {n} {{" for n in ns))

    lines.append(f"struct {meta.name} {{")
    lines.append(f"{indent}{meta.name}() = default;\n")

    # members
    for f in meta.fields:
        lines.append(f"{indent}{cpp_type(f.type)} {cpp_registry['naming']['member_name'](f.name)};")
    lines.append("")

    # meta tuples
    lines.append(f"{indent}// reflection meta tuples")
    lines.append(f"{indent}inline static const auto fields = std::make_tuple(")
    tup_entries = []
    for f in meta.fields:
        type_str = cpp_type(f.type)
        member_name = cpp_registry["naming"]["member_name"](f.name)
        csv_col = f.csv_column if f.csv_column else ""
        table_col = f.table_column if f.table_column else ""
        tup_entries.append(f'{indent*2}meta::Field<::{"{0}".format(meta.name)}, {type_str}, &::{"{0}".format(meta.name)}::{member_name}, nullptr, nullptr, meta::Prop::Serializable>("'
                           f'{type_str}", "{member_name}", "{csv_col}", meta::Mapping("{table_col}", "{csv_col}"))')
    lines.append(",\n".join(tup_entries))
    lines.append(f"{indent});")
    lines.append(f'{indent}inline static constexpr auto tableName = "{meta.table_name}";')
    cols = ", ".join(f'{cpp_registry["naming"]["member_name"](f.name)}' for f in meta.fields)
    lines.append(f'{indent}inline static constexpr auto query = "SELECT {cols} FROM {meta.table_name}";')

    lines.append("};")

    if ns:
        lines.append("\n".join(f"}} // namespace {n}" for n in reversed(ns)))

    # MetaTuple specialization
    lines.append("namespace meta {")
    lines.append(f"template<> struct MetaTuple<::{'{0}'.format(meta.name)}> {{")
    lines.append(f"    static inline const auto& fields = {meta.name}::fields;")
    lines.append(f"    static constexpr auto tableName = {meta.name}::tableName;")
    lines.append(f"    static constexpr auto query = {meta.name}::query;")
    lines.append("};")
    lines.append("}  // namespace meta")

    return "\n".join(lines)

# ----------------------
# Example usage
# ----------------------
if __name__ == "__main__":
    @table_meta(table_name="RESULT", class_name="result")
    @dataclass
    class Result:
        athlete: int = field_meta(csv_column="Athlete", table_column="Athlete")
        team1: Optional[int] = field_meta(csv_column="Team1", table_column="Team1")
        last: Optional[str] = field_meta(csv_column="Last", table_column="Last")
        age: Optional[int] = field_meta(csv_column="Age", table_column="Age")
        id_no: Optional[str] = field_meta(csv_column="ID_NO", table_column="ID_NO")
        created: Optional[datetime] = field_meta(csv_column="Created", table_column="Created")

    meta = build_meta(Result)
    cpp_code = generate_cpp_struct(meta, ns=["my", "ns"])
    print(cpp_code)


