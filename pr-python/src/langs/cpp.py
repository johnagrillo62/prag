from typing import List, Optional, Dict, Set, Tuple, get_origin, get_args
from datetime import datetime
from langs.utils import (
    MetaStruct, MetaField, MetaType, build_meta,
    table_meta, field_meta,
    to_pascal_case, to_snake_case
)


# ----------------------
# Inline C++ type lookup
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
    origin = typ.type_obj if get_origin(typ.type_obj) is None else get_origin(typ.type_obj)
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
        if f.type.args:
            for arg in f.type.args:
                if arg.type_obj is list: includes.add("<vector>")
                if arg.type_obj is dict: includes.add("<map>")
                if arg.type_obj is set: includes.add("<set>")
                if arg.type_obj is tuple: includes.add("<tuple>")
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
        
    # MetaTuple specialization with fully-qualified struct name
    fq_struct = "::" + "::".join(ns + [meta.name]) if ns else "::" + meta.name
    lines.append("namespace meta {")
    lines.append(f"template<> struct MetaTuple<{fq_struct}> {{")
    lines.append(f"    static inline const auto& fields = {fq_struct}::fields;")
    lines.append(f"    static constexpr auto tableName = {fq_struct}::tableName;")
    lines.append(f"    static constexpr auto query = {fq_struct}::query;")
    lines.append("};")
    lines.append("}  // namespace meta")

    return "\n".join(lines)        


# ----------------------
# Example usage
# ----------------------
if __name__ == "__main__":
    from dataclasses import dataclass
    @table_meta(table_name="RESULT", class_name="result")
    @dataclass
    class Result:
        athlete: int = field_meta(csv_column="Athlete", table_column="Athlete")
        team1: Optional[int] = field_meta(csv_column="Team1", table_column="team 1")
        last: Optional[str] = field_meta(csv_column="Last", table_column="Last")
        age: Optional[int] = field_meta(csv_column="Age", table_column="Age")
        id_no: Optional[str] = field_meta(csv_column="ID_NO", table_column="ID_NO")
        created: Optional[datetime] = field_meta(csv_column="Created", table_column="Created")

    meta = build_meta(Result)
    cpp_code = generate_cpp_struct(meta, ns=["my", "ns"])
    print(cpp_code)
