#pragma once
#include <nlohmann/json.hpp>
#include <stack>
#include "registry_ast_walker.h"

namespace bhw
{

using json = nlohmann::ordered_json;

/**
 * Convert bhw::Ast to prag JSON AST format
 * Outputs universal JSON that prag can read
 */
class PragAstWalker : public RegistryAstWalker
{
public:
    PragAstWalker() : RegistryAstWalker(Language::Prag)
    {
    }

private:
    json items_array = json::array();
    std::stack<std::pair<json, int>> struct_stack_;
    std::stack<json> enum_stack_;
    
    std::string generateHeader(const bhw::Ast&) override
    {
        items_array = json::array();
        while (!struct_stack_.empty()) struct_stack_.pop();
        while (!enum_stack_.empty()) enum_stack_.pop();
        return "";
    }

    std::string generateFooter(const bhw::Ast&) override
    {
        json output;
        output["type"] = "Module";
        output["items"] = items_array;
        return output.dump(2) + "\n";
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        json struct_json;
        struct_json["type"] = "Struct";
        struct_json["name"] = s.name;
        struct_json["fields"] = json::array();
        
        struct_stack_.push({struct_json, (int)struct_stack_.size()});
        return "";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        if (struct_stack_.empty())
            return "";

        auto top = struct_stack_.top();
        struct_stack_.pop();
        
        auto struct_json = top.first;
        int depth = top.second;

        if (depth == 0)
        {
            items_array.push_back(struct_json);
        }
        else if (!struct_stack_.empty())
        {
            struct_stack_.top().first["fields"].push_back(struct_json);
        }

        return "";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        if (struct_stack_.empty())
            return "";

        json field_json;
        field_json["name"] = field.name;
        field_json["type"] = typeToJson(*field.type);

        struct_stack_.top().first["fields"].push_back(field_json);
        return "";
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        json enum_json;
        enum_json["type"] = "Enum";
        enum_json["name"] = e.name;
        enum_json["variants"] = json::array();

        enum_stack_.push(enum_json);
        return "";
    }

    std::string generateEnumValue(const EnumValue& val, bool, const WalkContext& ctx) override
    {
        if (enum_stack_.empty())
            return "";

        json variant;
        variant["name"] = val.name;
        enum_stack_.top()["variants"].push_back(variant);

        return "";
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        if (enum_stack_.empty())
            return "";

        auto enum_json = enum_stack_.top();
        enum_stack_.pop();

        items_array.push_back(enum_json);
        return "";
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (struct_stack_.empty())
            return "";

        json oneof_json;
        oneof_json["type"] = "Oneof";
        oneof_json["name"] = oneof.name;
        oneof_json["variants"] = json::array();

        for (const auto& field : oneof.fields)
        {
            json variant;
            variant["name"] = field.name;
            variant["type"] = typeToJson(*field.type);
            oneof_json["variants"].push_back(variant);
        }

        struct_stack_.top().first["fields"].push_back(oneof_json);
        return "";
    }

    json typeToJson(const Type& type)
    {
        if (type.isSimple())
        {
            auto& st = std::get<SimpleType>(type.value);
            std::string type_name = st.srcTypeString;
            if (type_name.empty())
            {
                type_name = mapReifiedTypeToName(st.reifiedType);
            }
            return {
                {"kind", "primitive"},
                {"name", type_name}
            };
        }
        else if (type.isStructRef())
        {
            auto& ref = std::get<StructRefType>(type.value);
            std::string type_name = ref.srcTypeString;
            if (type_name.empty())
            {
                type_name = mapReifiedTypeToName(ref.reifiedType);
            }
            return {
                {"kind", "struct"},
                {"name", type_name}
            };
        }
        else if (type.isGeneric())
        {
            auto& gt = std::get<GenericType>(type.value);
            
            json args = json::array();
            for (const auto& arg : gt.args)
            {
                args.push_back(typeToJson(*arg));
            }

            std::string container = getGenericName(gt.reifiedType);
            
            return {
                {"kind", "generic"},
                {"name", container},
                {"args", args}
            };
        }
        else if (type.isPointer())
        {
            auto& pt = std::get<PointerType>(type.value);
            return {
                {"kind", "generic"},
                {"name", "Option"},
                {"args", json::array({typeToJson(*pt.pointee)})}
            };
        }

        return {{"kind", "unknown"}};
    }
    
    std::string mapReifiedTypeToName(ReifiedTypeId id)
    {
        switch (id)
        {
        case ReifiedTypeId::Bool:
            return "bool";
        case ReifiedTypeId::Int8:
            return "i8";
        case ReifiedTypeId::UInt8:
            return "u8";
        case ReifiedTypeId::Int16:
            return "i16";
        case ReifiedTypeId::UInt16:
            return "u16";
        case ReifiedTypeId::Int32:
            return "i32";
        case ReifiedTypeId::UInt32:
            return "u32";
        case ReifiedTypeId::Int64:
            return "i64";
        case ReifiedTypeId::UInt64:
            return "u64";
        case ReifiedTypeId::Float32:
            return "f32";
        case ReifiedTypeId::Float64:
            return "f64";
        case ReifiedTypeId::String:
            return "String";
        case ReifiedTypeId::Bytes:
            return "Vec<u8>";
        case ReifiedTypeId::Char:
            return "char";
        case ReifiedTypeId::DateTime:
            return "DateTime";
        case ReifiedTypeId::Date:
            return "Date";
        case ReifiedTypeId::Time:
            return "Time";
        case ReifiedTypeId::Duration:
            return "Duration";
        case ReifiedTypeId::UUID:
            return "Uuid";
        case ReifiedTypeId::Decimal:
            return "Decimal";
        case ReifiedTypeId::URL:
            return "String";
        case ReifiedTypeId::Email:
            return "String";
        default:
            return "unknown";
        }
    }

    std::string getGenericName(ReifiedTypeId id)
    {
        switch (id)
        {
        case ReifiedTypeId::List:
            return "Vec";
        case ReifiedTypeId::Map:
            return "Map";
        case ReifiedTypeId::Optional:
            return "Option";
        case ReifiedTypeId::Set:
            return "Set";
        case ReifiedTypeId::Tuple:
            return "Tuple";
        case ReifiedTypeId::Array:
            return "Array";
        case ReifiedTypeId::UnorderedMap:
            return "HashMap";
        case ReifiedTypeId::UnorderedSet:
            return "HashSet";
        default:
            return "Unknown";
        }
    }
};

} // namespace bhw

