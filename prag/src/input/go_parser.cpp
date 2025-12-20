#include "go_parser.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
using namespace bhw;
namespace
{
// Go type string ¡÷ Canonical enums mapping
const std::map<std::string, bhw::ReifiedTypeId> GO_TO_CANONICAL = {
    // Primitives
    {"bool", bhw::ReifiedTypeId::Bool},
    {"byte", bhw::ReifiedTypeId::UInt8},
    {"int8", bhw::ReifiedTypeId::Int8},
    {"uint8", bhw::ReifiedTypeId::UInt8},
    {"int16", bhw::ReifiedTypeId::Int16},
    {"uint16", bhw::ReifiedTypeId::UInt16},
    {"int32", bhw::ReifiedTypeId::Int32},
    {"uint32", bhw::ReifiedTypeId::UInt32},
    {"int64", bhw::ReifiedTypeId::Int64},
    {"uint64", bhw::ReifiedTypeId::UInt64},
    {"int", bhw::ReifiedTypeId::Int64},
    {"uint", bhw::ReifiedTypeId::UInt64},
    {"float32", bhw::ReifiedTypeId::Float32},
    {"float64", bhw::ReifiedTypeId::Float64},
    {"string", bhw::ReifiedTypeId::String},
    {"rune", bhw::ReifiedTypeId::Int32},

    // Time types
    {"time.Time", bhw::ReifiedTypeId::DateTime},
    {"time.Duration", bhw::ReifiedTypeId::Duration},

    // Special types
    {"error", bhw::ReifiedTypeId::String},
    {"interface{}", bhw::ReifiedTypeId::Variant},
    {"any", bhw::ReifiedTypeId::Variant},
};
} // namespace

// ============================================================================
// GoLexer Implementation
// ============================================================================


char GoLexer::current() const
{
    bool condition = pos < source.size();
    if (condition)
    {
        char result = source[pos];
        return result;
    }
    else
    {
        return '\0';
    }
}

char GoLexer::peek(size_t offset) const
{
    return (pos + offset) < source.size() ? source[pos + offset] : '\0';
}

void GoLexer::advance()
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

void GoLexer::skipWhitespace()
{
    while (std::isspace(current()))
        advance();
}

void GoLexer::skipLineComment()
{
    while (current() != '\n' && current() != '\0')
        advance();
    if (current() == '\n')
        advance();
}

void GoLexer::skipBlockComment()
{
    while (true)
    {
        if (current() == '\0')
            throw std::runtime_error("Unterminated block comment");

        if (current() == '*' && peek() == '/')
        {
            advance(); // *
            advance(); // /
            break;
        }
        advance();
    }
}

GoToken GoLexer::makeToken(GoTokenType type, const std::string& value)
{
    return GoToken{type, value, line, column};
}

GoToken GoLexer::readNumber()
{
    std::string value;
    while (std::isdigit(current()) || current() == '.' || current() == 'e' || current() == 'E')
    {
        value += current();
        advance();
    }
    return makeToken(GoTokenType::Number, value);
}

GoToken GoLexer::readIdentifier()
{
    std::string value;
    while (std::isalnum(current()) || current() == '_')
    {
        value += current();
        advance();
    }

    // Check for keywords
    static const std::map<std::string, GoTokenType> keywords = {
        {"package", GoTokenType::Package},
        {"import", GoTokenType::Import},
        {"type", GoTokenType::Type},
        {"struct", GoTokenType::Struct},
        {"interface", GoTokenType::Interface},
        {"const", GoTokenType::Const},
        {"var", GoTokenType::Var},
        {"func", GoTokenType::Func},
        {"map", GoTokenType::Map},
        {"chan", GoTokenType::Chan},
    };

    auto it = keywords.find(value);
    if (it != keywords.end())
        return makeToken(it->second, value);

    return makeToken(GoTokenType::Identifier, value);
}

GoToken GoLexer::readString()
{
    std::string value;
    char quote = current();
    advance(); // skip opening quote

    while (current() != quote && current() != '\0')
    {
        if (current() == '\\')
        {
            advance();
            value += current();
            advance();
        }
        else
        {
            value += current();
            advance();
        }
    }

    if (current() != quote)
        throw std::runtime_error("Unterminated string literal");

    advance(); // skip closing quote
    return makeToken(GoTokenType::String, value);
}

GoToken GoLexer::readRawString()
{
    std::string value;
    advance(); // skip opening `

    while (current() != '`' && current() != '\0')
    {
        value += current();
        advance();
    }

    if (current() != '`')
        throw std::runtime_error("Unterminated raw string literal");

    advance(); // skip closing `
    return makeToken(GoTokenType::String, value);
}

GoToken GoLexer::nextToken()
{
    while (true)
    {
        skipWhitespace();

        if (current() == '/' && peek() == '/')
        {
            advance();
            advance();
            skipLineComment();
            continue;
        }

        if (current() == '/' && peek() == '*')
        {
            advance();
            advance();
            skipBlockComment();
            continue;
        }

        break;
    }

    if (current() == '\0')
        return makeToken(GoTokenType::Eof, "");

    if (std::isdigit(current()))
        return readNumber();

    if (std::isalpha(current()) || current() == '_')
        return readIdentifier();

    if (current() == '"')
        return readString();

    if (current() == '`')
        return readRawString();

    char ch = current();
    size_t start_line = line;
    size_t start_col = column;
    advance();

    switch (ch)
    {
    case '{':
        return GoToken{GoTokenType::LBrace, "{", start_line, start_col};
    case '}':
        return GoToken{GoTokenType::RBrace, "}", start_line, start_col};
    case '(':
        return GoToken{GoTokenType::LParen, "(", start_line, start_col};
    case ')':
        return GoToken{GoTokenType::RParen, ")", start_line, start_col};
    case '[':
        return GoToken{GoTokenType::LBracket, "[", start_line, start_col};
    case ']':
        return GoToken{GoTokenType::RBracket, "]", start_line, start_col};
    case ',':
        return GoToken{GoTokenType::Comma, ",", start_line, start_col};
    case '.':
        return GoToken{GoTokenType::Dot, ".", start_line, start_col};
    case '*':
        return GoToken{GoTokenType::Star, "*", start_line, start_col};
    case '=':
        return GoToken{GoTokenType::Equals, "=", start_line, start_col};
    default:
        return GoToken{GoTokenType::Unknown, std::string(1, ch), start_line, start_col};
    }
}

// ============================================================================
// GoParser Implementation
// ============================================================================

//GoParser::GoParser(const std::string& source) : lexer(source)
//{
//    advance();
//}
void GoParser::advance()
{
    current_token = lexer.nextToken();
}

bool GoParser::match(GoTokenType type) const
{
    return current_token.type == type;
}

bool GoParser::expect(GoTokenType type)
{
    if (!match(type))
        return false;
    advance();
    return true;
}

std::string GoParser::parseQualifiedName()
{
    std::string name = current_token.value;
    expect(GoTokenType::Identifier);

    while (match(GoTokenType::Dot))
    {
        advance();
        name += ".";
        name += current_token.value;
        expect(GoTokenType::Identifier);
    }

    return name;
}

void GoParser::registerUserType(const std::string& type_name)
{
    known_user_types_.insert(type_name);
}

std::unique_ptr<Type> GoParser::resolveSimpleType(const std::string& type_name)
{
    // Try canonical type first
    auto it = GO_TO_CANONICAL.find(type_name);
    if (it != GO_TO_CANONICAL.end())
    {
        SimpleType st;
        st.reifiedType = it->second;
        st.srcTypeString = type_name;
        return std::make_unique<Type>(st);
    }

    // Try user-defined type
    if (known_user_types_.count(type_name))
    {
        StructRefType structRef;
        structRef.srcTypeString = type_name;
        structRef.reifiedType = ReifiedTypeId::StructRefType;
        return std::make_unique<Type>(std::move(structRef));
    }

    // ERROR: Unknown type
    throw std::runtime_error("Unknown type: '" + type_name + "' at line " +
                             std::to_string(current_token.line));
}

std::unique_ptr<Type> GoParser::parseType()
{
    // Handle pointer
    if (match(GoTokenType::Star))
    {
        advance();
        auto pointee = parseType();

        return std::make_unique<Type>(
            PointerType{std::move(pointee), bhw::ReifiedTypeId::PointerType});
    }

    // Handle slice []T
    if (match(GoTokenType::LBracket))
    {
        advance();

        if (match(GoTokenType::RBracket))
        {
            advance();

            if (match(GoTokenType::Identifier) && current_token.value == "byte")
            {
                advance();
                SimpleType st;
                st.srcTypeString = "[]byte";
                st.reifiedType = ReifiedTypeId::Bytes;
                return std::make_unique<Type>(std::move(st));
            }

            auto elem = parseType();
            std::vector<std::unique_ptr<Type>> args;
            args.emplace_back(std::move(elem));
            return std::make_unique<Type>(GenericType{
                std::move(args),
                bhw::ReifiedTypeId::List,
            });
        }
        else if (match(GoTokenType::Number))
        {
            // Array [N]T
            advance();
            expect(GoTokenType::RBracket);
            auto elem = parseType();
            std::vector<std::unique_ptr<Type>> args;
            args.emplace_back(std::move(elem));
            return std::make_unique<Type>(GenericType{std::move(args), bhw::ReifiedTypeId::Array});
        }
        else
        {
            throw std::runtime_error("Expected ] or number after [");
        }
    }

    // Handle map[K]V
    if (match(GoTokenType::Map))
    {
        advance();
        expect(GoTokenType::LBracket);
        auto key = parseType();
        expect(GoTokenType::RBracket);
        auto value = parseType();

        std::vector<std::unique_ptr<Type>> args;
        args.emplace_back(std::move(key));
        args.emplace_back(std::move(value));
        return std::make_unique<Type>(GenericType{std::move(args), bhw::ReifiedTypeId::Map});
    }

    // Handle anonymous inline struct
    if (match(GoTokenType::Struct))
    {
        Struct nested_struct = parseStructBody();
        nested_struct.name = "<anonymous>";
        nested_struct.isAnonymous = true;
        return std::make_unique<Type>(
            StructType{std::make_unique<Struct>(std::move(nested_struct))});
    }

    // Simple type
    std::string type_name = parseQualifiedName();
    return resolveSimpleType(type_name);
}

Field GoParser::parseField()
{
    std::string name = current_token.value;
    expect(GoTokenType::Identifier);

    auto type = parseType();

    // NEW: If the type is an anonymous struct, set its variableName to the field name
    if (type && type->isStruct())
    {
        auto& structType = std::get<StructType>(type->value);
        if (structType.value && structType.value->isAnonymous)
        {
            structType.value->variableName = name;
        }
    }

    // Parse optional struct tags
    std::vector<Attribute> attrs;
    if (match(GoTokenType::String))
    {
        std::string tag = current_token.value;
        advance();
        // Parse struct tags like `json:"name,omitempty"`
        attrs.emplace_back(Attribute{"tag", tag});
    }

    return Field{name, std::move(type), std::move(attrs)};
}



Struct GoParser::parseStructBody()
{
    expect(GoTokenType::Struct);
    expect(GoTokenType::LBrace);

    std::vector<StructMember> members;

    while (!match(GoTokenType::RBrace) && !match(GoTokenType::Eof))
    {
        Field field = parseField();
        members.emplace_back(std::move(field));
    }

    expect(GoTokenType::RBrace);

    // Create and return struct
    Struct s;
    s.members = std::move(members);
    return s;
}

Enum GoParser::parseEnum(const std::string& enumName)
{
    Enum e;
    e.name = enumName;

    expect(GoTokenType::Const);
    expect(GoTokenType::LParen);

    int currentValue = 0;

    while (!match(GoTokenType::RParen) && !match(GoTokenType::Eof))
    {
        if (!match(GoTokenType::Identifier))
        {
            advance();
            continue;
        }

        EnumValue val;
        val.name = current_token.value;
        advance();

        // Check for = value or = iota
        if (match(GoTokenType::Equals))
        {
            advance();
            if (match(GoTokenType::Identifier) && current_token.value == "iota")
            {
                val.number = 0;
                currentValue = 1; // Next value will be 1
                advance();
            }
            else if (match(GoTokenType::Number))
            {
                val.number = std::stoi(current_token.value);
                currentValue = val.number + 1;
                advance();
            }
        }
        else
        {
            val.number = currentValue++;
        }

        e.values.push_back(std::move(val));
    }

    expect(GoTokenType::RParen);
    return e;
}

bhw::Ast GoParser::parseToAst(const std::string& src)
{
    bhw::Ast ast;

    lexer.source = src;
    
    // ========== PASS 1: Register all type names ==========
    while (!match(GoTokenType::Eof))
    {
        if (match(GoTokenType::Type))
        {
            advance();
            if (match(GoTokenType::Identifier))
            {
                registerUserType(current_token.value);
            }
        }
        advance();
    }

    // Reset lexer for pass 2
    lexer = GoLexer();
    lexer.source = src;
    advance();

    // Parse package declaration
    if (match(GoTokenType::Package))
    {
        advance();
        ast.srcName = current_token.value;
        expect(GoTokenType::Identifier);
    }

    // Skip imports
    while (match(GoTokenType::Import))
    {
        advance();
        if (match(GoTokenType::String))
        {
            advance();
        }
        else if (match(GoTokenType::LParen))
        {
            advance();
            while (!match(GoTokenType::RParen) && !match(GoTokenType::Eof))
            {
                if (match(GoTokenType::String))
                    advance();
                else
                    advance();
            }
            expect(GoTokenType::RParen);
        }
    }

    // Parse type declarations
    int loop_count = 0;
    while (!match(GoTokenType::Eof))
    {
        loop_count++;
        if (loop_count > 100)
        {
            std::cerr << "DEBUG: ERROR - Infinite loop detected!\n";
            break;
        }

        if (match(GoTokenType::Type))
        {
            advance();

            if (!match(GoTokenType::Identifier))
            {
                break;
            }

            std::string name = current_token.value;
            expect(GoTokenType::Identifier);

            registerUserType(name);

            if (match(GoTokenType::Struct))
            {
                Struct s = parseStructBody();
                s.name = name;
                ast.nodes.emplace_back(std::move(s));
            }
            else if (match(GoTokenType::Interface))
            {
                // Skip interface declarations - they're generated variant types
                advance(); // consume 'interface'
                if (match(GoTokenType::LBrace))
                {
                    advance();
                    int depth = 1;
                    while (depth > 0 && !match(GoTokenType::Eof))
                    {
                        if (match(GoTokenType::LBrace))
                            depth++;
                        if (match(GoTokenType::RBrace))
                            depth--;
                        advance();
                    }
                }
            }
            else
            {
                // Parse the type (could be int, string, another struct, etc.)
                auto type = parseType();

                // Check if next token is 'const' - this indicates an enum
                if (match(GoTokenType::Const))
                {
                    Enum e = parseEnum(name);
                    ast.nodes.emplace_back(std::move(e));
                }
                // Otherwise it's just a type alias, skip it
            }
        }
        else if (match(GoTokenType::Const))
        {
            advance();
            if (match(GoTokenType::LParen))
            {
                advance();
                int depth = 1;
                while (depth > 0 && !match(GoTokenType::Eof))
                {
                    if (match(GoTokenType::LParen))
                        depth++;
                    if (match(GoTokenType::RParen))
                        depth--;
                    advance();
                }
            }
            else
            {
                while (!match(GoTokenType::Eof) && !match(GoTokenType::Type) &&
                       !match(GoTokenType::Const) && !match(GoTokenType::Var) &&
                       !match(GoTokenType::Func))
                {
                    advance();
                }
            }
        }
        else if (match(GoTokenType::Var) || match(GoTokenType::Func))
        {
            advance();
            int depth = 0;
            while (!match(GoTokenType::Eof))
            {
                if (match(GoTokenType::LBrace))
                    depth++;
                if (match(GoTokenType::RBrace))
                {
                    depth--;
                    advance();
                    if (depth == 0)
                        break;
                    continue;
                }

                if (depth == 0 && (match(GoTokenType::Type) || match(GoTokenType::Const) ||
                                   match(GoTokenType::Var) || match(GoTokenType::Func)))
                {
                    break;
                }
                advance();
            }
        }
        else
        {
            advance();
        }
    }

    return ast;
}

