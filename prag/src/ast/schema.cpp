#include "schema.h"

#include <cctype>
#include <charconv>

#include "enums.h"

namespace schema
{

std::optional<int64_t> IntegerField::parseInteger(const std::string& s) const
{
    if (s.empty())
        return std::nullopt;

    int64_t value = 0;
    auto first = s.data();
    auto last = s.data() + s.size();

    auto [ptr, ec] = std::from_chars(first, last, value);

    // Must consume the entire string and not overflow
    if (ec == std::errc() && ptr == last)
    {
        return value;
    }

    return std::nullopt;
}

auto IntegerField::validate(const std::string& intStr) -> OptErr const
{
    const auto parsedInt = parseInteger(intStr);
    if (!parsedInt.has_value())
    {
        return ValidationError{name, "Invalid integer value '" + intStr + "'"};
    }

    auto value = parsedInt.value();

    if (mMinValue.has_value() && value < mMinValue.value())
    {
        return ValidationError{
            name, "Value " + intStr + " below minimum " + std::to_string(mMinValue.value())};
    }

    if (mMaxValue.has_value() && value > mMaxValue.value())
    {
        return ValidationError{
            name, "Value " + intStr + " above maximum " + std::to_string(mMaxValue.value())};
    }
    return std::nullopt;
}

std::optional<double> FloatField::parseFloat(const std::string& s) const
{
    if (s.empty())
        return std::nullopt;

    size_t pos = 0;
    try
    {
        // std::stdod is still the best way
        // even with c++20
        double value = std::stod(s, &pos);

        if (pos != s.length())
            return std::nullopt; // trailing garbage invalid

        return value;
    }
    catch (...)
    {
        return std::nullopt; // invalid format
    }
}

auto FloatField::validate(const std::string& fltStr) -> OptErr const
{
    const auto parsedFlt = parseFloat(fltStr);
    if (!parsedFlt.has_value())
    {
        return ValidationError{name, "Invalid float value '" + fltStr + "'"};
    }

    double value = parsedFlt.value();

    if (mMinValue.has_value() && value < mMinValue.value())
    {
        return ValidationError{
            name, "Value " + fltStr + " below minimum " + std::to_string(mMinValue.value())};
    }

    if (mMaxValue.has_value() && value > mMaxValue.value())
    {
        return ValidationError{
            name, "Value " + fltStr + " above maximum " + std::to_string(mMaxValue.value())};
    }
}

ValidationResult Schema::validate(const YAMLNode& node) const
{
    ValidationResult result;

    for (const auto& field : fields_)
    {
        std::cout << field->name << "\n";
        // Check required fields exist
        if (field->requirement == FieldRequirement::Required && !node.has(field->name))
        {
            result.addError(field->name, "Missing required field");
            continue;
        }

        // Skip optional missing fields
        if (!node.has(field->name))
        {
            continue;
        }

        if (auto error = field->validate(node.get(field->name)))
        {
            std::cout << field->name << "\n";
        
            result.addError(error.value().path, error.value().message);
        }
    }

    // Outer loop over all YAML keys
    // 
    /*
    for (const auto& key : node.keys()) // M = # of keys in YAML
    {
        bool found = false;

        // Inner loop over all schema fields
        for (const auto& field : fields_) // N = # of fields in schema
        {
            if (field->name == key)
            {
                found = true;
                break;
            }
        }

        if (!found)
            result.addError(key, "Unknown field not defined in schema");
    }
    */
    return result;
}

#ifdef YAML_CPP

ValidationResult Schema::validate(const YAML::Node& node) const
{
    ValidationResult result;

    for (const auto& field : fields_)
    {
        // Check required fields
        if (field->requirement == FieldRequirement::Required && !node[field->name])
        {
            result.addError(field->name, "Missing required field");
            continue;
        }

        // Skip optional missing fields
        if (!node[field->name])
            continue;

        // Directly call the field's validate
        if (auto err = field->validate(node[field->name].as<std::string>()))
        {
            result.addError(err->path, err->message);
        }
    }

    return result;
}
#endif


} // namespace schema