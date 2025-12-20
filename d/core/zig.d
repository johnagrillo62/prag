module core.zig;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple;
import std.array : replicate;

// ============================================================================
// ZIG NAMESPACE GENERATORS
// ============================================================================

string zigNamespaceStart(string[] namespaceParts, bool nested) {
    return "";  // Zig uses file structure
}

string zigNamespaceEnd(string[] namespaceParts, bool nested) {
    return "";
}

string toZigNamespaceStyle(string input) {
    return input.toLower;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyZigModifier(string baseType, FieldModifier modifier) {
    final switch(modifier) {
        case FieldModifier.None: 
            return baseType;
        case FieldModifier.Optional: 
            return format("?%s", baseType);
        case FieldModifier.Unique: 
            return baseType;
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
// ZIG CODE GENERATORS
// ============================================================================

string generateZigConstructor(string className, string[] paramList) {
    return format("    pub fn init() %s {\n        return .{};\n    }", className);
}

string generateZigGetter(string fieldName, string typeName, string className, 
                        AccessLevel access, bool isStatic) {
    return "";  // Zig uses direct field access
}

string generateZigSetter(string fieldName, string typeName, string className, 
                        AccessLevel access, bool isStatic) {
    return "";  // Zig uses direct field access
}

// ============================================================================
// ZIG STRUCT GENERATOR
// ============================================================================

string generateZigClass(T)(string className, int depth = 0) {
    auto info = languageRegistry[Language.ZIG];
    string indent = "    ".replicate(depth);
    string result = "";
    
    result ~= generateNestedClasses!T(Language.ZIG, depth);
    
    result ~= indent ~ format("pub const %s = struct {\n", className);
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            result ~= indent ~ format("    %s: %s,\n", fieldName, fieldType);
        }
    }
    
    result ~= "\n" ~ indent ~ format("    pub fn init() %s {\n", className);
    result ~= indent ~ "        return .{\n";
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string defaultValue = getDefaultValue!FieldType(info);
            if (defaultValue.length > 0) {
                result ~= indent ~ format("            .%s = %s,\n", fieldName, defaultValue);
            }
        }
    }
    
    result ~= indent ~ "        };\n";
    result ~= indent ~ "    }\n";
    result ~= indent ~ "};\n";
    
    return result;
}

// ============================================================================
// ZIG LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.ZIG, LanguageInfo(
        config: LanguageConfig(
            fileExt: "zig",
            commentStyle: "//",
            indentStyle: "    "
        ),
        
        primitives: PrimitiveTypes(
            bool_: ["bool", "false"],
            string_: ["[]const u8", `""`],
            float_: ["f32", "0.0"],
            double_: ["f64", "0.0"],
            byte_: ["i8", "0"],
            ubyte_: ["u8", "0"],
            short_: ["i16", "0"],
            ushort_: ["u16", "0"],
            int_: ["i32", "0"],
            uint_: ["u32", "0"],
            long_: ["i64", "0"],
            ulong_: ["u64", "0"]
        ),
        
        complex: ComplexTypes(
         vector_: ["[]%s", "null"],
         map_: ["std.AutoHashMap(%s, %s)", "null"],  // Two %s for key and value
         optional_: ["?%s", "null"],
	 tuple_: ["struct { %s }", "struct{}"]
       ),

        
        namespace: NamespaceConfig(
            separator: ".",
            keyword: "",
            startGenerator: &zigNamespaceStart,
            endGenerator: &zigNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toPascalCase,
            privateMemberName: &toSnakeCase,
	    publicMemberName: &toSnakeCase,
            fileName: &toSnakeCase,
            namespaceName: &toZigNamespaceStyle
        ),
        
        generators: CodeGenerators(
            getter: &generateZigGetter,
            setter: &generateZigSetter,
            typeModifier: &applyZigModifier,
            constructor: &generateZigConstructor
        )
    ));
}

