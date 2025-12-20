// cpp_generator.cpp

#include "cpp_generator.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <sstream>

#include "ast.h"

namespace codegen
{

// =========================
// Canonical type → includes
// =========================
static const std::map<CanonicalType, std::vector<std::string>> TYPE_TO_INCLUDES = {
    {CanonicalType::Int8, {"<cstdint>"}},
    {CanonicalType::UInt8, {"<cstdint>"}},
    {CanonicalType::Int16, {"<cstdint>"}},
    {CanonicalType::UInt16, {"<cstdint>"}},
    {CanonicalType::Int32, {"<cstdint>"}},
    {CanonicalType::UInt32, {"<cstdint>"}},
    {CanonicalType::Int64, {"<cstdint>"}},
    {CanonicalType::UInt64, {"<cstdint>"}},
    {CanonicalType::String, {"<string>"}},
    {CanonicalType::Bytes, {"<vector>", "<cstdint>"}},
    {CanonicalType::Char, {}},
    {CanonicalType::Bool, {}},
    {CanonicalType::Float32, {}},
    {CanonicalType::Float64, {}},

    {CanonicalType::DateTime, {"<chrono>"}},
    {CanonicalType::Date, {"<chrono>"}},
    {CanonicalType::Time, {"<chrono>"}},
    {CanonicalType::Duration, {"<chrono>"}},
    {CanonicalType::UUID, {"<array>", "<cstdint>"}},

    {CanonicalType::List, {"<vector>"}},
    {CanonicalType::Map, {"<map>"}},
    {CanonicalType::Set, {"<set>"}},
    {CanonicalType::Optional, {"<optional>"}},
    {CanonicalType::Tuple, {"<tuple>"}},
    {CanonicalType::Variant, {"<variant>"}},
    {CanonicalType::UniquePtr, {"<memory>"}},
    {CanonicalType::SharedPtr, {"<memory>"}},
    {CanonicalType::UnorderedMap, {"<unordered_map>"}},
    {CanonicalType::UnorderedSet, {"<unordered_set>"}},
    {CanonicalType::Pair, {"<utility>"}},
    {CanonicalType::Array, {"<array>"}},
};

// ======================
// Helper type converters
// ======================

std::string CppGenerator::canonicalToCppType(CanonicalType type)
{
    switch (type)
    {
    case CanonicalType::Bool:
        return "bool";
    case CanonicalType::Int8:
        return "int8_t";
    case CanonicalType::UInt8:
        return "uint8_t";
    case CanonicalType::Int16:
        return "int16_t";
    case CanonicalType::UInt16:
        return "uint16_t";
    case CanonicalType::Int32:
        return "int32_t";
    case CanonicalType::UInt32:
        return "uint32_t";
    case CanonicalType::Int64:
        return "int64_t";
    case CanonicalType::UInt64:
        return "uint64_t";
    case CanonicalType::Float32:
        return "float";
    case CanonicalType::Float64:
        return "double";
    case CanonicalType::String:
        return "std::string";
    default:
        return "UNKNOWN";
    }
}

std::string CppGenerator::containerToCppType(CanonicalType type)
{
    switch (type)
    {
    case CanonicalType::List:
        return "std::vector";
    case CanonicalType::Map:
        return "std::map";
    case CanonicalType::Set:
        return "std::set";
    case CanonicalType::Optional:
        return "std::optional";
    case CanonicalType::Pair:
        return "std::pair";
    case CanonicalType::UnorderedMap:
        return "std::unordered_map";
    case CanonicalType::UnorderedSet:
        return "std::unordered_set";
    default:
        return "UNKNOWN_CONTAINER";
    }
}

void CppGenerator::generateFieldType(const Type& type, std::stringstream& out, size_t  indent )
{
    out << CppGenerator::resolveType(type, indent);
}

std::string CppGenerator::generateDefaultValue(const Type& type, int /*indent*/)
{
    std::stringstream out;
    if (type.isSimple())
    {
        const auto& simple = std::get<SimpleType>(type.value);
        if (simple.isCanonical())
        {
            switch (simple.getCanonical())
            {
            case CanonicalType::Bool:
                out << "false";
                break;
            case CanonicalType::String:
                out << "\"\"";
                break;
            default:
                out << "0";
                break;
            }
        }
        else
        {
            out << "{}";
        }
    }
    else
    {
        out << "{}";
    }
    return out.str();
}

// ==========================
// Utilities
// ==========================

std::string CppGenerator::toSnakeCase(const std::string& name)
{
    std::string result;
    for (size_t i = 0; i < name.size(); i++)
    {
        if (std::isupper(name[i]))
        {
            if (i > 0 && std::islower(name[i - 1]))
                result += '_';
            result += std::tolower(name[i]);
        }
        else
        {
            result += name[i];
        }
    }
    return result;
}

// ==========================
// Include collection
// ==========================

std::set<std::string> CppGenerator::collectIncludes(const Type& t)
{
    std::set<std::string> includes;

    if (t.isSimple())
    {
        const auto& simple = std::get<SimpleType>(t.value);
        if (simple.isCanonical())
        {
            auto canon = simple.getCanonical();
            if (TYPE_TO_INCLUDES.count(canon))
            {
                const auto& headers = TYPE_TO_INCLUDES.at(canon);
                includes.insert(headers.begin(), headers.end());
            }
        }
    }
    else if (t.isPointer())
    {
        const auto& ptr = std::get<PointerType>(t.value);
        auto inner = collectIncludes(*ptr.pointee);
        includes.insert(inner.begin(), inner.end());
    }
    else if (t.isGeneric())
    {
        const auto& gen = std::get<GenericType>(t.value);
        if (TYPE_TO_INCLUDES.count(gen.container))
        {
            const auto& headers = TYPE_TO_INCLUDES.at(gen.container);
            includes.insert(headers.begin(), headers.end());
        }

        for (const auto& arg : gen.args)
        {
            auto inner = collectIncludes(*arg);
            includes.insert(inner.begin(), inner.end());
        }
    }

    return includes;
}

// ==========================
// Type resolver
// ==========================

std::string CppGenerator::resolveType(const Type& type, size_t  indent )
{
    std::stringstream out;
    std::string ind(indent * 2, ' ');

    if (type.isSimple())
    {
        const auto& simple = std::get<SimpleType>(type.value);
        if (simple.isCanonical())
        {
            if (!simple.original_type_string.empty())
            {
                out << simple.original_type_string;
            }
            else
            {
                out << canonicalToCppType(simple.getCanonical());
            }
        }
        else
            out << simple.getUserDefined();
    }
    else if (type.isPointer())
    {
        const auto& ptr = std::get<PointerType>(type.value);
        generateFieldType(*ptr.pointee, out, indent);
        out << "*";
    }
    else if (type.isGeneric())
    {
        const auto& gen = std::get<GenericType>(type.value);
        out << containerToCppType(gen.container) << "<";
        for (size_t i = 0; i < gen.args.size(); ++i)
        {
            if (i > 0)
                out << ", ";
            generateFieldType(*gen.args[i], out, indent);
        }
        out << ">";
    }

    else if (type.isStruct())
    {
        const auto& structType = std::get<StructType>(type.value);
        const auto& nested = structType.value;
	
        out << ind << "struct " << nested->name << " {\n";
	/*
        for (const auto& f : nested->fields)
        {
            out << std::string((indent + 1) * 2, ' ');
            out << resolveType(*f.type, indent + 1) << " " << f.name;

            // Only simple types get default values
            if (f.type->isSimple())
                out << " = " << generateDefaultValue(*f.type, indent + 1);

            out << ";\n";
        }
	*/
        out << ind << "};"; // semicolon for nested struct
    }

    else
    {
        out << "UNKNOWN_TYPE";
    }

    return out.str();
}

// ==========================
// Include formatter
// ==========================

std::string CppGenerator::formatIncludes(const std::set<std::string>& includes)
{
    std::stringstream ss;
    for (const auto& inc : includes)
        ss << "#include " << inc << "\n";
    return ss.str();
}

// ==========================
// Struct generator
// ==========================

std::string CppGenerator::generateNode(const StructMember& node, size_t  indent )
{
    std::string ind(indent, ' ');
    std::stringstream ss;
    if (std::holds_alternative<Field>(node))
    {
        const auto& field = std::get<Field>(node);
        std::string cpp_type = resolveType(*field.type.get());
        std::string field_name = toSnakeCase(field.name);

        for (auto& attr : field.attributes)
            ss << "    // @" << attr.name << "(\"" << attr.value << "\")\n";

        ss << ind << "  " << cpp_type << " " << field_name << ";\n"; //" = {};\n";
    }

    if (std::holds_alternative<Enum>(node))
    {
        const auto& num = std::get<Enum>(node);

        ss << ind << "  "
           << "enum " << (num.scoped ? "class" : "") << " " << num.name << "\n"
           << ind << "  "
           << "{\n";
        for (auto& [name, number, _] : num.values)
        {
            ss << ind << "    " << name << " = " << number << ",\n";
        }

        // for (auto& attr : enum.attributes)
        // ss << "    // @" << attr.name << "(\"" << attr.value << "\")\n";
        // ss << ind << "  " << cpp_type << " " << field_name << ";\n"; //" = {};\n";
        ss << " "
           << "};\n";
    }

    else if (std::holds_alternative<Struct>(node))
    {
        const auto& strct = std::get<Struct>(node);
        ss << ind << "  struct " << (strct.isAnonymous ? "" : strct.name) << " {\n";
        for (const auto& node : strct.members)
        {
            ss << generateNode(node, indent + 2);
        }
        ss << ind << " }";

        if (!strct.variableName.empty())
        {
            ss << " " << strct.variableName << " ";
        }
        ss << ";\n";
    }

    return ss.str();
}

/*
std::string CppGenerator::generateHeader(const Struct& s, const std::string& namespace_name)
{
    std::stringstream ss;

    // Collect includes
    std::set<std::string> includes;
    for (const auto& field : s.fields)
    {
        auto field_includes = collectIncludes(*field.type);
        includes.insert(field_includes.begin(), field_includes.end());
    }

    ss << "#pragma once\n\n";
    if (!includes.empty())
        ss << formatIncludes(includes) << "\n";

    if (!namespace_name.empty())
        ss << "namespace " << namespace_name << " {\n\n";

    ss << "struct " << s.name << " {\n";
    for (const auto& m : s.members)
    {
        ss << generateNode(m);
    }
    ss << "};\n";

      if (!namespace_name.empty())
        ss << "\n}  // namespace " << namespace_name << "\n";

    return ss.str();
}

std::string CppGenerator::generate(const bhw::Ast& ast)
{
    std::stringstream ss;

    return ss.str();
}
*/

} // namespace codegen
