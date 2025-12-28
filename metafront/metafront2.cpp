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


Why It's Unique:
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
    bool isAnonymousStruct = false;
    std::vector<FieldInfo> nestedFields;  // For anonymous structs
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


void generateTuples(std::vector<FieldInfo> fields, std::string shortName, std::string ns, std::string qname, std::string tablename, const std::string& parentDecltype = "");


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
      "-isystem", "/usr/lib/gcc/x86_64-linux-gnu/13/include",  // GCC builtin headers (stddef.h, etc)
      "-isystem", "/usr/include/c++/13",
      "-isystem", "/usr/include/x86_64-linux-gnu/c++/13",
      "-isystem", "/usr/include/c++/13/backward",
      "-isystem", "/usr/local/include",
      "-isystem", "/usr/include/x86_64-linux-gnu",
      "-isystem", "/usr/include"
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

    // Debug: Check for diagnostics
    unsigned numDiags = clang_getNumDiagnostics(tu);
    std::cerr << "// Number of diagnostics: " << numDiags << "\n";
    for (unsigned i = 0; i < numDiags; ++i)
    {
        CXDiagnostic diag = clang_getDiagnostic(tu, i);
        CXString diagStr = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
        std::cerr << "// Diagnostic: " << clang_getCString(diagStr) << "\n";
        clang_disposeString(diagStr);
        clang_disposeDiagnostic(diag);
    }

    std::cerr << "//Parsing file: " << headerFile << "\n";
    std::cerr << "//Target class: " << (fullClassName ? fullClassName.value() : "ALL") << "\n";

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

    // Return cleaned
    return result;
}

std::vector<std::string> parseAnnotations(const std::string& rawComment)
{
    std::vector<std::string> annotations;
    
    // Example: /** @MyAnnotation @AnotherOne */
    std::regex atPattern(R"(@(\w+(?::\w+)?))");
    auto begin = std::sregex_iterator(rawComment.begin(), rawComment.end(), atPattern);
    auto end = std::sregex_iterator();
    
    for (auto it = begin; it != end; ++it)
    {
        annotations.push_back((*it)[1].str());
    }
    
    return annotations;
}

std::vector<std::string> getAnnotations(CXCursor cursor)
{
    std::vector<std::string> annotations;
    
    CXString rawComment = clang_Cursor_getRawCommentText(cursor);
    const char* commentStr = clang_getCString(rawComment);
    if (commentStr)
    {
        annotations = parseAnnotations(commentStr);
    }
    clang_disposeString(rawComment);
    
    return annotations;
}

std::string getQualifiedName(CXCursor cursor)
{
    std::vector<std::string> parts;
    CXCursor current = cursor;
    
    while (true)
    {
        CXCursorKind kind = clang_getCursorKind(current);
        if (kind == CXCursor_TranslationUnit)
            break;
        
        CXString spelling = clang_getCursorSpelling(current);
        const char* str = clang_getCString(spelling);
        if (str && str[0] != '\0')
        {
            parts.push_back(str);
        }
        clang_disposeString(spelling);
        
        current = clang_getCursorSemanticParent(current);
    }
    
    std::reverse(parts.begin(), parts.end());
    
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        result += parts[i];
        if (i + 1 < parts.size())
            result += "::";
    }
    return result;
}

// Check if a type is an anonymous struct
bool isAnonymousStruct(const std::string& typeSpelling)
{
    return typeSpelling.find("(unnamed struct at") != std::string::npos ||
           typeSpelling.find("(anonymous struct") != std::string::npos;
}

// Recursively dump struct fields, including nested anonymous structs
std::vector<FieldInfo> dumpStruct(CXCursor structCursor)
{
    std::vector<FieldInfo> fields;
    
    //std::cerr << "dumpStruct called on cursor\n";
    
    clang_visitChildren(
        structCursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) -> CXChildVisitResult
        {
            auto* fields = static_cast<std::vector<FieldInfo>*>(client_data);
            
            CXCursorKind childKind = clang_getCursorKind(c);
            CXString kindStr = clang_getCursorKindSpelling(childKind);
            //std::cerr << "  Child kind: " << clang_getCString(kindStr) << "\n";
            clang_disposeString(kindStr);
            
            if (clang_getCursorKind(c) == CXCursor_FieldDecl)
            {
  	      //std::cerr << "  Found FieldDecl!\n";
                
                FieldInfo field;
                
                CXString fieldName = clang_getCursorSpelling(c);
                field.name = clang_getCString(fieldName);
                //std::cerr << "    Field name: " << field.name << "\n";
                clang_disposeString(fieldName);
                
                CXType fieldType = clang_getCursorType(c);
                CXString typeSpelling = clang_getTypeSpelling(fieldType);
                field.type = cleanCppType(clang_getCString(typeSpelling));
                std::string typeStr = clang_getCString(typeSpelling);
                //std::cerr << "    Field type: " << typeStr << "\n";
                clang_disposeString(typeSpelling);
                
                field.access = clang_getCXXAccessSpecifier(c);
                //std::cerr << "    Access: " << field.access << " (0=private, 1=protected, 2=public)";
                //if (field.access != CX_CXXPublic)
		//  std::cerr << " - WILL BE SKIPPED";
                //std::cerr << "\n";
                field.annotations = getAnnotations(c);
                
                // Check if this field is an anonymous struct
                if (isAnonymousStruct(typeStr))
                {
		    //std::cerr << "    This is an anonymous struct!\n";
                    field.isAnonymousStruct = true;
                    
                    // Get the type declaration cursor
                    CXCursor typeDecl = clang_getTypeDeclaration(fieldType);
                    if (clang_getCursorKind(typeDecl) == CXCursor_StructDecl)
                    {
		      //std::cerr << "    Recursing into nested struct...\n";
                        // Recursively dump the nested struct fields
                        field.nestedFields = dumpStruct(typeDecl);
                        //std::cerr << "    Found " << field.nestedFields.size() << " nested fields\n";
                    }
                }
                
                fields->push_back(field);
            }
            
            return CXChildVisit_Continue;
        },
        &fields);
    
    //std::cerr << "dumpStruct returning " << fields.size() << " fields\n";
    return fields;
}

struct WalkData
{
    std::optional<std::string> target;
    CXFile mainFile;
};

void walkAST(CXCursor cursor,
             const std::optional<std::string> fullClassName,
             std::vector<FieldInfo>& outFields)
{
    // Get the main file from the translation unit
    CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
    CXFile mainFile = clang_getFile(tu, clang_getCString(clang_getTranslationUnitSpelling(tu)));

    WalkData data;
    data.target = fullClassName;
    data.mainFile = mainFile;
    
    CXString mainFileName = clang_getFileName(mainFile);
   std::cerr << "//Main file set to: " << clang_getCString(mainFileName) << "\n";
    clang_disposeString(mainFileName);

    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) -> CXChildVisitResult
        {
            auto* data = static_cast<WalkData*>(client_data);
            CXCursorKind kind = clang_getCursorKind(c);

            // Process struct/class declarations
            if (kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl)
            {
	      //std::cerr << "Found struct/class cursor\n";
                
                CXSourceLocation loc = clang_getCursorLocation(c);
                CXFile cursorFile;
                clang_getFileLocation(loc, &cursorFile, nullptr, nullptr, nullptr);
                
                CXString fileName = clang_getFileName(cursorFile);
                //std::cerr << "  In file: " << clang_getCString(fileName) << "\n";
                clang_disposeString(fileName);
                
                CXString mainFileName = clang_getFileName(data->mainFile);
                //std::cerr << "  Main file: " << clang_getCString(mainFileName) << "\n";
                clang_disposeString(mainFileName);
                
                // Skip if not in main file
                if (!clang_File_isEqual(cursorFile, data->mainFile))
                {
		  //std::cerr << "  SKIPPING: Not in main file\n";
                    return CXChildVisit_Continue;
                }

                std::string qname = getQualifiedName(c);
                //std::cerr << "  Qualified name: " << qname << "\n";

                // Skip anonymous structs - they'll be processed as nested fields
                if (qname.find("(unnamed struct at") != std::string::npos ||
                    qname.find("(anonymous struct") != std::string::npos)
                {
		  //std::cerr << "  SKIPPING: Anonymous struct (will be processed as nested field)\n";
                    return CXChildVisit_Continue;
                }

                // Skip meta::Mapping and other meta namespace types
                if (qname.find("meta::") == 0)
                {
		  //std::cerr << "  SKIPPING: In meta namespace\n";
                    return CXChildVisit_Continue;
                }

                // fullClassName is optional - process if no target OR if target matches
                //std::cerr << "  Target check: target=" << (data->target ? data->target.value() : "NONE") 
		//       << " qname=" << qname << "\n";
                if (!data->target || data->target.value() == qname)
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
                    //std::cerr << "  PROCESSING struct " << qname << " with " << fields.size() << " fields\n";
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

// Forward declaration for recursion
void generateNestedTuples(const std::vector<FieldInfo>& fields, 
                         const std::string& parentQname,
                         const std::string& parentDecltype,
                         int indent = 0);

void generateNestedMetaTuples(const std::vector<FieldInfo>& fields, 
                              const std::string& parentShortName,
                              const std::string& parentTypeRef);

void generateTuples(std::vector<FieldInfo> fields, std::string shortName, std::string ns, std::string qname, std::string tablename, const std::string& parentDecltype)
{
  //std::cerr << "generateTuples called: shortName=" << shortName << " qname=" << qname 
  //          << " fields=" << fields.size() << "\n";
    
    if (fields.size() == 0)
    {
        std::cerr << "  No fields, returning\n";
        return;
    }
    
    // Count public fields
    size_t publicFieldCount = 0;
    for (const auto& field : fields)
    {
        if (field.access == CX_CXXPublic)
            publicFieldCount++;
    }
    
    if (publicFieldCount == 0)
    {
        std::cerr << "  WARNING: No public fields found! All fields are private/protected.\n";
        std::cerr << "  HINT: Use 'struct' instead of 'class', or mark fields as 'public:'\n";
        return;
    }

    std::string indentStr = "";
    
    // Create metadata in meta::ClassName namespace
    std::cout << "namespace meta\n{\n";
    std::cout << "namespace " << shortName << "\n{\n";
    
    std::cout << "inline const auto fields = std::make_tuple(\n";

    // Output each Field with typed annotations
    bool firstField = true;
    for (size_t i = 0; i < fields.size(); ++i)
    {
        // Skip private fields entirely (can't reflect them anyway)
        if (fields[i].access != CX_CXXPublic)
            continue;

        if (!firstField)
            std::cout << ",\n";
        firstField = false;

        // Generate Field with or without annotations
        std::string typePrefix = parentDecltype.empty() ? "::" + qname : parentDecltype;
        std::cout << "    meta::field<&" << typePrefix << "::" << fields[i].name << ">";
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
    }
    std::cout << ");\n";

    // Generate nested namespaces for anonymous struct fields
    generateNestedTuples(fields, qname, parentDecltype, 0);
    
    // Close meta::ClassName namespace
    std::cout << "} // namespace " << shortName << "\n"
              << "} // namespace meta\n\n";
      
    // MetaTuple specialization for main struct
    std::string typeSpec = parentDecltype.empty() ? "::" + qname : parentDecltype;
    std::cout << "namespace meta\n{\n"
              << "template <> struct MetaTuple<" << typeSpec << ">\n{\n"
              << "  static inline const auto& fields = meta::" << shortName << "::fields;\n";
    
    // Only add tableName and query for top-level structs (not nested anonymous ones)
    if (parentDecltype.empty())
    {
        std::string tableNameValue = tablename.empty() ? shortName : tablename;
        std::cout << "  static constexpr auto tableName = \"" << tableNameValue << "\";\n"
                  << "  static constexpr auto query = \"select ";
        
        bool firstQueryField = true;
        for (size_t i = 0; i < fields.size(); ++i)
        {
            if (fields[i].access != CX_CXXPublic)
                continue;
            if (!firstQueryField)
                std::cout << ", ";
            firstQueryField = false;
            std::cout << fields[i].name;
        }
        std::cout << " from " << tableNameValue << "\";\n";
    }
    
    std::cout << "};\n";
    
    // Generate MetaTuple specializations for nested anonymous structs (recursively)
    //std::cerr << "About to call generateNestedMetaTuples with " << fields.size() << " fields\n";
    generateNestedMetaTuples(fields, shortName, parentDecltype.empty() ? "::" + qname : parentDecltype);
    //std::cerr << "Finished generateNestedMetaTuples\n";
    
    std::cout << "} // namespace meta\n";
}

// Helper to recursively generate MetaTuple specializations for all nested anonymous structs
void generateNestedMetaTuples(const std::vector<FieldInfo>& fields, 
                              const std::string& parentShortName,
                              const std::string& parentTypeRef)
{
  //std::cerr << "generateNestedMetaTuples called: parentShortName=" << parentShortName 
  //          << " parentTypeRef=" << parentTypeRef << " fields=" << fields.size() << "\n";
    
    for (const auto& field : fields)
    {
      //std::cerr << "  Checking field: " << field.name << " isAnonymous=" << field.isAnonymousStruct 
  //          << " access=" << field.access << "\n";
        
        // If this field is an anonymous struct, generate its MetaTuple
        // Don't check access here - if we're processing the parent, process all nested anonymous structs
        if (field.isAnonymousStruct)
        {
            std::string fieldDecltype = "decltype(" + parentTypeRef + "::" + field.name + ")";
            
            //std::cerr << "  Generating MetaTuple for: " << fieldDecltype << "\n";
            
            std::cout << "template <> struct MetaTuple<" << fieldDecltype << ">\n{\n"
                      << "  static inline const auto& fields = meta::" << parentShortName 
                      << "::" << field.name << "::fields;\n"
                      << "};\n";
            
            // Recursively process nested anonymous structs
            if (!field.nestedFields.empty())
            {
	      //std::cerr << "  Recursing into " << field.nestedFields.size() << " nested fields\n";
                generateNestedMetaTuples(field.nestedFields, 
                                        parentShortName + "::" + field.name,
                                        fieldDecltype);
            }
        }
    }
}

void generateNestedTuples(const std::vector<FieldInfo>& fields, 
                         const std::string& parentQname,
                         const std::string& parentDecltype,
                         int indent)
{
    for (const auto& field : fields)
    {
        if (field.isAnonymousStruct && field.access == CX_CXXPublic)
        {
            // Generate namespace for this anonymous struct field
            std::cout << "namespace " << field.name << "\n{\n";
            
            std::cout << "inline const auto fields = std::make_tuple(\n";
            
            // Generate fields for the nested struct
            bool firstNestedField = true;
            for (const auto& nestedField : field.nestedFields)
            {
                if (nestedField.access != CX_CXXPublic)
                    continue;
                
                if (!firstNestedField)
                    std::cout << ",\n";
                firstNestedField = false;
                
                // Build the proper decltype chain
                std::string currentDecltype;
                if (parentDecltype.empty())
                {
                    currentDecltype = "decltype(::" + parentQname + "::" + field.name + ")";
                }
                else
                {
                    currentDecltype = "decltype(" + parentDecltype + "::" + field.name + ")";
                }
                
                std::cout << "    meta::field<&" << currentDecltype << "::" << nestedField.name << ">"
                         << "(\"" << nestedField.name << "\")";
            }
            std::cout << ");\n";
            
            // Recursively process nested anonymous structs
            std::string currentDecltype;
            if (parentDecltype.empty())
            {
                currentDecltype = "decltype(::" + parentQname + "::" + field.name + ")";
            }
            else
            {
                currentDecltype = "decltype(" + parentDecltype + "::" + field.name + ")";
            }
            generateNestedTuples(field.nestedFields, parentQname, currentDecltype, indent + 1);
            
            std::cout << "} // namespace " << field.name << "\n";
        }
    }
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
