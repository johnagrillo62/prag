// capnp_parser.h
#pragma once

#include <cctype>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "ast.h"
#include "ast_parser.h"

namespace bhw
{
enum class CapnProtoTokenType
{
    // Keywords
    Struct,
    Enum,
    Interface,
    Annotation,
    Using,
    Const,
    Import,

    // Types
    Bool,
    Int8,
    Int16,
    Int32,
    Int64,
    UInt8,
    UInt16,
    UInt32,
    UInt64,
    Float32,
    Float64,
    Text,
    Data,
    Void,
    List,
    AnyPointer,

    // Symbols
    LBrace,
    RBrace,
    LParen,
    RParen,
    LBracket,
    RBracket,
    Colon,
    Semicolon,
    Comma,
    At,
    Equals,
    Arrow,
    Dollar,

    // Literals and identifiers
    Identifier,
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    HexLiteral,

    // Special
    EndOfFile,
    Unknown
};

struct CapnProtoToken
{
    CapnProtoTokenType type;
    std::string value;
    int line;
    int column;
};

class CapnProtoLexer
{
  public:
    CapnProtoLexer(std::string_view source);
    std::vector<CapnProtoToken> tokenize();

  private:
    bool isAtEnd() const;
    char peek() const;
    char peekNext() const;
    char advance();
    void skipWhitespace();
    void skipComment();
    CapnProtoToken number();
    CapnProtoToken string();
    CapnProtoToken identifier();
    CapnProtoToken hexLiteral();

    std::string_view source_;
    size_t current_ = 0;
    int line_ = 1;
    int column_ = 1;
};

class CapnProtoParser : public AstParser, public AutoRegisterParser<CapnProtoParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"capnp"};
    }

  public:
    auto parseToAst(const std::string& src) -> bhw::Ast override;
    auto getLang() -> bhw::Language override
    {
        return Language::Capnp;
    }

  private:
    bool isEnumValueName() const;
    CapnProtoToken advance();
    bool match(CapnProtoTokenType type);
    bool check(CapnProtoTokenType type) const;
    void expect(CapnProtoTokenType type, const std::string& message);

    Struct parseStruct();
    Enum parseEnum();
    Field parseField();
    std::unique_ptr<Type> parseType();

    ReifiedTypeId mapCapnProtoType(const std::string& type_name);

    std::vector<CapnProtoToken> tokens_;
    size_t pos_ = 0;
};

// ============================================================================
// IMPLEMENTATION
// ============================================================================

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

inline CapnProtoLexer::CapnProtoLexer(std::string_view source) : source_(source)
{
}

inline std::vector<CapnProtoToken> CapnProtoLexer::tokenize()
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
        case '@':
            token.type = CapnProtoTokenType::At;
            token.value = "@";
            advance();
            break;
        case '=':
            token.type = CapnProtoTokenType::Equals;
            token.value = "=";
            advance();
            break;
        case '$':
            token.type = CapnProtoTokenType::Dollar;
            token.value = "$";
            advance();
            break;
        case '-':
            if (peekNext() == '>')
            {
                token.type = CapnProtoTokenType::Arrow;
                token.value = "->";
                advance();
                advance();
            }
            else
            {
                token = number();
            }
            break;
        case '"':
            token = string();
            break;
        default:
            if (std::isdigit(c))
            {
                token = number();
            }
            else if (std::isalpha(c) || c == '_')
            {
                token = identifier();
            }
            else if (c == '0' && peekNext() == 'x')
            {
                token = hexLiteral();
            }
            else
            {
                token.type = CapnProtoTokenType::Unknown;
                token.value = std::string(1, c);
                advance();
            }
            break;
        }

        tokens.push_back(token);
    }

    CapnProtoToken eof;
    eof.type = CapnProtoTokenType::EndOfFile;
    eof.line = line_;
    eof.column = column_;
    tokens.push_back(eof);

    return tokens;
}

inline bool CapnProtoLexer::isAtEnd() const
{
    return current_ >= source_.size();
}

inline char CapnProtoLexer::peek() const
{
    if (isAtEnd())
        return '\0';
    return source_[current_];
}

inline char CapnProtoLexer::peekNext() const
{
    if (current_ + 1 >= source_.size())
        return '\0';
    return source_[current_ + 1];
}

inline char CapnProtoLexer::advance()
{
    char c = source_[current_++];
    if (c == '\n')
    {
        line_++;
        column_ = 1;
    }
    else
    {
        column_++;
    }
    return c;
}

inline void CapnProtoLexer::skipWhitespace()
{
    while (!isAtEnd() && std::isspace(peek()))
    {
        advance();
    }
}

inline void CapnProtoLexer::skipComment()
{
    // Skip until end of line
    while (!isAtEnd() && peek() != '\n')
    {
        advance();
    }
}

inline CapnProtoToken CapnProtoLexer::number()
{
    CapnProtoToken token;
    std::string value;

    if (peek() == '-')
    {
        value += advance();
    }

    while (!isAtEnd() && (std::isdigit(peek()) || peek() == '.'))
    {
        value += advance();
    }

    token.value = value;
    token.type = (value.find('.') != std::string::npos) ? CapnProtoTokenType::FloatLiteral
                                                        : CapnProtoTokenType::IntLiteral;

    return token;
}

inline CapnProtoToken CapnProtoLexer::string()
{
    CapnProtoToken token;
    std::string value;

    advance(); // skip opening "

    while (!isAtEnd() && peek() != '"')
    {
        if (peek() == '\\')
        {
            advance();
            if (!isAtEnd())
            {
                char escaped = advance();
                switch (escaped)
                {
                case 'n':
                    value += '\n';
                    break;
                case 't':
                    value += '\t';
                    break;
                case 'r':
                    value += '\r';
                    break;
                case '"':
                    value += '"';
                    break;
                case '\\':
                    value += '\\';
                    break;
                default:
                    value += escaped;
                    break;
                }
            }
        }
        else
        {
            value += advance();
        }
    }

    if (!isAtEnd())
        advance(); // skip closing "

    token.type = CapnProtoTokenType::StringLiteral;
    token.value = value;
    return token;
}

inline CapnProtoToken CapnProtoLexer::identifier()
{
    CapnProtoToken token;
    std::string value;

    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_'))
    {
        value += advance();
    }

    token.value = value;

    auto it = KEYWORDS.find(value);
    if (it != KEYWORDS.end())
    {
        token.type = it->second;
    }
    else
    {
        token.type = CapnProtoTokenType::Identifier;
    }

    return token;
}

inline CapnProtoToken CapnProtoLexer::hexLiteral()
{
    CapnProtoToken token;
    std::string value;

    value += advance(); // '0'
    value += advance(); // 'x'

    while (!isAtEnd() && std::isxdigit(peek()))
    {
        value += advance();
    }

    token.type = CapnProtoTokenType::HexLiteral;
    token.value = value;
    return token;
}

// inline CapnProtoParser::CapnProtoParser(const std::string& source)
//{
//     CapnProtoLexer lexer(source);
//     tokens_ = lexer.tokenize();
//     pos_ = 0;
// }

inline auto CapnProtoParser::parseToAst(const std::string& src) -> bhw::Ast
{
    CapnProtoLexer lexer(src);
    tokens_ = lexer.tokenize();
    pos_ = 0;

    Ast ast;

    while (!check(CapnProtoTokenType::EndOfFile))
    {
        // Skip hex literals (file IDs)
        if (check(CapnProtoTokenType::At) || check(CapnProtoTokenType::HexLiteral))
        {
            advance();
            if (check(CapnProtoTokenType::HexLiteral))
            {
                advance();
            }
            if (match(CapnProtoTokenType::Semicolon))
            {
            }
            continue;
        }

        if (match(CapnProtoTokenType::Struct))
        {
            ast.nodes.emplace_back(parseStruct());
        }
        else if (match(CapnProtoTokenType::Enum))
        {
            ast.nodes.emplace_back(parseEnum());
        }
        else
        {
            advance();
        }
    }

    return ast;
}
inline Enum CapnProtoParser::parseEnum()
{
    // enum Status { active @0; inactive @1; }
    Enum e;

    expect(CapnProtoTokenType::Identifier, "Expected enum name");
    e.name = tokens_[pos_ - 1].value;

    expect(CapnProtoTokenType::LBrace, "Expected '{'");

    while (!check(CapnProtoTokenType::RBrace) && !check(CapnProtoTokenType::EndOfFile))
    {
        if (isEnumValueName()) // <-- CHANGED: use helper instead of check(Identifier)
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

            // Consume optional semicolon
            if (match(CapnProtoTokenType::Semicolon))
            {
            }
        }
        else
        {
            advance(); // Skip unexpected tokens
        }
    }

    expect(CapnProtoTokenType::RBrace, "Expected '}'");

    return e;
}
inline CapnProtoToken CapnProtoParser::advance()
{
    if (pos_ < tokens_.size())
    {
        return tokens_[pos_++];
    }
    return tokens_.back();
}

inline bool CapnProtoParser::match(CapnProtoTokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}
inline bool CapnProtoParser::isEnumValueName() const
{
    if (pos_ >= tokens_.size())
        return false;

    // Enum value names can be identifiers OR type keywords
    auto type = tokens_[pos_].type;

    return type == CapnProtoTokenType::Identifier || type == CapnProtoTokenType::Bool ||
           type == CapnProtoTokenType::Int8 || type == CapnProtoTokenType::Int16 ||
           type == CapnProtoTokenType::Int32 || type == CapnProtoTokenType::Int64 ||
           type == CapnProtoTokenType::UInt8 || type == CapnProtoTokenType::UInt16 ||
           type == CapnProtoTokenType::UInt32 || type == CapnProtoTokenType::UInt64 ||
           type == CapnProtoTokenType::Float32 || type == CapnProtoTokenType::Float64 ||
           type == CapnProtoTokenType::Text || type == CapnProtoTokenType::List ||

           type == CapnProtoTokenType::Data;
}

inline bool CapnProtoParser::check(CapnProtoTokenType type) const
{
    if (pos_ >= tokens_.size())
        return false;
    return tokens_[pos_].type == type;
}

inline void CapnProtoParser::expect(CapnProtoTokenType type, const std::string& message)
{
    if (!match(type))
    {
        throw std::runtime_error(message);
    }
}

inline Struct CapnProtoParser::parseStruct()
{
    // struct Event { id @0 :Int64; }
    Struct s;

    expect(CapnProtoTokenType::Identifier, "Expected struct name");
    s.name = tokens_[pos_ - 1].value;

    expect(CapnProtoTokenType::LBrace, "Expected '{'");

    while (!check(CapnProtoTokenType::RBrace) && !check(CapnProtoTokenType::EndOfFile))
    {
        if (check(CapnProtoTokenType::Identifier))
        {
            s.members.emplace_back(parseField());
        }
        else
        {
            advance();
        }
    }

    expect(CapnProtoTokenType::RBrace, "Expected '}'");

    return s;
}

inline Field CapnProtoParser::parseField()
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

inline std::unique_ptr<Type> CapnProtoParser::parseType()
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

inline bhw::ReifiedTypeId CapnProtoParser::mapCapnProtoType(const std::string& type_name)
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

} // namespace bhw
