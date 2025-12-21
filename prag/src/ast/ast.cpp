#include "ast.h"

#include <sstream>

auto bhw::operator<<(std::ostream& os, const bhw::SimpleType& s) -> std::ostream&
{
    os << s.reifiedType << "(" << s.srcTypeString << ")";
    return os;
}

auto bhw::operator<<(std::ostream& os, const bhw::StructRefType& s) -> std::ostream&
{
    os << s.reifiedType << "(" << s.srcTypeString << ")";
    return os;
}

std::string bhw::readFile(const std::string& filename)
{
    std::ifstream file(filename);
    if (!file)
    {
        throw std::runtime_error("Cannot open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

std::string bhw::getFileExtension(const std::string& filename)
{
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos)
        return "";
    return filename.substr(dot_pos);
}

// Convert string to uppercase safely (C++17)
std::string bhw::toUpper(std::string& s)
{
    std::transform(s.begin(),
                   s.end(),
                   s.begin(),
                   [](char c) -> char
                   { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });
    return s;
}

// Convert string to lowercase safely (C++17)
std::string bhw::toLower(std::string& s)
{
    std::transform(s.begin(),
                   s.end(),
                   s.begin(),
                   [](char c) -> char
                   { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); });
    return s;
}

std::string bhw::showStruct(const bhw::Struct& s, size_t indent)
{
    std::stringstream str;
    std::string ind(indent * 2, ' ');

    str << ind << "Struct: " << s.name << " " << (s.isAnonymous ? "anonymous " : "")
        << s.variableName << "\n";
    str << ind << "  Namespace: ";
    for (const auto& ns : s.namespaces)
    {
        str << ind << "    " << ns << "::";
    }
    str << "\n";

    str << ind << "  Attributes: ";
    for (const auto& a : s.attributes)
    {
        str << ind << "    " << a.name << "=" << a.value << "\n";
    }
    str << "\n";

    str << ind << "  Members:\n";
    for (const auto& m : s.members)
    {
        if (std::holds_alternative<Field>(m))
        {
            const auto& f = std::get<Field>(m);
            str << ind << "    Field name: " << f.name << "\n"
                << ind << "          type: " << showType(*f.type, indent + 1) << "\n";

            if (!f.attributes.empty())
            {
                std::string prefix = ind + "          attrs: ";
                for (const auto& a : f.attributes)
                {
                    str << prefix << a.name << "=" << a.value << "\n";
                    prefix = ind + "                 ";
                }
            }
        }
        else if (std::holds_alternative<Oneof>(m))
        {
            const auto& o = std::get<Oneof>(m);
            str << ind << "    Oneof: " << o.name << "\n";
            str << ind << "      Fields:\n";
            for (const auto& f : o.fields)
            {
                str << ind << "        " << f.name << ": " << showType(*f.type, indent + 3) << "\n";
            }
        }
        else if (std::holds_alternative<Enum>(m))
        {
            auto& num = std::get<Enum>(m);
            str << ind << "Enum " << num.name << "\n";
            for (auto& [name, number, x, y] : num.values)
            {
                str << "        " << name << " " << number << "\n";
            }
        }
        else if (std::holds_alternative<Struct>(m))
        {
            auto& strct = std::get<Struct>(m);
            str << showStruct(strct, indent + 2) << "\n";
        }
    }
    return str.str();
}

auto bhw::showType(const bhw::Type& type, size_t level) -> std::string
{
    const std::string ind(level, ' ');
    std::stringstream str;

    if (type.isSimple())
    {
        const auto& s = std::get<SimpleType>(type.value);
        str << s;
    }
    else if (type.isStructRef())
    {
        const auto& s = std::get<StructRefType>(type.value);
        str << s;
    }
    else if (type.isPointer())
    {
        const auto& p = std::get<PointerType>(type.value);
        str << "PointerType -> " << showType(*p.pointee, level + 1);
    }
    else if (type.isGeneric())
    {
        const auto& g = std::get<GenericType>(type.value);
        str << g.reifiedType << "[";
        std::string sep;
        for (const auto& arg : g.args)
        {
            str << sep << showType(*arg, level + 1);
            sep = ", ";
        }
        str << "]";
    }
    else if (type.isStruct())
    {
        const auto& s = std::get<StructType>(type.value).value;
        str << ind << "StructType:\n";
        str << showStruct(*s, level + 2);
    }

    return str.str();
}

std::string bhw::showField(const Field& field, size_t indent)
{
    std::stringstream str;
    str << showType(*field.type, indent);
    str << "  attrs: ";
    for (const auto& a : field.attributes)
    {
        str << " " << a.name << "=" << a.value << " ";
    }
    str << "\n";
    return str.str();
}

std::string showNodes(const std::vector<bhw::AstRootNode>& nodes, size_t indent = 0)
{
    std::stringstream ss;
    const std::string ind(indent * 2, ' ');
    for (const auto& node : nodes)
    {
        if (std::holds_alternative<bhw::Enum>(node))
        {
            auto& num = std::get<bhw::Enum>(node);
            ss << ind << "Enum : " << num.name << "\n";

            for (auto& [name, number, x, y] : num.values)
            {
	      ss << "        " << name << " " << number << "\n";
            }

        }
        else if (std::holds_alternative<bhw::Struct>(node))
        {
            ss << bhw::showStruct(std::get<bhw::Struct>(node));
        }

        else if (std::holds_alternative<bhw::Namespace>(node))
        {
            const auto& ns = std::get<bhw::Namespace>(node);
            ss << ind << "namespace " << ns.name << "\n";

            ss << showNodes(ns.nodes, indent + 2);
        }
    }
    return ss.str();
}
void bhw::Ast::flattenNestedTypes()
{
    std::vector<Enum> flattenedEnums;
    std::vector<Struct> flattenedStructs;

    // Process all top-level structs
    for (auto& node : nodes)
    {
        if (std::holds_alternative<Struct>(node))
        {
            flattenStructMembers(std::get<Struct>(node), flattenedStructs, flattenedEnums);
        }
    }

    for (auto it = flattenedStructs.rbegin(); it != flattenedStructs.rend(); ++it)
    {
        nodes.insert(nodes.begin(), std::move(*it));
    }

    for (auto it = flattenedEnums.rbegin(); it != flattenedEnums.rend(); ++it)
    {
        nodes.insert(nodes.begin(), std::move(*it));
    }
}
auto bhw::Ast::showAst(size_t indent) const -> std::string
{
    std::stringstream ss;
    std::string ind(indent * 2, ' ');
    ss << showNodes(nodes);
    return ss.str();
}



void bhw::Ast::flattenStructMembers(Struct& s,
                                    std::vector<Struct>& flattenedStructs,
                                    std::vector<Enum>& flattenedEnums)
{
    std::vector<StructMember> newMembers;
    for (auto& member : s.members)
    {
        if (auto* nested = std::get_if<Struct>(&member))
        {
            bool isAnonymous = nested->isAnonymous || nested->name.empty() ||
                               nested->name.find("anonymous") != std::string::npos;
            if (isAnonymous && !nested->variableName.empty())
            {
                // Generate a name for the anonymous struct based on variable name
                std::string generatedName = "Anonymous";
                generatedName += (char)std::toupper(nested->variableName[0]);
                generatedName += nested->variableName.substr(1);
                nested->name = generatedName;
                nested->isAnonymous = false;
                // Recursively flatten
                flattenStructMembers(*nested, flattenedStructs, flattenedEnums);
                // Replace with field reference
                Field field;
                field.name = nested->variableName;
                field.type = std::make_unique<Type>(
                    StructRefType{generatedName, ReifiedTypeId::StructRefType});
                field.attributes = nested->attributes;
                newMembers.push_back(std::move(field));
                // Hoist to top level
                flattenedStructs.push_back(std::move(*nested));
            }
            else if (!isAnonymous)
            {
                // Named struct - flatten recursively and hoist
                flattenStructMembers(*nested, flattenedStructs, flattenedEnums);
                if (!nested->variableName.empty())
                {
                    Field field;
                    field.name = nested->variableName;
                    field.type = std::make_unique<Type>(
                        StructRefType{nested->name, ReifiedTypeId::StructRefType});
                    field.attributes = nested->attributes;
                    newMembers.push_back(std::move(field));
                }
                flattenedStructs.push_back(std::move(*nested));
            }
            else
            {
                // Anonymous struct with no variable name - inline fields (rare case)
                flattenStructMembers(*nested, flattenedStructs, flattenedEnums);
                for (auto& nestedMember : nested->members)
                {
                    newMembers.push_back(std::move(nestedMember));
                }
            }
        }
        else if (auto* nestedEnum = std::get_if<Enum>(&member))
        {
            // Hoist enums to top level
            flattenedEnums.push_back(std::move(*nestedEnum));
            // DO NOT add to newMembers - enums are not fields!
        }

else if (auto* field = std::get_if<Field>(&member))
{
    // NEW: Handle Fields with anonymous StructType
    if (field->type && field->type->isStruct())
    {
        auto& structType = std::get<StructType>(field->type->value);
        if (structType.value && 
            (structType.value->isAnonymous || structType.value->name == "<anonymous>") &&
            !structType.value->variableName.empty())
        {
            // Generate name: ParentStructName + FieldName
            std::string generatedName = s.name;
            generatedName += (char)std::toupper(field->name[0]);
            generatedName += field->name.substr(1);
            
            // Update the nested struct name
            structType.value->name = generatedName;
            structType.value->isAnonymous = false;
            
            // Recursively flatten the nested struct
            flattenStructMembers(*structType.value, flattenedStructs, flattenedEnums);
            
            // Hoist the struct to top level (make a copy before we change the field type)
            flattenedStructs.push_back(std::move(*structType.value));
            
            // NOW change field type to a StructRefType reference
            field->type = std::make_unique<Type>(
                StructRefType{generatedName, ReifiedTypeId::StructRefType});
        }
    }
    // Keep the field
    newMembers.push_back(std::move(*field));
}
	
    }
    s.members = std::move(newMembers);
}

