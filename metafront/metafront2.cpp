/*
================================================================================
cpp-reflect-ast - C++ Reflection Metadata Generator using Clang AST
================================================================================

Author: John Grillo
Date: 2024-2025
Version: 2.0 (Final)

Description:
------------
A production-ready tool that generates type-safe reflection metadata for C++ 
structs and classes using libclang's AST parser.

Features:
---------
✓ Full C++ parsing (handles any valid C++ code)
✓ Nested anonymous struct support with decltype() references
✓ Generates clean, namespaced metadata
✓ CLI11-based command-line interface
✓ Multiple include directory support (-I, --isystem)
✓ Output to file or stdout (-o)
✓ Quiet mode for build scripts (-q)
✓ Verbose mode for debugging (-v)
✓ Automatic help text (--help)

Usage:
------
  ./cpp-reflect-ast myfile.h -o myfile.meta.h
  ./cpp-reflect-ast myfile.h -I../include -o output.h -q
  ./cpp-reflect-ast myfile.h -v  # verbose debug output
  cat myfile.h | ./cpp-reflect-ast -o output.h  # from stdin
  ./cpp-reflect-ast - < myfile.h  # explicit stdin
  preprocessor myfile.h | ./cpp-reflect-ast  # pipe from preprocessor

Generated Output:
-----------------
  namespace meta::MyStruct {
    inline const auto fields = std::make_tuple(
      meta::field<&::MyStruct::field1>("field1"),
      meta::field<&::MyStruct::field2>("field2")
    );
  }
  
  template <> struct MetaTuple<::MyStruct> {
    static inline const auto& fields = meta::MyStruct::fields;
    static constexpr auto tableName = "MyStruct";
    static constexpr auto query = "select field1, field2 from MyStruct";
  };

Dependencies:
-------------
- libclang (system library)
- CLI11.hpp (single-header, included)
- meta/meta_field.h (your reflection framework)

License:
--------
Apache License 2.0

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
#include <fstream>
#include <cstdlib>
#include <unistd.h>

#include "CLI11.hpp"
#include "meta/meta_field.h"

// Global verbose flag
bool g_verbose = false;

#define DEBUG(...) do { if (g_verbose) { std::cerr << __VA_ARGS__; } } while(0)

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
    CLI::App app{"cpp-reflect-ast - C++ Reflection Metadata Generator using Clang AST"};
    
    std::string inputFile;
    std::string outputFile;
    std::vector<std::string> includeDirs;
    std::vector<std::string> systemIncludeDirs;
    std::optional<std::string> targetClass;
    bool quiet = false;
    bool verbose = false;
    
    // Positional arguments
    app.add_option("input", inputFile, "Input C++ header file (use '-' or omit for stdin)")
        ->check(CLI::ExistingFile | CLI::IsMember({"-"}));
    
    app.add_option("class", targetClass, "Optional: Fully qualified class name to process");
    
    // Options
    app.add_option("-o,--output", outputFile, "Output file (default: stdout)");
    
    app.add_option("-I,--include", includeDirs, "Add include directory")
        ->allow_extra_args(false);
    
    app.add_option("--isystem", systemIncludeDirs, "Add system include directory")
        ->allow_extra_args(false);
    
    app.add_flag("-q,--quiet", quiet, "Suppress all diagnostic and debug output");
    
    app.add_flag("-v,--verbose", verbose, "Show debug output (AST walking details)");
    
    CLI11_PARSE(app, argc, argv);
    
    // Set global verbose flag for use in other functions
    extern bool g_verbose;
    g_verbose = verbose && !quiet;
    
    // Redirect output if specified
    std::ofstream outFile;
    std::streambuf* coutBuf = std::cout.rdbuf();
    if (!outputFile.empty())
    {
        outFile.open(outputFile);
        if (!outFile)
        {
            std::cerr << "Error: Cannot open output file: " << outputFile << "\n";
            return 1;
        }
        std::cout.rdbuf(outFile.rdbuf());
    }
    
    // Redirect stderr if quiet mode
    std::ofstream nullStream;
    std::streambuf* cerrBuf = std::cerr.rdbuf();
    if (quiet)
    {
        nullStream.open("/dev/null");
        std::cerr.rdbuf(nullStream.rdbuf());
    }
    
    const std::string headerFile = inputFile;
    const std::optional<std::string> fullClassName = targetClass;

    // Handle stdin input
    std::string tempFileName;
    bool usedStdin = false;
    
    if (inputFile.empty() || inputFile == "-")
    {
        // Read from stdin
        usedStdin = true;
        tempFileName = "/tmp/cpp-reflect-ast-stdin-XXXXXX";
        
        // Create unique temp file
        char tempTemplate[] = "/tmp/cpp-reflect-ast-stdin-XXXXXX";
        int fd = mkstemp(tempTemplate);
        if (fd == -1)
        {
            std::cerr << "Error: Cannot create temporary file\n";
            return 1;
        }
        tempFileName = tempTemplate;
        
        // Read stdin and write to temp file
        std::string line;
        std::ofstream tempFile(tempFileName);
        while (std::getline(std::cin, line))
        {
            tempFile << line << "\n";
        }
        tempFile.close();
        close(fd);
        
        if (!quiet && g_verbose)
        {
            std::cerr << "// Reading from stdin, using temp file: " << tempFileName << "\n";
        }
    }
    
    const std::string actualFile = usedStdin ? tempFileName : headerFile;

    CXIndex index = clang_createIndex(0, 0);

    // Build compiler arguments
    std::vector<std::string> argsStorage;
    std::vector<const char*> args = {
      "-std=c++20",
      "-x", "c++",
    };
    
    // Add user include directories first (higher priority)
    for (const auto& dir : includeDirs)
    {
        args.push_back("-I");
        argsStorage.push_back(dir);
        args.push_back(argsStorage.back().c_str());
    }
    
    // Add user system includes
    for (const auto& dir : systemIncludeDirs)
    {
        args.push_back("-isystem");
        argsStorage.push_back(dir);
        args.push_back(argsStorage.back().c_str());
    }
    
    // Add default system includes
    args.insert(args.end(), {
      "-isystem", "/usr/lib/gcc/x86_64-linux-gnu/13/include",
      "-isystem", "/usr/include/c++/13",
      "-isystem", "/usr/include/x86_64-linux-gnu/c++/13",
      "-isystem", "/usr/include/c++/13/backward",
      "-isystem", "/usr/local/include",
      "-isystem", "/usr/include/x86_64-linux-gnu",
      "-isystem", "/usr/include"
    });
    
    CXTranslationUnit tu =
        clang_parseTranslationUnit(index,
                                   actualFile.c_str(),
                                   args.data(),
                                   args.size(),
                                   nullptr,
                                   0,
                                   CXTranslationUnit_DetailedPreprocessingRecord);

    if (!tu)
    {
        std::cerr << "Error: Failed to parse translation unit.\n";
        return 1;
    }

    // Always show diagnostics (parse errors/warnings) unless -q
    unsigned numDiags = clang_getNumDiagnostics(tu);
    if (numDiags > 0)
    {
        std::cerr << "// " << numDiags << " diagnostic" << (numDiags > 1 ? "s" : "") << ":\n";
        for (unsigned i = 0; i < numDiags; ++i)
        {
            CXDiagnostic diag = clang_getDiagnostic(tu, i);
            CXString diagStr = clang_formatDiagnostic(diag, clang_defaultDiagnosticDisplayOptions());
            std::cerr << "// " << clang_getCString(diagStr) << "\n";
            clang_disposeString(diagStr);
            clang_disposeDiagnostic(diag);
        }
    }

    // Show basic info unless -q
    if (g_verbose)
    {
        std::cerr << "// Parsing file: " << (usedStdin ? "stdin" : actualFile) << "\n";
        std::cerr << "// Target class: " << (fullClassName ? fullClassName.value() : "ALL") << "\n";
    }

    CXCursor cursor = clang_getTranslationUnitCursor(tu);
    std::vector<FieldInfo> fields;
    walkAST(cursor, fullClassName, fields);

    clang_disposeTranslationUnit(tu);
    clang_disposeIndex(index);
    
    // Clean up temp file if we used stdin
    if (usedStdin)
    {
        unlink(tempFileName.c_str());
    }
    
    // Restore streams
    std::cout.rdbuf(coutBuf);
    std::cerr.rdbuf(cerrBuf);
    if (outFile.is_open())
        outFile.close();
    if (nullStream.is_open())
        nullStream.close();
    
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

// Extract annotations from [[clang::annotate("...")]] attributes
std::vector<std::string> getClangAnnotations(CXCursor cursor)
{
    std::vector<std::string> annotations;
    
    // Check if cursor has attributes
    if (!clang_Cursor_hasAttrs(cursor))
    {
        DEBUG("//     No attributes on cursor\n");
        return annotations;
    }
    
    DEBUG("//     Cursor has attributes, visiting...\n");
    
    // Visit attributes to find annotate attrs
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) -> CXChildVisitResult
        {
            auto* annotations = static_cast<std::vector<std::string>*>(client_data);
            
            CXCursorKind kind = clang_getCursorKind(c);
            DEBUG("//       Attr kind: " << kind << " (AnnotateAttr=" << CXCursor_AnnotateAttr << ")\n");
            
            // Look for AnnotateAttr nodes
            if (kind == CXCursor_AnnotateAttr)
            {
                CXString spelling = clang_getCursorSpelling(c);
                const char* str = clang_getCString(spelling);
                if (str && str[0] != '\0')
                {
                    DEBUG("//       Found annotation: " << str << "\n");
                    annotations->push_back(str);
                }
                clang_disposeString(spelling);
            }
            
            return CXChildVisit_Continue;
        },
        &annotations);
    
    DEBUG("//     Total annotations found: " << annotations.size() << "\n");
    return annotations;
}

std::vector<std::string> getAnnotations(CXCursor cursor)
{
    std::vector<std::string> annotations;
    
    DEBUG("//     Getting annotations for cursor\n");
    
    // First, try clang::annotate attributes
    annotations = getClangAnnotations(cursor);
    
    // If no clang attributes, try comment-based annotations
    if (annotations.empty())
    {
        DEBUG("//     No clang attributes, trying comments\n");
        CXString rawComment = clang_Cursor_getRawCommentText(cursor);
        const char* commentStr = clang_getCString(rawComment);
        if (commentStr)
        {
            annotations = parseAnnotations(commentStr);
            DEBUG("//     Found " << annotations.size() << " comment annotations\n");
        }
        clang_disposeString(rawComment);
    }
    else
    {
        DEBUG("//     Using " << annotations.size() << " clang attributes\n");
    }
    
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
    
    DEBUG("// dumpStruct called\n");
    
    clang_visitChildren(
        structCursor,
        [](CXCursor c, CXCursor parent, CXClientData client_data) -> CXChildVisitResult
        {
            auto* fields = static_cast<std::vector<FieldInfo>*>(client_data);
            
            CXCursorKind childKind = clang_getCursorKind(c);
            CXString kindStr = clang_getCursorKindSpelling(childKind);
            DEBUG("//   Child: " << clang_getCString(kindStr) << "\n");
            clang_disposeString(kindStr);
            
            if (clang_getCursorKind(c) == CXCursor_FieldDecl)
            {
                DEBUG("//   Found FieldDecl\n");
                
                FieldInfo field;
                
                CXString fieldName = clang_getCursorSpelling(c);
                field.name = clang_getCString(fieldName);
                DEBUG("//     name: " << field.name << "\n");
                clang_disposeString(fieldName);
                
                CXType fieldType = clang_getCursorType(c);
                CXString typeSpelling = clang_getTypeSpelling(fieldType);
                field.type = cleanCppType(clang_getCString(typeSpelling));
                std::string typeStr = clang_getCString(typeSpelling);
                DEBUG("//     type: " << typeStr << "\n");
                clang_disposeString(typeSpelling);
                
                field.access = clang_getCXXAccessSpecifier(c);
                DEBUG("//     access: " << field.access << " (0=private, 1=protected, 2=public)");
                if (field.access != CX_CXXPublic)
                    DEBUG(" - WILL BE SKIPPED");
                DEBUG("\n");
                field.annotations = getAnnotations(c);
                
                // Check if this field is an anonymous struct
                if (isAnonymousStruct(typeStr))
                {
                    DEBUG("//     Anonymous struct detected\n");
                    field.isAnonymousStruct = true;
                    
                    // Get the type declaration cursor
                    CXCursor typeDecl = clang_getTypeDeclaration(fieldType);
                    if (clang_getCursorKind(typeDecl) == CXCursor_StructDecl)
                    {
                        DEBUG("//     Recursing into nested struct...\n");
                        // Recursively dump the nested struct fields
                        field.nestedFields = dumpStruct(typeDecl);
                        DEBUG("//     Found " << field.nestedFields.size() << " nested fields\n");
                    }
                }
                
                fields->push_back(field);
            }
            
            return CXChildVisit_Continue;
        },
        &fields);
    
    DEBUG("// dumpStruct returning " << fields.size() << " fields\n");
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
    DEBUG("// Main file: " << clang_getCString(mainFileName) << "\n");
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
                DEBUG("// Found struct/class cursor\n");
                
                CXSourceLocation loc = clang_getCursorLocation(c);
                CXFile cursorFile;
                clang_getFileLocation(loc, &cursorFile, nullptr, nullptr, nullptr);
                
                CXString fileName = clang_getFileName(cursorFile);
                DEBUG("//   In file: " << clang_getCString(fileName) << "\n");
                clang_disposeString(fileName);
                
                CXString mainFileName = clang_getFileName(data->mainFile);
                DEBUG("//   Main file: " << clang_getCString(mainFileName) << "\n");
                clang_disposeString(mainFileName);
                
                // Skip if not in main file
                if (!clang_File_isEqual(cursorFile, data->mainFile))
                {
                    DEBUG("//   SKIPPING: Not in main file\n");
                    return CXChildVisit_Continue;
                }

                std::string qname = getQualifiedName(c);
                DEBUG("//   Qualified name: " << qname << "\n");

                // Skip anonymous structs - they'll be processed as nested fields
                if (qname.find("(unnamed struct at") != std::string::npos ||
                    qname.find("(anonymous struct") != std::string::npos)
                {
                    DEBUG("//   SKIPPING: Anonymous struct\n");
                    return CXChildVisit_Continue;
                }

                // Skip meta::Mapping and other meta namespace types
                if (qname.find("meta::") == 0)
                {
                    DEBUG("//   SKIPPING: In meta namespace\n");
                    return CXChildVisit_Continue;
                }

                // fullClassName is optional - process if no target OR if target matches
                DEBUG("//   Target: " << (data->target ? data->target.value() : "ALL") 
                         << ", processing: " << qname << "\n");
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
                    DEBUG("//   PROCESSING: " << qname << " (" << fields.size() << " fields)\n");
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
    DEBUG("// generateTuples: " << shortName << " (" << fields.size() << " fields)\n");
    
    if (fields.size() == 0)
    {
        DEBUG("//   No fields\n");
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
        std::cerr << "// WARNING: No public fields in " << shortName << "!\n";
        std::cerr << "// HINT: Use 'struct' instead of 'class', or mark fields as 'public:'\n";
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
        
        DEBUG("//   Outputting field: " << fields[i].name 
              << " with " << fields[i].annotations.size() << " annotations\n");
        for (const auto& ann : fields[i].annotations)
        {
            DEBUG("//     Annotation: " << ann << "\n");
        }
        
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
    DEBUG("// Generating nested MetaTuples for " << fields.size() << " fields\n");
    generateNestedMetaTuples(fields, shortName, parentDecltype.empty() ? "::" + qname : parentDecltype);
    DEBUG("// Finished nested MetaTuples\n");
    
    std::cout << "} // namespace meta\n";
}

// Helper to recursively generate MetaTuple specializations for all nested anonymous structs
void generateNestedMetaTuples(const std::vector<FieldInfo>& fields, 
                              const std::string& parentShortName,
                              const std::string& parentTypeRef)
{
    DEBUG("// generateNestedMetaTuples: " << parentShortName << "\n");
    
    for (const auto& field : fields)
    {
        DEBUG("//   Checking: " << field.name << " (anonymous=" << field.isAnonymousStruct << ")\n");
        
        // If this field is an anonymous struct, generate its MetaTuple
        // Don't check access here - if we're processing the parent, process all nested anonymous structs
        if (field.isAnonymousStruct)
        {
            std::string fieldDecltype = "decltype(" + parentTypeRef + "::" + field.name + ")";
            
            DEBUG("//   Generating MetaTuple for: " << fieldDecltype << "\n");
            
            std::cout << "template <> struct MetaTuple<" << fieldDecltype << ">\n{\n"
                      << "  static inline const auto& fields = meta::" << parentShortName 
                      << "::" << field.name << "::fields;\n"
                      << "};\n";
            
            // Recursively process nested anonymous structs
            if (!field.nestedFields.empty())
            {
                DEBUG("//   Recursing into " << field.nestedFields.size() << " nested fields\n");
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
        DEBUG("// Class annotation: " << annotation << "\n");
        
        // Support both "table_name:Value" and "TableName:Value"
        if (annotation.find("table_name:") == 0) {
            return annotation.substr(11);
        }
        if (annotation.find("TableName:") == 0) {
            return annotation.substr(10);
        }
    }
    return "";
}

