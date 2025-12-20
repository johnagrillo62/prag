module core.yaml;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple;
import std.array : replicate;

// ============================================================================
// YAML NAMESPACE GENERATORS
// ============================================================================

string yamlNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    return format("# Namespace: %s\n", namespaceParts.join("."));
}

string yamlNamespaceEnd(string[] namespaceParts, bool nested) {
    return "";
}

string toYamlNamespaceStyle(string input) {
    return input.toLower;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyYamlModifier(string baseType, FieldModifier modifier) {
    return baseType;
}

// ============================================================================
// YAML SCHEMA GENERATOR
// ============================================================================

string generateYamlClass(T)(string className, int depth = 0) {
    auto info = languageRegistry[Language.YAML];
    string indent = "  ".replicate(depth);
    string result = "";
    
    // Generate nested classes first
    result ~= generateNestedClasses!T(Language.YAML, depth);
    
    // YAML header
    result ~= format("# YAML Schema for %s\n", className);
    result ~= format("# Generated from D struct: %s\n", T.stringof);
    result ~= "---\n\n";
    
    // Schema definition
    result ~= format("%s:\n", className.toLower);
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string customName = getCustomYAMLName!(T, fieldName);
            string yamlType = mapToYAMLType!FieldType();
            string defaultValue = getYAMLDefaultValue!FieldType();
            
            result ~= format("  %s: %s  # type: %s\n", customName, defaultValue, yamlType);
        }
    }
    
    // Add example section
    result ~= "\n# Example usage:\n";
    result ~= format("# %s:\n", className.toLower);
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            string customName = getCustomYAMLName!(T, fieldName);
            result ~= format("#   %s: example_value\n", customName);
        }
    }
    
    return result;
}

// Helper to map D types to YAML type descriptions
string mapToYAMLType(T)() {
    static if (is(T == int) || is(T == uint) || is(T == byte) || is(T == ubyte) ||
               is(T == short) || is(T == ushort) || is(T == long) || is(T == ulong)) {
        return "integer";
    } else static if (is(T == float) || is(T == double)) {
        return "float";
    } else static if (is(T == bool)) {
        return "boolean";
    } else static if (is(T == string)) {
        return "string";
    } else static if (isArray!T) {
        return "array";
    } else static if (isAssociativeArray!T) {
        return "object";
    } else {
        return "mixed";
    }
}

// Helper to get YAML default values
string getYAMLDefaultValue(T)() {
    static if (is(T == int) || is(T == uint) || is(T == byte) || is(T == ubyte) ||
               is(T == short) || is(T == ushort) || is(T == long) || is(T == ulong)) {
        return "0";
    } else static if (is(T == float) || is(T == double)) {
        return "0.0";
    } else static if (is(T == bool)) {
        return "false";
    } else static if (is(T == string)) {
        return "\"\"";
    } else static if (isArray!T) {
        return "[]";
    } else static if (isAssociativeArray!T) {
        return "{}";
    } else {
        return "null";
    }
}

// Helper to get custom YAML name
string getCustomYAMLName(T, string fieldName)() {
    import std.traits : getUDAs;
    alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
    static if (fieldAttrs.length > 0 && fieldAttrs[0].yamlName.length > 0) {
        return fieldAttrs[0].yamlName;
    } else {
        return fieldName.toLower;
    }
}

// ============================================================================
// YAML LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.YAML, LanguageInfo(
        config: LanguageConfig(
            fileExt: "yaml",
            commentStyle: "#",
            indentStyle: "  "
        ),
        
        primitives: PrimitiveTypes(
            bool_: ["boolean", "false"],
            string_: ["string", "\"\""],
            float_: ["float", "0.0"],
            double_: ["float", "0.0"],
            byte_: ["integer", "0"],
            ubyte_: ["integer", "0"],
            short_: ["integer", "0"],
            ushort_: ["integer", "0"],
            int_: ["integer", "0"],
            uint_: ["integer", "0"],
            long_: ["integer", "0"],
            ulong_: ["integer", "0"]
        ),
        
        complex: ComplexTypes(
            vector_: ["array", "[]"],
            map_: ["object", "{}"],
            optional_: ["%s", "null"],
            tuple_: ["[%s]", "[]"],
        ),
        
        namespace: NamespaceConfig(
            separator: ".",
            keyword: "",
            startGenerator: &yamlNamespaceStart,
            endGenerator: &yamlNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toLowercase,
            privateMemberName: &toLowercase,
            publicMemberName: &toLowercase,	    
            fileName: &toFileSnakeCase,
            namespaceName: &toYamlNamespaceStyle
        ),
        
    ));
}
