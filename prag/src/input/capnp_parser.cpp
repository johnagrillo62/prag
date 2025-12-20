



// capnp_parser.cpp
#include "capnp_parser.h"

#include <cctype>
#include <map>
#include <stdexcept>

using namespace bhw;

static const std::map<std::string, CapnProtoTokenType> KEYWORDS = {
    {"struct", CapnProtoTokenType::Struct},       {"enum", CapnProtoTokenType::Enum},
    {"interface", CapnProtoTokenType::Interface}, {"annotation", CapnProtoTokenType::Annotation},
    {"using", CapnProtoTokenType::Using},         {"const", CapnProtoTokenType::Const},
    {"import", CapnProtoTokenType::Import},

    {"Bool", CapnProtoTokenType::Bool},           {"Int8", CapnProtoTokenType::Int8},
    {"Int16", CapnProtoTokenType::Int16},         {"Int32", CapnProtoTokenType::Int32},
    {"Int64", CapnProtoTokenType::Int64},         {"UInt8", CapnProtoTokenType::UInt8},
    {"UInt16", CapnProtoTokenType::UInt16},       {"UInt32", CapnProtoTokenType::UInt32},
    {"UInt64", CapnProtoTokenType::UInt64},       {"Float32", CapnProtoTokenType::Float32},
    {"Float64", CapnProtoTokenType::Float64},     {"Text", CapnProtoTokenType::Text},
    {"Data", CapnProtoTokenType::Data},           {"Void", CapnProtoTokenType::Void},
    {"List", CapnProtoTokenType::List},           {"AnyPointer", CapnProtoTokenType::AnyPointer},
};

CapnProtoLexer::CapnProtoLexer(std::string_view source) : source_(source)
{
}

std::vector<CapnProtoToken> CapnProtoLexer::tokenize()
{
    std::vector<CapnProtoToken> tokens;

    while (!isAtEnd())
    {
        skipWhitespace();
        if (isAtEnd())
            break;

        char c = peek();
        int start_line = line_;
        int start_col = column_;

        // Comments
        if (c == '#')
        {
            skipComment();
            continue;
        }

        CapnProtoToken token;
        token.line = start_line;
        token.column = start_col;

        switch (c)
        {
        case '{':
            token.type = CapnProtoTokenType::LBrace;
            token.value = "{";
            advance();
            break;
        case '}':
            token.type = CapnProtoTokenType::RBrace;
            token.value = "}";
            advance();
            break;
        case '(':
            token.type = CapnProtoTokenType::LParen;
            token.value = "(";
            advance();
            break;
        case ')':
            token.type = CapnProtoTokenType::RParen;
            token.value = ")";
            advance();
            break;
        case '[':
            token.type = CapnProtoTokenType::LBracket;
            token.value = "[";
            advance();
            break;
        case ']':
            token.type = CapnProtoTokenType::RBracket;
            token.value = "]";
            advance();
            break;
        case ':':
            token.type = CapnProtoTokenType::Colon;
            token.value = ":";
            advance();
            break;
        case ';':
            token.type = CapnProtoTokenType::Semicolon;
            token.value = ";";
            advance();
            break;
        case ',':
            token.type = CapnProtoTokenType::Comma;
            token.value = ",";
            advance();
            break;
        case '=':
            token.type = CapnProtoTokenType::Equal;
            token.value = "=";
            advance();
            break;
        case '.':
            token.type = CapnProtoTokenType::Dot;
            token.value = ".";
            advance();
            break;
        case '@':
            token.type = CapnProtoTokenType::At;
            token.value = "@";
            advance();
            break;
        case '$':
            token.type = CapnProtoTokenType::Dollar;
            token.value = "$";
            advance();
            break;
        case '-':
            if (peek(1) == '>')
            {
                token.type = CapnProtoTokenType::Arrow;
                token.value = "->";
                advance();
                advance();
                break;
            }
            // Fall through

        default:
            if (std::isalpha(c) || c == '_')
            {
                token = readIdentifierOrKeyword();
            }
            else if (c == '0' && peek(1) == 'x')
            {
                token = readHexNumber();
            }
            else if (std::isdigit(c))
            {
                token = readNumber();
            }
            else if (c == '"')
            {
                token = readString();
            }
            else
            {
                token.type = CapnProtoTokenType::Unknown;
                token.value = std::string(1, c);
                advance();
            }
        }

        tokens.push_back(token);
    }

    tokens.push_back({CapnProtoTokenType::EndOfFile, "", line_, column_});
    return tokens;
}

CapnProtoToken CapnProtoLexer::readIdentifierOrKeyword()
{
    int start_line = line_;
    int start_col = column_;
    std::string value;

    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_'))
    {
        value += advance();
    }

    auto it = KEYWORDS.find(value);
    return {it != KEYWORDS.end() ? it->second : CapnProtoTokenType::Identifier,
            value,
            start_line,
            start_col};
}

CapnProtoToken CapnProtoLexer::readNumber()
{
    int start_line = line_;
    int start_col = column_;
    std::string value;

    while (!isAtEnd() && std::isdigit(peek()))
    {
        value += advance();
    }

    return {CapnProtoTokenType::IntLiteral, value, start_line, start_col};
}

CapnProtoToken CapnProtoLexer::readHexNumber()
{
    int start_line = line_;
    int start_col = column_;
    std::string value;

    advance(); // 0
    advance(); // x
    value = "0x";

    while (!isAtEnd() && std::isxdigit(peek()))
    {
        value += advance();
    }

    return {CapnProtoTokenType::HexLiteral, value, start_line, start_col};
}

CapnProtoToken CapnProtoLexer::readString()
{
    int start_line = line_;
    int start_col = column_;
    std::string value;

    advance(); // Skip opening "
    while (!isAtEnd() && peek() != '"')
    {
        if (peek() == '\\')
        {
            advance();
            if (!isAtEnd())
                value += advance();
        }
        else
        {
            value += advance();
        }
    }
    if (!isAtEnd())
        advance(); // Skip closing "

    return {CapnProtoTokenType::StringLiteral, value, start_line, start_col};
}

void CapnProtoLexer::skipWhitespace()
{
    while (!isAtEnd() && std::isspace(peek()))
    {
        if (peek() == '\n')
        {
            line_++;
            column_ = 1;
        }
        else
        {
            column_++;
        }
        pos_++;
    }
}

void CapnProtoLexer::skipComment()
{
    while (!isAtEnd() && peek() != '\n')
    {
        advance();
    }
}

char CapnProtoLexer::peek(int offset) const
{
    if (pos_ + offset >= source_.size())
        return '\0';
    return source_[pos_ + offset];
}

char CapnProtoLexer::advance()
{
    char c = source_[pos_++];
    column_++;
    return c;
}

bool CapnProtoLexer::isAtEnd() const
{
    return pos_ >= source_.size();
}

CapnProtoParser::CapnProtoParser(const std::string& source)
{
    CapnProtoLexer lexer(source);
    tokens_ = lexer.tokenize();
}

auto CapnProtoParser::parseToAst(const std::string& src) -> bhw::Ast
{
    bhw::Ast ast;

    while (!check(CapnProtoTokenType::EndOfFile))
    {
        if (match(CapnProtoTokenType::Struct))
        {
            ast.nodes.emplace_back(parseStruct());
        }
        else if (match(CapnProtoTokenType::Enum))
        {
            ast.nodes.emplace_back(parseEnum());
        }
        else if (match(CapnProtoTokenType::At))
        {
            // Skip file-level annotations like @0x123...
            advance(); // Skip hex number or identifier
            if (match(CapnProtoTokenType::Semicolon))
            {
            }
        }
        else
        {
            advance(); // Skip unknown
        }
    }
    return ast;
}

Struct CapnProtoParser::parseStruct()
{
    // struct User { id @0 : Int32; name @1 : Text; }
    Struct s;

    expect(CapnProtoTokenType::Identifier, "Expected struct name");
    s.name = tokens_[pos_ - 1].value;

    expect(CapnProtoTokenType::LBrace, "Expected '{'");

    while (!check(CapnProtoTokenType::RBrace) && !check(CapnProtoTokenType::EndOfFile))
    {
        s.members.emplace_back(parseField());
    }

    expect(CapnProtoTokenType::RBrace, "Expected '}'");

    return s;
}

Enum CapnProtoParser::parseEnum()
{
    // enum Status { active @0; inactive @1; }
    Enum e;

    expect(CapnProtoTokenType::Identifier, "Expected enum name");
    e.name = tokens_[pos_ - 1].value;

    expect(CapnProtoTokenType::LBrace, "Expected '{'");

    while (!check(CapnProtoTokenType::RBrace) && !check(CapnProtoTokenType::EndOfFile))
    {
        if (check(CapnProtoTokenType::Identifier))
        {
            EnumValue ev;
            ev.name = advance().value;

            // @N annotation - use as enum value number
            if (match(CapnProtoTokenType::At))
            {
                if (check(CapnProtoTokenType::IntLiteral))
                {
                    ev.number = std::stoi(advance().value);
                }
            }

            e.values.emplace_back(std::move(ev));
        }

        if (match(CapnProtoTokenType::Semicolon))
        {
        }
    }

    expect(CapnProtoTokenType::RBrace, "Expected '}'");

    return e;
}

Field CapnProtoParser::parseField()
{
    // name @N : Type;
    // tags @2 : List(Text);

    Field f;

    expect(CapnProtoTokenType::Identifier, "Expected field name");
    f.name = tokens_[pos_ - 1].value;

    // @N annotation - CAPTURE IT AS ATTRIBUTE!
    expect(CapnProtoTokenType::At, "Expected '@'");
    if (check(CapnProtoTokenType::IntLiteral))
    {
        Attribute attr;
        attr.name = "field_number";
        attr.value = advance().value;
        f.attributes.emplace_back(attr);
    }

    expect(CapnProtoTokenType::Colon, "Expected ':'");

    f.type = parseType();

    if (match(CapnProtoTokenType::Semicolon))
    {
    }

    return f;
}

std::unique_ptr<Type> CapnProtoParser::parseType()
{
    // List(Type) or Type

    const auto& token = advance();

    // Check for List(...)
    if (token.value == "List")
    {
        expect(CapnProtoTokenType::LParen, "Expected '('");
        auto element_type = parseType();
        expect(CapnProtoTokenType::RParen, "Expected ')'");

        GenericType gt;
        gt.reifiedType = bhw::ReifiedTypeId::List;
        gt.args.emplace_back(std::move(element_type));

        return std::make_unique<Type>(std::move(gt));
    }

    // Map canonical types
    bhw::ReifiedTypeId canonical = mapCapnProtoType(token.value);

    if (canonical != bhw::ReifiedTypeId::StructRefType)
    {
        SimpleType st;
        st.reifiedType = canonical;
        return std::make_unique<Type>(std::move(st));
    }

    // User type - use StructRefType!
    StructRefType srt;
    srt.srcTypeString = token.value;
    srt.reifiedType = bhw::ReifiedTypeId::StructRefType;
    return std::make_unique<Type>(std::move(srt));
}

bhw::ReifiedTypeId CapnProtoParser::mapCapnProtoType(const std::string& type_name)
{
    if (type_name == "Bool")
        return bhw::ReifiedTypeId::Bool;
    if (type_name == "Int8")
        return bhw::ReifiedTypeId::Int8;
    if (type_name == "Int16")
        return bhw::ReifiedTypeId::Int16;
    if (type_name == "Int32")
        return bhw::ReifiedTypeId::Int32;
    if (type_name == "Int64")
        return bhw::ReifiedTypeId::Int64;
    if (type_name == "UInt8")
        return bhw::ReifiedTypeId::UInt8;
    if (type_name == "UInt16")
        return bhw::ReifiedTypeId::UInt16;
    if (type_name == "UInt32")
        return bhw::ReifiedTypeId::UInt32;
    if (type_name == "UInt64")
        return bhw::ReifiedTypeId::UInt64;
    if (type_name == "Float32")
        return bhw::ReifiedTypeId::Float32;
    if (type_name == "Float64")
        return bhw::ReifiedTypeId::Float64;
    if (type_name == "Text")
        return bhw::ReifiedTypeId::String;
    if (type_name == "Data")
        return bhw::ReifiedTypeId::Bytes;

    return bhw::ReifiedTypeId::StructRefType;
}

const CapnProtoToken& CapnProtoParser::peek(int offset) const
{
    size_t index = pos_ + offset;
    if (index >= tokens_.size())
        return tokens_.back();
    return tokens_[index];
}

const CapnProtoToken& CapnProtoParser::advance()
{
    if (pos_ < tokens_.size())
        pos_++;
    return tokens_[pos_ - 1];
}

bool CapnProtoParser::match(CapnProtoTokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}

bool CapnProtoParser::check(CapnProtoTokenType type) const
{
    return peek().type == type;
}

void CapnProtoParser::expect(CapnProtoTokenType type, const std::string& message)
{
    if (!match(type))
    {
        throw std::runtime_error(message + " at line " + std::to_string(peek().line));
    }
}
