module core.cpp;

import core.types;
import std.format;
import std.traits : FieldNameTuple, getUDAs, hasUDA, Unqual, isPointer, isIntegral;
import std.array : replicate;
import std.string : split, join, endsWith;
import std.algorithm : startsWith, sort;
import std.string : toLower;
import std.uni : toUpper;
import std.stdio;
import std.typecons : Nullable;

// ============================================================================
// GLOBAL STATE FOR RECURSION PREVENTION
// ============================================================================

__gshared bool[string] cppGeneratingTypes;

void resetCppTypeGeneration() {
    cppGeneratingTypes.clear();
}

// Group fields by access level
struct FieldInfo {
    string name;
    string mappedType;
    AccessLevel access;
    bool isUnique;
    bool isShared;
}

string cppNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    
    if (nested) {
        string result = "";
        foreach (part; namespaceParts) {
            result ~= format("namespace %s {\n", part);
        }
        return result;
    } else {
        return format("namespace %s {\n", namespaceParts.join("::"));
    }
}

string cppNamespaceEnd(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    
    if (nested) {
        string result = "";
        foreach_reverse (part; namespaceParts) {
            result ~= format("}  // namespace %s\n", part);
        }
        return result;
    } else {
        return format("}  // namespace %s\n", namespaceParts.join("::"));
    }
}

string toNamespaceStyle(string input) {
    return input.toLower;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

bool shouldPassByValue(string mappedType) {
    return mappedType == "bool" ||
           mappedType == "int8_t" ||
           mappedType == "uint8_t" ||
           mappedType == "int16_t" ||
           mappedType == "uint16_t" ||
           mappedType == "int32_t" ||
           mappedType == "uint32_t" ||
           mappedType == "int64_t" ||
           mappedType == "uint64_t" ||
           mappedType == "float" ||
           mappedType == "double";
}

string extractTemplateType(string fullType, string templateName) {
    if (fullType.startsWith(templateName ~ "<") && fullType[$-1] == '>') {
        return fullType[templateName.length + 1 .. $-1];
    }
    return fullType;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyCppModifier(string baseType, FieldModifier modifier) {
    final switch(modifier) {
        case FieldModifier.None: return baseType;
        case FieldModifier.Optional: return format("std::optional<%s>", baseType);
        case FieldModifier.Unique: return format("std::unique_ptr<%s>", baseType);
        case FieldModifier.Shared: return format("std::shared_ptr<%s>", baseType);
        case FieldModifier.Weak: return format("std::weak_ptr<%s>", baseType);
        case FieldModifier.Immutable: return format("const %s", baseType);
        case FieldModifier.Static: return format("static %s", baseType);
    }
}

// ============================================================================
// C++ INCLUDE GENERATION
// ============================================================================

string generateCppIncludes(T)() {
    import std.algorithm : canFind;
    import std.array : array;
    import std.datetime : DateTime;
    
    bool[string] includes;
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            alias BaseType = Unqual!FieldType;

            static if (isIntegral!BaseType) {
                includes["<cstdint>"] = true;
            }
            else static if (is(BaseType == string)) {
                includes["<string>"] = true;
            }
            else static if (is(BaseType == DateTime)) {
                includes["<chrono>"] = true;
            }
            else static if (isArray!BaseType && !is(BaseType == string)) {
                includes["<vector>"] = true;
            }
            else static if (isAssociativeArray!BaseType) {
                includes["<map>"] = true;
            }
            static if (is(BaseType == Nullable!U, U)) {
                includes["<optional>"] = true;
            }
            static if (isPointer!BaseType) {
                includes["<memory>"] = true;
            }
        }
    }
    
    string result;
    foreach (include; includes.byKey.array.sort) {
        result ~= "#include " ~ include ~ "\n";
    }
    return result;
}

// ============================================================================
// MAIN C++ CLASS GENERATOR
// ============================================================================

string generateCppClass(T)(string className, string [] ns,int depth = 0) {
    auto info = languageRegistry[Language.CPP26];
    string indent = "    ".replicate(depth);
    string result = "";
    
    string typeName = T.stringof;

    if (typeName in cppGeneratingTypes) {
        return indent ~ "// Forward declaration for " ~ className ~ " (circular reference)\n" ~
               indent ~ "class " ~ className ~ ";\n\n";
    }
    
    cppGeneratingTypes[typeName] = true;
    
    result ~= generateNestedClasses!T(Language.CPP26, depth);
    result ~= generateCppIncludes!T();
    result ~= "\n";

    result ~= info.namespace.startGenerator(ns, true);
    result ~= "\n";        	   

    // Check for @typeStyle annotation
    TypeStyle style = TypeStyle.Auto;
    static if (hasUDA!(T, typeStyle)) {
        style = getUDAs!(T, typeStyle)[0].style;
    }
    
    // VALUE OBJECT: struct with ALL public fields
    if (style == TypeStyle.ValueObject) {
        result ~= generateValueObject!T(className, info, ns, indent);
    } 
    // ENTITY: full class with access control
    else if (style == TypeStyle.Entity) {
        result ~= generateFullClass!T(className, info, ns, indent);
    }
    // DEFAULT: Modern Init/from pattern
    else {
        result ~= generateModernCppClass!T(className, info, indent);
    }
    //result =~ info.namespace.endGenerator(ns, true);
    result ~= "\n";        	   
    return result;
}

// ============================================================================
// MODERN C++ CLASS GENERATOR (Init/from pattern)
// ============================================================================

string generateModernCppClass(T)(string className, ref LanguageInfo info, string indent) {
    string result = "";
    
    result ~= indent ~ format("class %s\n{\n", className);
    result ~= indent ~ "public:\n";
    result ~= indent ~ format("    %s() = delete;\n\n", className);
    
    result ~= generateInitStruct!T(className, info, indent);
    result ~= "\n\n";
    
    result ~= generateFromFactory!T(className, indent);
    result ~= "\n\n";
    
    result ~= generateGetters!T(info, indent);
    
    result ~= indent ~ "private:\n";
    
    result ~= generatePrivateConstructor!T(className, info, indent);
    result ~= "\n\n";
    
    result ~= generateMembers!T(info, indent);
    
    result ~= indent ~ "};\n";
    return result;
}

string generateInitStruct(T)(string className, ref LanguageInfo info, string indent) {
    string result = indent ~ "    struct Init {\n";
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            auto fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            auto mappedType = convertType!FieldType(info);
            
            string initFieldName = fname;
            if (initFieldName.endsWith("_")) {
                initFieldName = initFieldName[0..$-1];
            }


            result ~= indent ~ format("        %s %s;\n", mappedType, initFieldName);
        }
    }
    
    result ~= indent ~ "    };";
    return result;
}

string generateFromFactory(T)(string className, string indent) {
    return indent ~ format("    static auto from(Init init) -> %s {\n", className) ~
           indent ~ format("        return %s{std::move(init)};\n", className) ~
           indent ~ "    }";
}

string generateGetters(T)(ref LanguageInfo info, string indent) {
    string result;
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            auto fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.getterName);
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            auto mappedType = convertType!FieldType(info);
            
            string baseFieldName = fname;
            if (baseFieldName.endsWith("_")) {
                baseFieldName = baseFieldName[0..$-1];
            }
            
            string getterName = "get" ~ baseFieldName[0..1].toUpper ~ baseFieldName[1..$];
            fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
            
            static if (is(FieldType == string)) {
                result ~= indent ~ format("    [[nodiscard]] auto %s() const noexcept -> std::string_view { return %s; }\n", 
                                getterName, fname);
            } else {
                result ~= indent ~ format("    [[nodiscard]] auto %s() const noexcept { return %s; }\n", 
                                getterName, fname);
            }
        }
    }
    
    return result;
}

string generatePrivateConstructor(T)(string className, ref LanguageInfo info, string indent) {
    string result = indent ~ format("    explicit %s(Init init)\n", className);
    result ~= indent ~ "        : ";
    
    bool first = true;
    static foreach (fieldName; FieldNameTuple!T) {
        {
            auto fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            
            string initFieldName = fname;
            if (initFieldName.endsWith("_")) {
                initFieldName = initFieldName[0..$-1];
            }
            
            if (!first) result ~= ",\n" ~ indent ~ "          ";
            first = false;
            
            static if (is(FieldType == string)) {
                result ~= format("%s(std::move(init.%s))", fname, initFieldName);
            } else {
                result ~= format("%s(init.%s)", fname, initFieldName);
            }
        }
    }
    
    result ~= "\n" ~ indent ~ "    {}";
    return result;
}

string generateMembers(T)(ref LanguageInfo info, string indent) {
    string result;
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            auto fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            auto mappedType = convertType!FieldType(info);
            
            result ~= indent ~ format("    %s %s{};\n", mappedType, fname);
        }
    }
    
    return result;
}

// ============================================================================
// VALUE OBJECT GENERATOR
// ============================================================================

string generateValueObject(T)(string className, ref LanguageInfo info, string [] ns, string indent) {
    // Build the aggregate initialization example

    auto result = indent ~ format("struct %s\n{\n", className);
    
    // ALL fields are public in value objects
    static foreach (fieldName; FieldNameTuple!T) {
        {
            auto fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.publicMemberName);
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string mappedType = convertType!FieldType(info);
	    result ~= indent ~ "    " ~ mappedType ~ " " ~ fname ~ "{};\n";
        }
    }
    
    result ~= "\n";
    
    // Generate reflection metadata
    result ~= generateReflectionMetadata!T(className, info, indent ~ "    ", true);
    
    result ~= indent ~ "};\n";

    result ~= indent ~ "}\n}\n";
    
    // Generate MetaTuple specialization
    result ~= generateMetaTupleSpecialization!T(className, info, ns, indent);
    return result;
}



// ============================================================================
// FULL CLASS GENERATOR (with access control)
// ============================================================================

string generateFullClass(T)(string className, ref LanguageInfo info, string[] ns, string indent) {
    string result = indent ~ format("class %s {\n", className);
    
    FieldInfo[] privateFields;
    FieldInfo[] publicFields;
    FieldInfo[] protectedFields;
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            auto fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
            string mappedType = mapCollectionType!FieldType(Language.CPP26);
            
            AccessLevel access = AccessLevel.Private;
            bool isUnique = false;
            bool isShared = false;
            
            alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
            static if (fieldAttrs.length > 0) {
                access = fieldAttrs[0].access;
                isUnique = (fieldAttrs[0].modifier == FieldModifier.Unique);
                isShared = (fieldAttrs[0].modifier == FieldModifier.Shared);
            }
            
            FieldInfo finfo = FieldInfo(fname, mappedType, access, isUnique, isShared);
            if (access == AccessLevel.Public) {
                publicFields ~= finfo;
            } else if (access == AccessLevel.Protected) {
                protectedFields ~= finfo;
            } else {
                privateFields ~= finfo;
            }
        }
    }
    
    // PRIVATE section
    if (privateFields.length > 0) {
        result ~= indent ~ "private:\n";
        foreach (finfo; privateFields) {
            result ~= indent ~ format("    %s %s_ = {};\n", finfo.mappedType, finfo.name);
        }
        result ~= "\n";
    }
    
    // PROTECTED section
    if (protectedFields.length > 0) {
        result ~= indent ~ "protected:\n";
        foreach (finfo; protectedFields) {
            result ~= indent ~ format("    %s %s_ = {};\n", finfo.mappedType, finfo.name);
        }
        result ~= "\n";
    }
    
    // PUBLIC section
    result ~= indent ~ "public:\n";
    
    if (publicFields.length > 0) {
        foreach (finfo; publicFields) {
            result ~= indent ~ format("    %s %s_ = {};\n", finfo.mappedType, finfo.name);
        }
        result ~= "\n";
    }
    
    // Generate reflection metadata (can access private members since it's inside the class)
    result ~= generateReflectionMetadata!T(className, info, indent ~ "    ", false);
    result ~= "\n";
    
    // Constructors
    result ~= generateConstructorsWithAccess!T(className, info, indent);
    
    // Getters/setters
    result ~= generateGettersSettersWithAccess!T(info, indent, privateFields, protectedFields);
    
    result ~= indent ~ "};\n\n";
    
    // MetaTuple specialization
    result ~= generateMetaTupleSpecialization!T(className, info, ns, indent);
    
    return result;
}

// ============================================================================
// REFLECTION METADATA GENERATION
// ============================================================================
string generateReflectionMetadataNew(T)(string className, ref LanguageInfo info, string indent, bool isValueObject) {
    string result = "";
    
    result ~= "namespace meta {\n";
    result ~= format("namespace %s {\n", className);
    result ~= "// clang-format off\n";
    result ~= "inline const auto fields = std::make_tuple(\n";
    
    // Generate each field
    bool first = true;
    static foreach (fieldName; FieldNameTuple!T) {
        {
            string memberFieldName = isValueObject ? 
                getNameFromAttr!(T, fieldName, memberName)(info.naming.publicMemberName) :
                getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
            
            // Remove trailing _ for access (it's added back in the pointer)
            string cleanFieldName = memberFieldName;
            if (cleanFieldName.endsWith("_")) {
                cleanFieldName = cleanFieldName[0..$-1];
            }
            
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string mappedType = convertType!FieldType(info);
            
            string fieldProps = getFieldProperties!(T, fieldName)();
            string tblName = getTableFieldName!(T, fieldName)();
            string csvName = getCSVFieldName!(T, fieldName)();
            
            if (!first) result ~= ",\n";
            first = false;
            
            string ptrAccess = isValueObject ? cleanFieldName : (memberFieldName ~ "_");
            
            result ~= format("    meta::Field<::%s, %s, &::%s::%s, nullptr, nullptr, %s>(\n",
                            className, mappedType, className, ptrAccess, fieldProps);
            result ~= format("        \"%s\",\n", mappedType);
            result ~= format("        \"%s\",\n", cleanFieldName);
            result ~= format("        \"%s\",\n", cleanFieldName);
            result ~= "        {\n";
            result ~= format("            {meta::CSV_COLUMN, \"%s\"},\n", csvName);
            result ~= format("            {meta::SQL_COLUMN, \"%s\"}\n", tblName);
            result ~= "        })";
        }
    }
    
    result ~= "\n);\n";
    result ~= format("inline constexpr auto tableName = \"%s\";\n", getCustomTableName!T());
    
    string queryStr = generateSqlQuery!T(className);
    result ~= format("inline constexpr auto query = \"%s\";\n", queryStr);
    
    result ~= "// clang-format on\n";
    result ~= format("}  // namespace %s\n", className);
    result ~= "}  // namespace meta\n\n";
    
    // Add template specialization
    result ~= "namespace meta {\n";
    result ~= "// Template specialization for type-based reflection lookup\n";
    result ~= "template<>\n";
    result ~= format("struct MetaTuple<::%s> {\n", className);
    result ~= format("    static inline const auto& fields = meta::%s::fields;\n", className);
    result ~= format("    static constexpr auto tableName = meta::%s::tableName;\n", className);
    result ~= format("    static constexpr auto query = meta::%s::query;\n", className);
    result ~= "};\n";
    result ~= "}  // namespace meta\n";
    
    return result;
}

string generateReflectionMetadata(T)(string className, ref LanguageInfo info, string indent, bool isValueObject) {
    string result = "    // clang-format off\n";
    
    // Generate the tuple definition in meta namespace
    result ~= format("    inline const auto fields = std::make_tuple(\n");
    
    // First pass: template parameters
    bool first = true;
    static foreach (fieldName; FieldNameTuple!T) {
        {
            string memberFieldName = isValueObject ? 
                getNameFromAttr!(T, fieldName, memberName)(info.naming.publicMemberName) :
                getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
            
            // Remove trailing _ for access (it's added back in the pointer)
            string cleanFieldName = memberFieldName;
            if (cleanFieldName.endsWith("_")) {
                cleanFieldName = cleanFieldName[0..$-1];
            }
            
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string mappedType = convertType!FieldType(info);

            string fieldProps = getFieldProperties!(T, fieldName)();
	    string tblName = getTableFieldName!(T, fieldName)();
            string csvName = getCSVFieldName!(T, fieldName)();

	    // Get the @memberName annotation value
            string annotatedMemberName;
            alias memberAttrs = getUDAs!(__traits(getMember, T, fieldName), memberName);
            static if (memberAttrs.length > 0) {
                annotatedMemberName = memberAttrs[0].name;
            } else {
                annotatedMemberName = fieldName;  // fallback to field name
            }
            if (!first) result ~= ",\n";
            first = false;
            
            string ptrAccess = isValueObject ? cleanFieldName : (memberFieldName ~ "_");

            result ~= format("    meta::Field<::%s, %s, &::%s::%s, nullptr, nullptr, %s>(\n", 
                            className, mappedType, className, ptrAccess, fieldProps);
            result ~= format("        \"%s\",\n", mappedType);
            result ~= format("        \"%s\",\n", fieldName);
            result ~= format("        \"%s\",\n", annotatedMemberName);
            result ~= "        {\n";
            result ~= format("            {meta::CSV_COLUMN, \"%s\"},\n", csvName);
            result ~= format("            {meta::SQL_COLUMN, \"%s\"}\n", tblName);
            result ~= "        })";
        }
    }
    
    result ~= "\n    >{\n";


/*
    // Second pass: constructor arguments
    first = true;
    static foreach (fieldName; FieldNameTuple!T) {
        {
            string memberFieldName = isValueObject ? 
                getNameFromAttr!(T, fieldName, memberName)(info.naming.publicMemberName) :
                getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
            
            string cleanFieldName = memberFieldName;
            if (cleanFieldName.endsWith("_")) {
                cleanFieldName = cleanFieldName[0..$-1];
            }
            
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string mappedType = convertType!FieldType(info);
            
            string tblName = getTableFieldName!(T, fieldName)();
            string csvName = getCSVFieldName!(T, fieldName)();
            
            if (!first) result ~= ",\n";
            first = false;

	    // Replace the format line with simple concatenation:
	    result ~= "        { \"" ~ mappedType ~ "\", \"" ~ cleanFieldName ~ "\", \"" ~ tblName ~ "\", \"" ~ csvName ~ "\"}";
        }
    }
    
     result ~= "\n    };\n";

*/
     result ~= format(indent ~ "static constexpr auto tableName = \"%s\";\n",
                     getCustomTableName!T());

     string queryStr = generateSqlQuery!T(className);
     result ~= format(indent ~ "static constexpr auto query = \"%s\";\n", queryStr);
     
    result ~= "    // clang-format on\n";
    return result;
}


string generateMetaTupleSpecialization(T)(string className,
                                          ref LanguageInfo info,
					  string [] ns,
					  string indent) {
    string result = "";
    // Create ::namespaced::className if namespace exists
    
    string fullClassName = className;
    if (ns.length > 0)
    {
        fullClassName = "::" ~ ns.join("::") ~ "::" ~ className;
    }

    result ~= indent ~ "namespace meta {\n";
    result ~= indent ~ "// Template specialization for type-based reflection lookup\n";
    result ~= indent ~ "template<>\n";
    result ~= indent ~ format("struct MetaTuple<%s> {\n", fullClassName);
    result ~= indent ~ format("    static constexpr auto& fields = %s::fields;\n", fullClassName);
    
    static if (hasUDA!(T, tableName)) {
      result ~= indent ~ format("    static constexpr auto tableName = %s::tableName;\n", fullClassName);
      result ~= indent ~ format("    static constexpr auto query = %s::query;\n", fullClassName);
    }
    
    result ~= indent ~ "};\n";
    result ~= indent ~ "}  // namespace meta\n";
    
    return result;
}

// ============================================================================
// REFLECTION HELPER FUNCTIONS
// ============================================================================




string getFieldProperties(T, string fieldName)() {
    import std.array : join;
    
    string[] props;
    
    alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
    static if (fieldAttrs.length > 0) {
        auto attr = fieldAttrs[0];
        
        static if (__traits(hasMember, attr, "isPrimaryKey") && attr.isPrimaryKey) {
            props ~= "meta::Prop::PrimaryKey";
        }
        
        static if (__traits(hasMember, attr, "serializable") && attr.serializable) {
            props ~= "meta::Prop::Serializable";
        }
        
        static if (__traits(hasMember, attr, "excludeCompare") && attr.excludeCompare) {
            props ~= "meta::Prop::ExcludeCompare";
        }
        
        static if (__traits(hasMember, attr, "excludeString") && attr.excludeString) {
            props ~= "meta::Prop::ExcludeString";
        }
        
        static if (__traits(hasMember, attr, "excludeHash") && attr.excludeHash) {
            props ~= "meta::Prop::ExcludeHash";
        }
    }
    
    if (props.length == 0) {
        return "meta::Prop::Serializable";
    }
    
    return props.join(" | ");
}

string generateSqlQuery(T)(string className) {
    string tableName = getCustomTableName!T();
    string[] columns;
    
    // Collect all column names
    static foreach (fieldName; FieldNameTuple!T) {
        {
            string colName = getTableFieldName!(T, fieldName)();
            columns ~= colName;
        }
    }
    
    // Build the query
    string query = format("SELECT %s FROM %s", columns.join(", "), tableName);
    return query;
}



string getTableFieldName(T, string fieldName)() {
    alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), columnName);
    static if (fieldAttrs.length > 0) {
        return fieldAttrs[0].name;
    } else {
        auto info = languageRegistry[Language.CPP26];
        auto fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.publicMemberName);
        return fname;
    }
}

string getCSVFieldName(T, string fieldName)() {
    alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), csvName);
    static if (fieldAttrs.length > 0) {
        return fieldAttrs[0].name;
    } else {
        return getTableFieldName!(T, fieldName)();
    }
}

// ============================================================================
// CONSTRUCTOR AND GETTER/SETTER GENERATION
// ============================================================================

string generateConstructorsWithAccess(T)(string className, ref LanguageInfo info, string indent) {
    string result = "";
    
    result ~= indent ~ format("    %s() = default;\n\n", className);
    
    string[] paramList;
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string mappedType = mapCollectionType!FieldType(Language.CPP26);
            
            alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
            static if (fieldAttrs.length > 0) {
                FieldModifier mod = fieldAttrs[0].modifier;
                if (mod == FieldModifier.Unique) {
                    string baseType = extractTemplateType(mappedType, "std::unique_ptr");
                    paramList ~= format("std::unique_ptr<%s> %s", baseType, fieldName);
                } else if (mod == FieldModifier.Shared) {
                    string baseType = extractTemplateType(mappedType, "std::shared_ptr");
                    paramList ~= format("std::shared_ptr<%s> %s", baseType, fieldName);
                } else if (shouldPassByValue(mappedType)) {
                    paramList ~= format("%s %s", mappedType, fieldName);
                } else {
                    paramList ~= format("const %s& %s", mappedType, fieldName);
                }
            } else if (shouldPassByValue(mappedType)) {
                paramList ~= format("%s %s", mappedType, fieldName);
            } else {
                paramList ~= format("const %s& %s", mappedType, fieldName);
            }
        }
    }
    
    if (paramList.length > 0) {
        result ~= indent ~ format("    %s(", className);
        foreach (i, param; paramList) {
            if (i > 0) result ~= ", ";
            result ~= param;
        }
        result ~= ")\n        : ";
        
        bool first = true;
        static foreach (fieldName; FieldNameTuple!T) {
            {
                auto fname = getNameFromAttr!(T, fieldName, memberName)(info.naming.privateMemberName);
                if (!first) result ~= ", ";
                first = false;
                
                alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
                static if (fieldAttrs.length > 0 && 
                          (fieldAttrs[0].modifier == FieldModifier.Unique || 
                           fieldAttrs[0].modifier == FieldModifier.Shared)) {
                    result ~= format("%s_(std::move(%s))", fname, fieldName);
                } else {
                    result ~= format("%s_(%s)", fname, fieldName);
                }
            }
        }
        result ~= " {}\n\n";
    }
    
    return result;
}

string generateGettersSettersWithAccess(T)(ref LanguageInfo info, string indent, 
                                           FieldInfo[] privateFields, FieldInfo[] protectedFields) {
    string result = "";
    
    foreach (finfo; privateFields ~ protectedFields) {
        string getterName = "get" ~ finfo.name[0..1].toUpper ~ finfo.name[1..$];
        string setterName = "set" ~ finfo.name[0..1].toUpper ~ finfo.name[1..$];
        
        result ~= indent ~ format("    const %s& %s() const { return %s_; }\n", 
                                 finfo.mappedType, getterName, finfo.name);
        
        result ~= indent ~ format("    %s& %s() { return %s_; }\n", 
                                 finfo.mappedType, getterName, finfo.name);
        
        if (finfo.isUnique || finfo.isShared) {
            result ~= indent ~ format("    void %s(%s value) { %s_ = std::move(value); }\n\n", 
                                     setterName, finfo.mappedType, finfo.name);
        } else {
            result ~= indent ~ format("    void %s(const %s& value) { %s_ = value; }\n\n", 
                                     setterName, finfo.mappedType, finfo.name);
        }
    }
    
    return result;
}

// ============================================================================
// LEGACY GENERATORS
// ============================================================================

string generateCppConstructor(string className, string[] paramList) {
    string result = "";
    result ~= format("    %s() = default;\n", className);
    
    if (paramList.length > 0) {
        result ~= format("\n    %s(", className);
        foreach (i, param; paramList) {
            if (i > 0) result ~= ", ";
            result ~= param;
        }
        result ~= ")\n        : ";
        
        foreach (i, param; paramList) {
            if (i > 0) result ~= ", ";
            auto parts = param.split(" ");
            if (parts.length >= 2) {
                string fieldName = parts[$-1];
                string typeName = parts[0..$-1].join(" ");
                if (typeName.startsWith("std::unique_ptr") || typeName.startsWith("std::shared_ptr")) {
                    result ~= format("%s_(std::move(%s))", fieldName, fieldName);
                } else {
                    result ~= format("%s_(%s)", fieldName, fieldName);
                }
            }
        }
        result ~= " {}\n";
    }
    
    return result;
}

string generateCppGetter(string fieldName, string typeName, string className, AccessLevel access, bool isStatic) {
    return format("    const %s& get%s() const { return %s_; }", typeName, fieldName.toPascalCase, fieldName);
}

string generateCppSetter(string fieldName, string typeName, string className, AccessLevel access, bool isStatic) {
    return format("    void set%s(const %s& value) { %s_ = value; }", fieldName.toPascalCase, typeName, fieldName);
}

// Alias for backward compatibility
alias generateCppClass2 = generateCppClass;

// ============================================================================
// C++ LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.CPP26, LanguageInfo(
        name: Language.CPP26,
        
        config: LanguageConfig(
            fileExt: "h",
            commentStyle: "//",
            indentStyle: "    "
        ),

        primitives: PrimitiveTypes(
            bool_: ["bool", "{}"],
            string_: ["std::string", "{}"],
            float_: ["float", "{}"],
            double_: ["double", "{}"],
            byte_: ["int8_t", "{}"],
            ubyte_: ["uint8_t", "{}"],
            short_: ["int16_t", "{}"],
            ushort_: ["uint16_t", "{}"],
            int_: ["int32_t", "{}"],
            uint_: ["uint32_t", "{}"],
            long_: ["int64_t", "{}"],
            ulong_: ["uint64_t", "{}"]
        ),
        
        standard: StandardTypes(
            datetime_: ["std::chrono::system_clock::time_point", "{}"],
        ),

        complex: ComplexTypes(
            vector_: ["std::vector<%s>", "{}"],
            map_: ["std::map<%s, %s>", "{}"],
            optional_: ["std::optional<%s>", "std::nullopt"],
            tuple_: ["std::tuple<%s>", "{}"],
            set_: ["std::set<%s>", "{}"]
        ),
        
        namespace: NamespaceConfig(
            separator: "::",
            keyword: "namespace",
            startGenerator: &cppNamespaceStart,
            endGenerator: &cppNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toPascalCase,
            privateMemberName: &toSnakeCaseUnderScore,
            publicMemberName: &toCamelCase,
            fileName: &toFileSnakeCase,
            namespaceName: &toNamespaceStyle,
            getterName: &toPascalCase,
        ),
        
        generators: CodeGenerators(
            getter: &generateCppGetter,
            setter: &generateCppSetter,
            typeModifier: &applyCppModifier,
            constructor: &generateCppConstructor
        )
    ));
}
