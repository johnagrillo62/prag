#include "thrift_parser.h"

#include <cctype>
#include <stdexcept>

#include "ast.h"

using namespace bhw;

// Lexer implementation
void ThriftParser::skipWhitespaceAndComments()
{
    while (pos < source.length())
    {
        char c = source[pos];

        if (std::isspace(c))
        {
            pos++;
            continue;
        }

        // Line comments: // or #
        if ((c == '/' && pos + 1 < source.length() && source[pos + 1] == '/') || c == '#')
        {
            while (pos < source.length() && source[pos] != '\n')
            {
                pos++;
            }
            continue;
        }

        // Block comments /* */
        if (c == '/' && pos + 1 < source.length() && source[pos + 1] == '*')
        {
            pos += 2;
            while (pos + 1 < source.length())
            {
                if (source[pos] == '*' && source[pos + 1] == '/')
                {
                    pos += 2;
                    break;
                }
                pos++;
            }
            continue;
        }

        break;
    }
}

ThriftToken ThriftParser::readIdentifier()
{
    size_t start = pos;
    while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_'))
    {
        pos++;
    }
    std::string value = source.substr(start, pos - start);

    // Check keywords
    if (value == "namespace")
        return {ThriftTokenType::Namespace, value};
    if (value == "include")
        return {ThriftTokenType::Include, value};
    if (value == "struct")
        return {ThriftTokenType::Struct, value};
    if (value == "enum")
        return {ThriftTokenType::Enum, value};
    if (value == "service")
        return {ThriftTokenType::Service, value};
    if (value == "exception")
        return {ThriftTokenType::Exception, value};
    if (value == "typedef")
        return {ThriftTokenType::Typedef, value};
    if (value == "const")
        return {ThriftTokenType::Const, value};
    if (value == "required")
        return {ThriftTokenType::Required, value};
    if (value == "optional")
        return {ThriftTokenType::Optional, value};
    if (value == "oneway")
        return {ThriftTokenType::Oneway, value};

    // Type keywords
    if (value == "bool")
        return {ThriftTokenType::Bool, value};
    if (value == "byte")
        return {ThriftTokenType::Byte, value};
    if (value == "i8")
        return {ThriftTokenType::I8, value};
    if (value == "i16")
        return {ThriftTokenType::I16, value};
    if (value == "i32")
        return {ThriftTokenType::I32, value};
    if (value == "i64")
        return {ThriftTokenType::I64, value};
    if (value == "double")
        return {ThriftTokenType::Double, value};
    if (value == "string")
        return {ThriftTokenType::String, value};
    if (value == "binary")
        return {ThriftTokenType::Binary, value};
    if (value == "list")
        return {ThriftTokenType::List, value};
    if (value == "set")
        return {ThriftTokenType::Set, value};
    if (value == "map")
        return {ThriftTokenType::Map, value};

    return {ThriftTokenType::Identifier, value};
}

ThriftToken ThriftParser::readNumber()
{
    size_t start = pos;
    while (pos < source.length() && (std::isdigit(source[pos]) || source[pos] == '.' ||
                                     source[pos] == '-' || source[pos] == '+'))
    {
        pos++;
    }
    return {ThriftTokenType::Number, source.substr(start, pos - start)};
}

ThriftToken ThriftParser::readStringLiteral()
{
    char quote = source[pos];
    pos++; // Skip opening quote
    size_t start = pos;

    while (pos < source.length() && source[pos] != quote)
    {
        if (source[pos] == '\\')
            pos++; // Skip escape
        pos++;
    }

    std::string value = source.substr(start, pos - start);
    if (pos < source.length())
        pos++; // Skip closing quote

    return {ThriftTokenType::StringLiteral, value};
}

ThriftToken ThriftParser::nextToken()
{
    skipWhitespaceAndComments();

    if (pos >= source.length())
    {
        return {ThriftTokenType::EndOfFile, ""};
    }

    char c = source[pos];

    // Single character tokens
    if (c == '{')
    {
        pos++;
        return {ThriftTokenType::LBrace, "{"};
    }
    if (c == '}')
    {
        pos++;
        return {ThriftTokenType::RBrace, "}"};
    }
    if (c == '(')
    {
        pos++;
        return {ThriftTokenType::LParen, "("};
    }
    if (c == ')')
    {
        pos++;
        return {ThriftTokenType::RParen, ")"};
    }
    if (c == '<')
    {
        pos++;
        return {ThriftTokenType::LT, "<"};
    }
    if (c == '>')
    {
        pos++;
        return {ThriftTokenType::GT, ">"};
    }
    if (c == ',')
    {
        pos++;
        return {ThriftTokenType::Comma, ","};
    }
    if (c == ';')
    {
        pos++;
        return {ThriftTokenType::Semicolon, ";"};
    }
    if (c == ':')
    {
        pos++;
        return {ThriftTokenType::Colon, ":"};
    }
    if (c == '=')
    {
        pos++;
        return {ThriftTokenType::Equals, "="};
    }

    // String literals
    if (c == '"' || c == '\'')
    {
        return readStringLiteral();
    }

    // Numbers
    if (std::isdigit(c) || c == '-')
    {
        return readNumber();
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

void ThriftParser::advance()
{
    current_token = nextToken();
}

bool ThriftParser::match(ThriftTokenType type)
{
    if (current_token.type == type)
    {
        advance();
        return true;
    }
    return false;
}

void ThriftParser::expect(ThriftTokenType type)
{
    if (!match(type))
    {
        throw std::runtime_error("Unexpected token in Thrift parser");
    }
}

std::string ThriftParser::parseIdentifier()
{
    if (current_token.type != ThriftTokenType::Identifier)
    {
        throw std::runtime_error("Expected identifier");
    }
    std::string value = current_token.value;
    advance();
    return value;
}

// AstParser implementation
std::unique_ptr<Type> ThriftParser::parseType()
{
    ReifiedTypeId canonical = ReifiedTypeId::Unknown;
    std::string type_name;

    if (match(ThriftTokenType::Bool))
    {
        canonical = ReifiedTypeId::Bool;
        type_name = "bool";
    }
    else if (match(ThriftTokenType::Byte) || match(ThriftTokenType::I8))
    {
        canonical = ReifiedTypeId::Int8;
        type_name = "i8";
    }
    else if (match(ThriftTokenType::I16))
    {
        canonical = ReifiedTypeId::Int16;
        type_name = "i16";
    }
    else if (match(ThriftTokenType::I32))
    {
        canonical = ReifiedTypeId::Int32;
        type_name = "i32";
    }
    else if (match(ThriftTokenType::I64))
    {
        canonical = ReifiedTypeId::Int64;
        type_name = "i64";
    }
    else if (match(ThriftTokenType::Double))
    {
        canonical = ReifiedTypeId::Float64;
        type_name = "double";
    }
    else if (match(ThriftTokenType::String))
    {
        canonical = ReifiedTypeId::String;
        type_name = "string";
    }
    else if (match(ThriftTokenType::Binary))
    {
        canonical = ReifiedTypeId::Bytes;
        type_name = "binary";
    }
    else if (match(ThriftTokenType::List))
    {
        expect(ThriftTokenType::LT);
        auto elem = parseType();
        expect(ThriftTokenType::GT);

        GenericType generic_type;
        generic_type.reifiedType = ReifiedTypeId::List;
        generic_type.args.emplace_back(std::move(elem));

        return std::make_unique<Type>(std::move(generic_type));
    }
    else if (match(ThriftTokenType::Set))
    {
        expect(ThriftTokenType::LT);
        auto elem = parseType();
        expect(ThriftTokenType::GT);

        GenericType generic_type;
        generic_type.reifiedType = ReifiedTypeId::List;
        generic_type.args.emplace_back(std::move(elem));

        return std::make_unique<Type>(std::move(generic_type));
    }
    else if (match(ThriftTokenType::Map))
    {
        expect(ThriftTokenType::LT);
        auto key = parseType();
        expect(ThriftTokenType::Comma);
        auto val = parseType();
        expect(ThriftTokenType::GT);

        GenericType generic_type;
        generic_type.reifiedType = ReifiedTypeId::Map;
        generic_type.args.emplace_back(std::move(key));
        generic_type.args.emplace_back(std::move(val));

        return std::make_unique<Type>(std::move(generic_type));
    }
    else
    {
        type_name = parseIdentifier();
    }

    // Create SimpleType
    SimpleType simple_type;
    simple_type.reifiedType = canonical;
    simple_type.srcTypeString = type_name;

    return std::make_unique<Type>(simple_type);
}

Field ThriftParser::parseField()
{
    Field field;

    // Field ID: 1:, 2:, etc.
    if (current_token.type == ThriftTokenType::Number)
    {
        Attribute attr;
        attr.name = "field_id";
        attr.value = current_token.value;
        field.attributes.emplace_back(attr);
        advance();
        expect(ThriftTokenType::Colon);
    }

    // Required/optional
    bool is_required = false;
    if (match(ThriftTokenType::Required))
    {
        is_required = true;
    }
    else if (match(ThriftTokenType::Optional))
    {
        is_required = false;
    }

    // Type
    field.type = parseType();

    // Make optional if not required
    if (!is_required)
    {
        PointerType ptr_type;
        ptr_type.pointee = std::move(field.type);
        field.type = std::make_unique<Type>(std::move(ptr_type));
    }

    // Field name
    field.name = parseIdentifier();

    // Default value
    if (match(ThriftTokenType::Equals))
    {
        Attribute attr;
        attr.name = "default";
        attr.value = current_token.value;
        field.attributes.emplace_back(attr);
        advance();
    }

    // Optional comma or semicolon
    match(ThriftTokenType::Comma);
    match(ThriftTokenType::Semicolon);

    return field;
}

Struct ThriftParser::parseStruct()
{
    match(ThriftTokenType::Struct);

    Struct entity;
    entity.name = parseIdentifier();

    expect(ThriftTokenType::LBrace);

    while (!match(ThriftTokenType::RBrace))
    {
        Field field = parseField();
        entity.members.emplace_back(std::move(field));
    }

    return entity;
}

Enum ThriftParser::parseEnum()
{
    expect(ThriftTokenType::Enum);

    Enum enum_type;
    enum_type.name = parseIdentifier();

    expect(ThriftTokenType::LBrace);

    int auto_value = 0;
    while (!match(ThriftTokenType::RBrace))
    {
        EnumValue ev;
        ev.name = parseIdentifier();

        if (match(ThriftTokenType::Equals))
        {
            ev.number = std::stoi(current_token.value);
            auto_value = ev.number + 1;
            advance();
        }
        else
        {
            ev.number = auto_value++;
        }

        enum_type.values.emplace_back(std::move(ev));

        match(ThriftTokenType::Comma);
        match(ThriftTokenType::Semicolon);
    }

    return enum_type;
}

Service ThriftParser::parseService()
{
    expect(ThriftTokenType::Service);

    Service service;
    service.name = parseIdentifier();

    expect(ThriftTokenType::LBrace);

    while (!match(ThriftTokenType::RBrace))
    {
        RpcMethod method;

        // Optional oneway
        if (match(ThriftTokenType::Oneway))
        {
            Attribute attr;
            attr.name = "oneway";
            attr.value = "true";
            method.attributes.emplace_back(attr);
        }

        // Return type
        auto return_type = parseType();
        // Store return type name (simplified)
        if (return_type->isSimple())
        {
            auto& simple = std::get<SimpleType>(return_type->value);
            // Convert canonical to string
            method.response_type = simple.srcTypeString;
        }

        // Method name
        method.name = parseIdentifier();

        // Parameters
        expect(ThriftTokenType::LParen);

        // Parse parameters (simplified - store as request_type)
        if (!match(ThriftTokenType::RParen))
        {
            while (true)
            {
                parseField(); // Skip for now
                if (match(ThriftTokenType::RParen))
                    break;
                expect(ThriftTokenType::Comma);
            }
        }

        // Throws clause (optional)
        if (current_token.type == ThriftTokenType::Identifier && current_token.value == "throws")
        {
            advance();
            expect(ThriftTokenType::LParen);
            while (!match(ThriftTokenType::RParen))
            {
                parseField();
                match(ThriftTokenType::Comma);
            }
        }

        match(ThriftTokenType::Comma);
        match(ThriftTokenType::Semicolon);

        service.methods.emplace_back(method);
    }

    return service;
}

auto ThriftParser::parseToAst(const std::string& src) -> bhw::Ast
{
    bhw::Ast ast;
    source = src;
    pos = 0;  // ⭐ Reset position
    advance();

    while (current_token.type != ThriftTokenType::EndOfFile)
    {
        if (current_token.type == ThriftTokenType::Struct ||
            current_token.type == ThriftTokenType::Exception)
        {
            ast.nodes.emplace_back(parseStruct());  // ⭐ Add directly to ast
        }
        else if (current_token.type == ThriftTokenType::Enum)
        {
            ast.nodes.emplace_back(parseEnum());  // ⭐ Add directly to ast
        }
        else if (current_token.type == ThriftTokenType::Service)
        {
            ast.nodes.emplace_back(parseService());  // ⭐ Add directly to ast
        }
        else
        {
            advance();
        }
    }

    return ast;
}

std::vector<Enum> ThriftParser::parseEnums()
{
    std::vector<Enum> enums;

    while (current_token.type != ThriftTokenType::EndOfFile)
    {
        if (current_token.type == ThriftTokenType::Enum)
        {
            enums.emplace_back(parseEnum());
        }
        else
        {
            advance();
        }
    }

    return enums;
}

std::vector<Service> ThriftParser::parseServices()
{
    std::vector<Service> services;

    while (current_token.type != ThriftTokenType::EndOfFile)
    {
        if (current_token.type == ThriftTokenType::Service)
        {
            services.emplace_back(parseService());
        }
        else
        {
            advance();
        }
    }

    return services;
}

