#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <type_traits>
#include <concepts>
#include <optional>
#include <iostream>

// ==============================================================================
// DATABASE WRITER FOR METAFRONT REFLECTION SYSTEM
// ==============================================================================

namespace db {

// Helper to get field name from mapping
template<typename FieldMeta>
std::string getFieldName(const FieldMeta& fieldMeta) {
    // Check if table_column exists in mapping
    if (fieldMeta.getSqlColumn().has_value()) {
      return fieldMeta.getSqlColumn().value();
    }
    // Fall back to fieldName
    return fieldMeta.fieldName;
}

// Helper to get table name from metadata
template<typename T>
std::string getTableName() {
    if constexpr (requires { meta::MetaTuple<T>::tableName; }) {
        return std::string(meta::MetaTuple<T>::tableName);
    } else {
        return "";
    }
}

// Helper to check if type has metadata
template<typename T>
concept HasMetadata = requires {
    meta::MetaTuple<T>::fields;
};

// ==============================================================================
// SQL TYPE MAPPING
// ==============================================================================

class SQLTypeMapper {
public:
    static std::string mapCppToSQL(const std::string& cppType) {
        static const std::map<std::string, std::string> typeMap = {
            {"int", "INTEGER"},
            {"int32_t", "INTEGER"},
            {"int64_t", "BIGINT"},
            {"uint32_t", "INTEGER UNSIGNED"},
            {"uint64_t", "BIGINT UNSIGNED"},
            {"float", "FLOAT"},
            {"double", "DOUBLE"},
            {"std::string", "VARCHAR(255)"},
            {"bool", "BOOLEAN"},
            {"char", "CHAR(1)"},
            {"std::chrono::system_clock::time_point", "TIMESTAMP"},
            {"std::vector<std::string>", "JSON"},
            {"std::map<std::string,std::string>", "JSON"}
        };
        
        auto it = typeMap.find(cppType);
        if (it != typeMap.end()) {
            return it->second;
        }
        
        // Handle some common patterns
        if (cppType.find("std::vector<") == 0) return "JSON";
        if (cppType.find("std::map<") == 0) return "JSON";
        if (cppType.find("std::array<") == 0) return "JSON";
        if (cppType.find("std::optional<") == 0) {
            // Extract inner type and map it
            size_t start = cppType.find('<') + 1;
            size_t end = cppType.rfind('>');
            if (start < end) {
                std::string innerType = cppType.substr(start, end - start);
                return mapCppToSQL(innerType);
            }
        }
        
        return "TEXT"; // Default fallback
    }
};

// Helper function to get constraints from field metadata
template<typename FieldMeta>
std::string getConstraints(const FieldMeta& fieldMeta) {
    std::string constraints;
    
    // Check properties
    if constexpr (FieldMeta::properties & meta::Prop::PrimaryKey) {
        constraints += " PRIMARY KEY";
    }
    
    return constraints;
}

// ==============================================================================
// DATABASE WRITER CLASS
// ==============================================================================

template<typename T>
requires HasMetadata<T>
class DatabaseWriter {
private:
    static inline const auto& fields = meta::MetaTuple<T>::fields;
    static inline const std::string tableName = getTableName<T>();
    
public:
    // ==============================================================================
    // CREATE TABLE STATEMENT
    // ==============================================================================
    
    static std::string generateCreateTable() {
        if (tableName.empty()) {
            throw std::runtime_error("Type must have a tableName in MetaTuple");
        }
        
        std::ostringstream sql;
        sql << "CREATE TABLE " << tableName << " (\n";
        
        std::apply([&sql](auto&&... field_metas) {
            bool first = true;
            ((processCreateField(sql, field_metas, first)), ...);
        }, fields);
        
        sql << "\n);";
        return sql.str();
    }
    
private:
    template<typename FieldMeta>
    static void processCreateField(std::ostringstream& sql, const FieldMeta& fieldMeta, bool& first) {
        // Skip private fields for table creation
        if constexpr (FieldMeta::properties & meta::Prop::Private) {
            return;
        }
        
        if (!first) {
            sql << ",\n";
        }
        first = false;
        
        sql << "    " << getFieldName(fieldMeta) << " ";
        
        // Get SQL type from fieldType string
        std::string sqlType = SQLTypeMapper::mapCppToSQL(fieldMeta.fieldType);
        sql << sqlType;
        
        // Get constraints
        sql << db::getConstraints(fieldMeta);
    }
    
public:
    // ==============================================================================
    // INSERT STATEMENT
    // ==============================================================================
    
    static std::string generateInsert() {
        if (tableName.empty()) {
            throw std::runtime_error("Type must have a tableName in MetaTuple");
        }
        
        std::ostringstream sql;
        sql << "INSERT INTO " << tableName << " (";
        
        // Get field names
        std::vector<std::string> fieldNames;
        std::apply([&fieldNames](auto&&... field_metas) {
            ((addInsertField(fieldNames, field_metas)), ...);
        }, fields);
        
        // Add field names to SQL
        for (size_t i = 0; i < fieldNames.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << fieldNames[i];
        }
        
        sql << ") VALUES (";
        
        // Add placeholders
        for (size_t i = 0; i < fieldNames.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << "?";
        }
        
        sql << ")";
        return sql.str();
    }
    
    static std::string generateInsertWithValues(const T& obj) {
        if (tableName.empty()) {
            throw std::runtime_error("Type must have a tableName in MetaTuple");
        }
        
        std::ostringstream sql;
        sql << "INSERT INTO " << tableName << " (";
        
        std::vector<std::string> fieldNames;
        std::vector<std::string> values;
        
        std::apply([&](auto&&... field_metas) {
            ((addInsertFieldWithValue(fieldNames, values, field_metas, obj)), ...);
        }, fields);
        
        // Add field names
        for (size_t i = 0; i < fieldNames.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << fieldNames[i];
        }
        
        sql << ") VALUES (";
        
        // Add values
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << values[i];
        }
        
        sql << ")";
        return sql.str();
    }
    
private:
    template<typename FieldMeta>
    static void addInsertField(std::vector<std::string>& fieldNames, const FieldMeta& fieldMeta) {
        // Include public fields
        if constexpr (!(FieldMeta::properties & meta::Prop::Private)) {
            if constexpr (FieldMeta::memberPtr != nullptr) {
                fieldNames.push_back(getFieldName(fieldMeta));
            }
        }
    }
    
    template<typename FieldMeta>
    static void addInsertFieldWithValue(std::vector<std::string>& fieldNames, 
                                       std::vector<std::string>& values,
                                       const FieldMeta& fieldMeta, 
                                       const T& obj) {
        if constexpr (!(FieldMeta::properties & meta::Prop::Private)) {
            if constexpr (FieldMeta::memberPtr != nullptr) {
                fieldNames.push_back(getFieldName(fieldMeta));
                
                // Get value and convert to SQL string
                auto value = fieldMeta.get(obj);
                values.push_back(valueToSQL(value));
            }
        }
    }
    
public:
    // ==============================================================================
    // UPDATE STATEMENT
    // ==============================================================================
    
    static std::string generateUpdate() {
        if (tableName.empty()) {
            throw std::runtime_error("Type must have a tableName in MetaTuple");
        }
        
        std::ostringstream sql;
        sql << "UPDATE " << tableName << " SET ";
        
        std::vector<std::string> setFields;
        std::apply([&setFields](auto&&... field_metas) {
            ((addUpdateField(setFields, field_metas)), ...);
        }, fields);
        
        for (size_t i = 0; i < setFields.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << setFields[i] << " = ?";
        }
        
        sql << " WHERE id = ?"; // Assuming 'id' is primary key
        return sql.str();
    }
    
    static std::string generateUpdateWithValues(const T& obj) {
        if (tableName.empty()) {
            throw std::runtime_error("Type must have a tableName in MetaTuple");
        }
        
        std::ostringstream sql;
        sql << "UPDATE " << tableName << " SET ";
        
        std::vector<std::string> updates;
        std::string primaryKeyValue;
        
        std::apply([&](auto&&... field_metas) {
            ((addUpdateFieldWithValue(updates, primaryKeyValue, field_metas, obj)), ...);
        }, fields);
        
        for (size_t i = 0; i < updates.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << updates[i];
        }
        
        if (!primaryKeyValue.empty()) {
            sql << " WHERE id = " << primaryKeyValue;
        }
        return sql.str();
    }
    
private:
    template<typename FieldMeta>
    static void addUpdateField(std::vector<std::string>& setFields, const FieldMeta& fieldMeta) {
        if constexpr (!(FieldMeta::properties & meta::Prop::Private)) {
            if constexpr (FieldMeta::memberPtr != nullptr) {
                // Skip primary key fields
                if constexpr (!(FieldMeta::properties & meta::Prop::PrimaryKey)) {
                    setFields.push_back(getFieldName(fieldMeta));
                }
            }
        }
    }
    
    template<typename FieldMeta>
    static void addUpdateFieldWithValue(std::vector<std::string>& updates,
                                       std::string& primaryKeyValue,
                                       const FieldMeta& fieldMeta, 
                                       const T& obj) {
        if constexpr (!(FieldMeta::properties & meta::Prop::Private)) {
            if constexpr (FieldMeta::memberPtr != nullptr) {
                auto value = fieldMeta.get(obj);
                
                if constexpr (FieldMeta::properties & meta::Prop::PrimaryKey) {
                    primaryKeyValue = valueToSQL(value);
                } else {
                    updates.push_back(getFieldName(fieldMeta) + " = " + valueToSQL(value));
                }
            }
        }
    }
    
    // ==============================================================================
    // SELECT STATEMENTS
    // ==============================================================================
    
public:
    static std::string generateSelect() {
        if (tableName.empty()) {
            throw std::runtime_error("Type must have a tableName in MetaTuple");
        }
        return "SELECT * FROM " + tableName;
    }
    
    static std::string generateSelectById() {
        if (tableName.empty()) {
            throw std::runtime_error("Type must have a tableName in MetaTuple");
        }
        return "SELECT * FROM " + tableName + " WHERE id = ?";
    }
    
    static std::string generateDelete() {
        if (tableName.empty()) {
            throw std::runtime_error("Type must have a tableName in MetaTuple");
        }
        return "DELETE FROM " + tableName + " WHERE id = ?";
    }
    
    // ==============================================================================
    // UTILITY FUNCTIONS
    // ==============================================================================
    
    static std::vector<std::string> getFieldNames() {
        std::vector<std::string> names;
        std::apply([&names](auto&&... field_metas) {
            ((addFieldName(names, field_metas)), ...);
        }, fields);
        return names;
    }
    
private:
    template<typename FieldMeta>
    static void addFieldName(std::vector<std::string>& names, const FieldMeta& fieldMeta) {
        if constexpr (!(FieldMeta::properties & meta::Prop::Private)) {
            if constexpr (FieldMeta::memberPtr != nullptr) {
                names.push_back(getFieldName(fieldMeta));
            }
        }
    }
    
    // Convert C++ value to SQL string representation
    template<typename ValueType>
    static std::string valueToSQL(const ValueType& value) {
        if constexpr (std::is_same_v<std::decay_t<ValueType>, std::string>) {
            return "'" + value + "'";
        } else if constexpr (std::is_arithmetic_v<ValueType>) {
            return std::to_string(value);
        } else if constexpr (std::is_same_v<std::decay_t<ValueType>, bool>) {
            return value ? "TRUE" : "FALSE";
        } else {
            // For complex types, you might want to serialize to JSON
            return "'NULL'"; // Placeholder
        }
    }
    
public:
    // ==============================================================================
    // INFORMATION METHODS
    // ==============================================================================
    
    static std::string getTableNameStatic() {
        return tableName;
    }
    
    static constexpr size_t getFieldCount() {
        return std::tuple_size_v<std::decay_t<decltype(fields)>>;
    }
    
    static void printFieldInfo() {
        std::cout << "Table: " << tableName << "\n";
        std::cout << "Fields:\n";
        
        std::apply([](auto&&... field_metas) {
            ((printField(field_metas)), ...);
        }, fields);
    }
    
private:
    template<typename FieldMeta>
    static void printField(const FieldMeta& fieldMeta) {
        std::cout << "  " << fieldMeta.fieldType << " " << fieldMeta.fieldName;
        
        // Print mapping info
        std::cout << " -> table: " << getFieldName(fieldMeta);
        
        std::cout << "\n";
    }
};

// ==============================================================================
// CONVENIENCE FUNCTIONS
// ==============================================================================

template<typename T>
requires HasMetadata<T>
std::string createTable() {
    return DatabaseWriter<T>::generateCreateTable();
}

template<typename T>
requires HasMetadata<T>
std::string insertSQL() {
    return DatabaseWriter<T>::generateInsert();
}

template<typename T>
requires HasMetadata<T>
std::string insertSQL(const T& obj) {
    return DatabaseWriter<T>::generateInsertWithValues(obj);
}

template<typename T>
requires HasMetadata<T>
std::string updateSQL() {
    return DatabaseWriter<T>::generateUpdate();
}

template<typename T>
requires HasMetadata<T>
std::string updateSQL(const T& obj) {
    return DatabaseWriter<T>::generateUpdateWithValues(obj);
}

template<typename T>
requires HasMetadata<T>
std::string selectSQL() {
    return DatabaseWriter<T>::generateSelect();
}

template<typename T>
requires HasMetadata<T>
std::string deleteSQL() {
    return DatabaseWriter<T>::generateDelete();
}

} // namespace db
