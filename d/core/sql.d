module core.sql;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple;
import std.array : replicate;

// ============================================================================
// SQL NAMESPACE GENERATORS (SCHEMA)
// ============================================================================

string sqlNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    return format("-- Schema: %s\n", namespaceParts.join("."));
}

string sqlNamespaceEnd(string[] namespaceParts, bool nested) {
    return "";
}

string toSqlNamespaceStyle(string input) {
    return input.toLower;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applySqlModifier(string baseType, FieldModifier modifier) {
    final switch(modifier) {
        case FieldModifier.None: 
            return baseType;
        case FieldModifier.Optional: 
            return baseType ~ " NULL";
        case FieldModifier.Unique: 
            return baseType ~ " UNIQUE";
        case FieldModifier.Shared: 
            return baseType;
        case FieldModifier.Weak: 
            return baseType;
        case FieldModifier.Immutable: 
            return baseType;
        case FieldModifier.Static: 
            return baseType;
    }
}

// ============================================================================
// SQL TABLE GENERATOR
// ============================================================================

string generateSqlClass(T)(string className, int depth = 0) {
    auto info = languageRegistry[Language.SQL];
    string indent = "    ".replicate(depth);
    string result = "";
    
    string tableName = getCustomTableName!T();
    string schema = getCustomSchema!T();
    
    // Table comment
    result ~= format("-- Table: %s\n", tableName);
    result ~= format("-- Generated from D struct: %s\n\n", T.stringof);
    
    // CREATE TABLE statement
    if (schema.length > 0) {
        result ~= format("CREATE TABLE %s.%s (\n", schema, tableName);
    } else {
        result ~= format("CREATE TABLE %s (\n", tableName);
    }
    
    // Columns
    string[] columns;
    static foreach (i, fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string columnName = getCustomColumnName!(T, fieldName);
            string sqlType = mapToSQLType!FieldType();
            
            columns ~= format("    %s %s", columnName, sqlType);
        }
    }
    
    result ~= columns.join(",\n") ~ "\n";
    result ~= ");\n\n";
    
    // Indexes
    result ~= format("-- Indexes for %s\n", tableName);
    result ~= format("-- CREATE INDEX idx_%s_id ON %s(id);\n\n", tableName, tableName);
    
    // Comments
    result ~= format("-- Column descriptions for %s:\n", tableName);
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string columnName = getCustomColumnName!(T, fieldName);
            result ~= format("-- %s: %s\n", columnName, FieldType.stringof);
        }
    }
    
    return result;
}

// Helper to map D types to SQL types
string mapToSQLType(T)() {
    static if (is(T == int) || is(T == uint)) {
        return "INTEGER";
    } else static if (is(T == long) || is(T == ulong)) {
        return "BIGINT";
    } else static if (is(T == short) || is(T == ushort)) {
        return "SMALLINT";
    } else static if (is(T == byte) || is(T == ubyte)) {
        return "TINYINT";
    } else static if (is(T == float)) {
        return "REAL";
    } else static if (is(T == double)) {
        return "DOUBLE PRECISION";
    } else static if (is(T == bool)) {
        return "BOOLEAN";
    } else static if (is(T == string)) {
        return "VARCHAR(255)";
    } else static if (isArray!T) {
        return "TEXT";  // Store as JSON
    } else static if (isAssociativeArray!T) {
        return "TEXT";  // Store as JSON
    } else {
        return "TEXT";
    }
}

// ============================================================================
// SQL LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.SQL, LanguageInfo(
        config: LanguageConfig(
            fileExt: "sql",
            commentStyle: "--",
            indentStyle: "    "
        ),
        
        primitives: PrimitiveTypes(
            bool_: ["BOOLEAN"],
            string_: ["VARCHAR(255)"],
            float_: ["REAL"],
            double_: ["DOUBLE PRECISION"],
            byte_: ["TINYINT"],
            ubyte_: ["TINYINT"],
            short_: ["SMALLINT"],
            ushort_: ["SMALLINT"],
            int_: ["INTEGER"],
            uint_: ["INTEGER"],
            long_: ["BIGINT"],
            ulong_: ["BIGINT"]
        ),
        
        complex: ComplexTypes(
            vector_: ["TEXT"],    
            map_: ["TEXT"],       
            optional_: ["%s NULL"],
            tuple_: ["", ""],
        ),
        
        namespace: NamespaceConfig(
            separator: ".",
            keyword: "SCHEMA",
            startGenerator: &sqlNamespaceStart,
            endGenerator: &sqlNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toSnakeCase,
            publicMemberName: &toSnakeCase,
            privateMemberName: &toSnakeCase,
            fileName: &toSnakeCase,
            namespaceName: &toSqlNamespaceStyle
        ),
        
    ));
}

