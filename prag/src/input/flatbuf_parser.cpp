// flatbuf_parser.cpp
#include "flatbuf_parser.h"

#include <cctype>
#include <map>
#include <stdexcept>

namespace bhw
{
static const std::map<std::string, FlatBufTokenType> FlatKEYWORDS = {
    {"namespace", FlatBufTokenType::Namespace},
    {"table", FlatBufTokenType::Table},
    {"struct", FlatBufTokenType::Struct},
    {"enum", FlatBufTokenType::Enum},
    {"union", FlatBufTokenType::Union},
    {"root_type", FlatBufTokenType::Root_Type},
    {"file_identifier", FlatBufTokenType::File_Identifier},

    {"bool", FlatBufTokenType::Bool},
    {"byte", FlatBufTokenType::Byte},
    {"ubyte", FlatBufTokenType::UByte},
    {"short", FlatBufTokenType::Short},
    {"ushort", FlatBufTokenType::UShort},
    {"int", FlatBufTokenType::Int},
    {"uint", FlatBufTokenType::UInt},
    {"float", FlatBufTokenType::Float},
    {"long", FlatBufTokenType::Long},
    {"ulong", FlatBufTokenType::ULong},
    {"double", FlatBufTokenType::Double},
    {"string", FlatBufTokenType::String},
};

FlatBufLexer::FlatBufLexer(std::string_view source) : source_(source)
{
}

std::vector<FlatBufToken> FlatBufLexer::tokenize()
{
    std::vector<FlatBufToken> tokens;

    while (!isAtEnd())
    {
        skipWhitespace();
        if (isAtEnd())
            break;

        char c = peek();
        int start_line = line_;
        int start_col = column_;

        // Comments
        if (c == '/' && peek(1) == '/')
        {
            skipComment();
            continue;
        }

        FlatBufToken token;
        token.line = start_line;
        token.column = start_col;

        switch (c)
        {
        case '{':
            token.type = FlatBufTokenType::LBrace;
            token.value = "{";
            advance();
            break;
        case '}':
            token.type = FlatBufTokenType::RBrace;
            token.value = "}";
            advance();
            break;
        case '[':
            token.type = FlatBufTokenType::LBracket;
            token.value = "[";
            advance();
            break;
        case ']':
            token.type = FlatBufTokenType::RBracket;
            token.value = "]";
            advance();
            break;
        case ':':
            token.type = FlatBufTokenType::Colon;
            token.value = ":";
            advance();
            break;
        case ';':
            token.type = FlatBufTokenType::Semicolon;
            token.value = ";";
            advance();
            break;
        case ',':
            token.type = FlatBufTokenType::Comma;
            token.value = ",";
            advance();
            break;
        case '=':
            token.type = FlatBufTokenType::Equal;
            token.value = "=";
            advance();
            break;

        default:
            if (std::isalpha(c) || c == '_')
            {
                token = readIdentifierOrKeyword();
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
                token.type = FlatBufTokenType::Unknown;
                token.value = std::string(1, c);
                advance();
            }
        }

        tokens.push_back(token);
    }

    tokens.push_back({FlatBufTokenType::EndOfFile, "", line_, column_});
    return tokens;
}

FlatBufToken FlatBufLexer::readIdentifierOrKeyword()
{
    int start_line = line_;
    int start_col = column_;
    std::string value;

    while (!isAtEnd() && (std::isalnum(peek()) || peek() == '_'))
    {
        value += advance();
    }

    auto it = FlatKEYWORDS.find(value);
    return {it != FlatKEYWORDS.end() ? it->second : FlatBufTokenType::Identifier,
            value,
            start_line,
            start_col};
}

FlatBufToken FlatBufLexer::readNumber()
{
    int start_line = line_;
    int start_col = column_;
    std::string value;

    while (!isAtEnd() && std::isdigit(peek()))
    {
        value += advance();
    }

    return {FlatBufTokenType::IntLiteral, value, start_line, start_col};
}

FlatBufToken FlatBufLexer::readString()
{
    int start_line = line_;
    int start_col = column_;
    std::string value;

    advance(); // Skip opening "
    while (!isAtEnd() && peek() != '"')
    {
        value += advance();
    }
    if (!isAtEnd())
        advance(); // Skip closing "

    return {FlatBufTokenType::StringLiteral, value, start_line, start_col};
}

void FlatBufLexer::skipWhitespace()
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

void FlatBufLexer::skipComment()
{
    while (!isAtEnd() && peek() != '\n')
    {
        advance();
    }
}

char FlatBufLexer::peek(int offset) const
{
    if (pos_ + offset >= source_.size())
        return '\0';
    return source_[pos_ + offset];
}

char FlatBufLexer::advance()
{
    char c = source_[pos_++];
    column_++;
    return c;
}

bool FlatBufLexer::isAtEnd() const
{
    return pos_ >= source_.size();
}

//FlatBufParser::FlatBufParser(const std::string& source)
//{
//}

auto FlatBufParser::parseToAst(const std::string& src) -> bhw::Ast
{
    bhw::Ast ast;
    FlatBufLexer lexer(src);
    tokens_ = lexer.tokenize();

    while (!check(FlatBufTokenType::EndOfFile))
    {
        if (match(FlatBufTokenType::Namespace))
        {
            current_namespace_ = parseNamespace();
        }
        else if (match(FlatBufTokenType::Table))
        {
            ast.nodes.emplace_back(parseTable());
        }
        else if (match(FlatBufTokenType::Struct))
        {
            ast.nodes.emplace_back(parseStruct());
        }
        else if (match(FlatBufTokenType::Enum))
        {
            ast.nodes.emplace_back(parseEnum());
        }
        else
        {
            advance(); // Skip unknown
        }
    }

    return ast;
}

std::string FlatBufParser::parseNamespace()
{
    // namespace example;
    std::string ns;

    while (!check(FlatBufTokenType::Semicolon) && !check(FlatBufTokenType::EndOfFile))
    {
        if (check(FlatBufTokenType::Identifier))
        {
            if (!ns.empty())
                ns += ".";
            ns += advance().value;
        }
        else
        {
            advance();
        }
    }

    if (match(FlatBufTokenType::Semicolon))
    {
    }
    return ns;
}

Struct FlatBufParser::parseTable()
{
    // table User { ... }
    Struct s;

    expect(FlatBufTokenType::Identifier, "Expected table name");
    s.name = tokens_[pos_ - 1].value;

    if (!current_namespace_.empty())
    {
        s.namespaces.emplace_back(current_namespace_);
    }

    expect(FlatBufTokenType::LBrace, "Expected '{'");

    while (!check(FlatBufTokenType::RBrace) && !check(FlatBufTokenType::EndOfFile))
    {
        s.members.emplace_back(parseField());
    }

    expect(FlatBufTokenType::RBrace, "Expected '}'");

    return s;
}

Struct FlatBufParser::parseStruct()
{
    // Same as table for our purposes
    return parseTable();
}

Struct FlatBufParser::parseEnum()
{
    // enum Status : byte { Active = 0, Inactive = 1 }
    Struct s;

    expect(FlatBufTokenType::Identifier, "Expected enum name");
    s.name = tokens_[pos_ - 1].value;

    if (!current_namespace_.empty())
    {
        s.namespaces.emplace_back(current_namespace_);
    }

    // Skip optional type annotation
    if (match(FlatBufTokenType::Colon))
    {
        advance(); // Skip type
    }

    expect(FlatBufTokenType::LBrace, "Expected '{'");

    while (!check(FlatBufTokenType::RBrace) && !check(FlatBufTokenType::EndOfFile))
    {
        if (check(FlatBufTokenType::Identifier))
        {
            Field f;
            f.name = advance().value;

            // Enum value
            SimpleType st;
            st.reifiedType = ReifiedTypeId::Int32;
            f.type = std::make_unique<Type>(std::move(st));

            // Optional = value
            if (match(FlatBufTokenType::Equal))
            {
                if (check(FlatBufTokenType::IntLiteral))
                {
                    Attribute attr;
                    attr.name = "enum_value";
                    attr.value = advance().value;
                    f.attributes.emplace_back(attr);
                }
            }

            s.members.emplace_back(std::move(f));
        }

        if (match(FlatBufTokenType::Comma))
        {
        }
    }

    expect(FlatBufTokenType::RBrace, "Expected '}'");

    return s;
}

Field FlatBufParser::parseField()
{
    // name: type;
    // name: [type];  // array

    Field f;

    expect(FlatBufTokenType::Identifier, "Expected field name");
    f.name = tokens_[pos_ - 1].value;

    expect(FlatBufTokenType::Colon, "Expected ':'");

    f.type = parseType();

    if (match(FlatBufTokenType::Semicolon))
    {
    }

    return f;
}

std::unique_ptr<Type> FlatBufParser::parseType()
{
    // Check for array syntax: [type]
    if (match(FlatBufTokenType::LBracket))
    {
        auto element_type = parseType();
        expect(FlatBufTokenType::RBracket, "Expected ']'");

        // Wrap in List
        GenericType gt;
        gt.reifiedType = ReifiedTypeId::List;
        gt.args.emplace_back(std::move(element_type));

        return std::make_unique<Type>(std::move(gt));
    }

    // Scalar or user type
    const auto& token = advance();

    // Check if it's a FlatBuffers canonical type
    ReifiedTypeId canonical = mapFlatBufType(token.type);

    if (canonical != ReifiedTypeId::StructRefType)
    {
        SimpleType st;
        st.reifiedType = canonical;
        return std::make_unique<Type>(std::move(st));
    }

    // User-defined type
    SimpleType st;
    st.srcTypeString = token.value;
    return std::make_unique<Type>(std::move(st));
}

ReifiedTypeId FlatBufParser::mapFlatBufType(FlatBufTokenType type)
{
    switch (type)
    {
    case FlatBufTokenType::Bool:
        return ReifiedTypeId::Bool;
    case FlatBufTokenType::Byte:
        return ReifiedTypeId::Int8;
    case FlatBufTokenType::UByte:
        return ReifiedTypeId::UInt8;
    case FlatBufTokenType::Short:
        return ReifiedTypeId::Int16;
    case FlatBufTokenType::UShort:
        return ReifiedTypeId::UInt16;
    case FlatBufTokenType::Int:
        return ReifiedTypeId::Int32;
    case FlatBufTokenType::UInt:
        return ReifiedTypeId::UInt32;
    case FlatBufTokenType::Long:
        return ReifiedTypeId::Int64;
    case FlatBufTokenType::ULong:
        return ReifiedTypeId::UInt64;
    case FlatBufTokenType::Float:
        return ReifiedTypeId::Float32;
    case FlatBufTokenType::Double:
        return ReifiedTypeId::Float64;
    case FlatBufTokenType::String:
        return ReifiedTypeId::String;
    default:
        return ReifiedTypeId::StructRefType;
    }
}

const FlatBufToken& FlatBufParser::peek(int offset) const
{
    size_t index = pos_ + offset;
    if (index >= tokens_.size())
        return tokens_.back();
    return tokens_[index];
}

const FlatBufToken& FlatBufParser::advance()
{
    if (pos_ < tokens_.size())
        pos_++;
    return tokens_[pos_ - 1];
}

bool FlatBufParser::match(FlatBufTokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}

bool FlatBufParser::check(FlatBufTokenType type) const
{
    return peek().type == type;
}

void FlatBufParser::expect(FlatBufTokenType type, const std::string& message)
{
    if (!match(type))
    {
        throw std::runtime_error(message + " at line " + std::to_string(peek().line));
    }
}
} // namespace bhw
