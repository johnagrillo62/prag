module core.python;

import core.types;
import std.format;
import std.string;
import std.traits : FieldNameTuple;
import std.array : replicate;

// ============================================================================
// PYTHON NAMESPACE GENERATORS
// ============================================================================

string pythonNamespaceStart(string[] namespaceParts, bool nested) {
    return "";  // Python doesn't have explicit namespaces, uses file structure
}

string pythonNamespaceEnd(string[] namespaceParts, bool nested) {
    return "";
}

string toPythonNamespaceStyle(string input) {
    return input.toLower;
}

// ============================================================================
// TYPE MODIFIER FUNCTIONS
// ============================================================================

string applyPythonModifier(string baseType, FieldModifier modifier) {
    final switch(modifier) {
        case FieldModifier.None: 
            return baseType;
        case FieldModifier.Optional: 
            return format("Optional[%s]", baseType);
        case FieldModifier.Unique: 
            return baseType;
        case FieldModifier.Shared: 
            return baseType;
        case FieldModifier.Weak: 
            return format("weakref.ref[%s]", baseType);
        case FieldModifier.Immutable: 
            return baseType;
        case FieldModifier.Static: 
            return format("ClassVar[%s]", baseType);
    }
}

// ============================================================================
// PYTHON CODE GENERATORS
// ============================================================================

string generatePythonConstructor(string className, string[] paramList) {
    return format("    def __init__(self):\n        pass");
}

string generatePythonGetter(string fieldName, string typeName, string className, 
                            AccessLevel access, bool isStatic) {
    return format("    @property\n    def %s(self) -> %s:\n        return self._%s", 
        fieldName, typeName, fieldName);
}

string generatePythonSetter(string fieldName, string typeName, string className, 
                            AccessLevel access, bool isStatic) {
    return format("    @%s.setter\n    def %s(self, value: %s):\n        self._%s = value", 
        fieldName, fieldName, typeName, fieldName);
}

// ============================================================================
// PYTHON CLASS GENERATOR
// ============================================================================

string generatePythonClass(T)(string className, int depth = 0) {
    auto info = languageRegistry[Language.PYTHON];
    string indent = "    ".replicate(depth);
    string result = "";
    
    // Generate nested classes first
    result ~= generateNestedClasses!T(Language.PYTHON, depth);
    
    // Imports
    if (depth == 0) {
        result ~= "from typing import List, Dict, Optional, Any\n";
        result ~= "from dataclasses import dataclass, field\n\n";
    }
    
    // Dataclass decorator
    result ~= indent ~ "@dataclass\n";
    result ~= indent ~ format("class %s:\n", className);
    
    // Fields with type hints
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            string fieldType = convertType!FieldType(info);
            string defaultValue = getDefaultValue!FieldType(info);
            
            result ~= indent ~ format("    %s: %s", fieldName, fieldType);
            if (defaultValue.length > 0) {
                result ~= format(" = %s", defaultValue);
            }
            result ~= "\n";
        }
    }
    
    // Post-init for custom initialization
    result ~= "\n" ~ indent ~ "    def __post_init__(self):\n";
    
    // Initialize collections
    bool hasCollections = false;
    static foreach (fieldName; FieldNameTuple!T) {
        {
            alias FieldType = typeof(__traits(getMember, T.init, fieldName));
            static if (isArray!FieldType) {
                result ~= indent ~ format("        if self.%s is None:\n", fieldName);
                result ~= indent ~ format("            self.%s = []\n", fieldName);
                hasCollections = true;
            } else static if (isAssociativeArray!FieldType) {
                result ~= indent ~ format("        if self.%s is None:\n", fieldName);
                result ~= indent ~ format("            self.%s = {}\n", fieldName);
                hasCollections = true;
            }
        }
    }
    
    if (!hasCollections) {
        result ~= indent ~ "        pass\n";
    }
    
    return result;
}

// ============================================================================
// PYTHON LANGUAGE REGISTRATION
// ============================================================================

static this() {
    registerLanguage(Language.PYTHON, LanguageInfo(
        config: LanguageConfig(
            fileExt: "py",
            commentStyle: "#",
            indentStyle: "    "
        ),
        
        primitives: PrimitiveTypes(
            bool_: ["bool", "False"],
            string_: ["str", "''"],
            float_: ["float", "0.0"],
            double_: ["float", "0.0"],
            byte_: ["int", "0"],
            ubyte_: ["int", "0"],
            short_: ["int", "0"],
            ushort_: ["int", "0"],
            int_: ["int", "0"],
            uint_: ["int", "0"],
            long_: ["int", "0"],
            ulong_: ["int", "0"]
        ),
        
        complex: ComplexTypes(
            vector_: ["List[%s]", "None"],
            map_: ["Dict[%s, %s]", "None"],
            optional_: ["Optional[%s]", "None"],
            tuple_: ["tuple[", "]"],
            set_: ["set[%s]", "None"]
        ),
        
        namespace: NamespaceConfig(
            separator: ".",
            keyword: "",
            startGenerator: &pythonNamespaceStart,
            endGenerator: &pythonNamespaceEnd
        ),
        
        naming: NamingConventions(
            className: &toPascalCase,
            privateMemberName: &toSnakeCase,
            publicMemberName: &toSnakeCase,
            fileName: &toFileSnakeCase,
            namespaceName: &toPythonNamespaceStyle
        ),
        
        generators: CodeGenerators(
            getter: &generatePythonGetter,
            setter: &generatePythonSetter,
            typeModifier: &applyPythonModifier,
            constructor: &generatePythonConstructor
        )
    ));
}

