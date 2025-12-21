
// avro_walker.h
#pragma once
#include <nlohmann/json.hpp>

#include "ast_walker.h"

namespace bhw
{

using json = nlohmann::ordered_json;

class AvroAstWalker : public AstWalker
{
  public:
    Language getLang() override
    {
        return Language::Avro;
    }

  private:
    json schemas;
    std::string currentSchema;
    size_t currentStructIndex = 0;

    std::string generateHeader(const bhw::Ast&) override
    {
        schemas = json::array();
        return "";
    }

    std::string generateFooter(const bhw::Ast&) override
    {
        // Reorder: enums first, then records
        json reordered = json::array();

        // Enums first
        for (auto& s : schemas)
        {
            if (s["type"] == "enum")
                reordered.push_back(s);
        }
        // Records second
        for (auto& s : schemas)
        {
            if (s["type"] == "record")
                reordered.push_back(s);
        }

        if (reordered.size() == 1)
            return reordered[0].dump(2) + "\n";
        return reordered.dump(2) + "\n";
    }
    std::string generateStructOpen(const Struct& s, size_t) override
    {
        json record;
        record["type"] = "record";
        record["name"] = s.name;
        record["fields"] = json::array();
        schemas.push_back(record);
        currentStructIndex = schemas.size() - 1; // track this struct
        return "";
    }

    std::string generateStructClose(const Struct&, size_t) override
    {
        return "";
    }

    std::string generateField(const Field& field, size_t) override
    {
        json f;
        f["name"] = field.name;
        f["type"] = typeToJson(*field.type);
        schemas[currentStructIndex]["fields"].push_back(f); // use tracked index
        return "";
    }
    std::string generateEnumOpen(const Enum& e, size_t) override
    {
        json en;
        en["type"] = "enum";
        en["name"] = e.name;
        en["symbols"] = json::array();
        schemas.push_back(en);
        // Don't update currentStructIndex - fields still go to parent struct
        return "";
    }

    std::string generateEnumValue(const EnumValue& val, bool, size_t) override
    {
        schemas.back()["symbols"].push_back(val.name);
        return "";
    }

    std::string generateEnumClose(const Enum&, size_t) override
    {
        return "";
    }

    std::string generateOneof(const Oneof& oneof, size_t) override
    {
        // In Avro, oneofs are represented as union types
        json field;
        field["name"] = oneof.name;

        // Create union type - array of possible types
        json unionTypes = json::array();

        // Avro unions typically include null as first option
        unionTypes.push_back("null");

        // Add each oneof field as a record type in the union
        for (const auto& oneofField : oneof.fields)
        {
            json recordType;
            recordType["type"] = "record";
            recordType["name"] = capitalize(oneof.name) + "_" + capitalize(oneofField.name);
            recordType["fields"] = json::array();

            // Add the value field
            json valueField;
            valueField["name"] = "value";
            valueField["type"] = typeToJson(*oneofField.type);
            recordType["fields"].push_back(valueField);

            unionTypes.push_back(recordType);
        }

        field["type"] = unionTypes;
        schemas[currentStructIndex]["fields"].push_back(field);

        return "";
    }

  private:
    std::string capitalize(const std::string& s)
    {
        if (s.empty())
            return s;
        std::string result = s;
        result[0] = std::toupper(static_cast<unsigned char>(result[0]));
        return result;
    }
    json typeToJson(const Type& type)
    {
        if (std::holds_alternative<SimpleType>(type.value))
        {
            auto& st = std::get<SimpleType>(type.value);
            return canonicalToAvro(st.reifiedType);
        }
        if (std::holds_alternative<StructRefType>(type.value))
        {
            auto& ref = std::get<StructRefType>(type.value);
            return ref.srcTypeString;
        }
        if (std::holds_alternative<GenericType>(type.value))
        {
            auto& gt = std::get<GenericType>(type.value);
            switch (gt.reifiedType)
            {
            case ReifiedTypeId::List:
                return {{"type", "array"}, {"items", typeToJson(*gt.args[0])}};
            case ReifiedTypeId::Map:
                return {{"type", "map"}, {"values", typeToJson(*gt.args[1])}};
            case ReifiedTypeId::Optional:
                return json::array({"null", typeToJson(*gt.args[0])});
            case ReifiedTypeId::Variant:
            {
                json unionTypes = json::array();
                for (const auto& arg : gt.args)
                {
                    unionTypes.push_back(typeToJson(*arg));
                }
                return unionTypes;
            }
            default:
                return "bytes";
            }
        }
        return "string";
    }

    std::string canonicalToAvro(ReifiedTypeId type)
    {
        switch (type)
        {
        case ReifiedTypeId::Bool:
            return "boolean";
        case ReifiedTypeId::Int32:
            return "int";
        case ReifiedTypeId::Int64:
            return "long";
        case ReifiedTypeId::Float32:
            return "float";
        case ReifiedTypeId::Float64:
            return "double";
        case ReifiedTypeId::String:
            return "string";
        case ReifiedTypeId::Bytes:
            return "bytes";
        default:
            return "string";
        }
    }
};

} // namespace bhw
