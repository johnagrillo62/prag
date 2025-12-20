#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <variant>

#include "ast.h"
#include "cpp_parser.h"
#include "cpp_walker.h"
#include "registry_ast_walker.h"

namespace
{

struct Ret
{
    std::string name;
    std::string type;
};

using RetTuple = std::vector<std::tuple<std::string, std::string, bhw::AttributeVec, bool>>;
auto joinRanges(const std::vector<std::string>& vec, const std::string& sep) -> std::string;

class CppReflectWalker : public bhw::CppWalker
{
  public:
    auto walkNamespace(const bhw::Namespace& ns, size_t indent) -> std::string override;
    auto walkStruct(const bhw::Struct& s, size_t indent) -> std::string override;
    auto getFields(const bhw::StructMember& member,
                   const std::string& parentName,
                   const std::string& accessPath) -> RetTuple;
    std::string generateOneof(const bhw::Oneof& oneof, size_t) override
    {
        return "";
    }
      std::string generateHeader(const bhw::Ast& ast) override
    {
      return "";
    }

  protected:
    std::string generateGenericType(const bhw::GenericType& type, size_t indent) override
    {
        std::ostringstream out;

        // Map ReifiedTypeId to type string
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
            // Fall back to base class implementation
            return bhw::CppWalker::generateGenericType(type, indent);
        }

        out << baseType;

        if (!type.args.empty())
        {
            out << "<";
            for (size_t i = 0; i < type.args.size(); i++)
            {
                if (i > 0)
                    out << ", ";

                // Check if this argument is a struct reference
                if (auto* structRef = std::get_if<bhw::StructRefType>(&type.args[i]->value))
                {
                    // Fully qualify the struct name
                    std::string typeName = structRef->srcTypeString;

                    // If already qualified (starts with ::), keep it
                    if (typeName.find("::") == 0)
                    {
                        out << typeName;
                    }
                    else if (currentParentStruct_.empty())
                    {
                        // Top-level struct reference - just qualify with ::
                        out << "::" << typeName;
                    }
                    else
                    {
                        // Nested struct - fully qualify with parent
                        out << currentParentStruct_ << "::" << typeName;
                    }
                }
                else
                {
                    // For other types, use the base class implementation
                    out << walkType(*type.args[i], indent);
                }
            }
            out << ">";
        }

        return out.str();
    }

  private:
    std::string generateAccessorFunctions(const std::string& structName,
                                          const std::string& fieldName,
                                          const std::string& accessPath,
                                          const std::string& type);
    std::string sanitizeFunctionName(const std::string& path);
    std::string walkNestedTypeDefinitions(const bhw::Struct& s,
                                          const std::string& parentFullName,
                                          size_t indent);
    std::string walkNestedStruct(const bhw::Struct& s,
                                 const std::string& fullStructName,
                                 size_t indent);

    // ⭐ ADDED: Track current parent struct for nested type qualification
    std::string currentParentStruct_;
};

} // namespace


#include <CLI11.hpp>

int main(int argc, char* argv[])
{
    CLI::App app{"CppReflect - generate reflection metadata from C++ structs"};

    std::string inputFile;
    std::string overrideExt; // To override parser extension
    bool showAst = false;
    bool noOutput = false;

    // Positional input file: defaults to "-" (stdin) if not provided
    app.add_option("input", inputFile, "Input C++ source file ('-' for stdin)")
        ->default_val("-");

    // Optional override extension
    app.add_option("--ext", overrideExt, "Override input parser (file extension)");

    // Flags
    app.add_flag("--ast", showAst, "Dump AST to stderr");
    app.add_flag("--no-output", noOutput, "Suppress generated output");

    CLI11_PARSE(app, argc, argv);

    bool showOutput = !noOutput;

    try
    {
        std::string source;

        if (inputFile == "-")
        {
            // Read all stdin into a string
            std::ostringstream ss;
            ss << std::cin.rdbuf();
            source = ss.str();
        }
        else
        {
            source = bhw::readFile(inputFile);
        }

        // Determine extension: override if specified, otherwise use file extension or "cpp" for stdin
        std::string ext;
        if (!overrideExt.empty())
            ext = overrideExt;
        else if (inputFile == "-")
            ext = "cpp";
        else
            ext = bhw::getFileExtension(inputFile).substr(1);

        // Parse AST
        auto ast = bhw::CppParser{}.parseToAst(source);

        // Dump AST if requested
        if (showAst)
        {
            std::cerr << "********* AST **********\n";
            std::cerr << ast.showAst() << "\n";
        }

        // Generate reflection output
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


namespace
{
std::string CppReflectWalker::walkNamespace(const bhw::Namespace& ns, size_t indent)
{
    std::ostringstream out;

    // Walk all nodes in namespace
    for (const auto& node : ns.nodes)
    {
        out << walkRootNode(node, indent + 1);
    }
    return out.str();
}

std::string CppReflectWalker::sanitizeFunctionName(const std::string& path)
{
    // Convert "::Car::engine::name" to "engine_name"
    std::string result = path;

    // Remove leading "::"
    if (result.find("::") == 0)
        result = result.substr(2);

    // Find first "::" and remove everything before it (the struct name)
    auto pos = result.find("::");
    if (pos != std::string::npos)
        result = result.substr(pos + 2);

    // Replace remaining "::" with "_"
    while ((pos = result.find("::")) != std::string::npos)
    {
        result.replace(pos, 2, "_");
    }

    return result;
}

std::string CppReflectWalker::generateAccessorFunctions(const std::string& structName,
                                                        const std::string& fieldName,
                                                        const std::string& accessPath,
                                                        const std::string& type)
{
    std::stringstream str;

    std::string funcName = sanitizeFunctionName(fieldName);

    // Generate getter
    str << "inline " << type << " get_" << funcName << "(const " << structName << "& obj)\n{\n"
        << "    return obj" << accessPath << ";\n"
        << "}\n\n";

    // Generate setter
    str << "inline void set_" << funcName << "(" << structName << "& obj, const " << type
        << "& val)\n{\n"
        << "    obj" << accessPath << " = val;\n"
        << "}\n\n";

    return str.str();
}

std::string CppReflectWalker::walkNestedTypeDefinitions(const bhw::Struct& s,
                                                        const std::string& parentFullName,
                                                        size_t indent)
{
    std::stringstream str;

    // Generate reflection for nested type definitions
    for (const auto& member : s.members)
    {
        if (auto* nestedStruct = std::get_if<bhw::Struct>(&member))
        {
            // Check if this is a type definition (no instance created)
            if (nestedStruct->variableName.empty() && !nestedStruct->name.empty())
            {
                // This is a named nested struct without an instance
                // Build the nested struct's full name including parent
                std::string nestedFullName = parentFullName + "::" + nestedStruct->name;

                // ⭐ SET CONTEXT for nested struct
                std::string savedParent = currentParentStruct_;
                currentParentStruct_ = parentFullName;

                // Generate the reflection with corrected namespace
                str << "\n" << walkNestedStruct(*nestedStruct, nestedFullName, indent);

                // ⭐ RESTORE CONTEXT
                currentParentStruct_ = savedParent;
            }
        }
    }

    return str.str();
}

std::string CppReflectWalker::walkNestedStruct(const bhw::Struct& s,
                                               const std::string& fullStructName,
                                               size_t indent)
{
    std::stringstream str;

    str << "namespace meta\n{\n";
    str << "namespace " << s.name << "\n{\n";

    // Generate fields (no accessors needed for type definitions without instances)
    str << "// Field metadata\n";
    str << "inline const auto fields = std::make_tuple(\n";

    std::vector<std::string> fieldNames;
    auto sep = "";

    for (const auto& member : s.members)
    {
        for (const auto& [name, typeString, attrs, isNested] :
             getFields(member, fullStructName, ""))
        {
            std::string displayName = name;
            size_t pos = displayName.rfind("::");
            if (pos != std::string::npos)
            {
                displayName = displayName.substr(pos + 2);
            }

            fieldNames.emplace_back(displayName);

            str << sep << "    meta::Field< " << fullStructName << ", " << typeString << ", ";

            if (isNested)
            {
                std::string funcName = sanitizeFunctionName(name);
                str << "nullptr, meta::Prop::Serializable, "
                    << "&get_" << funcName << ", &set_" << funcName;
            }
            else
            {
                str << "&" << name << ", meta::Prop::Serializable, nullptr, nullptr";
            }

            str << ">(\"" << typeString << "\", \"" << displayName << "\")";
            sep = ",\n";
        }
    }

    str << ");\n";
    str << "} // namespace " << s.name << "\n";
    str << "} // namespace meta\n\n";

    // MetaTuple specialization
    auto tableName = s.name;
    std::stringstream select;
    auto fields = joinRanges(fieldNames, ", ");
    select << "select " << fields << " from " << s.name;

    str << "namespace meta\n{\n"
        << "// Template specialization for type-based reflection lookup\n"
        << "template <>  struct MetaTuple< " << fullStructName << ">\n{\n"
        << "  static inline const auto& fields = meta::" << s.name << "::fields;\n"
        << "  static constexpr auto tableName = \"" << tableName << "\";\n"
        << "  static constexpr auto query = \"" << select.str() << "\";\n"
        << "};\n"
        << "} // namespace meta\n";

    // Recursively handle nested type definitions within this nested struct
    str << walkNestedTypeDefinitions(s, fullStructName, indent);

    return str.str();
}

std::string CppReflectWalker::walkStruct(const bhw::Struct& s, size_t indent)
{
    std::stringstream str;

    std::string package = (s.namespaces.size() > 0) ? joinRanges(s.namespaces, "::") + "::" : "";
    std::string fullStructName = "::" + package + s.name;

    // ⭐ SET CONTEXT
    currentParentStruct_ = fullStructName;

    str << "namespace meta\n{\n";
    str << "namespace " << s.name << "\n{\n";

    // First pass: Generate accessor functions for nested anonymous struct members
    std::stringstream accessorStr;
    for (const auto& member : s.members)
    {
        for (const auto& [name, typeString, attrs, isNested] :
             getFields(member, fullStructName, ""))
        {
            if (isNested)
            {
                // Extract access path from the full name
                // e.g., "::Car::engine::name" -> ".engine.name"
                std::string accessPath = name;
                size_t pos = accessPath.find(fullStructName);
                if (pos != std::string::npos)
                {
                    accessPath = accessPath.substr(pos + fullStructName.length());
                    // Replace "::" with "."
                    while ((pos = accessPath.find("::")) != std::string::npos)
                    {
                        accessPath.replace(pos, 2, ".");
                    }
                }

                accessorStr << generateAccessorFunctions(
                    fullStructName, name, accessPath, typeString);
            }
        }
    }

    // Output accessor functions if any were generated
    if (!accessorStr.str().empty())
    {
        str << "// Accessor functions for nested anonymous struct members\n";
        str << accessorStr.str();
    }

    // Second pass: Generate fields tuple
    str << "// Field metadata\n";
    str << "inline const auto fields = std::make_tuple(\n";

    std::vector<std::string> fieldNames;
    auto sep = "";

    for (const auto& member : s.members)
    {
        for (const auto& [name, typeString, attrs, isNested] :
             getFields(member, fullStructName, ""))
        {
            // Extract simple field name for query
            std::string simpleName = name;
            size_t pos = simpleName.rfind("::");
            if (pos != std::string::npos)
            {
                simpleName = simpleName.substr(pos + 2);
            }

            // Build path for nested fields (e.g., "engine.name")
            std::string displayName = name;
            pos = displayName.find(fullStructName);
            if (pos != std::string::npos)
            {
                displayName = displayName.substr(pos + fullStructName.length() + 2); // Skip "::"
                // Replace "::" with "."
                while ((pos = displayName.find("::")) != std::string::npos)
                {
                    displayName.replace(pos, 2, ".");
                }
            }

            fieldNames.emplace_back(displayName);

            str << sep << "    meta::Field< " << fullStructName << ", " << typeString << ", ";

            if (isNested)
            {
                // For nested fields: use nullptr for member pointer, and function pointers for
                // accessors
                std::string funcName = sanitizeFunctionName(name);
                str << "nullptr, meta::Prop::Serializable, "
                    << "&get_" << funcName << ", &set_" << funcName;
            }
            else
            {
                // For direct fields: use member pointer
                str << "&" << name << ", meta::Prop::Serializable, nullptr, nullptr";
            }

            str << ">(\"" << typeString << "\", \"" << displayName << "\")";
            sep = ",\n";
        }
    }

    str << ");\n";
    str << "} // namespace " << s.name << "\n";
    str << "} // namespace meta\n\n";

    // Generate MetaTuple specialization
    auto tableName = s.name;
    std::stringstream select;
    auto fields = joinRanges(fieldNames, ", ");
    select << "select " << fields << " from " << s.name;

    str << "namespace meta\n{\n"
        << "// Template specialization for type-based reflection lookup\n"
        << "template <>  struct MetaTuple< " << fullStructName << ">\n{\n"
        << "  static inline const auto& fields = meta::" << s.name << "::fields;\n"
        << "  static constexpr auto tableName = \"" << tableName << "\";\n"
        << "  static constexpr auto query = \"" << select.str() << "\";\n"
        << "};\n"
        << "} // namespace meta\n";

    // Generate reflection for nested type definitions (if any)
    str << walkNestedTypeDefinitions(s, fullStructName, indent);

    // ⭐ CLEAR CONTEXT
    currentParentStruct_.clear();

    return str.str();
}

auto CppReflectWalker::getFields(const bhw::StructMember& member,
                                 const std::string& parentName,
                                 const std::string& accessPath) -> RetTuple
{
    return std::visit(
        [&](const auto& m) -> RetTuple
        {
            using M = std::decay_t<decltype(m)>;

            // ----- Field -----
            if constexpr (std::is_same_v<M, bhw::Field>)
            {
                // Visit the type variant
                return std::visit(
                    [&](const auto& t) -> RetTuple
                    {
                        using T = std::decay_t<decltype(t)>;
                        if constexpr (std::is_same_v<T, bhw::SimpleType>)
                        {
                            bool isNested = !accessPath.empty();
                            return {std::make_tuple<>(parentName + "::" + m.name,
                                                      generateSimpleType(t, 0),
                                                      m.attributes,
                                                      isNested)};
                        }
                        else if constexpr (std::is_same_v<T, bhw::GenericType>)
                        {
                            bool isNested = !accessPath.empty();
                            return {std::make_tuple<>(parentName + "::" + m.name,
                                                      generateGenericType(t, 0),
                                                      m.attributes,
                                                      isNested)};
                        }
                        return {std::make_tuple<>("", "", m.attributes, false)};
                    },
                    m.type->value);
            }

            // ----- Nested Struct -----
            if constexpr (std::is_same_v<M, bhw::Struct>)
            {
                // ⭐ KEY FIX: Check if this struct creates a field in the parent
                if (m.variableName.empty())
                {
                    // Named nested struct with NO instance
                    // Example: struct A { struct B { int y; }; };
                    //          ^ B is just a type definition, not a field
                    // This will be handled separately by walkNestedTypeDefinitions
                    return {};
                }

                // This struct DOES create a field (anonymous or named with instance)
                // Examples:
                //   struct { int x; } myField;  ← anonymous with instance
                //   struct B { int x; } myB;    ← named with instance
                RetTuple tuples;
                std::string newAccessPath = accessPath + "::" + m.variableName;

                for (const auto& node : m.members)
                {
                    for (const auto& [name, type, attrs, isNested] :
                         getFields(node, parentName + "::" + m.variableName, newAccessPath))
                    {
                        tuples.emplace_back(
                            std::make_tuple<>(name, type, attrs, true)); // Mark as nested
                    }
                }

                return tuples;
            }

            return {std::make_tuple<>("", "", m.attributes, false)};
        },
        member);
}

std::string joinRanges(const std::vector<std::string>& vec, const std::string& sep)
{
    if (vec.empty())
        return {};

    // Use accumulate to fold the strings with a separator
    return std::accumulate(std::next(vec.begin()),
                           vec.end(),
                           vec[0],
                           [&sep](const std::string& a, const std::string& b)
                           { return a + sep + b; });
}

} // namespace
