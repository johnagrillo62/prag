# langs/utils.py
import re
from dataclasses import field
from typing import Optional, Type, get_type_hints, Union, get_origin, get_args, Any


# ----------------------
# Naming helpers
# ----------------------
def to_pascal_case(name: str) -> str:
    """Convert 'some_name' -> 'SomeName'"""
    return "".join(word.capitalize() for word in re.split(r"[_\s]", name) if word)

def to_snake_case(name: str) -> str:
    """Convert 'SomeName' -> 'some_name'"""
    name = name.replace(" ", "_")
    s1 = re.sub('(.)([A-Z][a-z]+)', r'\1_\2', name)
    s2 = re.sub('([a-z0-9])([A-Z])', r'\1_\2', s1)
    return s2.lower()

def to_lower_space(name: str) -> str:
    """Convert 'SomeName' -> 'some name'"""
    return to_snake_case(name).replace("_", " ")

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

def field_meta(*, csv_column=None, table_column=None, member_name=None,
               primary_key=False, size=None, signed=None, **kwargs):
    typ = kwargs.get("type")
    return field(metadata={
        "csv_column": csv_column,
        "table_column": table_column,
        "member_name": member_name,
        "primary_key": primary_key,
        "size": size,
        "signed": signed
    }, **kwargs)


# ----------------------
# Meta structures
# ----------------------
class MetaType:
    def __init__(self, type_obj: Type, args=None, is_optional=False):
        self.type_obj = type_obj
        self.args = args or []
        self.is_optional = is_optional

class MetaField:
    def __init__(self, name: str, type_: MetaType, csv_column=None, table_column=None, member_name=None):
        self.name = name
        self.type = type_
        self.csv_column = csv_column
        self.table_column = table_column
        self.member_name = member_name

class MetaStruct:
    def __init__(self, name: str, fields=None, table_name=None):
        self.name = name
        self.fields = fields or []
        self.table_name = table_name

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
        member_name = f.metadata.get("member_name") if f else to_lower_space(name)
        fields_list.append(MetaField(
            name=name,
            type_=wrap_type(typ),
            csv_column=csv_col,
            table_column=table_col,
            member_name=member_name
        ))

    return MetaStruct(name=cls.__name__, fields=fields_list, table_name=table_name)


# langs/cpp.py
import re
from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional, List, Dict, Set, Tuple, get_origin, get_args

from langs.utils import (
    table_meta,
    field_meta,
    build_meta,
    MetaStruct,
    MetaField,
    MetaType,
)

# ----------------------
# Inline C++ type registry
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
# Generate C++ struct
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
        origin = get_origin(f.type.type_obj)
        if origin in (list, List): includes.add("<vector>")
        if origin in (dict, Dict): includes.add("<map>")
        if origin in (set, Set): includes.add("<set>")
        if origin in (tuple, Tuple): includes.add("<tuple>")
        if f.type.is_optional: includes.add("<optional>")
    lines.extend(f"#include {inc}" for inc in sorted(includes))
    lines.append("")

    # namespace
    if ns:
        lines.append("\n".join(f"namespace {n} {{" for n in ns))

    # struct definition
    lines.append(f"struct {meta.name} {{")
    lines.append(f"{indent}{meta.name}() = default;\n")

    # members
    for f in meta.fields:
        member_name = f.member_name if f.member_name else f.name
        lines.append(f"{indent}{cpp_type(f.type)} {member_name};")
    lines.append("")

    # meta tuples
    lines.append(f"{indent}// reflection meta tuples")
    lines.append(f"{indent}inline static const auto fields = std::make_tuple(")
    tup_entries = []
    for f in meta.fields:
        type_str = cpp_type(f.type)
        member_name = f.member_name if f.member_name else f.name
        csv_col = f.csv_column if f.csv_column else ""
        table_col = f.table_column if f.table_column else ""
        tup_entries.append(
            f'{indent*2}meta::Field<::{"{0}".format(meta.name)}, {type_str}, &::{"{0}".format(meta.name)}::{member_name}, nullptr, nullptr, meta::Prop::Serializable>("'
            f'{type_str}", "{member_name}", "{csv_col}", meta::Mapping("{table_col}", "{csv_col}"))'
        )
    lines.append(",\n".join(tup_entries))
    lines.append(f"{indent});")
    lines.append(f'{indent}inline static constexpr auto tableName = "{meta.table_name}";')
    cols = ", ".join(f'{f.member_name if f.member_name else f.name}' for f in meta.fields)
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
        athlete: int = field_meta(csv_column="Athlete", table_column="Athlete", member_name="athlete")
        team1: Optional[int] = field_meta(csv_column="Team1", table_column="Team1", member_name="team 1")
        last: Optional[str] = field_meta(csv_column="Last", table_column="Last", member_name="last")
        age: Optional[int] = field_meta(csv_column="Age", table_column="Age", member_name="age")
        id_no: Optional[str] = field_meta(csv_column="ID_NO", table_column="ID_NO", member_name="id no")
        created: Optional[datetime] = field_meta(csv_column="Created", table_column="Created", member_name="created")

    meta = build_meta(Result)
    cpp_code = generate_cpp_struct(meta, ns=["my", "ns"])
    print(cpp_code)

