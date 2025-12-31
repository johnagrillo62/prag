module core.haskell;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple;
import std.array : replicate, join;

// ============================================================================
// HASKELL NAMESPACE GENERATORS (MODULE)
// ============================================================================

string haskellNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    return format("module %s where\n\n", namespaceParts.join(".").toPascalCase);
}

string haskellNamespaceEnd(string[] namespaceParts, bool nested) {
    return "";
}

string toHaskellNamespaceStyle(string input) {
    return input.toPascalCase;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyHaskellModifier(string baseType, FieldModifier modifier) {
    final switch(modifier) {
        case FieldModifier.None: 
            return baseType;
        case FieldModifier.Optional: 
            return format("Maybe %s", baseType);
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
// HASKELL CODE GENERATORS
// ============================================================================

string generateHaskellConstructor(string className, string[] paramList) {
    return "";
}

string generateHaskellGetter(string fieldName, string typeName, string className, 
                            AccessLevel access, bool isStatic) {
    return "";
}

string generateHaskellSetter(string fieldName, string typeName, string className, 
                            AccessLevel access, bool isStatic) {
    return "";
}

// ============================================================================
// HASKELL DATA TYPE GENERATOR
// ============================================================================

string generateHaskellClass(T)(string className, int depth = 0) {
    auto info = languageRegistry[Language.HASKELL];
    string indent = "  ".replicate(depth);
    string result = "";
    
    result ~= generateNestedClasses!T(Language.HASKELL, depth);
    
    // Data type declaration with record syntax
    result ~= indent ~ format("data %s = %s\n", className, className);
    result ~= indent ~ "  { ";
    
    string[] fields;
    static foreach (i, fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            fields ~= format("%s :: %s", fieldName, fieldType);
        }
    }
    
    result ~= fields.join("\n  , ");
    result ~= "\n  } deriving (Show, Eq)\n";
    
    return result;
}

// ============================================================================
// HASKELL LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.HASKELL, LanguageInfo(
        name: Language.HASKELL,
        
        config: LanguageConfig(
            fileExt: "hs",
            commentStyle: "--",
            indentStyle: "  "
        ),
        
        features: LanguageFeatures(
            hasClasses: false,
            hasGettersSetters: false,
            hasConstructors: false,
            hasNamespaces: true,
            hasTypeModifiers: true
        ),
        
        outputs: [
            OutputFile("hs", GeneratorType.SOURCE)
        ],
        
        primitives: PrimitiveTypes(
            bool_: ["Bool", "False"],
            string_: ["String", `""`],
            float_: ["Float", "0.0"],
            double_: ["Double", "0.0"],
            byte_: ["Int", "0"],
            ubyte_: ["Int", "0"],
            short_: ["Int", "0"],
            ushort_: ["Int", "0"],
            int_: ["Int", "0"],
            uint_: ["Int", "0"],
            long_: ["Integer", "0"],
            ulong_: ["Integer", "0"]
        ),
        
        complex: ComplexTypes(
            vector_: ["[%s]", "[]"],
            map_: ["Map %s %s", "Map.empty"],
            optional_: ["Maybe %s", "Nothing"],
            tuple_: ["(", ")"],
            set_: ["Set %s", "Set.empty"]
        ),
        
        namespace: NamespaceConfig(
            separator: ".",
            keyword: "module",
            startGenerator: &haskellNamespaceStart,
            endGenerator: &haskellNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toPascalCase,
            privateMemberName: &toCamelCase,
            publicMemberName: &toCamelCase,
            fileName: &toPascalCase,
            namespaceName: &toHaskellNamespaceStyle
        ),
        
        generators: CodeGenerators(
            getter: &generateHaskellGetter,
            setter: &generateHaskellSetter,
            typeModifier: &applyHaskellModifier,
            constructor: &generateHaskellConstructor
        )
    ));
}
