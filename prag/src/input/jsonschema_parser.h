#pragma once
#include <nlohmann/json.hpp>
#include <string>

#include "ast/ast.h"
#include "ast_parser.h"
// #include "parser_registry2.h"

namespace bhw
{

class JSONSchemaParser : public AstParser, public AutoRegisterParser<JSONSchemaParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"json"};
    }
    using json = nlohmann::ordered_json;

  public:
    auto getLang() -> Language override
    {
        return Language::JSONSchema;
    }

    auto parseToAst(const std::string& src) -> Ast override
    {
        json j = json::parse(src);

        // Parse $defs/definitions directly
        if (j.contains("$defs"))
        {
            for (auto& [name, schema] : j["$defs"].items())
            {
                defs[name] = schema; // store for refs
                parseSchema(schema, name);
            }
        }
        if (j.contains("definitions"))
        {
            for (auto& [name, schema] : j["definitions"].items())
            {
                defs[name] = schema;
                parseSchema(schema, name);
            }
        }

        // Only parse root if it has actual schema content (not just $defs wrapper)
        if (j.contains("properties") || j.contains("type") || j.contains("enum"))
        {
            parseSchema(j, j.contains("title") ? j["title"].get<std::string>() : "Root");
        }

        Ast ast;
        ast.srcName = "jsonschema";

        // Enums first
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
    json defs;
    std::set<std::string> parsed;

    void parseDefs(const json& d)
    {
        defs = d;
    }

    std::string capitalize(const std::string& s)
    {
        if (s.empty())
            return s;
        std::string r = s;
        r[0] = std::toupper(r[0]);
        return r;
    }

    std::unique_ptr<Type> parseSchema(const json& j, const std::string& name)
    {
        // Handle $ref
        if (j.contains("$ref"))
        {
            std::string ref = j["$ref"].get<std::string>();
            auto pos = ref.rfind('/');
            std::string refName = ref.substr(pos + 1);

            if (defs.contains(refName) && parsed.find(refName) == parsed.end())
            {
                parseSchema(defs[refName], refName);
            }

            StructRefType sref;
            sref.srcTypeString = refName;
            sref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(sref));
        }

        // Get type
        std::string type;
        if (j.contains("type"))
        {
            if (j["type"].is_array())
                type = j["type"][0].get<std::string>();
            else
                type = j["type"].get<std::string>();
        }
        else if (j.contains("properties"))
        {
            type = "object";
        }

        // ENUM - check BEFORE primitives!
        if (j.contains("enum"))
        {
            if (parsed.find(name) != parsed.end())
            {
                StructRefType ref;
                ref.srcTypeString = name;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(ref));
            }
            parsed.insert(name);

            Enum e;
            e.name = name;
            e.scoped = true;
            int num = 0;
            for (auto& val : j["enum"])
            {
                EnumValue ev;
                ev.name = val.is_string() ? val.get<std::string>() : val.dump();
                ev.number = num++;
                e.values.push_back(std::move(ev));
            }
            enums.push_back(std::move(e));

            StructRefType ref;
            ref.srcTypeString = name;
            ref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(ref));
        }

        // Primitives
        if (type == "string")
        {
            SimpleType st;
            st.reifiedType = ReifiedTypeId::String;
            return std::make_unique<Type>(std::move(st));
        }
        if (type == "integer")
        {
            SimpleType st;
            st.reifiedType = ReifiedTypeId::Int64;
            return std::make_unique<Type>(std::move(st));
        }
        if (type == "number")
        {
            SimpleType st;
            st.reifiedType = ReifiedTypeId::Float64;
            return std::make_unique<Type>(std::move(st));
        }
        if (type == "boolean")
        {
            SimpleType st;
            st.reifiedType = ReifiedTypeId::Bool;
            return std::make_unique<Type>(std::move(st));
        }
        if (type == "null")
        {
            SimpleType st;
            st.srcTypeString = "null";
            st.reifiedType = ReifiedTypeId::Unknown;
            return std::make_unique<Type>(std::move(st));
        }

        // Array
        if (type == "array")
        {
            GenericType g;
            g.reifiedType = ReifiedTypeId::List;
            if (j.contains("items"))
                g.args.push_back(parseSchema(j["items"], name + "Item"));
            else
            {
                SimpleType any;
                any.srcTypeString = "any";
                any.reifiedType = ReifiedTypeId::Unknown;
                g.args.push_back(std::make_unique<Type>(std::move(any)));
            }
            return std::make_unique<Type>(std::move(g));
        }

        // MAP - object with ONLY additionalProperties, NO properties
        if (type == "object" && !j.contains("properties") && j.contains("additionalProperties"))
        {
            GenericType g;
            g.reifiedType = ReifiedTypeId::Map;
            SimpleType key;
            key.reifiedType = ReifiedTypeId::String;
            g.args.push_back(std::make_unique<Type>(std::move(key)));

            if (j["additionalProperties"].is_object())
                g.args.push_back(parseSchema(j["additionalProperties"], name + "Value"));
            else
            {
                SimpleType st;
                st.reifiedType = ReifiedTypeId::String;
                g.args.push_back(std::make_unique<Type>(std::move(st)));
            }
            return std::make_unique<Type>(std::move(g));
        }

        // Object (struct)
        if (type == "object" || j.contains("properties"))
        {
            if (parsed.find(name) != parsed.end())
            {
                StructRefType ref;
                ref.srcTypeString = name;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(ref));
            }
            parsed.insert(name);

            Struct s;
            s.name = name;

            if (j.contains("description"))
                s.attributes.push_back({"description", j["description"].get<std::string>()});

            std::vector<std::string> required;
            if (j.contains("required"))
                for (auto& r : j["required"])
                    required.push_back(r.get<std::string>());

            if (j.contains("properties"))
            {
                for (auto& [key, val] : j["properties"].items())
                {
                    Field f;
                    f.name = key;

                    std::string nestedName = name + capitalize(key);
                    auto fieldType = parseSchema(val, nestedName);

                    bool isRequired =
                        std::find(required.begin(), required.end(), key) != required.end();
                    if (!isRequired)
                    {
                        GenericType opt;
                        opt.reifiedType = ReifiedTypeId::Optional;
                        opt.args.push_back(std::move(fieldType));
                        f.type = std::make_unique<Type>(std::move(opt));
                    }
                    else
                    {
                        f.type = std::move(fieldType);
                    }

                    if (val.contains("description"))
                        f.attributes.push_back(
                            {"description", val["description"].get<std::string>()});
                    if (val.contains("default"))
                        f.attributes.push_back({"default", val["default"].dump()});

                    s.members.push_back(std::move(f));
                }
            }

            structs.push_back(std::move(s));

            StructRefType ref;
            ref.srcTypeString = name;
            ref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(ref));
        }

        // anyOf / oneOf
        if (j.contains("anyOf") || j.contains("oneOf"))
        {
            auto& opts = j.contains("anyOf") ? j["anyOf"] : j["oneOf"];
            for (auto& opt : opts)
            {
                // Collect ALL types
                std::vector<std::unique_ptr<Type>> types;
                bool hasNull = false;

                for (auto& opt : opts)
                {
                    if (opt.contains("type") && opt["type"].get<std::string>() == "null")
                        hasNull = true;
                    else
                        types.push_back(parseSchema(opt, name));
                }

                // Create Variant with all types
                if (types.size() > 1)
                {
                    GenericType g;
                    g.reifiedType = ReifiedTypeId::Variant;
                    g.args = std::move(types);
                    return std::make_unique<Type>(std::move(g));
                }
            }
        }

        // allOf
        if (j.contains("allOf") && !j["allOf"].empty())
        {
            return parseSchema(j["allOf"][0], name);
        }

        // Fallback
        SimpleType st;
        st.srcTypeString = "any";
        st.reifiedType = ReifiedTypeId::Unknown;
        return std::make_unique<Type>(std::move(st));
    }

    std::unique_ptr<Type> parseSchema2(const json& j, const std::string& name)
    {
        // Handle $ref
        if (j.contains("$ref"))
        {
            std::string ref = j["$ref"].get<std::string>();
            auto pos = ref.rfind('/');
            std::string refName = ref.substr(pos + 1);

            if (defs.contains(refName) && parsed.find(refName) == parsed.end())
            {
                parseSchema(defs[refName], refName);
            }

            StructRefType sref;
            sref.srcTypeString = refName;
            sref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(sref));
        }

        // Get type
        std::string type;
        if (j.contains("type"))
        {
            if (j["type"].is_array())
                type = j["type"][0].get<std::string>();
            else
                type = j["type"].get<std::string>();
        }
        else if (j.contains("properties"))
        {
            type = "object";
        }

        // ENUM - check BEFORE primitives!
        if (j.contains("enum"))
        {
            if (parsed.find(name) != parsed.end())
            {
                StructRefType ref;
                ref.srcTypeString = name;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(ref));
            }
            parsed.insert(name);

            Enum e;
            e.name = name;
            e.scoped = true;
            int num = 0;
            for (auto& val : j["enum"])
            {
                EnumValue ev;
                ev.name = val.is_string() ? val.get<std::string>() : val.dump();
                ev.number = num++;
                e.values.push_back(std::move(ev));
            }
            enums.push_back(std::move(e));

            StructRefType ref;
            ref.srcTypeString = name;
            ref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(ref));
        }

        // Primitives
        if (type == "string")
        {
            SimpleType st;
            st.reifiedType = ReifiedTypeId::String;
            return std::make_unique<Type>(std::move(st));
        }
        if (type == "integer")
        {
            SimpleType st;
            st.reifiedType = ReifiedTypeId::Int64;
            return std::make_unique<Type>(std::move(st));
        }
        if (type == "number")
        {
            SimpleType st;
            st.reifiedType = ReifiedTypeId::Float64;
            return std::make_unique<Type>(std::move(st));
        }
        if (type == "boolean")
        {
            SimpleType st;
            st.reifiedType = ReifiedTypeId::Bool;
            return std::make_unique<Type>(std::move(st));
        }
        if (type == "null")
        {
            SimpleType st;
            st.srcTypeString = "null";
            st.reifiedType = ReifiedTypeId::Unknown;
            return std::make_unique<Type>(std::move(st));
        }

        // Array
        if (type == "array")
        {
            GenericType g;
            g.reifiedType = ReifiedTypeId::List;
            if (j.contains("items"))
                g.args.push_back(parseSchema(j["items"], name + "Item"));
            else
            {
                SimpleType any;
                any.srcTypeString = "any";
                any.reifiedType = ReifiedTypeId::Unknown;
                g.args.push_back(std::make_unique<Type>(std::move(any)));
            }
            return std::make_unique<Type>(std::move(g));
        }

        // Object
        if (type == "object" || j.contains("properties"))
        {
            if (parsed.find(name) != parsed.end())
            {
                StructRefType ref;
                ref.srcTypeString = name;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                return std::make_unique<Type>(std::move(ref));
            }
            parsed.insert(name);

            Struct s;
            s.name = name;

            if (j.contains("description"))
                s.attributes.push_back({"description", j["description"].get<std::string>()});

            std::vector<std::string> required;
            if (j.contains("required"))
                for (auto& r : j["required"])
                    required.push_back(r.get<std::string>());

            if (j.contains("properties"))
            {
                for (auto& [key, val] : j["properties"].items())
                {
                    Field f;
                    f.name = key;

                    std::string nestedName = name + capitalize(key);
                    auto fieldType = parseSchema(val, nestedName);

                    bool isRequired =
                        std::find(required.begin(), required.end(), key) != required.end();
                    if (!isRequired)
                    {
                        GenericType opt;
                        opt.reifiedType = ReifiedTypeId::Optional;
                        opt.args.push_back(std::move(fieldType));
                        f.type = std::make_unique<Type>(std::move(opt));
                    }
                    else
                    {
                        f.type = std::move(fieldType);
                    }

                    if (val.contains("description"))
                        f.attributes.push_back(
                            {"description", val["description"].get<std::string>()});
                    if (val.contains("default"))
                        f.attributes.push_back({"default", val["default"].dump()});

                    s.members.push_back(std::move(f));
                }
            }

            if (j.contains("additionalProperties") && j["additionalProperties"].is_object())
            {
                Field f;
                f.name = "additionalProperties";
                GenericType g;
                g.reifiedType = ReifiedTypeId::Map;
                SimpleType key;
                key.srcTypeString = "string";
                key.reifiedType = ReifiedTypeId::String;
                g.args.push_back(std::make_unique<Type>(std::move(key)));
                g.args.push_back(parseSchema(j["additionalProperties"], name + "Value"));
                f.type = std::make_unique<Type>(std::move(g));
                s.members.push_back(std::move(f));
            }

            structs.push_back(std::move(s));

            StructRefType ref;
            ref.srcTypeString = name;
            ref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(ref));
        }

        // anyOf / oneOf
        if (j.contains("anyOf") || j.contains("oneOf"))
        {
            auto& opts = j.contains("anyOf") ? j["anyOf"] : j["oneOf"];
            for (auto& opt : opts)
            {
                if (!opt.contains("type") || opt["type"].get<std::string>() != "null")
                    return parseSchema(opt, name);
            }
        }

        // allOf
        if (j.contains("allOf") && !j["allOf"].empty())
        {
            return parseSchema(j["allOf"][0], name);
        }

        // Fallback
        SimpleType st;
        st.srcTypeString = "any";
        st.reifiedType = ReifiedTypeId::Unknown;
        return std::make_unique<Type>(std::move(st));
    }
};

} // namespace bhw
