// openapi_walker.h
#pragma once
#include <nlohmann/json.hpp>
#include <stack>

#include "ast_walker.h"

namespace bhw
{

using json = nlohmann::ordered_json;

class OpenApiAstWalker : public AstWalker
{
  public:
    Language getLang() override
    {
        return Language::OpenApi;
    }

  private:
    json schemas;
    std::stack<std::string> structStack;
    std::stack<std::vector<std::string>> requiredStack;
    std::vector<std::string> schemaNames;
    int anonCounter = 0;

    std::string generateHeader(const bhw::Ast&) override
    {
        schemas = json::object();
        schemaNames.clear();
        anonCounter = 0;
        while (!structStack.empty())
            structStack.pop();
        while (!requiredStack.empty())
            requiredStack.pop();
        return "";
    }

    std::string generateFooter(const bhw::Ast&) override
    {
        // Reorder: enums first, then structs
        json reordered = json::object();
        for (auto& name : schemaNames)
        {
            if (schemas[name].contains("enum"))
                reordered[name] = schemas[name];
        }
        for (auto& name : schemaNames)
        {
            if (!schemas[name].contains("enum"))
                reordered[name] = schemas[name];
        }

        json output;
        output["openapi"] = "3.0.0";
        output["info"] = {{"title", "Generated API"}, {"version", "1.0.0"}};
        output["components"]["schemas"] = reordered;
        return output.dump(2) + "\n";
    }

    std::string generateStructOpen(const Struct& s, const WalkContext& ctx) override
    {
        std::string name = s.name;
        if (name.empty() || name == "<anonymous>")
        {
            name = "Anonymous" + std::to_string(anonCounter++);
        }

        schemas[name] = {{"type", "object"}, {"properties", json::object()}};
        schemaNames.push_back(name);
        structStack.push(name);
        requiredStack.push({});
        return "";
    }

    std::string generateStructClose(const Struct&, const WalkContext& ctx) override
    {
        if (structStack.empty())
            return "";

        std::string name = structStack.top();
        auto required = requiredStack.top();

        if (!required.empty())
        {
            schemas[name]["required"] = required;
        }

        structStack.pop();
        requiredStack.pop();
        return "";
    }

    std::string generateField(const Field& field, const WalkContext& ctx) override
    {
        if (structStack.empty())
            return "";

        std::string structName = structStack.top();

        bool isOptional =
            std::holds_alternative<GenericType>(field.type->value) &&
            std::get<GenericType>(field.type->value).reifiedType == ReifiedTypeId::Optional;

        if (!isOptional)
        {
            requiredStack.top().push_back(field.name);
        }

        schemas[structName]["properties"][field.name] = typeToJson(*field.type);
        return "";
    }

    std::string generateEnumOpen(const Enum& e, const WalkContext& ctx) override
    {
        schemas[e.name] = {{"type", "string"}, {"enum", json::array()}};
        schemaNames.push_back(e.name);
        return "";
    }

    std::string generateEnumValue(const EnumValue& val, bool, const WalkContext& ctx) override
    {
        for (auto it = schemaNames.rbegin(); it != schemaNames.rend(); ++it)
        {
            if (schemas.contains(*it) && schemas[*it].contains("enum"))
            {
                schemas[*it]["enum"].push_back(val.name);
                break;
            }
        }
        return "";
    }

    std::string generateEnumClose(const Enum&, const WalkContext& ctx) override
    {
        return "";
    }

    std::string generateOneof(const Oneof& oneof, const WalkContext& ctx) override
    {
        if (structStack.empty())
            return "";

        std::string structName = structStack.top();

        // Build oneOf array with variant objects
        json oneOfArray = json::array();

        for (const auto& field : oneof.fields)
        {
            json variant = {{"type", "object"},
                            {"properties", {{field.name, typeToJson(*field.type)}}},
                            {"required", json::array({field.name})}};
            oneOfArray.push_back(variant);
        }

        // Add oneOf field to parent struct's properties
        schemas[structName]["properties"][oneof.name] = {{"oneOf", oneOfArray}};

        return "";
    }

    json typeToJson(const Type& type)
    {
        if (std::holds_alternative<SimpleType>(type.value))
        {
            auto& st = std::get<SimpleType>(type.value);
            return primitiveToJson(st.reifiedType);
        }
        if (std::holds_alternative<StructRefType>(type.value))
        {
            auto& ref = std::get<StructRefType>(type.value);
            return {{"$ref", "#/components/schemas/" + ref.srcTypeString}};
        }
        if (std::holds_alternative<GenericType>(type.value))
        {
            auto& gt = std::get<GenericType>(type.value);
            switch (gt.reifiedType)
            {
            case ReifiedTypeId::List:
                return {{"type", "array"}, {"items", typeToJson(*gt.args[0])}};
            case ReifiedTypeId::Map:
                return {{"type", "object"}, {"additionalProperties", typeToJson(*gt.args[1])}};
            case ReifiedTypeId::Optional:
                return typeToJson(*gt.args[0]);
            case ReifiedTypeId::Variant:
            {
                json oneOf = json::array();
                for (const auto& arg : gt.args)
                {
                    oneOf.push_back(typeToJson(*arg));
                }
                return {{"oneOf", oneOf}};
            }
            default:
                return {{"type", "string"}};
            }
        }
        return {{"type", "string"}};
    }

    json primitiveToJson(ReifiedTypeId type)
    {
        switch (type)
        {
        case ReifiedTypeId::Bool:
            return {{"type", "boolean"}};
        case ReifiedTypeId::Int32:
            return {{"type", "integer"}, {"format", "int32"}};
        case ReifiedTypeId::Int64:
            return {{"type", "integer"}, {"format", "int64"}};
        case ReifiedTypeId::Float32:
            return {{"type", "number"}, {"format", "float"}};
        case ReifiedTypeId::Float64:
            return {{"type", "number"}, {"format", "double"}};
        case ReifiedTypeId::String:
            return {{"type", "string"}};
        case ReifiedTypeId::Bytes:
            return {{"type", "string"}, {"format", "byte"}};
        case ReifiedTypeId::DateTime:
            return {{"type", "string"}, {"format", "date-time"}};
        default:
            return {{"type", "string"}};
        }
    }
};

} // namespace bhw