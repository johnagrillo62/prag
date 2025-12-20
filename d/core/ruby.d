module core.ruby;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple, getUDAs;
import std.array : replicate;
import std.algorithm : map;  // Add this for .map



// ============================================================================
// RUBY NAMESPACE GENERATORS (MODULE)
// ============================================================================

string rubyNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    
    string result = "";
    foreach (part; namespaceParts) {
        result ~= format("module %s\n", part.toPascalCase);
    }
    return result;
}

string rubyNamespaceEnd(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    
    string result = "";
    foreach_reverse (part; namespaceParts) {
        result ~= format("end  # module %s\n", part.toPascalCase);
    }
    return result;
}

string toRubyNamespaceStyle(string input) {
    return input.toPascalCase;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyRubyModifier(string baseType, FieldModifier modifier) {
    return baseType;  // Ruby is dynamically typed
}

// ============================================================================
// RUBY CODE GENERATORS
// ============================================================================

string generateRubyConstructor(string className, string[] paramList) {
    return "  def initialize\n  end\n";
}

string generateRubyGetter(string fieldName, string typeName, string className, 
                         AccessLevel access, bool isStatic) {
    return "";  // Handled by attr_reader
}

string generateRubySetter(string fieldName, string typeName, string className, 
                         AccessLevel access, bool isStatic) {
    return "";  // Handled by attr_writer
}

// ============================================================================
// RUBY CLASS GENERATOR
// ============================================================================

string generateRubyClass(T)(string className, int depth = 0) {
    auto info = languageRegistry[Language.RUBY];
    string indent = "  ".replicate(depth);
    string result = "";
    
    result ~= generateNestedClasses!T(Language.RUBY, depth);
    
    result ~= indent ~ format("class %s\n", className);
    
    // Collect field access patterns
    string[] readers;
    string[] writers;
    string[] accessors;
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            import std.traits : getUDAs;
            alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
            
            static if (fieldAttrs.length > 0) {
                AccessLevel access = fieldAttrs[0].access;
                if (access == AccessLevel.ReadOnly) {
                    readers ~= fieldName;
                } else if (access == AccessLevel.Private) {
                    // No accessor
                } else {
                    accessors ~= fieldName;
                }
            } else {
                accessors ~= fieldName;
            }
        }
    }
    
    // Generate attr_* declarations
    if (readers.length > 0) {
        result ~= indent ~ format("  attr_reader %s\n", 
            readers.map!(f => ":" ~ f).join(", "));
    }
    if (writers.length > 0) {
        result ~= indent ~ format("  attr_writer %s\n", 
            writers.map!(f => ":" ~ f).join(", "));
    }
    if (accessors.length > 0) {
        result ~= indent ~ format("  attr_accessor %s\n", 
            accessors.map!(f => ":" ~ f).join(", "));
    }
    
    result ~= "\n";
    
    // Constructor
    result ~= indent ~ "  def initialize\n";
    
    // Initialize collections
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            static if (isArray!FieldType) {
                result ~= indent ~ format("    @%s = []\n", fieldName);
            } else static if (isAssociativeArray!FieldType) {
                result ~= indent ~ format("    @%s = {}\n", fieldName);
            }
        }
    }
    
    result ~= indent ~ "  end\n";
    
    result ~= indent ~ "end\n";
    return result;
}

// ============================================================================
// RUBY LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.RUBY, LanguageInfo(
        name: Language.RUBY,
        
        config: LanguageConfig(
            fileExt: "rb",
            commentStyle: "#",
            indentStyle: "  "
        ),
        
        features: LanguageFeatures(
            hasClasses: true,
            hasGettersSetters: true,
            hasConstructors: true,
            hasNamespaces: true,
            hasTypeModifiers: false
        ),
        
        outputs: [
            OutputFile("rb", GeneratorType.SOURCE)
        ],
        
        primitives: PrimitiveTypes(
            bool_: ["true/false", "false"],
            string_: ["String", "''"],
            float_: ["Float", "0.0"],
            double_: ["Float", "0.0"],
            byte_: ["Integer", "0"],
            ubyte_: ["Integer", "0"],
            short_: ["Integer", "0"],
            ushort_: ["Integer", "0"],
            int_: ["Integer", "0"],
            uint_: ["Integer", "0"],
            long_: ["Integer", "0"],
            ulong_: ["Integer", "0"]
        ),
        
        complex: ComplexTypes(
            vector_: ["Array", "[]"],
            map_: ["Hash", "{}"],
            optional_: ["%s", "nil"],
            tuple_: ["[", "]"],
            set_: ["Set", "Set.new"]
        ),
        
        namespace: NamespaceConfig(
            separator: "::",
            keyword: "module",
            startGenerator: &rubyNamespaceStart,
            endGenerator: &rubyNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toPascalCase,
            privateMemberName: &toSnakeCase,
            publicMemberName: &toSnakeCase,
            fileName: &toSnakeCase,
            namespaceName: &toRubyNamespaceStyle
        ),
        
        generators: CodeGenerators(
            getter: &generateRubyGetter,
            setter: &generateRubySetter,
            typeModifier: &applyRubyModifier,
            constructor: &generateRubyConstructor
        )
    ));
}
