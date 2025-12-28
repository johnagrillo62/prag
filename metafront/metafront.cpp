/*
================================================================================
MetaFront - C++ Reflection/Serialization Tool
================================================================================

Author: John Grillo
Date: 2025-09-13
Version: 1.0

Description:
------------
This header documents the MetaFront tool, a standalone CLI program that reads
the AST of C++ source code and automatically generates type-safe, fully
namespaced reflection metadata for any data class.

Features:
---------
- Extracts all fields from any C++ class, including nested STL containers,
  tuples, maps, sets, and vectors.
- Generates type-safe C++ metadata tuples using `FieldMeta` and `MetaTuple`.
- Preserves original namespaces to avoid name collisions.
- Outputs headers ready for inclusion in projects for reflection,
  serialization, or generic code.
- Works as a standalone CLI; no compiler integration required.


Why It’s Unique:
----------------
- Automatically generates reflection metadata for all classes.
- Type-safe, fully namespaced, and immediately usable.
- Avoids manual macros, template registration, or IR reconstruction.
- Enables universal serialization and reflection across any C++ project.
- Solo implementation demonstrates the feasibility of practical compile-time
  reflection today, long before official C++ reflection features.

Usage:
------
1. Run the CLI tool on your C++ project.
2. The tool generates .meta  headers containing `MetaTuple` specializations
   for each class.
3. Include the generated headers in your project to access reflection
   metadata.
4. Use the metadata for serialization, debugging, or generic programming.

License:
--------
[Apache License]

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

#include "meta/meta_field.h"

// -------------------------
// Field info struct
// -------------------------
struct FieldInfo
{
    std::string name;
    std::string type;
    CX_CXXAccessSpecifier access = CX_CXXPrivate;
    std::vector<std::string> annotations;
};

// 0 Private
// 1 Protectded
// 2 GLobal

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
};

// -------------------------
void walkAST(CXCursor cursor,
             const std::optional<std::string> fullClassName,
             std::vector<FieldInfo>& fields);


void generateTuples(std::vector<FieldInfo> fields, std::string shortName, std::string ns, std::string qname, std::string tablename);


std::string getTableName(const std::vector<std::string>& annotations);

// -------------------------
// Main
// -------------------------
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <header.h> <Fully::Qualified::ClassName>\n";
        return 1;
    }

    const std::string headerFile = argv[1];

    const std::optional<std::string> fullClassName =
        (argc >= 3) ? std::make_optional(argv[2]) : std::nullopt;

    CXIndex index = clang_createIndex(0, 0);

    const char* args[] = {
      "-std=c++20",
      "-x", "c++",
      "-isystem/usr/include/c++/13",        // For std::vector, std::string
      "-isystem/usr/include/x86_64-linux-gnu/c++/13",
      "-isystem/usr/include",
      "-fno-delayed-template-parsing"
    };
    
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

std::string cleanCppType(const std::string& str)
{
    std::string s = str;

    // Trim leading/trailing spaces
    s.erase(s.begin(),
            std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }));
    s.erase(
        std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(),
        s.end());

    // Remove member name by scanning from end
    int angleLevel = 0;
    size_t typeEnd = s.size();
    for (size_t i = s.size(); i-- > 0;)
    {
        if (s[i] == '>')
            angleLevel++;
        else if (s[i] == '<')
            angleLevel--;
        else if (std::isspace(static_cast<unsigned char>(s[i])) && angleLevel == 0)
        {
            typeEnd = i;
            break;
        }
    }

    std::string typePart = s.substr(0, typeEnd);

    // Remove spaces around ::, <, >, ,
    const std::pair<std::string, std::string> patterns[] = {{" :: ", "::"},
                                                            {"< ", "<"},
                                                            {" <", "<"},
                                                            {"> ", ">"},
                                                            {" >", ">"},
                                                            {" , ", ","},
                                                            {" ,", ","}};
    for (auto& p : patterns)
    {
        size_t pos = 0;
        while ((pos = typePart.find(p.first, pos)) != std::string::npos)
            typePart.replace(pos, p.first.length(), p.second);
    }

    // Collapse multiple spaces to one (optional, mostly inside type names)
    size_t pos = 0;
    while ((pos = typePart.find("  ", pos)) != std::string::npos)
        typePart.replace(pos, 2, " ");

    return typePart;
}

// Clean up C++ type or field name strings
std::string cleanName(const std::string& str)
{
    std::string result = str;

    // Remove leading/trailing whitespace
    result.erase(result.begin(),
                 std::find_if(result.begin(),
                              result.end(),
                              [](unsigned char c) { return !std::isspace(c); }));
    result.erase(std::find_if(result.rbegin(),
                              result.rend(),
                              [](unsigned char c) { return !std::isspace(c); })
                     .base(),
                 result.end());

    // Remove extra spaces around :: or < , >
    std::string::size_type pos = 0;
    while ((pos = result.find(":: ", pos)) != std::string::npos)
    {
        result.replace(pos, 3, "::");
    }
    while ((pos = result.find(" < ", pos)) != std::string::npos)
    {
        result.replace(pos, 3, "<");
    }
    while ((pos = result.find(" > ", pos)) != std::string::npos)
    {
        result.replace(pos, 3, ">");
    }
    while ((pos = result.find(" , ", pos)) != std::string::npos)
    {
        result.replace(pos, 3, ", ");
    }

    // Optional: remove double spaces
    pos = 0;
    while ((pos = result.find("  ", pos)) != std::string::npos)
    {
        result.replace(pos, 2, " ");
    }

    return result;
}

// -------------------------
// Clean type string
// -------------------------
std::string cleanTypeStr(const std::string& str)
{
    std::string result = str;
    result.erase(std::remove(result.begin(), result.end(), '\n'), result.end());
    result.erase(std::remove(result.begin(), result.end(), '\t'), result.end());
    return result;
}

// -------------------------
// Detect STL container from source text
// -------------------------
std::string detectContainer(const std::string& typeStr)
{
    static const std::regex vectorRegex(R"(std::vector\s*<\s*(.+)\s*>)");
    static const std::regex arrayRegex(R"(std::array\s*<\s*(.+)\s*,\s*(\d+)\s*>)");
    static const std::regex mapRegex(R"(std::map\s*<\s*(.+)\s*,\s*(.+)\s*>)");
    static const std::regex setRegex(R"(std::set\s*<\s*(.+)\s*>)");

    std::smatch match;
    if (std::regex_search(typeStr, match, vectorRegex))
    {
        return "std::vector<" + match[1].str() + ">";
    }
    else if (std::regex_search(typeStr, match, arrayRegex))
    {
        return "std::array<" + match[1].str() + ", " + match[2].str() + ">";
    }
    else if (std::regex_search(typeStr, match, mapRegex))
    {
        return "std::map<" + match[1].str() + ", " + match[2].str() + ">";
    }
    else if (std::regex_search(typeStr, match, setRegex))
    {
        return "std::set<" + match[1].str() + ">";
    }
    return typeStr; // not a container
}

// -------------------------
// Get fully qualified name
// -------------------------
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

        if (!spelling.empty())
            names.push_back(spelling);

        current = clang_getCursorSemanticParent(current);
    }

    std::string fullname;
    for (auto it = names.rbegin(); it != names.rend(); ++it)
    {
        if (!fullname.empty())
            fullname += "::";
        fullname += *it;
    }

    return fullname;
}

// ==============================================================================
// DEBUGGING: First, let's see what's in your db1.h file
// ==============================================================================

// Your db1.h should look like this:
/*
namespace Database {
    class [[clang::annotate("table_name:users")]] User {
    public:
        int id;
        std::string name;
    };
}
*/

// First, add debug output to see what's happening:
std::vector<std::string> getAnnotations(CXCursor cursor)
{
    std::vector<std::string> annotations;
    
    clang_visitChildren(
        cursor,
        [](CXCursor child, CXCursor parent, CXClientData client_data) -> CXChildVisitResult
        {
            auto* annotations = static_cast<std::vector<std::string>*>(client_data);
            
            CXCursorKind childKind = clang_getCursorKind(child);

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

// -------------------------
// Dump struct/class fields (recursive)
// -------------------------
std::vector<FieldInfo> dumpStruct(CXCursor cursor, const std::string& prefix = "")
{
    struct LambdaData
    {
        std::vector<FieldInfo>* fields; // pointer to external vector
        std::string prefix;
    };

    // outside vector
    std::vector<FieldInfo> fields;

    // create LambdaData pointing to it
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

                // Get source text of the field declaration
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

                // Recursively dump nested structs/classes
                CXCursor typeDecl = clang_getTypeDeclaration(clang_getCursorType(c));
                CXCursorKind declKind = clang_getCursorKind(typeDecl);
                if ((declKind == CXCursor_StructDecl || declKind == CXCursor_ClassDecl) &&
                    !clang_Cursor_isAnonymous(typeDecl))
                {
                    auto fields = dumpStruct(typeDecl);

                }

                clang_disposeString(fieldName);
            }

            return CXChildVisit_Continue;
        },
        &data);

    return fields;
}




void walkAST(CXCursor cursor,
             const std::optional<std::string> fullClassName,
             std::vector<FieldInfo>& fields)
{
    struct LambdaData
    {
        std::optional<std::string> target;
        std::vector<FieldInfo>* fields;
        CXFile mainFile;
    };

    // Get the main file from the translation unit
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    CXString tuSpelling = clang_getTranslationUnitSpelling(tu);
    CXFile mainFile = clang_getFile(tu, clang_getCString(tuSpelling));
    clang_disposeString(tuSpelling);

    LambdaData data{fullClassName, &fields, mainFile};
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data)
        {
            auto* data = static_cast<LambdaData*>(client_data);
            CXCursorKind kind = clang_getCursorKind(c);

            if (kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl)
            {
                // CHECK if this cursor is in the main file
                CXSourceLocation cursorLoc = clang_getCursorLocation(c);
                CXFile cursorFile;
                clang_getFileLocation(cursorLoc, &cursorFile, nullptr, nullptr, nullptr);
                
                // Debug: print file info
                CXString fileName = clang_getFileName(cursorFile);
                CXString mainFileName = clang_getFileName(data->mainFile);
                clang_disposeString(fileName);
                clang_disposeString(mainFileName);
                
                // Skip if not in main file
                if (!clang_File_isEqual(cursorFile, data->mainFile))
                {
                    return CXChildVisit_Continue;
                }

                std::string qname = getQualifiedName(c);

                // Skip meta::Mapping and other meta namespace types
                if (qname.find("meta::") == 0)
                {
                    return CXChildVisit_Continue;
                }

                // fullClassName is optional
                if (!data->target || data->target.value() != qname)
                {
                    std::string shortName = (qname.find_last_of(':') != std::string::npos)
                                                ? qname.substr(qname.find_last_of(':') + 1)
                                                : qname;

                    std::string ns = qname;
                    std::size_t pos = ns.rfind("::");
                    if (pos != std::string::npos)
                    {
                        ns = ns.substr(0, pos);
                    }

                    auto fields = dumpStruct(c);

                    std::vector<FunctionInfo> functions;
                    clang_visitChildren(
                        c,
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
                                                           : "protected")
                                      << "\n";
                        }
                    }

                    auto classAnnotations = getAnnotations(c);
                    generateTuples(fields, shortName, ns, qname, getTableName(classAnnotations));
                }
            }
            
            return CXChildVisit_Recurse;
        },
        &data);
}

//
//`
// Write out c++ types to lift constexpr tuples into a c++ program
//
//


void generateTuples(std::vector<FieldInfo> fields, std::string shortName, std::string ns, std::string qname, std::string tablename)
{
    if (fields.size() == 0)
        return;

    // Create metadata in meta::ClassName namespace
    std::cout << "namespace meta\n{\n";
    std::cout << "namespace " << shortName << "\n{\n";
    
    std::cout << "inline const auto fields = std::make_tuple(\n";

    // Output each Field with typed annotations
    for (size_t i = 0; i < fields.size(); ++i)
    {
        // Skip private fields entirely (can't reflect them anyway)
        if (fields[i].access != CX_CXXPublic)
            continue;

        // Generate Field with or without annotations
        std::cout << "    field<&::" << qname << "::" << fields[i].name << ">";
        std::cout << "(\"" << fields[i].name << "\"";
        
        // Add typed annotations if present
        if (!fields[i].annotations.empty())
        {
            for (size_t j = 0; j < fields[i].annotations.size(); ++j)
            {
                std::cout << ", ";
                
                // Parse annotation: "Prefix:Value" or just "Flag"
                std::string ann = fields[i].annotations[j];
                size_t colon_pos = ann.find(':');
                
                if (colon_pos != std::string::npos)
                {
                    // Has value: "CsvColumn:FIELD6" → meta::CsvColumn{"FIELD6"}
                    std::string prefix = ann.substr(0, colon_pos);
                    std::string value = ann.substr(colon_pos + 1);
                    std::cout << "meta::" << prefix << "{\"" << value << "\"}";
                }
                else
                {
                    // Flag only: "PrimaryKey" → meta::PrimaryKey{}
                    std::cout << "meta::" << ann << "{}";
                }
            }
        }
        
        std::cout << ")";
        
        if (i + 1 < fields.size())
            std::cout << ",";
        std::cout << "\n";
    }

    std::cout << ");\n\n";
    
    // Table name
    std::string tableNameValue = tablename.empty() ? shortName : tablename;
    std::cout << "inline constexpr auto tableName = \"" << tableNameValue << "\";\n";
    
    // Generate query
    std::cout << "inline constexpr auto query = \"SELECT ";
    for (size_t i = 0; i < fields.size(); ++i)
    {
        if (fields[i].access != CX_CXXPublic)
            continue;
        std::cout << fields[i].name;
        if (i + 1 < fields.size())
            std::cout << ", ";
    }
    std::cout << " FROM " << tableNameValue << "\";\n";
    
    // Close meta::ClassName namespace
    std::cout << "} // namespace " << shortName << "\n"
              << "} // namespace meta\n\n";
      
    // MetaTuple specialization
    std::cout << "namespace meta\n{\n"
              << "template <> struct MetaTuple<::" << qname << ">\n{\n"
              << "    static inline const auto& fields = meta::" << shortName << "::fields;\n"
              << "    static constexpr auto tableName = meta::" << shortName << "::tableName;\n"
              << "    static constexpr auto query = meta::" << shortName << "::query;\n"
              << "};\n"
              << "} // namespace meta\n";
}


std::string getTableName(const std::vector<std::string>& annotations)
{
    for (const auto& annotation : annotations) {
      std::cerr << annotation << "\n";
        if (annotation.find("table_name:") == 0) {
            return annotation.substr(11);
        }
    }
    return "";
}

