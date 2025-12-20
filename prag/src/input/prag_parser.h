#pragma once
#include <nlohmann/json.hpp>

#include "ast.h"
#include "ast_parser.h"
#include "languages.h"
#include "parser_registry2.h"

namespace bhw
{

class PragParser : public AstParser, public AutoRegisterParser<PragParser>
{
    using json = nlohmann::ordered_json;

  public:
    static std::vector<std::string> extensions()
    {
        return {"json"};  // Reads .json files (prag AST format)
    }

    Ast parseToAst(const std::string& src) override
    {
        json j = json::parse(src);

        Ast ast;
        ast.srcName = "prag";

        // Parse all items in the module
        if (j.contains("items") && j["items"].is_array())
        {
            for (const auto& item : j["items"])
            {
                parseItem(item, ast);
            }
        }

        return ast;
    }

    auto getLang() -> bhw::Language override
    {
        return Language::Rust;  // Prag is language-agnostic, use Rust as placeholder
    }

  private:
    void parseItem(const json& item, Ast& ast)
    {
        std::string type = item.value("type", "");

        if (type == "Struct")
        {
            ast.nodes.push_back(parseStruct(item));
        }
        else if (type == "Enum")
        {
            ast.nodes.push_back(parseEnum(item));
        }
        else if (type == "Module")
        {
            if (item.contains("items") && item["items"].is_array())
            {
                for (const auto& subitem : item["items"])
                {
                    parseItem(subitem, ast);
                }
            }
        }
    }

    Struct parseStruct(const json& struct_json)
    {
        Struct s;
        s.name = struct_json.value("name", "");
        s.isAnonymous = false;

        if (struct_json.contains("fields") && struct_json["fields"].is_array())
        {
            for (const auto& field_json : struct_json["fields"])
            {
                Field f;
                f.name = field_json.value("name", "");
                f.type = parseType(field_json["type"]);
                s.members.push_back(std::move(f));
            }
        }

        return s;
    }

    Enum parseEnum(const json& enum_json)
    {
        Enum e;
        e.name = enum_json.value("name", "");
        e.scoped = true;

        if (enum_json.contains("variants") && enum_json["variants"].is_array())
        {
            int num = 0;
            for (const auto& variant : enum_json["variants"])
            {
                EnumValue ev;
                ev.name = variant.value("name", "");
                ev.number = num++;
                e.values.push_back(std::move(ev));
            }
        }

        return e;
    }

    std::unique_ptr<Type> parseType(const json& type_json)
    {
        if (!type_json.is_object())
        {
            SimpleType st;
            st.srcTypeString = "unknown";
            st.reifiedType = ReifiedTypeId::Unknown;
            return std::make_unique<Type>(std::move(st));
        }

        std::string kind = type_json.value("kind", "");
        std::string name = type_json.value("name", "");

        if (kind == "primitive")
        {
            SimpleType st;
            st.srcTypeString = type_json.value("srcTypeString", ""); 
            st.reifiedType = mapPrimitiveType(name);
            return std::make_unique<Type>(std::move(st));
        }
        else if (kind == "struct")
        {
            // Check if this is a nested struct definition (has fields)
            if (type_json.contains("fields") && type_json["fields"].is_array())
            {
                // Create nested struct from JSON
                Struct nested;
                nested.name = name;
                nested.isAnonymous = type_json.value("anonymous", false);
                
                for (const auto& field_json : type_json["fields"])
                {
                    Field f;
                    f.name = field_json.value("name", "");
                    f.type = parseType(field_json["type"]);
                    nested.members.push_back(std::move(f));
                }
                
                StructType st;
                st.value = std::make_unique<Struct>(std::move(nested));
                st.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(st));
            }
            else
            {
                // Just a reference to a struct
                StructRefType ref;
                ref.srcTypeString = name;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(ref));
            }
        }
        else if (kind == "generic")
        {
            GenericType g;
            g.reifiedType = mapGenericType(name);

            if (type_json.contains("args") && type_json["args"].is_array())
            {
                for (const auto& arg : type_json["args"])
                {
                    g.args.push_back(parseType(arg));
                }
            }

            return std::make_unique<Type>(std::move(g));
        }
        else if (kind == "enum")
        {
            StructRefType ref;
            ref.srcTypeString = name;
            ref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(ref));
        }

        SimpleType st;
        st.srcTypeString = "unknown";
        st.reifiedType = ReifiedTypeId::Unknown;
        return std::make_unique<Type>(std::move(st));
    }

    ReifiedTypeId mapPrimitiveType(const std::string& name)
    {
        if (name == "bool")
            return ReifiedTypeId::Bool;
        else if (name == "i8")
            return ReifiedTypeId::Int8;
        else if (name == "u8")
            return ReifiedTypeId::UInt8;
        else if (name == "i16")
            return ReifiedTypeId::Int16;
        else if (name == "u16")
            return ReifiedTypeId::UInt16;
        else if (name == "i32")
            return ReifiedTypeId::Int32;
        else if (name == "u32")
            return ReifiedTypeId::UInt32;
        else if (name == "i64")
            return ReifiedTypeId::Int64;
        else if (name == "u64")
            return ReifiedTypeId::UInt64;
        else if (name == "f32")
            return ReifiedTypeId::Float32;
        else if (name == "f64")
            return ReifiedTypeId::Float64;
        else if (name == "String" || name == "string")
            return ReifiedTypeId::String;
        else if (name == "Vec<u8>" || name == "bytes")
            return ReifiedTypeId::Bytes;
        else if (name == "char")
            return ReifiedTypeId::Char;
        else if (name == "DateTime")
            return ReifiedTypeId::DateTime;
        else if (name == "Date")
            return ReifiedTypeId::Date;
        else if (name == "Time")
            return ReifiedTypeId::Time;
        else if (name == "Duration")
            return ReifiedTypeId::Duration;
        else if (name == "Uuid")
            return ReifiedTypeId::UUID;
        else if (name == "Decimal")
            return ReifiedTypeId::Decimal;
        else
            return ReifiedTypeId::Unknown;
    }

    ReifiedTypeId mapGenericType(const std::string& name)
    {
        if (name == "Vec" || name == "vector")
            return ReifiedTypeId::List;
        else if (name == "Map" || name == "map")
            return ReifiedTypeId::Map;
        else if (name == "Set" || name == "set")
            return ReifiedTypeId::Set;
        else if (name == "Option" || name == "Optional")
            return ReifiedTypeId::Optional;
        else if (name == "Tuple" || name == "tuple")
            return ReifiedTypeId::Tuple;
        else if (name == "Array" || name == "array")
            return ReifiedTypeId::Array;
        else if (name == "HashMap")
            return ReifiedTypeId::UnorderedMap;
        else if (name == "HashSet")
            return ReifiedTypeId::UnorderedSet;
        else if (name == "Variant" || name == "enum")
            return ReifiedTypeId::Variant;
        else
            return ReifiedTypeId::Unknown;
    }
};

} // namespace bhw

