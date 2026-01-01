#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <variant>
#include <vector>

#include "ast.h"
#include "cpp_parser.h"
#include "cpp_walker.h"
#include "registry_ast_walker.h"

namespace
{

auto joinRanges(const std::vector<std::string>& vec, const std::string& sep) -> std::string;

// Helper to build nested decltype expressions
// buildNestedDecltype("::A", ["b", "c"]) -> "decltype(decltype(::A::b)::c)"
std::string buildNestedDecltype(const std::string& rootType, const std::vector<std::string>& path)
{
    if (path.empty())
        return rootType;
    
    std::string result;
    
    // Open N decltype( where N = path.size()
    for (size_t i = 0; i < path.size(); ++i)
        result += "decltype(";
    
    // Add root and first boundary
    result += rootType + "::" + path[0];
    
    // Close one paren and add each subsequent boundary
    for (size_t i = 1; i < path.size(); ++i)
        result += ")::" + path[i];
    
    // Close the final paren
    result += ")";
    
    return result;
}

class CppReflectWalker : public bhw::CppWalker
{
  public:
    auto walkNamespace(const bhw::Namespace& ns, const bhw::WalkContext& ctx) -> std::string override;
    auto walkStruct(const bhw::Struct& s, const bhw::WalkContext& ctx) -> std::string override;
    
    std::string generateOneof(const bhw::Oneof& oneof, const bhw::WalkContext& ctx) override
    {
        return "";
    }
    std::string generateHeader(const bhw::Ast& ast) override
    {
        return "";
    }

  protected:
    std::string generateGenericType(const bhw::GenericType& type, const bhw::WalkContext& ctx) override
    {
        std::ostringstream out;

        std::string baseType;
        switch (type.reifiedType)
        {
        case bhw::ReifiedTypeId::List:
            baseType = "std::vector";
            break;
        case bhw::ReifiedTypeId::Map:
            baseType = "std::map";
            break;
        case bhw::ReifiedTypeId::Set:
            baseType = "std::set";
            break;
        case bhw::ReifiedTypeId::Optional:
            baseType = "std::optional";
            break;
        case bhw::ReifiedTypeId::Variant:
            baseType = "std::variant";
            break;
        case bhw::ReifiedTypeId::Pair:
            baseType = "std::pair";
            break;
        case bhw::ReifiedTypeId::Tuple:
            baseType = "std::tuple";
            break;
        case bhw::ReifiedTypeId::Array:
            baseType = "std::array";
            break;
        case bhw::ReifiedTypeId::UniquePtr:
            baseType = "std::unique_ptr";
            break;
        case bhw::ReifiedTypeId::SharedPtr:
            baseType = "std::shared_ptr";
            break;
        default:
            return bhw::CppWalker::generateGenericType(type, ctx);
        }

        out << baseType;

        if (!type.args.empty())
        {
            out << "<";
            for (size_t i = 0; i < type.args.size(); i++)
            {
                if (i > 0)
                    out << ", ";

                if (auto* structRef = std::get_if<bhw::StructRefType>(&type.args[i]->value))
                {
                    std::string typeName = structRef->srcTypeString;

                    if (typeName.find("::") == 0)
                    {
                        out << typeName;
                    }
                    else if (currentParentStruct_.empty())
                    {
                        out << "::" << typeName;
                    }
                    else
                    {
                        out << currentParentStruct_ << "::" << typeName;
                    }
                }
                else
                {
                    out << walkType(*type.args[i], ctx);
                }
            }
            out << ">";
        }

        return out.str();
    }

  private:
    std::string walkNestedStruct(const bhw::Struct& s,
                                 const std::string& rootTypeName,
                                 const std::vector<std::string>& path,
                                 const std::string& namespaceName,
                                 const std::string& parentMetaPath);
    std::string walkNestedTypeDefinitions(const bhw::Struct& s,
                                          const std::string& parentFullName,
                                          const bhw::WalkContext& ctx);

    std::string currentParentStruct_;
};

std::string CppReflectWalker::walkNamespace(const bhw::Namespace& ns, const bhw::WalkContext& ctx)
{
    std::stringstream str;
    
    for (const auto& node : ns.nodes)
    {
        str << std::visit(
            [&](const auto& m) -> std::string
            {
                using M = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<M, bhw::Struct>)
                {
                    return walkStruct(m, ctx);
                }
                return "";
            },
            node);
    }
    
    return str.str();
}

std::string CppReflectWalker::walkNestedStruct(const bhw::Struct& s,
                                               const std::string& rootTypeName,
                                               const std::vector<std::string>& path,
                                               const std::string& namespaceName,
                                               const std::string& parentMetaPath)
{
    std::stringstream str;
    
    str << "namespace " << namespaceName << "\n{\n";
    
    // Build the proper decltype for this level
    std::string thisTypeDecltype = buildNestedDecltype(rootTypeName, path);
    
    // Collect direct fields only
    std::vector<std::string> fieldNames;
    std::vector<std::string> fieldDefs;
    
    for (const auto& member : s.members)
    {
        std::visit([&](const auto& m) {
            using M = std::decay_t<decltype(m)>;
            
            if constexpr (std::is_same_v<M, bhw::Field>)
            {
                fieldNames.push_back(m.name);
                std::stringstream fs;
                fs << "    meta::field<&" << thisTypeDecltype << "::" << m.name << ">(\"" << m.name << "\")";
                fieldDefs.push_back(fs.str());
            }
            else if constexpr (std::is_same_v<M, bhw::Struct>)
            {
                if (!m.variableName.empty())
                {
                    fieldNames.push_back(m.variableName);
                    std::stringstream fs;
                    fs << "    meta::field<&" << thisTypeDecltype << "::" << m.variableName << ">(\"" << m.variableName << "\")";
                    fieldDefs.push_back(fs.str());
                }
            }
        }, member);
    }
    
    // Generate fields tuple
    str << "inline const auto FieldsMeta = std::make_tuple(\n";
    for (size_t i = 0; i < fieldDefs.size(); ++i)
    {
        if (i > 0) str << ",\n";
        str << fieldDefs[i];
    }
    str << ");\n\n";
    
    // Recursively handle deeper nesting
    std::string currentMetaPath = parentMetaPath + "::" + namespaceName;
    for (const auto& member : s.members)
    {
        std::visit([&](const auto& m) {
            using M = std::decay_t<decltype(m)>;
            
            if constexpr (std::is_same_v<M, bhw::Struct>)
            {
                if (!m.variableName.empty())
                {
                    // Build new path by appending this variable
                    std::vector<std::string> newPath = path;
                    newPath.push_back(m.variableName);
                    
                    str << walkNestedStruct(m, rootTypeName, newPath, m.variableName, currentMetaPath);
                }
            }
        }, member);
    }
    
    str << "} // namespace " << namespaceName << "\n";
    
    return str.str();
}

std::string CppReflectWalker::walkNestedTypeDefinitions(const bhw::Struct& s,
                                                        const std::string& parentName,
                                                        const bhw::WalkContext& ctx)
{
    std::stringstream str;
    
    for (const auto& member : s.members)
    {
        std::visit(
            [&](const auto& m)
            {
                using M = std::decay_t<decltype(m)>;
                if constexpr (std::is_same_v<M, bhw::Struct>)
                {
                    if (!m.name.empty() && !m.variableName.empty())
                    {
                        str << walkStruct(m, ctx);
                    }
                }
            },
            member);
    }
    
    return str.str();
}

std::string CppReflectWalker::walkStruct(const bhw::Struct& s, const bhw::WalkContext& ctx)
{
    std::stringstream str;

    std::string package = (s.namespaces.size() > 0) ? joinRanges(s.namespaces, "::") + "::" : "";
    std::string fullStructName = "::" + package + s.name;

    currentParentStruct_ = fullStructName;

    str << "namespace meta\n{\n";
    str << "namespace " << s.name << "\n{\n";

    // Collect only DIRECT members of this struct
    std::vector<std::string> fieldNames;
    std::vector<std::string> fieldDefs;
    
    for (const auto& member : s.members)
    {
        std::visit([&](const auto& m) {
            using M = std::decay_t<decltype(m)>;
            
            if constexpr (std::is_same_v<M, bhw::Field>)
            {
                // Direct field
                fieldNames.push_back(m.name);
                std::stringstream fs;
                fs << "    meta::field<&" << fullStructName << "::" << m.name << ">(\"" << m.name << "\")";
                fieldDefs.push_back(fs.str());
            }
            else if constexpr (std::is_same_v<M, bhw::Struct>)
            {
                // Nested struct - add as whole member
                if (!m.variableName.empty())
                {
                    fieldNames.push_back(m.variableName);
                    std::stringstream fs;
                    fs << "    meta::field<&" << fullStructName << "::" << m.variableName << ">(\"" << m.variableName << "\")";
                    fieldDefs.push_back(fs.str());
                }
            }
        }, member);
    }

    // Generate fields tuple for THIS level only
    str << "inline const auto FieldsMeta = std::make_tuple(\n";
    for (size_t i = 0; i < fieldDefs.size(); ++i)
    {
        if (i > 0) str << ",\n";
        str << fieldDefs[i];
    }
    str << ");\n\n";
    
    // Now recursively generate nested namespaces for nested structs
    std::string parentMetaPath = "meta::" + s.name;
    for (const auto& member : s.members)
    {
        std::visit([&](const auto& m) {
            using M = std::decay_t<decltype(m)>;
            
            if constexpr (std::is_same_v<M, bhw::Struct>)
            {
                if (!m.variableName.empty())
                {
                    // Pass rootType and initial path
                    std::vector<std::string> initialPath = {m.variableName};
                    str << walkNestedStruct(m, fullStructName, initialPath, m.variableName, parentMetaPath);
                }
            }
        }, member);
    }
    
    str << "} // namespace " << s.name << "\n";
    str << "} // namespace meta\n\n";

    // Generate MetaTuple specialization for root struct
    auto tableName = s.name;
    std::stringstream select;
    auto fields = joinRanges(fieldNames, ", ");
    select << "select " << fields << " from " << s.name;

    str << "namespace meta\n{\n"
        << "template <> struct MetaTuple<" << fullStructName << ">\n{\n"
        << "  static inline const auto& FieldsMeta = meta::" << s.name << "::FieldsMeta;\n"
        << "  static constexpr auto tableName = \"" << tableName << "\";\n"
        << "  static constexpr auto query = \"" << select.str() << "\";\n"
        << "};\n";
    
    // Generate MetaTuple for all nested anonymous structs
    for (const auto& member : s.members)
    {
        std::visit([&](const auto& m) {
            using M = std::decay_t<decltype(m)>;
            
            if constexpr (std::is_same_v<M, bhw::Struct>)
            {
                if (!m.variableName.empty())
                {
                    std::vector<std::string> path = {m.variableName};
                    std::string nestedTypeDecltype = buildNestedDecltype(fullStructName, path);
                    std::string metaPath = "meta::" + s.name + "::" + m.variableName;
                    
                    str << "template <> struct MetaTuple<" << nestedTypeDecltype << ">\n{\n"
                        << "  static inline const auto& FieldsMeta = " << metaPath << "::FieldsMeta;\n"
                        << "};\n";
                    
                    // Recursively generate for deeper nesting
                    std::function<void(const bhw::Struct&, std::vector<std::string>, const std::string&)> generateNestedMetaTuples;
                    generateNestedMetaTuples = [&](const bhw::Struct& ns, std::vector<std::string> currentPath, const std::string& currentMetaPath) {
                        for (const auto& nmember : ns.members)
                        {
                            std::visit([&](const auto& nm) {
                                using NM = std::decay_t<decltype(nm)>;
                                if constexpr (std::is_same_v<NM, bhw::Struct>)
                                {
                                    if (!nm.variableName.empty())
                                    {
                                        std::vector<std::string> newPath = currentPath;
                                        newPath.push_back(nm.variableName);
                                        
                                        std::string deepTypeDecltype = buildNestedDecltype(fullStructName, newPath);
                                        std::string deepMetaPath = currentMetaPath + "::" + nm.variableName;
                                        
                                        str << "template <> struct MetaTuple<" << deepTypeDecltype << ">\n{\n"
                                            << "  static inline const auto& FieldsMeta = " << deepMetaPath << "::FieldsMeta;\n"
                                            << "};\n";
                                        
                                        generateNestedMetaTuples(nm, newPath, deepMetaPath);
                                    }
                                }
                            }, nmember);
                        }
                    };
                    
                    generateNestedMetaTuples(m, path, metaPath);
                }
            }
        }, member);
    }
    
    str << "} // namespace meta\n\n";

    str << walkNestedTypeDefinitions(s, fullStructName, ctx);

    return str.str();
}

std::string joinRanges(const std::vector<std::string>& vec, const std::string& sep)
{
    if (vec.empty())
        return {};

    return std::accumulate(std::next(vec.begin()),
                           vec.end(),
                           vec[0],
                           [&sep](const std::string& a, const std::string& b)
                           { return a + sep + b; });
}

} // namespace

#include <CLI11.hpp>

int main(int argc, char* argv[])
{
    CLI::App app{"CppReflect - generate reflection metadata from C++ structs"};

    std::string inputFile;
    std::string overrideExt;
    bool showAst = false;
    bool noOutput = false;

    app.add_option("input", inputFile, "Input C++ source file ('-' for stdin)")
        ->default_val("-");

    app.add_option("--ext", overrideExt, "Override input parser (file extension)");

    app.add_flag("--ast", showAst, "Dump AST to stderr");
    app.add_flag("--no-output", noOutput, "Suppress generated output");

    CLI11_PARSE(app, argc, argv);

    bool showOutput = !noOutput;

    try
    {
        std::string source;

        if (inputFile == "-")
        {
            std::ostringstream ss;
            ss << std::cin.rdbuf();
            source = ss.str();
        }
        else
        {
            source = bhw::readFile(inputFile);
        }

        std::string ext;
        if (!overrideExt.empty())
            ext = overrideExt;
        else if (inputFile == "-")
            ext = "cpp";
        else
            ext = bhw::getFileExtension(inputFile).substr(1);

        auto ast = bhw::CppParser{}.parseToAst(source);

        if (showAst)
        {
            std::cerr << "********* AST **********\n";
            std::cerr << ast.showAst() << "\n";
        }

        if (showOutput)
        {
            std::cout << CppReflectWalker{}.walk(std::move(ast)) << "\n";
        }
    }
    catch (std::runtime_error& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

