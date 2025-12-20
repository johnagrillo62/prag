#pragma once
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <optional>

namespace meta {
namespace csv {

// Helper to write CSV values with proper escaping
template<typename T>
void writeCSVValue(std::ostringstream& os, const T& value) {
    if constexpr (std::is_same_v<T, std::string>) {
        const std::string& str = value;
        if (str.find(',') != std::string::npos || 
            str.find('"') != std::string::npos || 
            str.find('\n') != std::string::npos || 
            str.find('\r') != std::string::npos) {
            
            // Escape the field by wrapping in quotes and doubling internal quotes
            os << '"';
            for (char c : str) {
                if (c == '"') {
                    os << "\"\"";
                } else {
                    os << c;
                }
            }
            os << '"';
        } else {
            os << str;
        }
    } else {
        // For non-string types, just output directly
        os << value;
    }
}

// Helper to get CSV column name from mapping
template<typename FieldMeta>
std::string getCSVColumnName(const FieldMeta& fieldMeta) {
    // Check if csv_column exists in mapping
  if (fieldMeta.getCsvColumn().has_value()) {
    return fieldMeta.getCsvColumn().value();
    }
    // Fall back to fieldName
    return fieldMeta.fieldName;
}

// Helper to check if field should be skipped (always false for now, can be extended)
template<typename FieldMeta>
bool shouldSkipField(const FieldMeta& fieldMeta) {
    // Could check for special properties or annotations in the future
    return false;
}

// Main CSV serialization function
template <typename ObjectType>
std::string serialize(const std::vector<ObjectType>& objects, const std::string& delimiter = ",")
{
    std::ostringstream os;
    if (objects.empty()) {
        return "";
    }
    
    constexpr auto& fields = meta::MetaTuple<ObjectType>::fields;
    
    // Write header
    std::apply([&](auto&&... fieldMeta) {
        bool first = true;
        auto writeHeader = [&](const auto& field) {
            if (!shouldSkipField(field)) {
                if (!first) os << delimiter;
                first = false;
                os << getCSVColumnName(field);
            }
        };
        (writeHeader(fieldMeta), ...);
        os << "\n";
    }, fields);
    
    // Write data rows
    for (const auto& obj : objects) {
        std::apply([&](auto&&... fieldMeta) {
            bool first = true;
            auto writeData = [&](const auto& field) {
                if (!shouldSkipField(field)) {
                    if (!first) os << delimiter;
                    first = false;
                    
                    if constexpr (field.memberPtr != nullptr) {
                        auto value = obj.*(field.memberPtr);
                        writeCSVValue(os, value);
                    }
                }
            };
            (writeData(fieldMeta), ...);
            os << "\n";
        }, fields);
    }
    
    return os.str();
}

// Advanced CSV serialization with more options
template <typename ObjectType>
std::string serializeAdvanced(const std::vector<ObjectType>& objects, 
                             const std::string& delimiter = ",",
                             bool includeHeader = true,
                             bool escapeStrings = true)
{
    std::ostringstream os;
    if (objects.empty()) {
        return "";
    }
    
    constexpr auto& fields = meta::MetaTuple<ObjectType>::fields;
    
    // Write header if requested
    if (includeHeader) {
        std::apply([&](auto&&... fieldMeta) {
            bool first = true;
            auto writeHeader = [&](const auto& field) {
                if (!shouldSkipField(field)) {
                    if (!first) os << delimiter;
                    first = false;
                    os << getCSVColumnName(field);
                }
            };
            (writeHeader(fieldMeta), ...);
            os << "\n";
        }, fields);
    }
    
    // Write data rows
    for (const auto& obj : objects) {
        std::apply([&](auto&&... fieldMeta) {
            bool first = true;
            auto writeData = [&](const auto& field) {
                if (!shouldSkipField(field)) {
                    if (!first) os << delimiter;
                    first = false;
                    
                    if constexpr (field.memberPtr != nullptr) {
                        auto value = obj.*(field.memberPtr);
                        if (escapeStrings) {
                            writeCSVValue(os, value);
                        } else {
                            os << value;
                        }
                    }
                }
            };
            (writeData(fieldMeta), ...);
            os << "\n";
        }, fields);
    }
    
    return os.str();
}

// Get CSV headers as vector of strings
template <typename ObjectType>
std::vector<std::string> getHeaders()
{
    std::vector<std::string> headers;
    constexpr auto& fields = meta::MetaTuple<ObjectType>::fields;
    
    std::apply([&](auto&&... fieldMeta) {
        auto addHeader = [&](const auto& field) {
            if (!shouldSkipField(field)) {
                headers.push_back(getCSVColumnName(field));
            }
        };
        (addHeader(fieldMeta), ...);
    }, fields);
    
    return headers;
}

// Get field count (excluding skipped fields)
template <typename ObjectType>
size_t getFieldCount()
{
    constexpr auto& fields = meta::MetaTuple<ObjectType>::fields;
    size_t count = 0;
    
    std::apply([&](auto&&... fieldMeta) {
        auto countField = [&](const auto& field) {
            if (!shouldSkipField(field)) {
                count++;
            }
        };
        (countField(fieldMeta), ...);
    }, fields);
    
    return count;
}

// Serialize single object to CSV row
template <typename ObjectType>
std::string serializeRow(const ObjectType& obj, const std::string& delimiter = ",")
{
    std::ostringstream os;
    constexpr auto& fields = meta::MetaTuple<ObjectType>::fields;
    
    std::apply([&](auto&&... fieldMeta) {
        bool first = true;
        auto writeData = [&](const auto& field) {
            if (!shouldSkipField(field)) {
                if (!first) os << delimiter;
                first = false;
                
                if constexpr (field.memberPtr != nullptr) {
                    auto value = obj.*(field.memberPtr);
                    writeCSVValue(os, value);
                }
            }
        };
        (writeData(fieldMeta), ...);
    }, fields);
    
    return os.str();
}

// Get header row as string
template <typename ObjectType>
std::string getHeaderRow(const std::string& delimiter = ",")
{
    std::ostringstream os;
    constexpr auto& fields = meta::MetaTuple<ObjectType>::fields;
    
    std::apply([&](auto&&... fieldMeta) {
        bool first = true;
        auto writeHeader = [&](const auto& field) {
            if (!shouldSkipField(field)) {
                if (!first) os << delimiter;
                first = false;
                os << getCSVColumnName(field);
            }
        };
        (writeHeader(fieldMeta), ...);
    }, fields);
    
    return os.str();
}

// Debug: Print field mappings
template <typename ObjectType>
void printFieldMappings()
{
    std::cout << "CSV Field Mappings:\n";
    constexpr auto& fields = meta::MetaTuple<ObjectType>::fields;
    
    std::apply([](auto&&... fieldMeta) {
        auto printField = [](const auto& field) {
            std::cout << "  C++ member: " << field.fieldName;
            
            if (shouldSkipField(field)) {
                std::cout << " -> SKIPPED";
            } else {
                std::string csvName = getCSVColumnName(field);
                if (csvName != field.fieldName) {
                    std::cout << " -> CSV column: " << csvName;
                } else {
                    std::cout << " (no rename)";
                }
            }
            std::cout << "\n";
        };
        (printField(fieldMeta), ...);
    }, fields);
}

// Utility: Check if type has CSV column mappings
template <typename ObjectType>
bool hasCSVMappings()
{
    constexpr auto& fields = meta::MetaTuple<ObjectType>::fields;
    bool hasMappings = false;
    
    std::apply([&](auto&&... fieldMeta) {
        auto checkField = [&](const auto& field) {
	  if (field.getCsvColumn().has_value()) {
                hasMappings = true;
            }
        };
        (checkField(fieldMeta), ...);
    }, fields);
    
    return hasMappings;
}

} // namespace csv
} // namespace meta
