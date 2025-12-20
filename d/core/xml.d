module core.xml;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple;
import std.array : replicate;

// ============================================================================
// XML NAMESPACE GENERATORS
// ============================================================================

string xmlNamespaceStart(string[] namespaceParts, bool nested) {
    if (namespaceParts.length == 0) return "";
    return format("<!-- Namespace: %s -->\n", namespaceParts.join("."));
}

string xmlNamespaceEnd(string[] namespaceParts, bool nested) {
    return "";
}

string toXmlNamespaceStyle(string input) {
    return input.toLower;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyXmlModifier(string baseType, FieldModifier modifier) {
    return baseType;
}

// ============================================================================
// XML CODE GENERATORS
// ============================================================================

string generateXmlConstructor(string className, string[] paramList) {
    return "";
}

string generateXmlGetter(string fieldName, string typeName, string className, 
                        AccessLevel access, bool isStatic) {
    return "";
}

string generateXmlSetter(string fieldName, string typeName, string className, 
                        AccessLevel access, bool isStatic) {
    return "";
}

// ============================================================================
// XML SCHEMA GENERATOR
// ============================================================================

string generateXmlClass(T)(string className, int depth = 0) {
    auto info = languageRegistry[Language.XML];
    string indent = "    ".replicate(depth);
    string result = "";
    
    result ~= generateNestedClasses!T(Language.XML, depth);
    
    result ~= "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    result ~= "<xs:schema xmlns:xs=\"http://www.w3.org/2001/XMLSchema\">\n\n";
    
    result ~= indent ~ format("<!-- Complex type definition for %s -->\n", className);
    result ~= indent ~ format("<xs:element name=\"%s\">\n", className.toLower);
    result ~= indent ~ "    <xs:complexType>\n";
    result ~= indent ~ "        <xs:sequence>\n";
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string xmlType = mapToXMLType!FieldType();
            string customName = getCustomXMLName!(T, fieldName);
            
            result ~= indent ~ format("            <xs:element name=\"%s\" type=\"%s\"/>\n", 
                customName, xmlType);
        }
    }
    
    result ~= indent ~ "        </xs:sequence>\n";
    result ~= indent ~ "    </xs:complexType>\n";
    result ~= indent ~ "</xs:element>\n\n";
    
    result ~= indent ~ format("<!-- Example %s instance -->\n", className);
    result ~= indent ~ format("<%s>\n", className.toLower);
    
    static foreach (fieldName; FieldNameTuple!T) {
        {
            string customName = getCustomXMLName!(T, fieldName);
            result ~= indent ~ format("    <%s>example_value</%s>\n", customName, customName);
        }
    }
    
    result ~= indent ~ format("</%s>\n", className.toLower);
    result ~= "\n</xs:schema>\n";
    
    return result;
}

string mapToXMLType(T)() {
    static if (is(T == int) || is(T == uint) || is(T == byte) || is(T == ubyte) ||
               is(T == short) || is(T == ushort)) {
        return "xs:int";
    } else static if (is(T == long) || is(T == ulong)) {
        return "xs:long";
    } else static if (is(T == float)) {
        return "xs:float";
    } else static if (is(T == double)) {
        return "xs:double";
    } else static if (is(T == bool)) {
        return "xs:boolean";
    } else static if (is(T == string)) {
        return "xs:string";
    } else {
        return "xs:string";
    }
}

string getCustomXMLName(T, string fieldName)() {
    import std.traits : getUDAs;
    alias fieldAttrs = getUDAs!(__traits(getMember, T, fieldName), field);
    static if (fieldAttrs.length > 0 && fieldAttrs[0].xmlName.length > 0) {
        return fieldAttrs[0].xmlName;
    } else {
        return fieldName.toLower;
    }
}

// ============================================================================
// XML LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.XML, LanguageInfo(
        config: LanguageConfig(
            fileExt: "xsd",
            commentStyle: "<!--",
            indentStyle: "    "
        ),
        
        primitives: PrimitiveTypes(
            bool_: ["xs:boolean"],
            string_: ["xs:string"],
            float_: ["xs:float"],
            double_: ["xs:double"],
            byte_: ["xs:byte"],
            ubyte_: ["xs:unsignedByte"],
            short_: ["xs:short"],
            ushort_: ["xs:unsignedShort"],
            int_: ["xs:int"],
            uint_: ["xs:unsignedInt"],
            long_: ["xs:long"],
            ulong_: ["xs:unsignedLong"]
        ),
        
        complex: ComplexTypes(
            vector_: ["xs:string"],
            map_: ["xs:string"],
            optional_: ["%s"],
            tuple_: ["", ""],
        ),
        
        namespace: NamespaceConfig(
            separator: ":",
            keyword: "xmlns",
            startGenerator: &xmlNamespaceStart,
            endGenerator: &xmlNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toLowercase,
            privateMemberName: &toLowercase,
            publicMemberName: &toLowercase,
	    fileName: &toFileSnakeCase,
            namespaceName: &toXmlNamespaceStyle
        )
        
    ));
}

