#include "graphql_parser.h"

#include <cctype>
#include <stdexcept>

#include "ast.h"

using namespace bhw;

// Lexer implementation
void GraphQLParser::skipWhitespaceAndComments()
{
    while (pos < source.length())
    {
        char c = source[pos];

        if (std::isspace(c))
        {
            pos++;
            continue;
        }

        // Comments: #
        if (c == '#')
        {
            while (pos < source.length() && source[pos] != '\n')
            {
                pos++;
            }
            continue;
        }

        break;
    }
}

GraphQLToken GraphQLParser::readIdentifier()
{
    size_t start = pos;
    while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_'))
    {
        pos++;
    }
    std::string value = source.substr(start, pos - start);

    // Check keywords
    if (value == "type")
        return {GraphQLTokenType::Type, value};
    if (value == "interface")
        return {GraphQLTokenType::Interface, value};
    if (value == "enum")
        return {GraphQLTokenType::Enum, value};
    if (value == "input")
        return {GraphQLTokenType::Input, value};
    if (value == "query")
        return {GraphQLTokenType::Query, value};
    if (value == "mutation")
        return {GraphQLTokenType::Mutation, value};
    if (value == "subscription")
        return {GraphQLTokenType::Subscription, value};

    // Check type keywords
    if (value == "Int")
        return {GraphQLTokenType::Int, value};
    if (value == "Float")
        return {GraphQLTokenType::Float, value};
    if (value == "String")
        return {GraphQLTokenType::String, value};
    if (value == "Boolean")
        return {GraphQLTokenType::Boolean, value};
    if (value == "ID")
        return {GraphQLTokenType::ID, value};

    return {GraphQLTokenType::Identifier, value};
}

GraphQLToken GraphQLParser::nextToken()
{
    skipWhitespaceAndComments();

    if (pos >= source.length())
    {
        return {GraphQLTokenType::EndOfFile, ""};
    }

    char c = source[pos];

    // Single character tokens
    if (c == '{')
    {
        pos++;
        return {GraphQLTokenType::LBrace, "{"};
    }
    if (c == '}')
    {
        pos++;
        return {GraphQLTokenType::RBrace, "}"};
    }
    if (c == '[')
    {
        pos++;
        return {GraphQLTokenType::LBracket, "["};
    }
    if (c == ']')
    {
        pos++;
        return {GraphQLTokenType::RBracket, "]"};
    }
    if (c == '(')
    {
        pos++;
        return {GraphQLTokenType::LParen, "("};
    }
    if (c == ')')
    {
        pos++;
        return {GraphQLTokenType::RParen, ")"};
    }
    if (c == ':')
    {
        pos++;
        return {GraphQLTokenType::Colon, ":"};
    }
    if (c == '!')
    {
        pos++;
        return {GraphQLTokenType::Exclamation, "!"};
    }
    if (c == '|')
    {
        pos++;
        return {GraphQLTokenType::Pipe, "|"};
    }
    if (c == '&')
    {
        pos++;
        return {GraphQLTokenType::Ampersand, "&"};
    }
    if (c == '@')
    {
        pos++;
        return {GraphQLTokenType::At, "@"};
    }

    // Identifiers
    if (std::isalpha(c) || c == '_')
    {
        return readIdentifier();
    }

    // Skip unknown
    pos++;
    return nextToken();
}

void GraphQLParser::advance()
{
    current_token = nextToken();
}

bool GraphQLParser::match(GraphQLTokenType type)
{
    if (current_token.type == type)
    {
        advance();
        return true;
    }
    return false;
}

void GraphQLParser::expect(GraphQLTokenType type)
{
    if (!match(type))
    {
        throw std::runtime_error("Unexpected token");
    }
}

std::unique_ptr<Type> GraphQLParser::parseType()
{
    // Check if this is a list type
    if (match(GraphQLTokenType::LBracket))
    {
        // Recursively parse the inner/element type
        auto element_type = parseType();

        // Match the closing bracket
        expect(GraphQLTokenType::RBracket);

        // Check if the list itself is non-null
        bool is_optional = !match(GraphQLTokenType::Exclamation);

        // Build the list type
        GenericType generic_type;
        generic_type.reifiedType = ReifiedTypeId::List;
        generic_type.args.emplace_back(std::move(element_type));

        if (is_optional)
        {
            PointerType ptr_type;
            ptr_type.pointee = std::make_unique<Type>(std::move(generic_type));
            return std::make_unique<Type>(std::move(ptr_type));
        }
        else
        {
            return std::make_unique<Type>(std::move(generic_type));
        }
    }

    // Parse base type (non-list)
    ReifiedTypeId canonical_type = ReifiedTypeId::Unknown;
    std::string type_name;
    bool is_primitive = false;

    if (match(GraphQLTokenType::Int))
    {
        canonical_type = ReifiedTypeId::Int32;
        is_primitive = true;
    }
    else if (match(GraphQLTokenType::Float))
    {
        canonical_type = ReifiedTypeId::Float64;
        is_primitive = true;
    }
    else if (match(GraphQLTokenType::String))
    {
        canonical_type = ReifiedTypeId::String;
        is_primitive = true;
    }
    else if (match(GraphQLTokenType::Boolean))
    {
        canonical_type = ReifiedTypeId::Bool;
        type_name = "Boolean";
        is_primitive = true;
    }
    else if (match(GraphQLTokenType::ID))
    {
        canonical_type = ReifiedTypeId::String;
        is_primitive = true;
    }
    else
    {
        // ⭐ Custom type (User, Post, Status, etc.)
        type_name = current_token.value;
        advance();
        
        // Check if non-null
        bool is_optional = !match(GraphQLTokenType::Exclamation);
        
        // Use StructRefType for custom types
        StructRefType struct_ref;
        struct_ref.srcTypeString = type_name;
        struct_ref.reifiedType = ReifiedTypeId::StructRefType;
        
        if (is_optional)
        {
            PointerType ptr_type;
            ptr_type.pointee = std::make_unique<Type>(std::move(struct_ref));
            return std::make_unique<Type>(std::move(ptr_type));
        }
        else
        {
            return std::make_unique<Type>(std::move(struct_ref));
        }
    }

    // Handle primitive types
    // Check if the base type is non-null
    bool is_optional = !match(GraphQLTokenType::Exclamation);

    // Build SimpleType
    SimpleType simple_type;
    simple_type.reifiedType = canonical_type;
    simple_type.srcTypeString = type_name;

    // Wrap in optional/pointer if needed
    if (is_optional)
    {
        PointerType ptr_type;
        ptr_type.pointee = std::make_unique<Type>(simple_type);
        return std::make_unique<Type>(std::move(ptr_type));
    }
    else
    {
        return std::make_unique<Type>(simple_type);
    }
}

Field GraphQLParser::parseField()
{
    Field field;
    field.name = current_token.value;
    advance();

    match(GraphQLTokenType::Colon);

    field.type = parseType();

    return field;
}

Struct GraphQLParser::parseTypeDefinition()
{
    match(GraphQLTokenType::Type);

    Struct s;
    s.name = current_token.value;
    advance();

    match(GraphQLTokenType::LBrace);

    while (!match(GraphQLTokenType::RBrace))
    {
        Field field = parseField();
        s.members.emplace_back(std::move(field));
    }

    return s;
}

Enum GraphQLParser::parseEnumDefinition()
{
    match(GraphQLTokenType::Enum);

    Enum enum_type;
    enum_type.name = current_token.value;
    advance();

    match(GraphQLTokenType::LBrace);

    int idx = 0;
    while (!match(GraphQLTokenType::RBrace))
    {
        EnumValue ev;
        ev.name = current_token.value;
        ev.number = idx++;
        advance();

        enum_type.values.emplace_back(std::move(ev));
    }

    return enum_type;
}

auto GraphQLParser::parseToAst(const std::string& src) -> bhw::Ast
{
    bhw::Ast ast;

    source = src;
    pos = 0;  // ⭐ Reset position
    advance();

    while (current_token.type != GraphQLTokenType::EndOfFile)
    {
        if (current_token.type == GraphQLTokenType::Type)
        {
            ast.nodes.emplace_back(parseTypeDefinition());
        }
        else if (current_token.type == GraphQLTokenType::Enum)
        {
            ast.nodes.emplace_back(parseEnumDefinition());  // ⭐ Add to AST
        }
        else
        {
            advance();
        }
    }

    return ast;
}
