/*
================================================================================
MetaFront - C++ Reflection/Serialization Tool with Inheritance & Nesting
================================================================================

Author: John Grillo
Date: 2025-09-13
Version: 2.0

Description:
------------
Enhanced MetaFront tool with getter/setter detection, inheritance support,
comprehensive type discovery, and function metadata generation.

Features:
---------
- Automatic getter/setter detection through AST body analysis
- Full inheritance chain processing with proper access handling
- Nested type discovery and processing
- Class registry to prevent duplicate processing
- Enhanced code generation with function pointers
- Complete function metadata tuples with properties

================================================================================
*/

#include <algorithm>
#include <cctype>
#include <clang-c/Index.h>
#include <iostream>
#include <optional>
#include <regex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
#include <set>
#include <map>

#include "meta/meta_field.h"

// Forward declarations for function metadata
namespace meta {
    enum class FunctionProp {
        Public = 1,
        Private = 2,
        Protected = 4,
        Static = 8,
        Virtual = 16,
        Override = 32,
        Final = 64,
        Constructor = 128,
        Destructor = 256
    };
    
    template<typename ClassType, typename FuncType, FuncType FuncPtr, FunctionProp Props>
    struct FunctionMeta {
        using class_type = ClassType;
        using function_type = FuncType;
        static constexpr FuncType function_ptr = FuncPtr;
        static constexpr FunctionProp properties = Props;
    };
    
    template<typename T>
    struct MetaFunctions;
}

// -------------------------
// Data Structures
// -------------------------
struct FieldInfo
{
    std::string name;
    std::string type;
    CX_CXXAccessSpecifier access = CX_CXXPrivate;
    std::vector<std::string> annotations;
};

struct ParameterInfo
{
    std::string type;
    std::string name;
};

struct FunctionInfo
{
    std::string name;
    std::string returnType;
    std::vector<ParameterInfo> parameters;
    bool isPublic = false;
    bool isPrivate = false;
    bool isProtected = false;
    bool isVirtual = false;
    bool isStatic = false;
    bool isOverride = false;
    bool isFinal = false;
    bool isConstructor = false;
    bool isDestructor = false;
};

struct ClassInfo
{
    std::string className;
    std::string qualifiedName;
    std::string namespaceName;
    std::vector<FieldInfo> fields;
    bool isAbstract = false;
    size_t sizeBytes = 0;
};

struct GetterSetterInfo {
    std::string getterName;
    std::string setterName;
    bool hasGetter = false;
    bool hasSetter = false;
    std::string returnedFieldName;
    std::string setFieldName;
};

// Global class registry for tracking discovered types
struct ClassRegistry {
    std::vector<std::tuple<ClassInfo, std::vector<FunctionInfo>, CXCursor>> discoveredClasses;
    std::set<std::string> processedTypes;
    
    void addClassIfNew(const ClassInfo& classInfo, const std::vector<FunctionInfo>& functions, CXCursor cursor) {
        if (processedTypes.find(classInfo.qualifiedName) == processedTypes.end()) {
            processedTypes.insert(classInfo.qualifiedName);
            discoveredClasses.emplace_back(classInfo, functions, cursor);
        }
    }
    
    bool isProcessed(const std::string& qualifiedName) const {
        return processedTypes.find(qualifiedName) != processedTypes.end();
    }
};

ClassRegistry g_classRegistry;

// -------------------------
// Forward Declarations
// -------------------------
void walkAST(CXCursor cursor, const std::optional<std::string> fullClassName, std::vector<FieldInfo>& fields);
void generateTuples(std::vector<FieldInfo> fields, std::vector<FunctionInfo> functions, CXCursor classCursor, std::string shortName, std::string ns, std::string qname, std::string tablename);
void generateFunctionTuples(const std::vector<FunctionInfo>& functions, const std::string& shortName, const std::string& ns, const std::string& qname);
std::string getTableName(const std::vector<std::string>& annotations);
std::vector<std::string> getAnnotations(CXCursor cursor);
std::string getQualifiedName(CXCursor cursor);
std::vector<FieldInfo> dumpStructWithNesting(CXCursor cursor, const std::string& prefix = "");
void processClassDeclaration(CXCursor classCursor);
void generateAllClassMetadata();
std::vector<FieldInfo> getAllFieldsWithInheritance(CXCursor classCursor);

// -------------------------
// Utility Functions
// -------------------------
std::string cleanCppType(const std::string& str)
{
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), s.end());

    int angleLevel = 0;
    size_t typeEnd = s.size();
    for (size_t i = s.size(); i-- > 0;)
    {
        if (s[i] == '>') angleLevel++;
        else if (s[i] == '<') angleLevel--;
        else if (std::isspace(static_cast<unsigned char>(s[i])) && angleLevel == 0)
        {
            typeEnd = i;
            break;
        }
    }

    std::string typePart = s.substr(0, typeEnd);
    const std::pair<std::string, std::string> patterns[] = {{" :: ", "::"}, {"< ", "<"}, {" <", "<"}, {"> ", ">"}, {" >", ">"}, {" , ", ","}, {" ,", ","}};
    for (auto& p : patterns)
    {
        size_t pos = 0;
        while ((pos = typePart.find(p.first, pos)) != std::string::npos)
            typePart.replace(pos, p.first.length(), p.second);
    }

    size_t pos = 0;
    while ((pos = typePart.find("  ", pos)) != std::string::npos)
        typePart.replace(pos, 2, " ");

    return typePart;
}

std::string cleanName(const std::string& str)
{
    std::string result = str;
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char c) { return !std::isspace(c); }));
    result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), result.end());

    std::string::size_type pos = 0;
    while ((pos = result.find(":: ", pos)) != std::string::npos) { result.replace(pos, 3, "::"); }
    while ((pos = result.find(" < ", pos)) != std::string::npos) { result.replace(pos, 3, "<"); }
    while ((pos = result.find(" > ", pos)) != std::string::npos) { result.replace(pos, 3, ">"); }
    while ((pos = result.find(" , ", pos)) != std::string::npos) { result.replace(pos, 3, ", "); }

    pos = 0;
    while ((pos = result.find("  ", pos)) != std::string::npos) { result.replace(pos, 2, " "); }

    return result;
}

std::string cleanTypeStr(const std::string& str)
{
    std::string result = str;
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\t'), result.end());
    return result;
}

std::string detectContainer(const std::string& typeStr)
{
    static const std::regex vectorRegex(R"(std::vector\s*<\s*(.+)\s*>)");
    static const std::regex arrayRegex(R"(std::array\s*<\s*(.+)\s*,\s*(\d+)\s*>)");
    static const std::regex mapRegex(R"(std::map\s*<\s*(.+)\s*,\s*(.+)\s*>)");
    static const std::regex setRegex(R"(std::set\s*<\s*(.+)\s*>)");

    std::smatch match;
    if (std::regex_search(typeStr, match, vectorRegex)) { return "std::vector<" + match[1].str() + ">"; }
    else if (std::regex_search(typeStr, match, arrayRegex)) { return "std::array<" + match[1].str() + ", " + match[2].str() + ">"; }
    else if (std::regex_search(typeStr, match, mapRegex)) { return "std::map<" + match[1].str() + ", " + match[2].str() + ">"; }
    else if (std::regex_search(typeStr, match, setRegex)) { return "std::set<" + match[1].str() + ">"; }
    return typeStr;
}

std::string getQualifiedName(CXCursor cursor)
{
    std::vector<std::string> names;
    CXCursor current = cursor;
    CXCursor tuCursor = clang_getTranslationUnitCursor(clang_Cursor_getTranslationUnit(cursor));

    while (!clang_equalCursors(current, tuCursor))
    {
        CXString s = clang_getCursorSpelling(current);
        std::string spelling = clang_getCString(s);
        clang_disposeString(s);

        if (!spelling.empty()) names.push_back(spelling);
        current = clang_getCursorSemanticParent(current);
    }

    std::string fullname;
    for (auto it = names.rbegin(); it != names.rend(); ++it)
    {
        if (!fullname.empty()) fullname += "::";
        fullname += *it;
    }

    return fullname;
}

// -------------------------
// Annotation Processing
// -------------------------
std::vector<std::string> getAnnotations(CXCursor cursor)
{
    std::vector<std::string> annotations;
    
    clang_visitChildren(
        cursor,
        [](CXCursor child, CXCursor parent, CXClientData client_data) -> CXChildVisitResult
        {
            auto* annotations = static_cast<std::vector<std::string>*>(client_data);
            
            if (clang_getCursorKind(child) == CXCursor_AnnotateAttr)
            {
                CXString annotation = clang_getCursorSpelling(child);
                std::string annotationStr = clang_getCString(annotation);
                annotations->push_back(annotationStr);
                clang_disposeString(annotation);
            }
            return CXChildVisit_Continue;
        },
        &annotations);
        
    return annotations;
}

std::string getTableName(const std::vector<std::string>& annotations)
{
    for (const auto& annotation : annotations) {
        if (annotation.find("table_name:") == 0) {
            return annotation.substr(11);
        }
    }
    return "";
}

// -------------------------
// Nested Type Discovery
// -------------------------
bool isUserDefinedType(const std::string& typeStr) {
    std::string cleanType = typeStr;
    
    std::vector<std::string> prefixesToRemove = {"const ", "volatile ", "mutable "};
    for (const auto& prefix : prefixesToRemove) {
        size_t pos = cleanType.find(prefix);
        if (pos == 0) { cleanType = cleanType.substr(prefix.length()); }
    }
    
    cleanType.erase(std::remove(cleanType.begin(), cleanType.end(), '&'), cleanType.end());
    cleanType.erase(std::remove(cleanType.begin(), cleanType.end(), '*'), cleanType.end());
    
    size_t templatePos = cleanType.find('<');
    if (templatePos != std::string::npos) { cleanType = cleanType.substr(0, templatePos); }
    
    cleanType.erase(cleanType.begin(), std::find_if(cleanType.begin(), cleanType.end(), [](unsigned char c) { return !std::isspace(c); }));
    cleanType.erase(std::find_if(cleanType.rbegin(), cleanType.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), cleanType.end());
    
    static const std::set<std::string> builtInTypes = {
        "void", "bool", "char", "wchar_t", "char16_t", "char32_t",
        "signed char", "unsigned char", "short", "unsigned short",
        "int", "unsigned int", "long", "unsigned long", "long long", "unsigned long long",
        "float", "double", "long double"
    };
    
    if (builtInTypes.find(cleanType) != builtInTypes.end()) { return false; }
    if (cleanType.find("std::") == 0) { return false; }
    
    return true;
}

void processNestedType(CXCursor fieldCursor, CXTranslationUnit tu) {
    CXType fieldType = clang_getCursorType(fieldCursor);
    CXCursor typeDecl = clang_getTypeDeclaration(fieldType);
    
    if (clang_getCursorKind(typeDecl) == CXCursor_StructDecl || 
        clang_getCursorKind(typeDecl) == CXCursor_ClassDecl) {
        
        std::string qualifiedName = getQualifiedName(typeDecl);
        
        if (g_classRegistry.isProcessed(qualifiedName) || clang_Cursor_isAnonymous(typeDecl)) {
            return;
        }
        
        std::cout << "// Discovered nested type: " << qualifiedName << "\n";
        processClassDeclaration(typeDecl);
    }
    
    CXType canonicalType = clang_getCanonicalType(fieldType);
    if (canonicalType.kind == CXType_Record) {
        int numTemplateArgs = clang_Type_getNumTemplateArguments(fieldType);
        for (int i = 0; i < numTemplateArgs; ++i) {
            CXType templateArg = clang_Type_getTemplateArgumentAsType(fieldType, i);
            CXCursor templateTypeDecl = clang_getTypeDeclaration(templateArg);
            
            if (clang_getCursorKind(templateTypeDecl) == CXCursor_StructDecl || 
                clang_getCursorKind(templateTypeDecl) == CXCursor_ClassDecl) {
                
                std::string templateQualifiedName = getQualifiedName(templateTypeDecl);
                if (!g_classRegistry.isProcessed(templateQualifiedName) && 
                    !clang_Cursor_isAnonymous(templateTypeDecl)) {
                    
                    std::cout << "// Discovered template parameter type: " << templateQualifiedName << "\n";
                    processClassDeclaration(templateTypeDecl);
                }
            }
        }
    }
}

// -------------------------
// Method Body Analysis for Getter/Setter Detection
// -------------------------
std::string analyzeGetterBody(CXCursor methodCursor, const std::vector<FieldInfo>& fields) {
    std::string returnedField;
    
    struct AnalysisData {
        std::string* returnedField;
        const std::vector<FieldInfo>* fields;
        int statementCount = 0;
        bool isSimpleReturn = false;
    };
    
    AnalysisData data{&returnedField, &fields, 0, false};
    
    clang_visitChildren(
        methodCursor,
        [](CXCursor cursor, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
            auto* data = static_cast<AnalysisData*>(client_data);
            CXCursorKind kind = clang_getCursorKind(cursor);
            
            if (kind == CXCursor_CompoundStmt) {
                clang_visitChildren(
                    cursor,
                    [](CXCursor child, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
                        auto* data = static_cast<AnalysisData*>(client_data);
                        CXCursorKind childKind = clang_getCursorKind(child);
                        
                        if (childKind == CXCursor_ReturnStmt || childKind == CXCursor_DeclStmt ||
                            childKind == CXCursor_UnexposedStmt || childKind == CXCursor_IfStmt ||
                            childKind == CXCursor_ForStmt || childKind == CXCursor_WhileStmt) {
                            data->statementCount++;
                            
                            if (childKind == CXCursor_ReturnStmt && data->statementCount == 1) {
                                clang_visitChildren(
                                    child,
                                    [](CXCursor returnChild, CXCursor returnParent, CXClientData client_data) -> CXChildVisitResult {
                                        auto* data = static_cast<AnalysisData*>(client_data);
                                        
                                        if (clang_getCursorKind(returnChild) == CXCursor_MemberRefExpr) {
                                            CXString memberName = clang_getCursorSpelling(returnChild);
                                            std::string memberStr = clang_getCString(memberName);
                                            
                                            for (const auto& field : *(data->fields)) {
                                                if (field.name == memberStr) {
                                                    *(data->returnedField) = memberStr;
                                                    data->isSimpleReturn = true;
                                                    break;
                                                }
                                            }
                                            
                                            clang_disposeString(memberName);
                                        }
                                        
                                        return CXChildVisit_Recurse;
                                    },
                                    client_data
                                );
                            }
                        }
                        
                        return CXChildVisit_Continue;
                    },
                    client_data
                );
                
                return CXChildVisit_Continue;
            }
            
            return CXChildVisit_Recurse;
        },
        &data
    );
    
    if (data.statementCount == 1 && data.isSimpleReturn) {
        return returnedField;
    }
    
    return "";
}

std::string analyzeSetterBody(CXCursor methodCursor, const std::vector<FieldInfo>& fields) {
    std::string setField;
    
    struct AnalysisData {
        std::string* setField;
        const std::vector<FieldInfo>* fields;
        int statementCount = 0;
        bool isSimpleAssignment = false;
    };
    
    AnalysisData data{&setField, &fields, 0, false};
    
    clang_visitChildren(
        methodCursor,
        [](CXCursor cursor, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
            auto* data = static_cast<AnalysisData*>(client_data);
            CXCursorKind kind = clang_getCursorKind(cursor);
            
            if (kind == CXCursor_CompoundStmt) {
                clang_visitChildren(
                    cursor,
                    [](CXCursor child, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
                        auto* data = static_cast<AnalysisData*>(client_data);
                        CXCursorKind childKind = clang_getCursorKind(child);
                        
                        if (childKind == CXCursor_UnexposedStmt || childKind == CXCursor_DeclStmt ||
                            childKind == CXCursor_ReturnStmt || childKind == CXCursor_IfStmt ||
                            childKind == CXCursor_ForStmt || childKind == CXCursor_WhileStmt) {
                            data->statementCount++;
                            
                            if (childKind == CXCursor_UnexposedStmt && data->statementCount == 1) {
                                clang_visitChildren(
                                    child,
                                    [](CXCursor exprChild, CXCursor exprParent, CXClientData client_data) -> CXChildVisitResult {
                                        auto* data = static_cast<AnalysisData*>(client_data);
                                        
                                        if (clang_getCursorKind(exprChild) == CXCursor_BinaryOperator) {
                                            CXSourceRange range = clang_getCursorExtent(exprChild);
                                            CXTranslationUnit tu = clang_Cursor_getTranslationUnit(exprChild);
                                            
                                            CXToken* tokens = nullptr;
                                            unsigned numTokens = 0;
                                            clang_tokenize(tu, range, &tokens, &numTokens);
                                            
                                            bool hasAssignment = false;
                                            for (unsigned i = 0; i < numTokens; ++i) {
                                                CXString spelling = clang_getTokenSpelling(tu, tokens[i]);
                                                std::string tokenStr = clang_getCString(spelling);
                                                if (tokenStr == "=") { hasAssignment = true; }
                                                clang_disposeString(spelling);
                                            }
                                            clang_disposeTokens(tu, tokens, numTokens);
                                            
                                            if (hasAssignment) {
                                                clang_visitChildren(
                                                    exprChild,
                                                    [](CXCursor assignChild, CXCursor assignParent, CXClientData client_data) -> CXChildVisitResult {
                                                        auto* data = static_cast<AnalysisData*>(client_data);
                                                        
                                                        if (clang_getCursorKind(assignChild) == CXCursor_MemberRefExpr) {
                                                            CXString memberName = clang_getCursorSpelling(assignChild);
                                                            std::string memberStr = clang_getCString(memberName);
                                                            
                                                            for (const auto& field : *(data->fields)) {
                                                                if (field.name == memberStr) {
                                                                    *(data->setField) = memberStr;
                                                                    data->isSimpleAssignment = true;
                                                                    break;
                                                                }
                                                            }
                                                            
                                                            clang_disposeString(memberName);
                                                        }
                                                        
                                                        return CXChildVisit_Continue;
                                                    },
                                                    client_data
                                                );
                                            }
                                        }
                                        
                                        return CXChildVisit_Recurse;
                                    },
                                    client_data
                                );
                            }
                        }
                        
                        return CXChildVisit_Continue;
                    },
                    client_data
                );
                
                return CXChildVisit_Continue;
            }
            
            return CXChildVisit_Recurse;
        },
        &data
    );
    
    if (data.statementCount == 1 && data.isSimpleAssignment) {
        return setField;
    }
    
    return "";
}

bool typesCompatible(const std::string& type1, const std::string& type2) {
    auto cleanType = [](std::string t) {
        size_t pos = t.find("const ");
        if (pos != std::string::npos) { t.erase(pos, 6); }
        pos = t.find('&');
        if (pos != std::string::npos) { t.erase(pos, 1); }
        t.erase(std::remove(t.begin(), t.end(), ' '), t.end());
        return t;
    };
    
    return cleanType(type1) == cleanType(type2);
}

std::vector<GetterSetterInfo> findGettersSetters(const std::vector<FieldInfo>& fields, const std::vector<FunctionInfo>& functions, CXCursor classCursor) {
    std::vector<GetterSetterInfo> results;
    
    auto data = std::make_tuple(&fields, &functions, &results);
    
    clang_visitChildren(
        classCursor,
        [](CXCursor child, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
            CXCursorKind childKind = clang_getCursorKind(child);
            
            if (childKind == CXCursor_CXXMethod) {
                auto* data = static_cast<std::tuple<const std::vector<FieldInfo>*, const std::vector<FunctionInfo>*, std::vector<GetterSetterInfo>*>*>(client_data);
                const auto& fields = *std::get<0>(*data);
                const auto& functions = *std::get<1>(*data);
                auto& results = *std::get<2>(*data);
                
                CX_CXXAccessSpecifier access = clang_getCXXAccessSpecifier(child);
                if (access != CX_CXXPublic) {
                    return CXChildVisit_Continue;
                }
                
                CXString methodName = clang_getCursorSpelling(child);
                std::string methodNameStr = clang_getCString(methodName);
                clang_disposeString(methodName);
                
                const FunctionInfo* funcInfo = nullptr;
                for (const auto& func : functions) {
                    if (func.name == methodNameStr) {
                        funcInfo = &func;
                        break;
                    }
                }
                
                if (!funcInfo) {
                    return CXChildVisit_Continue;
                }
                
                if (funcInfo->parameters.empty() && funcInfo->returnType != "void") {
                    std::string returnedField = analyzeGetterBody(child, fields);
                    if (!returnedField.empty()) {
                        auto it = std::find_if(results.begin(), results.end(), 
                            [&returnedField](const GetterSetterInfo& info) {
                                return info.returnedFieldName == returnedField || info.setFieldName == returnedField;
                            });
                        
                        if (it != results.end()) {
                            it->hasGetter = true;
                            it->getterName = methodNameStr;
                            it->returnedFieldName = returnedField;
                        } else {
                            GetterSetterInfo info;
                            info.hasGetter = true;
                            info.getterName = methodNameStr;
                            info.returnedFieldName = returnedField;
                            results.push_back(info);
                        }
                    }
                }
                
                if (funcInfo->parameters.size() == 1 && 
                    (funcInfo->returnType == "void" || typesCompatible(funcInfo->returnType, funcInfo->parameters[0].type))) {
                    std::string setField = analyzeSetterBody(child, fields);
                    if (!setField.empty()) {
                        auto it = std::find_if(results.begin(), results.end(), 
                            [&setField](const GetterSetterInfo& info) {
                                return info.returnedFieldName == setField || info.setFieldName == setField;
                            });
                        
                        if (it != results.end()) {
                            it->hasSetter = true;
                            it->setterName = methodNameStr;
                            it->setFieldName = setField;
                        } else {
                            GetterSetterInfo info;
                            info.hasSetter = true;
                            info.setterName = methodNameStr;
                            info.setFieldName = setField;
                            results.push_back(info);
                        }
                    }
                }
            }
            
            return CXChildVisit_Continue;
        },
        &data
    );
    
    return results;
}

GetterSetterInfo findGetterSetterForField(const FieldInfo& field, const std::vector<GetterSetterInfo>& gettersSetters) {
    for (const auto& info : gettersSetters) {
        if (info.returnedFieldName == field.name || info.setFieldName == field.name) {
            return info;
        }
    }
    return GetterSetterInfo{};
}

// -------------------------
// Field Extraction with Inheritance
// -------------------------
std::vector<FieldInfo> dumpStructWithNesting(CXCursor cursor, const std::string& prefix)
{
    struct LambdaData
    {
        std::vector<FieldInfo>* fields;
        std::string prefix;
    };

    std::vector<FieldInfo> fields;
    LambdaData data{&fields, prefix};

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data)
        {
            auto* data = static_cast<LambdaData*>(client_data);
            CXCursorKind kind = clang_getCursorKind(c);

            if (kind == CXCursor_FieldDecl)
            {
                CX_CXXAccessSpecifier access = clang_getCXXAccessSpecifier(c);
                CXString fieldName = clang_getCursorSpelling(c);
                std::string nameStr = clang_getCString(fieldName);

                CXSourceRange range = clang_getCursorExtent(c);
                CXTranslationUnit tu = clang_Cursor_getTranslationUnit(c);

                auto annotations = getAnnotations(c);

                CXToken* tokens = nullptr;
                unsigned numTokens = 0;
                clang_tokenize(tu, range, &tokens, &numTokens);

                std::string sourceText;
                for (unsigned i = 0; i < numTokens; ++i)
                {
                    CXString spelling = clang_getTokenSpelling(tu, tokens[i]);
                    sourceText += clang_getCString(spelling);
                    clang_disposeString(spelling);
                    sourceText += " ";
                }
                clang_disposeTokens(tu, tokens, numTokens);

                std::string typeStr = detectContainer(cleanTypeStr(sourceText));
                data->fields->push_back({data->prefix + nameStr, typeStr, access, annotations});

                processNestedType(c, tu);
                clang_disposeString(fieldName);
            }

            return CXChildVisit_Continue;
        },
        &data);

    return fields;
}

std::vector<FieldInfo> getAllFieldsWithInheritance(CXCursor classCursor) {
    std::vector<FieldInfo> allFields;
    
    clang_visitChildren(
        classCursor,
        [](CXCursor child, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
            auto* allFields = static_cast<std::vector<FieldInfo>*>(client_data);
            CXCursorKind childKind = clang_getCursorKind(child);
            
            if (childKind == CXCursor_CXXBaseSpecifier) {
                CXType baseType = clang_getCursorType(child);
                CXCursor baseDecl = clang_getTypeDeclaration(baseType);
                
                if (clang_getCursorKind(baseDecl) == CXCursor_StructDecl || 
                    clang_getCursorKind(baseDecl) == CXCursor_ClassDecl) {
                    
                    auto baseFields = getAllFieldsWithInheritance(baseDecl);
                    CX_CXXAccessSpecifier inheritanceAccess = clang_getCXXAccessSpecifier(child);
                    
                    for (auto field : baseFields) {
                        if (inheritanceAccess == CX_CXXPrivate) {
                            field.access = CX_CXXPrivate;
                        } else if (inheritanceAccess == CX_CXXProtected && field.access == CX_CXXPublic) {
                            field.access = CX_CXXProtected;
                        }
                        allFields->push_back(field);
                    }
                }
            }
            
            return CXChildVisit_Continue;
        },
        &allFields
    );
    
    auto ownFields = dumpStructWithNesting(classCursor);
    allFields.insert(allFields.end(), ownFields.begin(), ownFields.end());
    
    return allFields;
}

std::vector<FieldInfo> dumpStruct(CXCursor cursor, const std::string& prefix = "")
{
    return dumpStructWithNesting(cursor, prefix);
}

// -------------------------
// Class Processing
// -------------------------
void processClassDeclaration(CXCursor classCursor) {
    if (!clang_equalCursors(classCursor, clang_getNullCursor()) &&
        (clang_getCursorKind(classCursor) == CXCursor_StructDecl || 
         clang_getCursorKind(classCursor) == CXCursor_ClassDecl)) {
        
        std::string qname = getQualifiedName(classCursor);
        
        if (g_classRegistry.isProcessed(qname)) {
            return;
        }
        
        ClassInfo info;
        info.className = clang_getCString(clang_getCursorSpelling(classCursor));
        info.qualifiedName = qname;
        
        std::string ns = qname;
        std::size_t pos = ns.rfind("::");
        if (pos != std::string::npos) {
            ns = ns.substr(0, pos);
        } else {
            ns = "";
        }
        info.namespaceName = ns;

        info.isAbstract = false;
        info.sizeBytes = 0;
        
        // Process base classes
        clang_visitChildren(
            classCursor,
            [](CXCursor child, CXCursor parent, CXClientData client_data) -> CXChildVisitResult {
                CXCursorKind childKind = clang_getCursorKind(child);
                
                if (childKind == CXCursor_CXXBaseSpecifier) {
                    CXType baseType = clang_getCursorType(child);
                    CXString baseTypeStr = clang_getTypeSpelling(baseType);
                    std::string baseTypeName = clang_getCString(baseTypeStr);
                    clang_disposeString(baseTypeStr);
                    
                    CX_CXXAccessSpecifier access = clang_getCXXAccessSpecifier(child);
                    std::string accessStr = "private";
                    switch (access) {
                        case CX_CXXPublic: accessStr = "public"; break;
                        case CX_CXXProtected: accessStr = "protected"; break;
                        case CX_CXXPrivate: accessStr = "private"; break;
                    }
                    
                    bool isVirtual = false;
                    CXSourceRange range = clang_getCursorExtent(child);
                    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(child);
                    CXToken* tokens = nullptr;
                    unsigned numTokens = 0;
                    clang_tokenize(tu, range, &tokens, &numTokens);
                    
                    for (unsigned i = 0; i < numTokens; ++i) {
                        CXString spelling = clang_getTokenSpelling(tu, tokens[i]);
                        std::string tokenStr = clang_getCString(spelling);
                        if (tokenStr == "virtual") {
                            isVirtual = true;
                        }
                        clang_disposeString(spelling);
                    }
                    clang_disposeTokens(tu, tokens, numTokens);
                    
                    CXCursor baseDecl = clang_getTypeDeclaration(baseType);
                    if (clang_getCursorKind(baseDecl) == CXCursor_StructDecl || 
                        clang_getCursorKind(baseDecl) == CXCursor_ClassDecl) {
                        
                        std::string baseQualifiedName = getQualifiedName(baseDecl);
                        std::cout << "// Discovered base class: " << baseQualifiedName 
                                  << " (" << accessStr << (isVirtual ? ", virtual" : "") << ")\n";
                        
                        if (!g_classRegistry.isProcessed(baseQualifiedName)) {
                            processClassDeclaration(baseDecl);
                        }
                    }
                }
                
                return CXChildVisit_Continue;
            },
            nullptr
        );
        
        auto fields = dumpStructWithNesting(classCursor);

        std::vector<FunctionInfo> functions;
        clang_visitChildren(
            classCursor,
            [](CXCursor child, CXCursor parent, CXClientData func_data)
            {
                auto* functions = static_cast<std::vector<FunctionInfo>*>(func_data);
                CXCursorKind childKind = clang_getCursorKind(child);

                if (childKind == CXCursor_CXXMethod || childKind == CXCursor_FunctionDecl ||
                    childKind == CXCursor_Constructor || childKind == CXCursor_Destructor)
                {
                    FunctionInfo funcInfo;

                    CXString funcName = clang_getCursorSpelling(child);
                    funcInfo.name = clang_getCString(funcName);
                    clang_disposeString(funcName);

                    CXType returnType = clang_getCursorResultType(child);
                    CXString returnTypeStr = clang_getTypeSpelling(returnType);
                    funcInfo.returnType = clang_getCString(returnTypeStr);
                    clang_disposeString(returnTypeStr);

                    CX_CXXAccessSpecifier access = clang_getCXXAccessSpecifier(child);
                    funcInfo.isPublic = (access == CX_CXXPublic);
                    funcInfo.isPrivate = (access == CX_CXXPrivate);
                    funcInfo.isProtected = (access == CX_CXXProtected);

                    // Check for virtual, static, override, final properties
                    funcInfo.isVirtual = clang_CXXMethod_isVirtual(child);
                    funcInfo.isStatic = clang_CXXMethod_isStatic(child);
                    funcInfo.isOverride = false; // libclang doesn't have direct support, would need token analysis
                    funcInfo.isFinal = false;    // libclang doesn't have direct support, would need token analysis
                    funcInfo.isConstructor = (childKind == CXCursor_Constructor);
                    funcInfo.isDestructor = (childKind == CXCursor_Destructor);

                    int numParams = clang_Cursor_getNumArguments(child);
                    for (int i = 0; i < numParams; ++i)
                    {
                        CXCursor param = clang_Cursor_getArgument(child, i);
                        CXType paramType = clang_getCursorType(param);
                        CXString paramTypeStr = clang_getTypeSpelling(paramType);
                        CXString paramName = clang_getCursorSpelling(param);

                        ParameterInfo paramInfo;
                        paramInfo.type = clang_getCString(paramTypeStr);
                        paramInfo.name = clang_getCString(paramName);
                        funcInfo.parameters.push_back(paramInfo);

                        clang_disposeString(paramTypeStr);
                        clang_disposeString(paramName);
                    }

                    functions->push_back(funcInfo);
                }

                return CXChildVisit_Continue;
            },
            &functions);

        g_classRegistry.addClassIfNew(info, functions, classCursor);
    }
}

// -------------------------
// Function Tuple Generation
// -------------------------
void generateFunctionTuples(const std::vector<FunctionInfo>& functions, 
                           const std::string& shortName, 
                           const std::string& ns, 
                           const std::string& qname) {
    if (functions.empty()) return;
    
    std::cout << "// Function metadata for " << qname << "\n";
    std::cout << "namespace meta\n{\n";
    
    if (!ns.empty() && ns != shortName) {
        std::cout << "namespace " << ns << "\n{\n";
    }
    
    std::cout << "static constexpr auto " << shortName << "_functions = std::tuple<\n";
    
    for (size_t i = 0; i < functions.size(); ++i) {
        const auto& func = functions[i];
        
        // Generate function pointer type
        std::string funcPtrType = func.returnType + "(::";
        funcPtrType += qname + "::*)(";
        
        for (size_t j = 0; j < func.parameters.size(); ++j) {
            funcPtrType += func.parameters[j].type;
            if (j + 1 < func.parameters.size()) funcPtrType += ", ";
        }
        funcPtrType += ")";
        
        // Build properties using bitwise OR
        std::vector<std::string> props;
        if (func.isPublic) props.push_back("FunctionProp::Public");
        if (func.isPrivate) props.push_back("FunctionProp::Private");
        if (func.isProtected) props.push_back("FunctionProp::Protected");
        if (func.isStatic) props.push_back("FunctionProp::Static");
        if (func.isVirtual) props.push_back("FunctionProp::Virtual");
        if (func.isOverride) props.push_back("FunctionProp::Override");
        if (func.isFinal) props.push_back("FunctionProp::Final");
        if (func.isConstructor) props.push_back("FunctionProp::Constructor");
        if (func.isDestructor) props.push_back("FunctionProp::Destructor");
        
        std::string properties = "static_cast<FunctionProp>(";
        if (props.empty()) {
            properties += "0";
        } else {
            for (size_t p = 0; p < props.size(); ++p) {
                properties += props[p];
                if (p + 1 < props.size()) properties += " | ";
            }
        }
        properties += ")";
        
        std::cout << "    FunctionMeta<::" << qname << ", " << funcPtrType;
        
        // Add function pointer if it's a regular method (not constructor/destructor)
        if (!func.isConstructor && !func.isDestructor) {
            std::cout << ", &::" << qname << "::" << func.name;
        } else {
            std::cout << ", nullptr";
        }
        
        std::cout << ", " << properties << ">";
        
        if (i + 1 < functions.size()) std::cout << ",";
        std::cout << "\n";
    }
    
    std::cout << ">{\n";
    
    // Function metadata information
    for (size_t i = 0; i < functions.size(); ++i) {
        const auto& func = functions[i];
        std::cout << "    {\"" << func.returnType << "\", \"" << func.name << "\", {";
        
        for (size_t j = 0; j < func.parameters.size(); ++j) {
            std::cout << "{\"" << func.parameters[j].type << "\", \"" 
                      << func.parameters[j].name << "\"}";
            if (j + 1 < func.parameters.size()) std::cout << ", ";
        }
        
        std::cout << "}}";
        if (i + 1 < functions.size()) std::cout << ",";
        std::cout << "\n";
    }
    
    std::cout << "};\n";
    
    if (!ns.empty() && ns != shortName) {
        std::cout << "}\n";
    }
    
    std::cout << "}\n\n";
}

// -------------------------
// Code Generation
// -------------------------
void generateTuples(std::vector<FieldInfo> fields, 
                   std::vector<FunctionInfo> functions,
                   CXCursor classCursor,
                   std::string shortName, 
                   std::string ns, 
                   std::string qname, 
                   std::string tablename)
{
    // Generate function metadata first
    generateFunctionTuples(functions, shortName, ns, qname);
    
    if (fields.size() > 0)
    {
        auto gettersSetters = findGettersSetters(fields, functions, classCursor);
        
        std::cout << "namespace meta\n{\n";

        if (!ns.empty() && ns != shortName)
        {
            std::cout << "namespace " << ns << "\n{\n";
        }

        std::cout << "static constexpr auto " << shortName << " = "
                  << "std::tuple<\n";

        for (size_t i = 0; i < fields.size(); ++i)
        {
            const auto& field = fields[i];
            auto getterSetterInfo = findGetterSetterForField(field, gettersSetters);

            const std::string member = (field.access == CX_CXXPublic)
                                         ? "&::" + qname + "::" + field.name
                                         : "nullptr";

            std::string getter = "nullptr";
            if (getterSetterInfo.hasGetter) {
                getter = "&::" + qname + "::" + getterSetterInfo.getterName;
            }

            std::string setter = "nullptr";
            if (getterSetterInfo.hasSetter) {
                setter = "&::" + qname + "::" + getterSetterInfo.setterName;
            }

            FieldProp properties = FieldProp::Serializable;
            if (field.access != CX_CXXPublic) {
                properties = static_cast<FieldProp>(properties | FieldProp::Private);
            }
            if (getterSetterInfo.hasGetter) {
                properties = static_cast<FieldProp>(properties | FieldProp::Getter);
            }
            if (getterSetterInfo.hasSetter) {
                properties = static_cast<FieldProp>(properties | FieldProp::Setter);
            }

            std::cout << "        FieldMeta<" << "::" << qname << ", "
                      << cleanCppType(field.type) << ", " << member << ", " << getter << ", "
                      << setter << ", " << fieldPropsToString(properties) + ">";
            if (i + 1 < fields.size())
                std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "    >{\n";

        for (size_t i = 0; i < fields.size(); ++i)
        {
            std::cout << "        { \"" << cleanCppType(fields[i].type) << "\", \""
                      << fields[i].name << "\"";

            for (auto a : fields[i].annotations)
            {
                std::cout << ", \"" + a + "\"";
            }
            std::cout << "}";

            if (i + 1 < fields.size())
                std::cout << ",";
            std::cout << "\n";
        }

        std::cout << "   };\n";

        if (!ns.empty() && ns != shortName)
        {
            std::cout << "}\n";
        }

        std::cout << "}\n";
        std::cout << "namespace meta {\n"
                  << "// Specialization for " << qname << "\ntemplate<>\n"
                  << "struct MetaTuple<::" << qname << "> {\n"
                  << "static constexpr auto& fields = meta::" 
                  << (!ns.empty() && ns != shortName ? ns + "::" : "") << shortName << ";\n";

        if (!tablename.empty()) {
            std::cout << "static constexpr const char* " << "table_name = \"" 
                      << tablename << "\";\n";
        } else {
            std::cout << "static constexpr const char* " << "table_name = \"" 
                      << shortName << "\";\n";
        }

        std::cout << "};\n";
        
        // Add function metadata specialization
        if (!functions.empty()) {
            std::cout << "template<>\n"
                      << "struct MetaFunctions<::" << qname << "> {\n"
                      << "static constexpr auto& functions = meta::" 
                      << (!ns.empty() && ns != shortName ? ns + "::" : "") << shortName << "_functions;\n"
                      << "};\n";
        }
        
        std::cout << "}\n";
    }
    else if (!functions.empty()) {
        // Generate function metadata even if no fields
        std::cout << "namespace meta {\n"
                  << "template<>\n"
                  << "struct MetaFunctions<::" << qname << "> {\n"
                  << "static constexpr auto& functions = meta::" 
                  << (!ns.empty() && ns != shortName ? ns + "::" : "") << shortName << "_functions;\n"
                  << "};\n"
                  << "}\n";
    }
}

void generateAllClassMetadata() {
    std::cout << "// Generated reflection metadata for " << g_classRegistry.discoveredClasses.size() << " classes\n\n";
    
    for (const auto& [classInfo, functions, cursor] : g_classRegistry.discoveredClasses) {
        std::string shortName = classInfo.className;
        std::string ns = classInfo.namespaceName;
        std::string qname = classInfo.qualifiedName;
        
        auto classAnnotations = getAnnotations(cursor);
        std::string tablename = getTableName(classAnnotations);
        
        auto fields = getAllFieldsWithInheritance(cursor);
        
        if (!functions.empty())
        {
            std::cout << "\n// Functions found in " << qname << ":\n";
            for (const auto& func : functions)
            {
                std::cout << "// " << func.returnType << " " << func.name << "(";
                for (size_t i = 0; i < func.parameters.size(); ++i)
                {
                    std::cout << func.parameters[i].type << " " << func.parameters[i].name;
                    if (i + 1 < func.parameters.size())
                        std::cout << ", ";
                }
                std::cout << ") - Access: "
                          << (func.isPublic    ? "public"
                              : func.isPrivate ? "private"
                                               : "protected");
                if (func.isVirtual) std::cout << ", virtual";
                if (func.isStatic) std::cout << ", static";
                if (func.isConstructor) std::cout << ", constructor";
                if (func.isDestructor) std::cout << ", destructor";
                std::cout << "\n";
            }
        }
        
        generateTuples(fields, functions, cursor, shortName, ns, qname, tablename);
    }
}

// -------------------------
// AST Walking
// -------------------------
void walkAST(CXCursor cursor,
             const std::optional<std::string> fullClassName,
             std::vector<FieldInfo>& fields)
{
    g_classRegistry = ClassRegistry{};
    
    struct LambdaData
    {
        std::optional<std::string> target;
        std::vector<FieldInfo>* fields;
    };

    LambdaData data{fullClassName, &fields};
    
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data)
        {
            auto* data = static_cast<LambdaData*>(client_data);
            CXCursorKind kind = clang_getCursorKind(c);

            if (kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl)
            {
                std::string qname = getQualifiedName(c);

                if (!data->target || data->target.value() == qname)
                {
                    processClassDeclaration(c);
                }
            }

            return CXChildVisit_Recurse;
        },
        &data);

    generateAllClassMetadata();
}

// -------------------------
// Main
// -------------------------
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <header.h> [Fully::Qualified::ClassName]\n";
        std::cerr << "If no class name provided, processes all classes in the header.\n";
        return 1;
    }

    const std::string headerFile = argv[1];
    const std::optional<std::string> fullClassName =
        (argc >= 3) ? std::make_optional(argv[2]) : std::nullopt;

    g_classRegistry = ClassRegistry{};

    CXIndex index = clang_createIndex(0, 0);

    const char* args[] = {"-std=c++20",
                          "-x",
                          "c++-header",
                          "-fno-delayed-template-parsing",
                          "-I.",
                          "-Ibhw",
                          "-isystem/usr/include",
                          "-isystem/usr/include/x86_64-linux-gnu",
                          "-isystem/usr/local/include"};

    CXTranslationUnit tu =
        clang_parseTranslationUnit(index,
                                   headerFile.c_str(),
                                   args,
                                   sizeof(args) / sizeof(args[0]),
                                   nullptr,
                                   0,
                                   CXTranslationUnit_DetailedPreprocessingRecord);

    if (!tu)
    {
        std::cerr << "Failed to parse translation unit.\n";
        return 1;
    }

    CXCursor cursor = clang_getTranslationUnitCursor(tu);
    std::vector<FieldInfo> fields;
    walkAST(cursor, fullClassName, fields);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    return 0;
}
