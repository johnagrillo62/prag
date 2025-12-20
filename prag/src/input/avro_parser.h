#pragma once
#include <nlohmann/json.hpp>

#include "ast.h"
#include "ast_parser.h"
#include "languages.h"
#include "parser_registry2.h"

namespace bhw
{

class AvroParser : public AstParser, public AutoRegisterParser<AvroParser>
{
    using json = nlohmann::ordered_json;

  public:
    static std::vector<std::string> extensions()
    {
        return {"avsc"};
    }
    Ast parseToAst(const std::string& src) override
    {
        json j = json::parse(src);

        // Handle array of schemas
        if (j.is_array())
        {
            for (auto& schema : j)
            {
                parseType(schema);
            }
        }
        else
        {
            parseType(j);
        }

        Ast ast;
        ast.srcName = "avro";

        // Enums first (for C++ ordering)
        for (auto& e : enums)
            ast.nodes.push_back(std::move(e));
        for (auto& s : structs)
            ast.nodes.push_back(std::move(s));

        return ast;
    }

    auto getLang() -> bhw::Language override
    {
        return Language::Avro;
    }

  private:
    std::vector<Struct> structs;
    std::vector<Enum> enums;

    std::unique_ptr<Type> parseType(const json& j)
    {
        if (j.is_string())
        {
            std::string s = j.get<std::string>();
            SimpleType st;

            if (s == "null")
                st.reifiedType = ReifiedTypeId::Unknown;
            else if (s == "boolean")
                st.reifiedType = ReifiedTypeId::Bool;
            else if (s == "int")
                st.reifiedType = ReifiedTypeId::Int32;
            else if (s == "long")
                st.reifiedType = ReifiedTypeId::Int64;
            else if (s == "float")
                st.reifiedType = ReifiedTypeId::Float32;
            else if (s == "double")
                st.reifiedType = ReifiedTypeId::Float64;
            else if (s == "bytes")
                st.reifiedType = ReifiedTypeId::Bytes;
            else if (s == "string")
                st.reifiedType = ReifiedTypeId::String;
            else
            {
                StructRefType ref;
                ref.srcTypeString = s;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(ref));
            }
            return std::make_unique<Type>(std::move(st));
        }

        if (j.is_array())
        {
            // Collect all types, separating null from non-null
            std::vector<std::unique_ptr<Type>> types;
            bool hasNull = false;

            for (auto& opt : j)
            {
                if (opt.is_string() && opt.get<std::string>() == "null")
                    hasNull = true;
                else
                    types.push_back(parseType(opt));
            }

            // If only one non-null type
            if (types.size() == 1)
            {
                if (hasNull)
                {
                    // ["null", "type"] -> Optional<type>
                    GenericType g;
                    g.reifiedType = ReifiedTypeId::Optional;
                    g.args.push_back(std::move(types[0]));
                    return std::make_unique<Type>(std::move(g));
                }
                else
                {
                    // ["type"] -> type
                    return std::move(types[0]);
                }
            }

            // Multiple non-null types -> Variant
            if (types.size() > 1)
            {
                GenericType g;
                g.reifiedType = ReifiedTypeId::Variant;
                g.args = std::move(types);

                if (hasNull)
                {
                    // ["null", "type1", "type2", ...] -> Optional<Variant<type1, type2, ...>>
                    GenericType opt;
                    opt.reifiedType = ReifiedTypeId::Optional;
                    opt.args.push_back(std::make_unique<Type>(std::move(g)));
                    return std::make_unique<Type>(std::move(opt));
                }
                else
                {
                    // ["type1", "type2", ...] -> Variant<type1, type2, ...>
                    return std::make_unique<Type>(std::move(g));
                }
            }

            // Fallback for empty array or all nulls
            SimpleType st;
            st.reifiedType = ReifiedTypeId::Unknown;
            return std::make_unique<Type>(std::move(st));
        }

        if (j.is_object())
        {
            std::string type = j["type"].get<std::string>();

            if (type == "array")
            {
                GenericType g;
                g.reifiedType = ReifiedTypeId::List;
                g.args.push_back(parseType(j["items"]));
                return std::make_unique<Type>(std::move(g));
            }

            if (type == "map")
            {
                GenericType g;
                g.reifiedType = ReifiedTypeId::Map;
                SimpleType key;
                key.reifiedType = ReifiedTypeId::String;
                g.args.push_back(std::make_unique<Type>(std::move(key)));
                g.args.push_back(parseType(j["values"]));
                return std::make_unique<Type>(std::move(g));
            }

            if (type == "record")
            {
                Struct s;
                s.name = j["name"].get<std::string>();
                if (j.contains("namespace"))
                    s.namespaces.push_back(j["namespace"].get<std::string>());

                for (auto& jf : j["fields"])
                {
                    Field f;
                    f.name = jf["name"].get<std::string>();
                    f.type = parseType(jf["type"]);
                    if (jf.contains("default"))
                        f.attributes.push_back({"default", jf["default"].dump()});
                    if (jf.contains("doc"))
                        f.attributes.push_back({"doc", jf["doc"].get<std::string>()});
                    s.members.push_back(std::move(f));
                }

                std::string name = s.name;
                structs.push_back(std::move(s));

                StructRefType ref;
                ref.srcTypeString = name;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(ref));
            }

            if (type == "enum")
            {
                Enum e;
                e.name = j["name"].get<std::string>();
                e.scoped = true;
                if (j.contains("namespace"))
                    e.namespaces.push_back(j["namespace"].get<std::string>());

                int num = 0;
                for (auto& sym : j["symbols"])
                {
                    EnumValue ev;
                    ev.name = sym.get<std::string>();
                    ev.number = num++;
                    e.values.push_back(std::move(ev));
                }

                std::string name = e.name;
                enums.push_back(std::move(e));

                StructRefType ref;
                ref.srcTypeString = name;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(ref));
            }

            if (type == "fixed")
            {
                SimpleType st;
                // st.srcTypeString = j["name"].get<std::string>();
                st.reifiedType = ReifiedTypeId::Bytes;
                return std::make_unique<Type>(std::move(st));
            }
        }

        SimpleType st;
        st.srcTypeString = "unknown";
        st.reifiedType = ReifiedTypeId::Unknown;
        return std::make_unique<Type>(std::move(st));
    }
};

} // namespace bhw
