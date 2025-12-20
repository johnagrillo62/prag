

module core.typescript;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple;
import std.array : replicate;

// ============================================================================
// TYPESCRIPT NAMESPACE GENERATORS
// ============================================================================

string typescriptNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    
    if (nested) {
        string result = "";
        foreach (part; namespaceParts) {
            result ~= format("namespace %s {\n", part);
        }
        return result;
    } else {
        return format("namespace %s {\n", namespaceParts.join("."));
    }
}

string typescriptNamespaceEnd(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    
    if (nested) {
        string result = "";
        foreach_reverse (part; namespaceParts) {
            result ~= format("}  // namespace %s\n", part);
        }
        return result;
    } else {
        return format("}  // namespace %s\n", namespaceParts.join("."));
    }
}

string toTypescriptNamespaceStyle(string input) {
    return input.toPascalCase;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyTypescriptModifier(string baseType, FieldModifier modifier) {
    final switch(modifier) {
        case FieldModifier.None: 
            return baseType;
        case FieldModifier.Optional: 
            return format("%s | null", baseType);
        case FieldModifier.Unique: 
            return baseType;
        case FieldModifier.Shared: 
            return baseType;
        case FieldModifier.Weak: 
            return format("WeakRef<%s>", baseType);
        case FieldModifier.Immutable: 
            return format("Readonly<%s>", baseType);
        case FieldModifier.Static: 
            return format("static %s", baseType);
    }
}

// ============================================================================
// TYPESCRIPT CODE GENERATORS
// ============================================================================

string generateTypescriptConstructor(string className, string[] paramList) {
    return format("    constructor() {}\n");
}

string generateTypescriptGetter(string fieldName, string typeName, string className, 
                                AccessLevel access, bool isStatic) {
    return format("    get %s(): %s { return this._%s; }", 
        fieldName, typeName, fieldName);
}

string generateTypescriptSetter(string fieldName, string typeName, string className, 
                                AccessLevel access, bool isStatic) {
    return format("    set %s(value: %s) { this._%s = value; }", 
        fieldName, typeName, fieldName);
}

// ============================================================================
// TYPESCRIPT CLASS GENERATOR
// ============================================================================

string generateTypescriptClass(T)(string className, int depth = 0) {
    auto info = languageRegistry[Language.TYPESCRIPT];
    string indent = "    ".replicate(depth);
    string result = "";
    
    // Generate nested classes first
    result ~= generateNestedClasses!T(Language.TYPESCRIPT, depth);
    
    // Class declaration
    result ~= indent ~ format("export class %s {\n", className);
    
    // Private fields
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            string defaultValue = getDefaultValue!FieldType(info);
            
            result ~= indent ~ format("    private _%s: %s", fieldName, fieldType);
            if (defaultValue.length > 0) {
                result ~= format(" = %s", defaultValue);
            }
            result ~= ";\n";
        }
    }
    
    // Constructor
    result ~= "\n" ~ indent ~ "    constructor() {\n";
    
    // Initialize collections
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            static if (isArray!FieldType) {
                result ~= indent ~ format("        this._%s = [];\n", fieldName);
            } else static if (isAssociativeArray!FieldType) {
                result ~= indent ~ format("        this._%s = new Map();\n", fieldName);
            }
        }
    }
    
    result ~= indent ~ "    }\n";
    
    // Getters and setters
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            
            result ~= "\n" ~ indent ~ format("    get %s(): %s {\n", fieldName, fieldType);
            result ~= indent ~ format("        return this._%s;\n", fieldName);
            result ~= indent ~ "    }\n";
            
            result ~= "\n" ~ indent ~ format("    set %s(value: %s) {\n", fieldName, fieldType);
            result ~= indent ~ format("        this._%s = value;\n", fieldName);
            result ~= indent ~ "    }\n";
        }
    }
    
    result ~= indent ~ "}\n";
    return result;
}

// ============================================================================
// TYPESCRIPT LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.TYPESCRIPT, LanguageInfo(
        config: LanguageConfig(
            fileExt: "ts",
            commentStyle: "//",
            indentStyle: "    "
        ),
        
        primitives: PrimitiveTypes(
            bool_: ["boolean", "false"],
            string_: ["string", "''"],
            float_: ["number", "0"],
            double_: ["number", "0"],
            byte_: ["number", "0"],
            ubyte_: ["number", "0"],
            short_: ["number", "0"],
            ushort_: ["number", "0"],
            int_: ["number", "0"],
            uint_: ["number", "0"],
            long_: ["number", "0"],
            ulong_: ["number", "0"]
        ),
        
        complex: ComplexTypes(
            vector_: ["Array<%s>", "[]"],
            map_: ["Map<%s, %s>", "new Map()"],
            optional_: ["%s | null", "null"],
            tuple_: ["[", "]"],
            set_: ["Set<%s>", "new Set()"]
        ),
        
        namespace: NamespaceConfig(
            separator: ".",
            keyword: "namespace",
            startGenerator: &typescriptNamespaceStart,
            endGenerator: &typescriptNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toPascalCase,
            publicMemberName: &toCamelCase,
            privateMemberName: &toCamelCase,
            fileName: &toCamelCase,
            namespaceName: &toTypescriptNamespaceStyle
        ),
        
        generators: CodeGenerators(
            getter: &generateTypescriptGetter,
            setter: &generateTypescriptSetter,
            typeModifier: &applyTypescriptModifier,
            constructor: &generateTypescriptConstructor
        )
    ));
}
