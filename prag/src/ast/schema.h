#pragma once

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "enums.h"

namespace schema
{

struct ValidationError;
using OptInt = std::optional<int64_t>;
using OptFlt = std::optional<double>;
using OptErr = std::optional<ValidationError>;


// Field requirement - explicit is better than implicit
enum class FieldRequirement
{
    Required,
    Optional
};

// Validation error
struct ValidationError
{
    std::string path;
    std::string message;
};

// Validation result
struct ValidationResult
{
    bool valid = true;
    std::vector<ValidationError> errors;

    void addError(const std::string& path, const std::string& msg)
    {
        valid = false;
        errors.push_back({path, msg});
    }
};

// Base field class
class Field
{
  public:
    std::string name;
    FieldRequirement requirement;

    Field(const std::string& n, FieldRequirement req) : name(n), requirement(req)
    {
    }

    virtual ~Field() = default;
    virtual auto validate(const std::string& value) -> OptErr const = 0;
};

// String field - accepts any string
class StringField : public Field
{
  public:
    StringField(const std::string& n, FieldRequirement req) : Field(n, req)
    {
    }

    auto validate(const std::string& value) -> OptErr const override
    {
        return std::nullopt;
    }
};

// Integer field - with optional range
class IntegerField : public Field
{
  public:
    OptInt mMinValue{};
    OptInt mMaxValue{};

    IntegerField(const std::string& n, FieldRequirement req, OptInt minVal = {}, OptInt maxVal = {})
        : Field(n, req),
          mMinValue(minVal),
          mMaxValue(maxVal)
    {
    }

    auto validate(const std::string& value) -> OptErr const override;

  private:
    auto parseInteger(const std::string& s) const -> OptInt;
};

// Float field - with optional range
class FloatField : public Field
{
  public:
    OptFlt mMinValue{};
    OptFlt mMaxValue{};

    FloatField(const std::string& n, FieldRequirement req, OptFlt minVal = {}, OptFlt maxVal = {})
        : Field(n, req),
          mMinValue(minVal),
          mMaxValue(maxVal)
    {
    }

    auto validate(const std::string& value) -> OptErr const override;

  private:
    std::optional<double> parseFloat(const std::string& s) const;
};

// Enum field - validates against bhw::EnumMapping directly
class EnumField : public Field
{
  private:
    // Store a function pointer to the validation logic
    bool (*validateFn_)(const std::string&, std::string&) = nullptr;
    std::vector<std::string> cachedAllowedValues_; // For AST conversion

  public:
    // Constructor for manually provided values
    EnumField(const std::string& n, FieldRequirement req, const std::vector<std::string>& values)
        : Field(n, req),
          cachedAllowedValues_(values)
    {
        // Capture values in a static to avoid allocation
        static std::vector<std::string> storedValues = values;
        validateFn_ = +[](const std::string& value, std::string& errorMsg) -> bool
        {
            auto it = std::find(storedValues.begin(), storedValues.end(), value);
            if (it == storedValues.end())
            {
                errorMsg = "[";
                for (size_t i = 0; i < storedValues.size(); i++)
                {
                    errorMsg += storedValues[i];
                    if (i < storedValues.size() - 1)
                        errorMsg += ", ";
                }
                errorMsg += "]";
           
                return false;
            }
            return true;
        };
    }

    template <typename EnumT>
    EnumField(const std::string& n, FieldRequirement req, std::type_identity<EnumT>) : Field(n, req)
    {
        // Cache allowed values for AST conversion
        using Traits = typename bhw::EnumMapping<EnumT>::Type;
        for (auto [e, s] : Traits::mapping)
        {
            cachedAllowedValues_.push_back(std::string(s));
        }

        validateFn_ = +[](const std::string& value, std::string& errorMsg) -> bool
        {
            using Traits = typename bhw::EnumMapping<EnumT>::Type;

            // Query mapping directly
            for (auto [e, s] : Traits::mapping)
            {
                if (std::string(s) == value)
                {
                    return true;
                }
            }

            // Build error message
            errorMsg = "[";
            bool first = true;
            for (auto [e, s] : Traits::mapping)
            {
                std::cout << " map " << s << "\n";
                if (!first)
                    errorMsg += ", ";
                errorMsg += s;
                first = false;
            }
            errorMsg += "]";
            
            return false;
        };
    }

    auto validate(const std::string& value) -> OptErr const override
    {
        std::string errorMsg;
        if (!validateFn_(value, errorMsg))
        {
            return ValidationError{
                name,
                "Invalid enum value '" + value + "'. Expected one of: " + errorMsg
            };
        }
        return std::nullopt;
    }

    // Get allowed values for AST conversion
    const std::vector<std::string>& allowedValues() const
    {
        return cachedAllowedValues_;
    }
};

// Simple YAML-like node
class YAMLNode
{
  public:
    std::map<std::string, std::string> values;

    bool has(const std::string& key) const
    {
        return values.find(key) != values.end();
    }

    std::string get(const std::string& key) const
    {
        auto it = values.find(key);
        return (it != values.end()) ? it->second : "";
    }

    void set(const std::string& key, const std::string& value)
    {
        values[key] = value;
    }
};

class Schema
{
  public:
    // Constructor that takes unique_ptrs
    Schema(std::initializer_list<std::unique_ptr<Field>> fields)
    {
        for (auto& field : fields)
        {
            fields_.push_back(std::move(const_cast<std::unique_ptr<Field>&>(field)));
        }
    }

    // Variadic constructor that wraps fields automatically
    template <typename... Fields> static Schema create(Fields&&... fields)
    {
        std::vector<std::unique_ptr<Field>> fieldVec;
        (fieldVec.push_back(std::make_unique<std::decay_t<Fields>>(std::forward<Fields>(fields))),
         ...);

        Schema schema({});
        schema.fields_ = std::move(fieldVec);
        return schema;
    }

    // Validate a YAML node
    //ValidationResult validate(const YAMLNode& node) const;

    // Get the fields (for AST conversion)
    const std::vector<std::unique_ptr<Field>>& fields() const
    {
        return fields_;
    }

  private:
    std::vector<std::unique_ptr<Field>> fields_;
};

} // namespace schema