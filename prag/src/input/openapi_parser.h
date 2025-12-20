// openapi_parser.h - using nlohmann/json
#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "ast/ast.h"
#include "parser_registry2.h"

namespace bhw
{

class OpenApiParser : public AstParser, public AutoRegisterParser<OpenApiParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"openapi"};
    }
   
    using json = nlohmann::ordered_json;

  public:

    auto getLang() -> bhw::Language override
    {
        return Language::OpenApi;
    }

    Ast parseToAst(const std::string& src)
    {
        json root = json::parse(src);

        if (root.contains("components") && root["components"].contains("schemas"))
        {
            parseSchemas(root["components"]["schemas"]);
        }

        Ast ast;
        ast.srcName = "openapi";
        for (auto& e : enums)
            ast.nodes.push_back(std::move(e));
        for (auto& s : structs)
            ast.nodes.push_back(std::move(s));
        return ast;
    }

  private:
    std::string src;
    std::vector<Struct> structs;
    std::vector<Enum> enums;

    void parseSchemas(const json& schemas)
    {
        for (auto& [name, schema] : schemas.items())
        {
            parseSchema(name, schema);
        }
    }

    void parseSchema(const std::string& name, const json& schema)
    {
        std::string type = schema.value("type", "object");

        // Handle enum
        if (schema.contains("enum"))
        {
            Enum e;
            e.name = name;
            e.scoped = true;
            int num = 0;
            for (auto& val : schema["enum"])
            {
                EnumValue ev;
                ev.name = val.get<std::string>();
                ev.number = num++;
                e.values.push_back(std::move(ev));
            }
            enums.push_back(std::move(e));
            return;
        }

        // Handle object
        if (type == "object" || schema.contains("properties"))
        {
            Struct s;
            s.name = name;

            // Get required fields
            std::vector<std::string> required;
            if (schema.contains("required"))
            {
                for (auto& r : schema["required"])
                {
                    required.push_back(r.get<std::string>());
                }
            }

            // Parse properties
            if (schema.contains("properties"))
            {
                for (auto& [fieldName, prop] : schema["properties"].items())
                {
                    Field f;
                    f.name = fieldName;

                    bool isRequired =
                        std::find(required.begin(), required.end(), fieldName) != required.end();
                    f.type = parseType(prop, isRequired);

                    s.members.push_back(std::move(f));
                }
            }

            structs.push_back(std::move(s));
        }
    }

    std::unique_ptr<Type> parseType(const json& node, bool required)
    {
        // Handle $ref
        if (node.contains("$ref"))
        {
            std::string ref = node["$ref"].get<std::string>();
            size_t pos = ref.rfind('/');
            std::string typeName = (pos != std::string::npos) ? ref.substr(pos + 1) : ref;

            StructRefType sref;
            sref.srcTypeString = typeName;
            sref.reifiedType = ReifiedTypeId::StructRefType;
            auto t = std::make_unique<Type>(std::move(sref));
            return wrapOptional(std::move(t), required);
        }

        // Handle oneOf/anyOf - collect ALL types
        if (node.contains("oneOf") || node.contains("anyOf"))
        {
            auto& opts = node.contains("oneOf") ? node["oneOf"] : node["anyOf"];

            std::vector<std::unique_ptr<Type>> types;
            bool hasNull = false;

            for (auto& opt : opts)
            {
                // Check for null type
                if (opt.contains("type") && opt["type"].get<std::string>() == "null")
                    hasNull = true;
                else
                    types.push_back(parseType(opt, true));
            }

            // If only one non-null type
            if (types.size() == 1)
            {
                if (hasNull || !required)
                {
                    // Wrap in Optional
                    GenericType g;
                    g.reifiedType = ReifiedTypeId::Optional;
                    g.args.push_back(std::move(types[0]));
                    return std::make_unique<Type>(std::move(g));
                }
                else
                {
                    return std::move(types[0]);
                }
            }

            // Multiple non-null types -> Variant
            if (types.size() > 1)
            {
                GenericType g;
                g.reifiedType = ReifiedTypeId::Variant;
                g.args = std::move(types);

                if (hasNull || !required)
                {
                    // Wrap Variant in Optional
                    GenericType opt;
                    opt.reifiedType = ReifiedTypeId::Optional;
                    opt.args.push_back(std::make_unique<Type>(std::move(g)));
                    return std::make_unique<Type>(std::move(opt));
                }
                else
                {
                    return std::make_unique<Type>(std::move(g));
                }
            }
        }
        std::string type = node.value("type", "");
        std::string format = node.value("format", "");

        // Array
        if (type == "array")
        {
            GenericType g;
            g.reifiedType = ReifiedTypeId::List;
            if (node.contains("items"))
            {
                g.args.push_back(parseType(node["items"], true));
            }
            else
            {
                SimpleType st;
                st.reifiedType = ReifiedTypeId::String;
                g.args.push_back(std::make_unique<Type>(std::move(st)));
            }
            auto t = std::make_unique<Type>(std::move(g));
            return wrapOptional(std::move(t), required);
        }

        // MAP - object with additionalProperties (no properties or empty properties)
        if (type == "object" && node.contains("additionalProperties"))
        {
            GenericType g;
            g.reifiedType = ReifiedTypeId::Map;
            SimpleType key;
            key.reifiedType = ReifiedTypeId::String;
            g.args.push_back(std::make_unique<Type>(std::move(key)));

            if (node["additionalProperties"].is_object())
                g.args.push_back(parseType(node["additionalProperties"], true));
            else
            {
                SimpleType st;
                st.reifiedType = ReifiedTypeId::String;
                g.args.push_back(std::make_unique<Type>(std::move(st)));
            }
            auto t = std::make_unique<Type>(std::move(g));
            return wrapOptional(std::move(t), required);
        }

        // Primitives
        SimpleType st;
        if (type == "integer")
        {
            st.reifiedType = (format == "int64") ? ReifiedTypeId::Int64 : ReifiedTypeId::Int32;
        }
        else if (type == "number")
        {
            st.reifiedType = (format == "float") ? ReifiedTypeId::Float32 : ReifiedTypeId::Float64;
        }
        else if (type == "boolean")
        {
            st.reifiedType = ReifiedTypeId::Bool;
        }
        else if (type == "string")
        {
            if (format == "date-time" || format == "date")
                st.reifiedType = ReifiedTypeId::DateTime;
            else if (format == "byte" || format == "binary")
                st.reifiedType = ReifiedTypeId::Bytes;
            else
                st.reifiedType = ReifiedTypeId::String;
        }
        else
        {
            st.reifiedType = ReifiedTypeId::String;
        }

        auto t = std::make_unique<Type>(std::move(st));
        return wrapOptional(std::move(t), required);
    }

    std::unique_ptr<Type> wrapOptional(std::unique_ptr<Type> inner, bool required)
    {
        if (required)
            return inner;

        GenericType g;
        g.reifiedType = ReifiedTypeId::Optional;
        g.args.push_back(std::move(inner));
        return std::make_unique<Type>(std::move(g));
    }
};

} // namespace bhw