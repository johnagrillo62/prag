// protobuf_parser.cpp
#include "protobuf_parser.h"

#include <cctype>
#include <iostream>
#include <stdexcept>

using namespace bhw;
// ============================================================================
// ProtoLexer Implementation
// ============================================================================

//ProtoLexer::ProtoLexer(const std::string& src) : source(src)
//{
//}

char ProtoLexer::current() const
{
    return pos < source.size() ? source[pos] : '\0';
}

char ProtoLexer::peek(size_t offset) const
{
    return (pos + offset) < source.size() ? source[pos + offset] : '\0';
}

void ProtoLexer::advance()
{
    if (current() == '\n')
    {
        line++;
        column = 1;
    }
    else
    {
        column++;
    }
    pos++;
}

void ProtoLexer::skipWhitespace()
{
    while (std::isspace(current()))
        advance();
}

void ProtoLexer::skipComment()
{
    if (current() == '/' && peek() == '/')
    {
        // Line comment
        while (current() != '\n' && current() != '\0')
            advance();
    }
    else if (current() == '/' && peek() == '*')
    {
        // Block comment
        advance(); // skip /
        advance(); // skip *
        while (!(current() == '*' && peek() == '/') && current() != '\0')
            advance();
        if (current() == '*')
        {
            advance(); // skip *
            advance(); // skip /
        }
    }
}

ProtoToken ProtoLexer::makeToken(ProtoTokenType type, const std::string& value)
{
    return ProtoToken{type, value, line, column};
}

ProtoToken ProtoLexer::readNumber()
{
    std::string value;

    if (current() == '-')
    {
        value += current();
        advance();
    }

    while (std::isdigit(current()))
    {
        value += current();
        advance();
    }
    return makeToken(ProtoTokenType::Number, value);
}

ProtoToken ProtoLexer::readString()
{
    std::string value;
    char quote = current();
    advance(); // skip opening quote

    while (current() != quote && current() != '\0')
    {
        if (current() == '\\')
        {
            advance();
            if (current() != '\0')
            {
                value += current();
                advance();
            }
        }
        else
        {
            value += current();
            advance();
        }
    }

    if (current() == quote)
        advance(); // skip closing quote

    return makeToken(ProtoTokenType::StringLiteral, value);
}

ProtoToken ProtoLexer::readIdentifier()
{
    std::string value;
    while (std::isalnum(current()) || current() == '_')
    {
        value += current();
        advance();
    }

    // Check for keywords
    static const std::map<std::string, ProtoTokenType> keywords = {
        {"syntax", ProtoTokenType::Syntax},     {"package", ProtoTokenType::Package},
        {"import", ProtoTokenType::Import},     {"message", ProtoTokenType::Message},
        {"enum", ProtoTokenType::Enum},         {"service", ProtoTokenType::Service},
        {"rpc", ProtoTokenType::Rpc},           {"returns", ProtoTokenType::Returns},
        {"repeated", ProtoTokenType::Repeated}, {"optional", ProtoTokenType::Optional},
        {"required", ProtoTokenType::Required}, {"map", ProtoTokenType::Map},
        {"oneof", ProtoTokenType::Oneof},       {"stream", ProtoTokenType::Stream},
        {"double", ProtoTokenType::Double},     {"float", ProtoTokenType::Float},
        {"int32", ProtoTokenType::Int32},       {"int64", ProtoTokenType::Int64},
        {"uint32", ProtoTokenType::Uint32},     {"uint64", ProtoTokenType::Uint64},
        {"sint32", ProtoTokenType::Sint32},     {"sint64", ProtoTokenType::Sint64},
        {"fixed32", ProtoTokenType::Fixed32},   {"fixed64", ProtoTokenType::Fixed64},
        {"sfixed32", ProtoTokenType::Sfixed32}, {"sfixed64", ProtoTokenType::Sfixed64},
        {"bool", ProtoTokenType::Bool},         {"string", ProtoTokenType::String},
        {"bytes", ProtoTokenType::Bytes}};

    auto it = keywords.find(value);
    if (it != keywords.end())
        return makeToken(it->second, value);

    return makeToken(ProtoTokenType::Identifier, value);
}

ProtoToken ProtoLexer::nextToken()
{
    while (true)
    {
        skipWhitespace();

        if (current() == '/' && (peek() == '/' || peek() == '*'))
        {
            skipComment();
            continue;
        }
        break;
    }

    if (current() == '\0')
        return makeToken(ProtoTokenType::Eof, "");

    if (std::isdigit(current()) || (current() == '-' && std::isdigit(peek())))
        return readNumber();

    if (current() == '"' || current() == '\'')
        return readString();

    if (std::isalpha(current()) || current() == '_')
        return readIdentifier();

    char ch = current();
    advance();

    switch (ch)
    {
    case '{':
        return makeToken(ProtoTokenType::LBrace, "{");
    case '}':
        return makeToken(ProtoTokenType::RBrace, "}");
    case '(':
        return makeToken(ProtoTokenType::LParen, "(");
    case ')':
        return makeToken(ProtoTokenType::RParen, ")");
    case '<':
        return makeToken(ProtoTokenType::LAngle, "<");
    case '>':
        return makeToken(ProtoTokenType::RAngle, ">");
    case ';':
        return makeToken(ProtoTokenType::Semicolon, ";");
    case '=':
        return makeToken(ProtoTokenType::Equals, "=");
    case ',':
        return makeToken(ProtoTokenType::Comma, ",");
    case '.':
        return makeToken(ProtoTokenType::Dot, ".");
    default:
        return makeToken(ProtoTokenType::Unknown, std::string(1, ch));
    }
}

// ============================================================================
// ProtoBufParser Implementation
// ============================================================================

//ProtoBufParser::ProtoBufParser(const std::string& source) : lexer(source)
//{
//    advance();
//}

bhw::Ast ProtoBufParser::parseToAst(const std::string& src)
{
    bhw::Ast ast;

    lexer.source = src;
    advance();

    
    while (!match(ProtoTokenType::Eof))
    {
        if (match(ProtoTokenType::Syntax))
        {
            advance();
            expect(ProtoTokenType::Equals);
            expect(ProtoTokenType::StringLiteral);
            expect(ProtoTokenType::Semicolon);
        }
        else if (match(ProtoTokenType::Package))
        {
            advance();
            current_package.clear();

            if (match(ProtoTokenType::Identifier))
            {
                current_package.emplace_back(current_token.value);
                advance();

                while (match(ProtoTokenType::Dot))
                {
                    advance();
                    if (match(ProtoTokenType::Identifier))
                    {
                        current_package.emplace_back(current_token.value);
                        advance();
                    }
                }
            }
            expect(ProtoTokenType::Semicolon);
        }
        else if (match(ProtoTokenType::Import))
        {
            advance();
            expect(ProtoTokenType::StringLiteral);
            expect(ProtoTokenType::Semicolon);
        }
        else if (match(ProtoTokenType::Message))
        {
            ast.nodes.emplace_back(parseMessage());
        }
        else if (match(ProtoTokenType::Enum))
        {
            ast.nodes.emplace_back(parseEnum());
        }
        else if (match(ProtoTokenType::Service))
        {
            ast.nodes.emplace_back(parseService());
        }
        else
        {
            advance();
        }
    }

    return ast;
}

void ProtoBufParser::advance()
{
    current_token = lexer.nextToken();
}

bool ProtoBufParser::match(ProtoTokenType type) const
{
    return current_token.type == type;
}

bool ProtoBufParser::expect(ProtoTokenType type)
{
    if (!match(type))
        return false;
    advance();
    return true;
}

std::unique_ptr<Type> ProtoBufParser::parseScalarType(ProtoTokenType type)
{
    ReifiedTypeId canonical;
    switch (type)
    {
    case ProtoTokenType::Double:
        canonical = ReifiedTypeId::Float64;
        break;
    case ProtoTokenType::Float:
        canonical = ReifiedTypeId::Float32;
        break;
    case ProtoTokenType::Int32:
        canonical = ReifiedTypeId::Int32;
        break;
    case ProtoTokenType::Int64:
        canonical = ReifiedTypeId::Int64;
        break;
    case ProtoTokenType::Uint32:
        canonical = ReifiedTypeId::UInt32;
        break;
    case ProtoTokenType::Uint64:
        canonical = ReifiedTypeId::UInt64;
        break;
    case ProtoTokenType::Sint32:
        canonical = ReifiedTypeId::Int32;
        break;
    case ProtoTokenType::Sint64:
        canonical = ReifiedTypeId::Int64;
        break;
    case ProtoTokenType::Fixed32:
        canonical = ReifiedTypeId::UInt32;
        break;
    case ProtoTokenType::Fixed64:
        canonical = ReifiedTypeId::UInt64;
        break;
    case ProtoTokenType::Sfixed32:
        canonical = ReifiedTypeId::Int32;
        break;
    case ProtoTokenType::Sfixed64:
        canonical = ReifiedTypeId::Int64;
        break;
    case ProtoTokenType::Bool:
        canonical = ReifiedTypeId::Bool;
        break;
    case ProtoTokenType::String:
        canonical = ReifiedTypeId::String;
        break;
    case ProtoTokenType::Bytes:
        canonical = ReifiedTypeId::Bytes;
        break;
    default:
        canonical = ReifiedTypeId::Int32;
        break;
    }

    SimpleType simple;
    simple.reifiedType = canonical;
    return std::make_unique<Type>(simple);
}

std::unique_ptr<Type> ProtoBufParser::parseType()
{
    // Handle repeated
    if (match(ProtoTokenType::Repeated))
    {
        advance();
        auto element_type = parseType();

        GenericType generic;
        generic.reifiedType = ReifiedTypeId::List;
        generic.args.emplace_back(std::move(element_type));
        return std::make_unique<Type>(std::move(generic));
    }

    // Handle optional
    if (match(ProtoTokenType::Optional))
    {
        advance();
        auto inner_type = parseType();

        GenericType generic;
        generic.reifiedType = ReifiedTypeId::Optional;
        generic.args.emplace_back(std::move(inner_type));
        return std::make_unique<Type>(std::move(generic));
    }

    // Handle map<K, V>
    if (match(ProtoTokenType::Map))
    {
        advance();
        expect(ProtoTokenType::LAngle);

        auto key_type = parseType();
        expect(ProtoTokenType::Comma);
        auto value_type = parseType();

        expect(ProtoTokenType::RAngle);

        GenericType generic;
        generic.reifiedType = ReifiedTypeId::Map;
        generic.args.emplace_back(std::move(key_type));
        generic.args.emplace_back(std::move(value_type));
        return std::make_unique<Type>(std::move(generic));
    }

    // Handle scalar types
    if (current_token.type >= ProtoTokenType::Double && current_token.type <= ProtoTokenType::Bytes)
    {
        auto type = parseScalarType(current_token.type);
        advance();
        return type;
    }

    // Handle user-defined types (message names)
    if (match(ProtoTokenType::Identifier))
    {
        std::string type_name = current_token.value;
        std::string last_component = current_token.value;

        advance();

        // Handle nested type names (e.g., Outer.Inner)
        while (match(ProtoTokenType::Dot))
        {
            advance();
            if (match(ProtoTokenType::Identifier))
            {
                type_name += "." + current_token.value;
                last_component = current_token.value;
                advance();
            }
        }

        StructRefType structRef;
        structRef.srcTypeString = last_component;
        return std::make_unique<Type>(structRef);
    }

    return nullptr;
}

Field ProtoBufParser::parseField()
{
    Field field;

    field.type = parseType();

    // If parseType failed (returned nullptr), this is not a valid field
    if (!field.type)
    {
        // Return an empty field - caller should check for this
        return field;
    }

    // Field name can be an identifier OR a keyword used as identifier
    // (e.g., "string message = 2;" where 'message' is the field name)
    if (match(ProtoTokenType::Identifier))
    {
        field.name = current_token.value;
        advance();
    }
    else if (current_token.type >= ProtoTokenType::Syntax &&
             current_token.type <= ProtoTokenType::Bytes)
    {
        // Allow keywords to be used as field names
        field.name = current_token.value;
        advance();
    }

    if (expect(ProtoTokenType::Equals))
    {
        if (match(ProtoTokenType::Number))
        {
            Attribute field_num_attr;
            field_num_attr.name = "field_number";
            field_num_attr.value = current_token.value;
            field.attributes.emplace_back(field_num_attr);
            advance();
        }
    }

    expect(ProtoTokenType::Semicolon);

    return field;
}

Enum ProtoBufParser::parseEnum()
{
    Enum result;

    expect(ProtoTokenType::Enum);

    if (match(ProtoTokenType::Identifier))
    {
        result.name = current_token.value;
        advance();
    }

    result.namespaces = current_package;

    expect(ProtoTokenType::LBrace);

    while (!match(ProtoTokenType::RBrace) && !match(ProtoTokenType::Eof))
    {
        if (match(ProtoTokenType::Identifier))
        {
            EnumValue value;
            value.name = current_token.value;
            advance();

            if (expect(ProtoTokenType::Equals))
            {
                if (match(ProtoTokenType::Number))
                {
                    value.number = std::stoi(current_token.value);
                    advance();
                }
            }

            expect(ProtoTokenType::Semicolon);
            result.values.emplace_back(std::move(value));
        }
        else
        {
            advance();
        }
    }

    expect(ProtoTokenType::RBrace);

    return result;
}

// Parses a `oneof` and returns a Oneof AST node
Oneof ProtoBufParser::parseOneof()
{
    Oneof oneofNode;
    expect(ProtoTokenType::Oneof);

    // Capture the oneof name
    if (match(ProtoTokenType::Identifier))
    {
        oneofNode.name = current_token.value;
        advance();
    }

    expect(ProtoTokenType::LBrace);

    // Parse each field in the oneof
    while (!match(ProtoTokenType::RBrace) && !match(ProtoTokenType::Eof))
    {
        Field field;
        field.type = parseType();
        if (!field.type)
        {
            advance();
            continue;
        }

        if (match(ProtoTokenType::Identifier))
        {
            field.name = current_token.value;
            advance();
        }

        if (expect(ProtoTokenType::Equals))
        {
            if (match(ProtoTokenType::Number))
            {
                Attribute field_num_attr;
                field_num_attr.name = "field_number";
                field_num_attr.value = current_token.value;
                field.attributes.emplace_back(std::move(field_num_attr));
                advance();
            }
        }

        expect(ProtoTokenType::Semicolon);

        // Convert Field -> OneofField
        OneofField of;
        of.name = std::move(field.name);
        of.type = std::move(field.type);
        of.attributes = std::move(field.attributes);

        oneofNode.fields.emplace_back(std::move(of));
    }

    expect(ProtoTokenType::RBrace);
    return oneofNode;
}

Service ProtoBufParser::parseService()
{
    Service result;

    expect(ProtoTokenType::Service);

    if (match(ProtoTokenType::Identifier))
    {
        result.name = current_token.value;
        advance();
    }

    result.namespaces = current_package;

    expect(ProtoTokenType::LBrace);

    while (!match(ProtoTokenType::RBrace) && !match(ProtoTokenType::Eof))
    {
        if (match(ProtoTokenType::Rpc))
        {
            advance();

            RpcMethod method;

            if (match(ProtoTokenType::Identifier))
            {
                method.name = current_token.value;
                advance();
            }

            expect(ProtoTokenType::LParen);

            // Check for stream keyword (client streaming)
            if (match(ProtoTokenType::Stream))
            {
                method.client_streaming = true;
                advance();
            }

            if (match(ProtoTokenType::Identifier))
            {
                method.request_type = current_token.value;
                advance();
            }

            expect(ProtoTokenType::RParen);
            expect(ProtoTokenType::Returns);
            expect(ProtoTokenType::LParen);

            // Check for stream keyword (server streaming)
            if (match(ProtoTokenType::Stream))
            {
                method.server_streaming = true;
                advance();
            }

            if (match(ProtoTokenType::Identifier))
            {
                method.response_type = current_token.value;
                advance();
            }

            expect(ProtoTokenType::RParen);

            // Optional method body with options
            if (match(ProtoTokenType::LBrace))
            {
                // Skip method body
                int depth = 1;
                advance(); // Skip opening brace
                while (depth > 0 && !match(ProtoTokenType::Eof))
                {
                    if (match(ProtoTokenType::LBrace))
                        depth++;
                    else if (match(ProtoTokenType::RBrace))
                        depth--;

                    if (depth > 0) // Don't skip past final }
                        advance();
                }
                if (match(ProtoTokenType::RBrace))
                    advance(); // Skip closing brace
            }
            else
            {
                expect(ProtoTokenType::Semicolon);
            }

            result.methods.emplace_back(std::move(method));
        }
        else
        {
            // Skip unknown tokens (like comments that made it through)
            advance();
        }
    }

    expect(ProtoTokenType::RBrace);

    return result;
}

Struct ProtoBufParser::parseMessage()
{
    Struct result;

    expect(ProtoTokenType::Message);

    if (match(ProtoTokenType::Identifier))
    {
        result.name = current_token.value;
        advance();
    }

    result.namespaces = current_package;

    expect(ProtoTokenType::LBrace);

    while (!match(ProtoTokenType::RBrace) && !match(ProtoTokenType::Eof))
    {
        if (match(ProtoTokenType::Message))
        {
            result.members.emplace_back(parseMessage());
        }
        else if (match(ProtoTokenType::Enum))
        {
            result.members.emplace_back(parseEnum());
        }
        else if (match(ProtoTokenType::Oneof))
        {
            result.members.emplace_back(parseOneof());
        }
        else if (match(ProtoTokenType::Service) || match(ProtoTokenType::Syntax) ||
                 match(ProtoTokenType::Package) || match(ProtoTokenType::Import))
        {
            // Hit a top-level keyword - we're done with this message
            break;
        }

        else
        {
            Field field = parseField();
            if (!field.type)
            {
                break;
            }
            result.members.emplace_back(std::move(field));
        }
    }

    expect(ProtoTokenType::RBrace);

    return result;
}
