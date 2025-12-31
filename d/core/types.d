module core.types;

import std.stdio;
import std.traits;
import std.conv;
import std.string;
import std.algorithm;
import std.array;
import std.format;
import std.file;

import std.datetime : DateTime; 

// ============================================================================
// LANGUAGE NAME CONSTANTS
// ============================================================================

enum Language : string {
    CPP26 = "cpp26",
    CSV = "csv",
    GO = "go",
    HASKELL = "haskell",
    JAVA = "java",
    PYTHON = "python",
    RUBY = "ruby",
    RUST = "rust",
    SQL = "sql",
    TYPESCRIPT = "typescript",
    XML = "xml",
    YAML = "yaml",
    ZIG = "zig",
}

enum GeneratorType : string {
    HEADER = "header",
    SOURCE = "source",
    INTERFACE = "interface",
    SCHEMA = "schema"
}

// ============================================================================
// GENERATOR TYPE CONSTANTS
// ============================================================================

enum GEN_HEADER = "header";
enum GEN_SOURCE = "source";
enum GEN_INTERFACE = "interface";
enum GEN_SCHEMA = "schema";

// ============================================================================
// CORE TYPE SYSTEM
// ============================================================================

enum GetterSetterStyle { 
    JavaBean, Property, SwiftProperty, PythonProperty, RustField, JSGetSet, None
}

enum AccessLevel { 
    Public, Private, ReadOnly, Internal, Protected, PrivateNoAccess 
}

enum FieldModifier {
    None, Optional, Unique, Shared, Weak, Immutable, Static
}

// UDA

enum TypeStyle {
    Auto,
    ValueObject,
    Entity
}

struct typeStyle {
    TypeStyle style;
    this(TypeStyle s) { this.style = s; }
}


struct outputDirPrefix {
    string path;
}

struct langDir(Language lang) {
    string path;
}

enum ClassNameStyle { PascalCase, camelCase, snake_case, SCREAMING_SNAKE, kebab_case }
enum MemberNameStyle { camelCase, PascalCase, snake_case, m_prefix, kebab_case }
enum MethodNameStyle { camelCase, snake_case, kebab_case, PascalCase }
enum FileNameStyle { snake_case, camelCase, PascalCase, kebab_case, lowercase }

struct globalNameSpace {
    string[] names;  
    this(string[] names) { this.names = names; }
}

struct namespace {
    string[] names;  
    this(string[] names) { this.names = names; }
}

struct className {
    string name;
    this(string name) { this.name = name; }
}

struct tableName {
    string name;
    this(string name) { this.name = name; }
}

struct access {
    AccessLevel level;
    this(AccessLevel level) { this.level = level; }
}


struct memberName {
    string name;
    this(string name) { this.name = name; }
}

struct columnName {
    string name;
    this(string name) { this.name = name; }
}


struct csvName {
    string name;
    this(string name) { this.name = name; }
}

struct primaryKey {}


struct mutable {}



// ============================================================================
// FIELD ANNOTATION SYSTEM WITH CUSTOM NAMING
// ============================================================================


struct field {
    AccessLevel access = AccessLevel.Public;
    FieldModifier modifier = FieldModifier.None;
    string columnName = "";
    string csvName = "";
    string xmlName = "";
    string jsonName = "";
    string yamlName = "";
    
    this(AccessLevel access, FieldModifier modifier, string columnName) {
        this.access = access;
        this.modifier = modifier;
        this.columnName = columnName;
    }
    
    this(AccessLevel access) {
        this.access = access;
    }
    
    this(AccessLevel access, FieldModifier modifier) {
        this.access = access;
        this.modifier = modifier;
    }
    
    this(FieldModifier modifier) {
        this.modifier = modifier;
    }
    
    this(AccessLevel access, string columnName) {
        this.access = access;
        this.columnName = columnName;
    }
    
    this(string columnName) {
        this.columnName = columnName;
    }
}

struct table {
    string name = "";
    string schema = "";
    string csvFileName = "";
    string xmlRootName = "";
    string jsonSchemaId = "";
    
    this(string name) {
        this.name = name;
    }
    
    this(string name, string schema) {
        this.name = name;
        this.schema = schema;
    }
}

// ============================================================================
// LANGUAGE CONFIGURATION SYSTEM
// ============================================================================



struct DirectoryStructure {
    string[] pathComponents;  // Directory components to add after language name
    
    this(string[] components) {
        this.pathComponents = components;
    }
}
struct LanguageConfig {
    string fileExt;
    string commentStyle;
    string indentStyle;
    string[] directoryPath;
}

struct LanguageFeatures {
    bool hasClasses;
    bool hasGettersSetters;
    bool hasConstructors;
    bool hasNamespaces;
    bool hasTypeModifiers;
}

struct OutputFile {
    string extension;
    string type;
}

struct PrimitiveTypes {
    string[] bool_;
    string[] string_;
    string[] float_;
    string[] double_;
    string[] byte_;
    string[] ubyte_;
    string[] short_;
    string[] ushort_;
    string[] int_;
    string[] uint_;
    string[] long_;
    string[] ulong_;
}

struct ComplexTypes {
    string[] vector_;
    string[] map_;
    string[] optional_;
    string[] tuple_;
    string[] set_;
}

struct NamingConventions {
    string function(string) className;
    string function(string) privateMemberName;
    string function(string) publicMemberName;
    string function(string) fileName;
    string function(string) namespaceName;
    string function(string) getterName;
    string function(string) parameterName;
    
}

struct NamespaceConfig {
    string separator;
    string keyword;
    string function(string[], bool) startGenerator;
    string function(string[], bool) endGenerator;
}

struct CodeGenerators {
    string function(string, string, string, AccessLevel, bool) getter;
    string function(string, string, string, AccessLevel, bool) setter;
    string function(string, FieldModifier) typeModifier;
    string function(string, string[]) constructor;
}

struct StandardTypes {
    string[2] datetime_;
    string[2] uuid_;
    string[2] decimal_;
    string[2] uri_;
}

struct LanguageInfo {
    string name;
    LanguageConfig config;
    LanguageFeatures features;
    OutputFile[] outputs;
    PrimitiveTypes primitives;
    StandardTypes standard;   
    ComplexTypes complex;
    NamespaceConfig namespace;
    NamingConventions naming;
    CodeGenerators generators;
}

// ============================================================================
// SYSTEM-WIDE DEFAULT VALUES
// ============================================================================

immutable string[string] systemDefaults = [
    "int": "0",
    "byte": "0",
    "short": "0",
    "long": "0",
    "uint": "0",
    "ubyte": "0",
    "ushort": "0",
    "ulong": "0",
    "float": "0.0",
    "double": "0.0",
    "bool": "false",
    "string": "\"\""
];

// ============================================================================
// AUTO-REGISTRATION SYSTEM
// ============================================================================


// Change registry to use enum keys
__gshared LanguageInfo[Language] languageRegistry;
__gshared Language[] availableLanguages;

void registerLanguage(Language lang, LanguageInfo info) {
    languageRegistry[lang] = info;
    if (!availableLanguages.canFind(lang)) {
        availableLanguages ~= lang;
    }
}

LanguageInfo getLanguageInfo(Language languageName) {
    if (languageName !in languageRegistry) {
        throw new Exception("Language not registered: " ~ cast(string)languageName);
    }
    return languageRegistry[languageName];
}

Language[] getAllLanguages() { return availableLanguages.dup; }
bool isLanguageAvailable(Language lang) { return (lang in languageRegistry) !is null; }


// ============================================================================
// NAMING CONVENTION HELPERS
// ============================================================================

string getNameFromAttr(T, string fieldName, Attr)(string function(string) nameFunc) {
    static if (hasUDA!(__traits(getMember, T, fieldName), Attr)) {
        enum attr = getUDAs!(__traits(getMember, T, fieldName), Attr)[0];
        return nameFunc(attr.name);
    } else {
        return nameFunc(fieldName);
    }
}

string toPascalCase(string input) {
    import std.string : split;
    import std.array : join;
    import std.algorithm : map;
    import std.uni : toUpper, toLower;
    import std.regex : regex, replaceAll;
    
    if (input.length == 0) return input;
    
    // Replace spaces and hyphens with underscores first
    input = input.replaceAll(regex(r"[ \-]+"), "_");
    
    auto parts = input.split("_");
    auto capitalized = parts.map!(part => 
        part.length > 0 ? part[0..1].toUpper ~ part[1..$].toLower : ""
    );
    return capitalized.join("");
}

string toCamelCase(string input) {
    import std.string : split;
    import std.array : join;
    import std.algorithm : map;
    import std.uni : toUpper, toLower;
    import std.regex : regex, replaceAll;
    
    if (input.length == 0) return input;
    
    // Replace spaces and hyphens with underscores first
    input = input.replaceAll(regex(r"[ \-]+"), "_");
    
    auto parts = input.split("_");
    if (parts.length == 0) return input;
    
    string result = parts[0].toLower;
    foreach (part; parts[1..$]) {
        if (part.length > 0) {
            result ~= part[0..1].toUpper ~ part[1..$].toLower;
        }
    }
    
    return result;
}

string toSnakeCase(string input) {
    return input.replace(" ", "_").replace("-", "_").toLower;
}

string toSnakeCaseUnderScore(string input) {
       return input.replace(" ", "_").replace("-", "_").toLower ~  "_";
}

string toKebabCase(string input) {
    return input.replace("_", "-").toLower;
}

string toLowercase(string input) {
    return input.toLower;
}

string toFileSnakeCase(string input) {
    return input.replace(" ", "_").replace("-", "_").toLower;
}

string toFileKebabCase(string input) {
    return input.replace("_", "-").toLower;
}

// ============================================================================
// TYPE CONVERSION SYSTEM
// ============================================================================

string convertType(T)(ref LanguageInfo lang) {
    import std.traits : Unqual, isArray, isAssociativeArray;
    import std.typecons : Nullable, Tuple;
    
    alias BaseType = Unqual!T;
    enum typeName = BaseType.stringof;
    // Check for DateTime first
 
    static if (is(BaseType == DateTime)) {
        return lang.standard.datetime_[0];
    }
    switch (typeName) {
        case "bool":   return lang.primitives.bool_[0];
        case "string": return lang.primitives.string_[0];
        case "float":  return lang.primitives.float_[0];
        case "double": return lang.primitives.double_[0];
        case "byte":   return lang.primitives.byte_[0];
        case "ubyte":  return lang.primitives.ubyte_[0];
        case "short":  return lang.primitives.short_[0];
        case "ushort": return lang.primitives.ushort_[0];
        case "int":    return lang.primitives.int_[0];
        case "uint":   return lang.primitives.uint_[0];
        case "long":   return lang.primitives.long_[0];
        case "ulong":  return lang.primitives.ulong_[0];
        default: break;
    }
    
    static if (isArray!BaseType && !is(BaseType == string)) {
        alias ElemType = typeof(BaseType.init[0]);
        string elemTypeStr = convertType!ElemType(lang);
        return format(lang.complex.vector_[0], elemTypeStr);
    }
    else static if (isAssociativeArray!BaseType) {
        alias KeyType = typeof(BaseType.init.keys[0]);
        alias ValueType = typeof(BaseType.init.values[0]);
        string keyTypeStr = convertType!KeyType(lang);
        string valueTypeStr = convertType!ValueType(lang);
        return format(lang.complex.map_[0], keyTypeStr, valueTypeStr);
    }
    else static if (is(BaseType == Nullable!U, U)) {
        string innerTypeStr = convertType!U(lang);
        return format(lang.complex.optional_[0], innerTypeStr);
    }
    else static if (is(BaseType == Tuple!Args, Args...)) {
        string[] argTypes;
        foreach (Arg; Args) {
            argTypes ~= convertType!Arg(lang);
        }
        return format(lang.complex.tuple_[0], argTypes.join(", "));
    }
    
    return BaseType.stringof;
}

// Get filename from class using the language's naming convention
string getLanguageFileName(T)(Language lang) {
    auto info = languageRegistry[lang];
    
    // First try to get from @className attribute
    static if (hasUDA!(T, className)) {
        string name = getUDAs!(T, className)[0].name;
        return info.naming.fileName(name);
    } 
    // Fallback to type name
    else {
        return info.naming.fileName(T.stringof);
    }
}

string getDefaultValue(T)(ref LanguageInfo lang) {
    import std.traits : Unqual;
    
    alias BaseType = Unqual!T;
    enum typeName = BaseType.stringof;
    
    switch (typeName) {
        case "bool":   return lang.primitives.bool_.length > 1 ? lang.primitives.bool_[1] : "false";
        case "string": return lang.primitives.string_.length > 1 ? lang.primitives.string_[1] : `""`;
        case "float":  return lang.primitives.float_.length > 1 ? lang.primitives.float_[1] : "0.0";
        case "double": return lang.primitives.double_.length > 1 ? lang.primitives.double_[1] : "0.0";
        case "byte":   return lang.primitives.byte_.length > 1 ? lang.primitives.byte_[1] : "0";
        case "ubyte":  return lang.primitives.ubyte_.length > 1 ? lang.primitives.ubyte_[1] : "0";
        case "short":  return lang.primitives.short_.length > 1 ? lang.primitives.short_[1] : "0";
        case "ushort": return lang.primitives.ushort_.length > 1 ? lang.primitives.ushort_[1] : "0";
        case "int":    return lang.primitives.int_.length > 1 ? lang.primitives.int_[1] : "0";
        case "uint":   return lang.primitives.uint_.length > 1 ? lang.primitives.uint_[1] : "0";
        case "long":   return lang.primitives.long_.length > 1 ? lang.primitives.long_[1] : "0";
        case "ulong":  return lang.primitives.ulong_.length > 1 ? lang.primitives.ulong_[1] : "0";
        default: break;
    }
    
    static if (isArray!BaseType) {
        return lang.complex.vector_.length > 1 ? lang.complex.vector_[1] : "{}";
    }
    else static if (isAssociativeArray!BaseType) {
        return lang.complex.map_.length > 1 ? lang.complex.map_[1] : "{}";
    }
    
    return "";
}

// ============================================================================
// NESTED TYPE SUPPORT WITH COLLECTIONS
// ============================================================================

template isNestedStruct(T) {
    static if (is(T == struct)) {
        enum isNestedStruct = true;
    } else {
        enum isNestedStruct = false;
    }
}

template isArray(T) {
    static if (is(T : U[], U) && !is(T == string)) {
        enum isArray = true;
        alias ElementType = U;
    } else {
        enum isArray = false;
    }
}

template isAssociativeArray(T) {
    static if (is(T : V[K], K, V)) {
        enum isAssociativeArray = true;
        alias KeyType = K;
        alias ValueType = V;
    } else {
        enum isAssociativeArray = false;
    }
}

template hasNestedTypes(T) {
    static if (is(T == struct)) {
        enum hasNestedTypes = checkForNestedTypes!T();
    } else {
        enum hasNestedTypes = false;
    }
}

bool checkForNestedTypes(T)() {
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            
            static if (isNestedStruct!FieldType) {
                return true;
            } else static if (isArray!FieldType) {
                alias ElementType = typeof(FieldType.init[0]);
                static if (isNestedStruct!ElementType) {
                    return true;
                }
            } else static if (isAssociativeArray!FieldType) {
                alias ValueType = typeof(FieldType.init.values[0]);
                static if (isNestedStruct!ValueType) {
                    return true;
                }
            }
        }
    }
    return false;
}

// ============================================================================
// CUSTOM NAMING HELPERS
// ============================================================================

string getCustomColumnName(T, string fieldName)() {
    import std.traits : getUDAs;
    alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
    static if (fieldAttrs.length > 0 && fieldAttrs[0].columnName.length > 0) {
        return fieldAttrs[0].columnName;
    } else {
        return fieldName.toSnakeCase;
    }
}

string getCustomCSVName(T, string fieldName)() {
    import std.traits : getUDAs;
    alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
    static if (fieldAttrs.length > 0 && fieldAttrs[0].csvName.length > 0) {
        return fieldAttrs[0].csvName;
    } else {
        return fieldName;
    }
}

string getCustomTableName(T)() {
    import std.traits : getUDAs;
    alias tableAttrs = getUDAs!(T, table);
    static if (tableAttrs.length > 0 && tableAttrs[0].name.length > 0) {
        return tableAttrs[0].name;
    } else {
        return T.stringof.toSnakeCase;
    }
}

string getCustomSchema(T)() {
    import std.traits : getUDAs;
    alias tableAttrs = getUDAs!(T, table);
    static if (tableAttrs.length > 0 && tableAttrs[0].schema.length > 0) {
        return tableAttrs[0].schema;
    } else {
        return "";
    }
}

string[] getNamespace(alias Module)() {
    // Try to get globalNameSpace from the module
    static if (__traits(hasMember, Module, "globalNameSpace")) {
        auto ns = __traits(getMember, Module, "globalNameSpace");
        if (ns.length > 0) {
            // Expand the array as variadic arguments
            return ns;
        }
    }
    return [];
}

string getLanguagePath(alias Module)(Language lang) {
    import std.conv : to;
    import std.string : toLower;
    import std.path : buildPath;
    import std.array : array;
    
    auto langInfo = getLanguageInfo(lang);
    auto langName = lang.to!string.toLower;
    
    // Try to get outputDirPrefix from the module
    string prefix = "";
    static if (__traits(hasMember, Module, "outputDirPrefix")) {
        prefix = __traits(getMember, Module, "outputDirPrefix");
    }
    
    // Build base path
    string basePath;
    if (prefix.length > 0) {
        basePath = prefix ~ langName;
    } else {
        basePath = langName;
    }
    
    // Start building path parts
    string[] pathParts = [basePath];
    
    // Add language-specific directory path (like "src" or "src/main/java")
    if (langInfo.config.directoryPath !is null && langInfo.config.directoryPath.length > 0) {
        pathParts ~= langInfo.config.directoryPath;
    }
    
    // Try to get globalNameSpace from the module
    static if (__traits(hasMember, Module, "globalNameSpace")) {
        auto ns = __traits(getMember, Module, "globalNameSpace");
        if (ns !is null && ns.length > 0) {
            pathParts ~= ns;
        }
    }
    
    return buildPath(pathParts);
}
// =
// ===========================================================================
// COLLECTION TYPE MAPPERS (Legacy - use convertType instead)
// ============================================================================

string mapCollectionType(T)(Language lang) {
    auto langInfo = languageRegistry[lang];
    return convertType!T(langInfo);
}

string generateNestedClasses(T)(Language lang, int depth = 0) {
    string result = "";
    string indent = "    ".replicate(depth);
    
    __gshared static bool[string] generatedTypes;
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            // Skip standard library types like DateTime
            static if (is(FieldType == DateTime)) {
                // Don't generate nested class for DateTime

            }
            else static if (isNestedStruct!FieldType) {
                string typeName = FieldType.stringof;
                if (typeName !in generatedTypes) {
                    generatedTypes[typeName] = true;
                    result ~= indent ~ "// Nested class: " ~ typeName ~ "\n";
                    result ~= "\n";
                }
            }
            else static if (isArray!FieldType) {
                alias ElementType = typeof(FieldType.init[0]);
                static if (isNestedStruct!ElementType) {
                    string typeName = ElementType.stringof;
                    if (typeName !in generatedTypes) {
                        generatedTypes[typeName] = true;
                        result ~= indent ~ "// Nested class (from array): " ~ typeName ~ "\n";
                        result ~= "\n";
                    }
                }
            }
            else static if (isAssociativeArray!FieldType) {
                alias ValueType = typeof(FieldType.init.values[0]);
                static if (isNestedStruct!ValueType) {
                    string typeName = ValueType.stringof;
                    if (typeName !in generatedTypes) {
                        generatedTypes[typeName] = true;
                        result ~= indent ~ "// Nested class (from map): " ~ typeName ~ "\n";
                        result ~= "\n";
                    }
                }
            }
        }
    }
    return result;
}
