module core.java;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple, hasUDA, getUDAs;
import std.array : replicate;

// ============================================================================
// JAVA NAMESPACE GENERATORS
// ============================================================================

string javaNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    return format("package %s;\n", namespaceParts.join("."));
}

string javaNamespaceEnd(string[] namespaceParts, bool nested) {
    return "";  // Java doesn't need closing for packages
}

string toJavaNamespaceStyle(string input) {
    return input.toLower;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyJavaModifier(string baseType, FieldModifier modifier) {
    final switch(modifier) {
        case FieldModifier.None: 
            return baseType;
        case FieldModifier.Optional: 
            return baseType;  // Just use nullable types
        case FieldModifier.Unique: 
            return baseType;
        case FieldModifier.Shared: 
            return baseType;
        case FieldModifier.Weak: 
            return format("WeakReference<%s>", baseType);
        case FieldModifier.Immutable: 
            return format("final %s", baseType);
        case FieldModifier.Static: 
            return format("static %s", baseType);
    }
}

// ============================================================================
// JAVA CODE GENERATORS
// ============================================================================

string generateJavaConstructor(string className, string[] paramList) {
    return format("    public %s() {}\n", className);
}

string generateJavaGetter(string fieldName, string typeName, string className, 
                          AccessLevel access, bool isStatic) {
    return format("    public %s get%s() { return %s; }", 
        typeName, fieldName.toPascalCase, fieldName);
}

string generateJavaSetter(string fieldName, string typeName, string className, 
                          AccessLevel access, bool isStatic) {
    return format("    public void set%s(%s value) { this.%s = value; }", 
        fieldName.toPascalCase, typeName, fieldName);
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

// ============================================================================
// JAVA CLASS GENERATOR
// ============================================================================

string generateJavaClass(T)(string className, string[] ns, int depth = 0) {
    auto info = languageRegistry[Language.JAVA];
    string indent = "    ".replicate(depth);
    string result = "";
    
    result ~= javaNamespaceStart(ns, false);
    
    result ~= "\nimport bhw.MetaTuple;\n";
	       
    // Generate nested classes first
    result ~= generateNestedClasses!T(Language.JAVA, depth);
    
    // Package and imports
    if (depth == 0) {
        result ~= "\n";
        result ~= "import com.opencsv.bean.CsvBindByName;\n";
        result ~= "import java.util.List;\n";
        result ~= "import java.util.ArrayList;\n\n";
    }

    // Class declaration
    result ~= format("public class %s {\n\n", className);
    
    // Private fields with CSV annotations
    result ~= "    // Fields\n";
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            string colName = getColumnName!(T, fieldName);
            
            result ~= format("    @CsvBindByName(column = \"%s\")\n", colName);
            result ~= format("    private %s %s;\n", fieldType, fieldName);
        }
    }
    result ~= "\n";
    
    // Default constructor
    result ~= "    // Constructors\n";
    result ~= format("    public %s() {}\n\n", className);
    
    // Parameterized constructor
    result ~= "    public " ~ className ~ "(";
    
    string[] paramList;
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            paramList ~= format("%s %s", fieldType, fieldName);
        }
    }
    result ~= paramList.join(", ");
    result ~= ") {\n";
    
    // Constructor body - assign all parameters
    static foreach (fieldName; FieldNameTuple!T) {
        {
            result ~= format("        this.%s = %s;\n", fieldName, fieldName);
        }
    }
    result ~= "    }\n\n";
    
    // Getters and Setters
    result ~= "    // Getters and Setters\n";
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            
            result ~= format("    public %s get%s() {\n", 
                fieldType, fieldName.toPascalCase);
            result ~= format("        return %s;\n", fieldName);
            result ~= "    }\n\n";
            
            result ~= format("    public void set%s(%s value) {\n",
                fieldName.toPascalCase, fieldType);
            result ~= format("        this.%s = value;\n", fieldName);
            result ~= "    }\n\n";
        }
    }
    
    // META TUPLES METHOD
    result ~= "    // Meta Tuples - Runtime field metadata\n";
    result ~= "    public List<MetaTuple<?>> metaTuples() {\n";
    result ~= "        List<MetaTuple<?>> tuples = new ArrayList<>();\n";
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            string colName = getColumnName!(T, fieldName);
            
            result ~= format("        tuples.add(new MetaTuple<>(\"%s\", \"%s\", \"%s\", this.%s));\n",
                fieldName, colName, fieldType, fieldName);
        }
    }
    
    result ~= "        return tuples;\n";
    result ~= "    }\n\n";
    
    // TABLE NAME static method
    result ~= "    // Table metadata\n";
    result ~= "    public static String tableName() {\n";
    result ~= format("        return \"%s\";\n", className.toLower);
    result ~= "    }\n";
    
    result ~= "}\n";
    return result;
}

// ============================================================================
// SUPPORT CLASS GENERATORS
// ============================================================================

string generateMetaTupleClass(string[] ns) {
    string result = "";
    result ~= javaNamespaceStart(ns, false);
    result ~= "\n";
    
    result ~= q{public class MetaTuple<T> {
    public final String fieldName;      // Compile-time constant
    public final String columnName;     // Compile-time constant  
    public final String typeName;       // Compile-time constant
    public final T value;               // Runtime value
    
    public MetaTuple(String fieldName, String columnName, String typeName, T value) {
        this.fieldName = fieldName;
        this.columnName = columnName;
        this.typeName = typeName;
        this.value = value;
    }
    
    public T getValue() {
        return value;
    }
    
    public boolean isNull() {
        return value == null;
    }
    
    @Override
    public String toString() {
        return String.format("MetaTuple{field='%s', column='%s', type='%s', value=%s}",
            fieldName, columnName, typeName, value);
    }
}
};
    
    return result;
}



// ============================================================================
// JAVA LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.JAVA, LanguageInfo(
        config: LanguageConfig(
            fileExt: "java",
            commentStyle: "//",
            indentStyle: "    ",
            directoryPath: ["src", "main", "java"]
        ),
        
        primitives: PrimitiveTypes(
            bool_: ["Boolean", "false"],
            string_: ["String", "null"],
            float_: ["Float", "0.0f"],
            double_: ["Double", "0.0"],
            byte_: ["Byte", "0"],
            ubyte_: ["Short", "0"],
            short_: ["Short", "0"],
            ushort_: ["Integer", "0"],
            int_: ["Integer", "0"],
            uint_: ["Long", "0L"],
            long_: ["Long", "0L"],
            ulong_: ["Long", "0L"]
        ),
        
        standard: StandardTypes(
            datetime_: ["java.time.LocalDateTime", "null"],
            uuid_: ["UUID", "null"],
            decimal_: ["BigDecimal", "BigDecimal.ZERO"],
            uri_: ["URI", "null"]
        ),
        
        complex: ComplexTypes(
            vector_: ["List<%s>", "new ArrayList<>()"],
            map_: ["Map<%s, %s>", "new HashMap<>()"],
            optional_: ["%s", "null"],  // Just nullable type, no Optional wrapper
            tuple_: ["Tuple<", ">"],
            set_: ["Set<%s>", "new HashSet<>()"]
        ),
        
        namespace: NamespaceConfig(
            separator: ".",
            keyword: "package",
            startGenerator: &javaNamespaceStart,
            endGenerator: &javaNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toPascalCase,
            privateMemberName: &toCamelCase,
            publicMemberName: &toCamelCase,
            fileName: &toPascalCase,
            namespaceName: &toJavaNamespaceStyle
        ),
        
        generators: CodeGenerators(
            getter: &generateJavaGetter,
            setter: &generateJavaSetter,
            typeModifier: &applyJavaModifier,
            constructor: &generateJavaConstructor
        )
    ));
}


