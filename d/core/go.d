module core.go;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple, hasUDA, getUDAs;
import std.array : replicate;

// ============================================================================
// GO NAMESPACE GENERATORS (PACKAGE)
// ============================================================================

string goNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    return format("package %s\n\n", namespaceParts[$-1]);
}

string goNamespaceEnd(string[] namespaceParts, bool nested) {
    return "";
}

string toGoNamespaceStyle(string input) {
    return input.toLower;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyGoModifier(string baseType, FieldModifier modifier) {
    final switch(modifier) {
        case FieldModifier.None: 
            return baseType;
        case FieldModifier.Optional: 
            return format("*%s", baseType);
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
// HELPER FUNCTIONS
// ============================================================================

string getColumnName(T, string fieldName)() {
    static if (hasUDA!(__traits(getMember, T, fieldName), columnName)) {
        return getUDAs!(__traits(getMember, T, fieldName), columnName)[0].name;
    } else {
        return fieldName;
    }
}

string getTableName(T)() {
    static if (hasUDA!(T, tableName)) {
        return getUDAs!(T, tableName)[0].name;
    } else {
        return T.stringof;
    }
}



// ============================================================================
// GO CODE GENERATORS
// ============================================================================

string generateGoConstructor(string className, string[] paramList) {
    return format("func New%s() *%s {\n    return &%s{}\n}", className, className, className);
}

string generateGoGetter(string fieldName, string typeName, string className, 
                       AccessLevel access, bool isStatic) {
    return format("func (obj *%s) Get%s() %s {\n    return obj.%s\n}", 
        className, fieldName.toPascalCase, typeName, fieldName.toPascalCase);
}

string generateGoSetter(string fieldName, string typeName, string className, 
                       AccessLevel access, bool isStatic) {
    return format("func (obj *%s) Set%s(value %s) {\n    obj.%s = value\n}", 
        className, fieldName.toPascalCase, typeName, fieldName.toPascalCase);
}

// ============================================================================
// GO STRUCT GENERATOR WITH METATUPLES
// ============================================================================

string generateGoClass(T)(string className, string[] ns, int depth = 0) {
    auto info = languageRegistry[Language.GO];
    string indent = "    ".replicate(depth);
    string result = "";
    
    // Package declaration
    result ~= goNamespaceStart(ns, false);
    
    result ~= generateNestedClasses!T(Language.GO, depth);
    
    // Imports - add common package
    if (depth == 0) {
        result ~= "import (\n";
        result ~= "    \"hytek-go/common\"\n";
        result ~= ")\n\n";
    }


    // Struct definition
    result ~= indent ~ format("// %s represents the data model\n", className);
    result ~= indent ~ format("type %s struct {\n", className);
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string goFieldName = fieldName.toPascalCase;
            string fieldType = convertType!FieldType(info);
            string colName = getColumnName!(T, fieldName);
            
            result ~= indent ~ format("    %s %s `json:\"%s\" csv:\"%s\"`\n", 
                goFieldName, fieldType, fieldName, colName);
        }
    }
    
    result ~= indent ~ "}\n\n";
    
    // MetaTuples method - THE KEY PART!
    result ~= indent ~ "// MetaTuples returns field metadata with runtime values\n";
    result ~= indent ~ format("func (obj *%s) MetaTuples() []common.MetaTuple {\n", className);
    result ~= indent ~ "    return []common.MetaTuple{\n";
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            string colName = getColumnName!(T, fieldName);
            
            result ~= indent ~ format("        {FieldName: \"%s\", ColumnName: \"%s\", TypeName: \"%s\", Value: obj.%s},\n",
                fieldName, colName, fieldType, fieldName.toPascalCase);
        }
    }
    
    result ~= indent ~ "    }\n";
    result ~= indent ~ "}\n\n";
    
    // TableName method
    string tableName = getTableName!T();
    result ~= indent ~ format("func (*%s) TableName() string {\n", className);
    result ~= indent ~ format("    return \"%s\"\n", tableName.toLower);
    result ~= indent ~ "}\n\n";
    return result;
}

// ============================================================================
// REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.GO, LanguageInfo(
        config: LanguageConfig(
            fileExt: "go",
            commentStyle: "//",
            indentStyle: "    "
        ),
        
        primitives: PrimitiveTypes(
            bool_: ["bool", "false"],
            string_: ["string", `""`],
            float_: ["float32", "0.0"],
            double_: ["float64", "0.0"],
            byte_: ["int8", "0"],
            ubyte_: ["uint8", "0"],
            short_: ["int16", "0"],
            ushort_: ["uint16", "0"],
            int_: ["int32", "0"],
            uint_: ["uint32", "0"],
            long_: ["int64", "0"],
            ulong_: ["uint64", "0"]
        ),
        
        standard: StandardTypes(
            datetime_: ["time.Time", "time.Time{}"],
            uuid_: ["string", `""`],
            decimal_: ["float64", "0.0"],
            uri_: ["string", `""`]
        ),
        
        complex: ComplexTypes(
            vector_: ["[]%s", "nil"],
            map_: ["map[%s]%s", "nil"],
            optional_: ["*%s", "nil"],
            tuple_: ["struct{%s}", "struct{}{}"],
            set_: ["map[%s]bool", "nil"]
        ),
        
        namespace: NamespaceConfig(
            separator: ".",
            keyword: "package",
            startGenerator: &goNamespaceStart,
            endGenerator: &goNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toPascalCase,
            privateMemberName: &toCamelCase,
            publicMemberName: &toPascalCase,
            fileName: &toSnakeCase,
            namespaceName: &toGoNamespaceStyle
        ),
        
        generators: CodeGenerators(
            getter: &generateGoGetter,
            setter: &generateGoSetter,
            typeModifier: &applyGoModifier,
            constructor: &generateGoConstructor
        )
    ));
}

