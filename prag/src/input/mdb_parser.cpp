#include "mdb_parser.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <unordered_set>

std::string makeSafeIdentifier(const std::string& input)
{
    std::string result;

    // If first character is digit, prefix with '_'
    if (!input.empty() && std::isdigit(input[0]))
        result += '_';

    for (char c : input)
    {
        if (std::isalnum(c) || c == '_')
        {
            result += c;
        }
        else
        {
            result += '_'; // replace invalid chars with '_'
        }
    }

    // Optional: lowercase or snake_case conversion
    // std::transform(result.begin(), result.end(), result.begin(), ::tolower);

    // Avoid reserved keywords (example: Rust keywords)
    static const std::unordered_set<std::string> reserved = {"type",
                                                             "enum",
                                                             "struct",
                                                             "fn",
                                                             "let",
                                                             "mod",
                                                             "as",
                                                             "match",
                                                             "if",
                                                             "else",
                                                             "for",
                                                             "while",
                                                             "loop"};
    if (reserved.count(result))
        result = result + "_";

    return result;
}

using namespace bhw;
// ============================================================================
// Type Factory - Efficient Type Creation
// ============================================================================

std::unique_ptr<Type> MdbParser::makeType(ReifiedTypeId ct)
{
    SimpleType simple;
    simple.reifiedType = ct;
    return std::make_unique<Type>(std::move(simple));
}

std::unique_ptr<Type> MdbParser::makeOptional(std::unique_ptr<Type> inner)
{
    GenericType generic;
    generic.reifiedType = ReifiedTypeId::Optional;
    generic.args.emplace_back(std::move(inner));
    return std::make_unique<Type>(std::move(generic));
}

// ============================================================================
// MdbLexer Implementation
// ============================================================================

char MdbLexer::current() const
{
    return pos < source.size() ? source[pos] : '\0';
}

char MdbLexer::peek(size_t offset) const
{
    size_t peek_pos = pos + offset;
    return peek_pos < source.size() ? source[peek_pos] : '\0';
}

void MdbLexer::advance()
{
    if (pos < source.size())
    {
        if (source[pos] == '\n')
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
}

void MdbLexer::skipWhitespace()
{
    while (std::isspace(current()))
        advance();
}

void MdbLexer::skipComment()
{
    if (current() == '-' && peek() == '-')
    {
        while (current() != '\n' && current() != '\0')
            advance();
    }
    else if (current() == '/' && peek() == '*')
    {
        advance();
        advance();
        while (!(current() == '*' && peek() == '/') && current() != '\0')
            advance();
        if (current() == '*')
        {
            advance();
            advance();
        }
    }
}

MdbToken MdbLexer::makeToken(MdbTokenType type, const std::string& value)
{
    return MdbToken{type, value, line, column};
}

MdbToken MdbLexer::readNumber()
{
    std::string num;
    while (std::isdigit(current()) || current() == '.')
    {
        num += current();
        advance();
    }
    return makeToken(MdbTokenType::Number, num);
}

MdbToken MdbLexer::readString()
{
    char quote = current();
    advance();

    std::string str;
    while (current() != quote && current() != '\0')
    {
        if (current() == '\\')
        {
            advance();
            if (current() != '\0')
            {
                str += current();
                advance();
            }
        }
        else
        {
            str += current();
            advance();
        }
    }

    if (current() == quote)
        advance();

    return makeToken(MdbTokenType::String, str);
}

MdbToken MdbLexer::readBracketedIdentifier()
{
    advance();

    std::string id;
    while (current() != ']' && current() != '\0')
    {
        id += current();
        advance();
    }

    if (current() == ']')
        advance();

    return makeToken(MdbTokenType::Identifier, id);
}

MdbToken MdbLexer::readIdentifier()
{
    std::string id;
    while (std::isalnum(current()) || current() == '_')
    {
        id += current();
        advance();
    }

    std::string upper = id;

    std::transform(upper.begin(),
                   upper.end(),
                   upper.begin(),
                   [](char c) -> char
                   { return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); });

    // Keywords mapping
    if (upper == "CREATE")
        return makeToken(MdbTokenType::Create, id);
    if (upper == "TABLE")
        return makeToken(MdbTokenType::Table, id);
    if (upper == "INT")
        return makeToken(MdbTokenType::Int, id);
    if (upper == "INTEGER")
        return makeToken(MdbTokenType::Integer, id);
    if (upper == "BIGINT")
        return makeToken(MdbTokenType::Bigint, id);
    if (upper == "SMALLINT")
        return makeToken(MdbTokenType::Smallint, id);
    if (upper == "TINYINT")
        return makeToken(MdbTokenType::Tinyint, id);
    if (upper == "VARCHAR")
        return makeToken(MdbTokenType::Varchar, id);
    if (upper == "TEXT")
        return makeToken(MdbTokenType::Text, id);
    if (upper == "CHAR")
        return makeToken(MdbTokenType::Char, id);
    if (upper == "DECIMAL")
        return makeToken(MdbTokenType::Decimal, id);
    if (upper == "FLOAT")
        return makeToken(MdbTokenType::Float, id);
    if (upper == "DOUBLE")
        return makeToken(MdbTokenType::Double, id);
    if (upper == "REAL")
        return makeToken(MdbTokenType::Real, id);
    if (upper == "DATE")
        return makeToken(MdbTokenType::Date, id);
    if (upper == "TIME")
        return makeToken(MdbTokenType::Time, id);
    if (upper == "DATETIME")
        return makeToken(MdbTokenType::DateTime, id);
    if (upper == "TIMESTAMP")
        return makeToken(MdbTokenType::Timestamp, id);
    if (upper == "BOOLEAN")
        return makeToken(MdbTokenType::Boolean, id);
    if (upper == "BOOL")
        return makeToken(MdbTokenType::Bool, id);
    if (upper == "BINARY")
        return makeToken(MdbTokenType::Binary, id);
    if (upper == "VARBINARY")
        return makeToken(MdbTokenType::Varbinary, id);
    if (upper == "BLOB")
        return makeToken(MdbTokenType::Blob, id);
    if (upper == "NULL")
        return makeToken(MdbTokenType::Null, id);
    if (upper == "NOT")
        return makeToken(MdbTokenType::Not, id);
    if (upper == "PRIMARY")
        return makeToken(MdbTokenType::Primary, id);
    if (upper == "KEY")
        return makeToken(MdbTokenType::Key, id);
    if (upper == "LONG")
        return makeToken(MdbTokenType::Long, id);
    if (upper == "SINGLE")
        return makeToken(MdbTokenType::Single, id);
    if (upper == "BYTE")
        return makeToken(MdbTokenType::Byte, id);

    return makeToken(MdbTokenType::Identifier, id);
}

MdbToken MdbLexer::nextToken()
{
    skipWhitespace();

    while ((current() == '-' && peek() == '-') || (current() == '/' && peek() == '*'))
    {
        skipComment();
        skipWhitespace();
    }

    if (current() == '\0')
        return makeToken(MdbTokenType::Eof, "");

    if (current() == '(')
    {
        advance();
        return makeToken(MdbTokenType::LeftParen, "(");
    }
    if (current() == ')')
    {
        advance();
        return makeToken(MdbTokenType::RightParen, ")");
    }
    if (current() == '[')
        return readBracketedIdentifier();
    if (current() == ',')
    {
        advance();
        return makeToken(MdbTokenType::Comma, ",");
    }
    if (current() == ';')
    {
        advance();
        return makeToken(MdbTokenType::Semicolon, ";");
    }

    if (current() == '\'' || current() == '"')
        return readString();

    if (std::isdigit(current()))
        return readNumber();

    if (std::isalpha(current()) || current() == '_')
        return readIdentifier();

    std::string unknown(1, current());
    advance();
    return makeToken(MdbTokenType::Unknown, unknown);
}

// ============================================================================
// MdbParser Implementation
// ============================================================================

//MdbParser::MdbParser(const std::string& source) : lexer(source)
//{
//    advance();
//}

void MdbParser::advance()
{
    current_token = lexer.nextToken();
}

bool MdbParser::match(MdbTokenType type)
{
    if (check(type))
    {
        advance();
        return true;
    }
    return false;
}

bool MdbParser::check(MdbTokenType type) const
{
    return current_token.type == type;
}

MdbToken MdbParser::expect(MdbTokenType type)
{
    if (!check(type))
    {
        throw std::runtime_error("Expected token type at line " +
                                 std::to_string(current_token.line) +
                                 ", got: " + current_token.value);
    }
    MdbToken token = current_token;
    advance();
    return token;
}

std::string MdbParser::parseIdentifier()
{
    if (check(MdbTokenType::Identifier))
    {
        std::string id = current_token.value;
        advance();
        return id;
    }
    throw std::runtime_error("Expected identifier");
}

std::unique_ptr<Type> MdbParser::parseType()
{
    // Handle "Long Integer" - special case for MDB
    if (check(MdbTokenType::Long))
    {
        advance();
        if (match(MdbTokenType::Integer))
        {
            // "Long Integer" -> Int64
        }
        return makeType(ReifiedTypeId::Int64);
    }

    // Check type and return immediately
    if (match(MdbTokenType::Integer) || match(MdbTokenType::Int))
        return makeType(ReifiedTypeId::Int32);

    if (match(MdbTokenType::Bigint))
        return makeType(ReifiedTypeId::Int64);

    if (match(MdbTokenType::Smallint))
        return makeType(ReifiedTypeId::Int16);

    if (match(MdbTokenType::Tinyint))
        return makeType(ReifiedTypeId::Int8);

    if (match(MdbTokenType::Byte))
        return makeType(ReifiedTypeId::UInt8);

    if (match(MdbTokenType::Single))
        return makeType(ReifiedTypeId::Float32);

    if (match(MdbTokenType::Float) || match(MdbTokenType::Real))
        return makeType(ReifiedTypeId::Float32);

    if (match(MdbTokenType::Double))
        return makeType(ReifiedTypeId::Float64);

    if (match(MdbTokenType::Boolean) || match(MdbTokenType::Bool))
        return makeType(ReifiedTypeId::Bool);

    if (match(MdbTokenType::DateTime) || match(MdbTokenType::Timestamp))
        return makeType(ReifiedTypeId::DateTime);

    if (match(MdbTokenType::Date))
        return makeType(ReifiedTypeId::Date);

    if (match(MdbTokenType::Time))
        return makeType(ReifiedTypeId::Time);

    // String types with optional size
    if (match(MdbTokenType::Text) || match(MdbTokenType::Varchar) || match(MdbTokenType::Char))
    {
        if (match(MdbTokenType::LeftParen))
        {
            expect(MdbTokenType::Number);
            expect(MdbTokenType::RightParen);
        }
        return makeType(ReifiedTypeId::String);
    }

    // Decimal with optional precision
    if (match(MdbTokenType::Decimal))
    {
        if (match(MdbTokenType::LeftParen))
        {
            expect(MdbTokenType::Number);
            if (match(MdbTokenType::Comma))
                expect(MdbTokenType::Number);
            expect(MdbTokenType::RightParen);
        }
        return makeType(ReifiedTypeId::Decimal);
    }

    // Binary types with optional size
    if (match(MdbTokenType::Binary) || match(MdbTokenType::Varbinary) || match(MdbTokenType::Blob))
    {
        if (match(MdbTokenType::LeftParen))
        {
            expect(MdbTokenType::Number);
            expect(MdbTokenType::RightParen);
        }
        return makeType(ReifiedTypeId::Bytes);
    }

    throw std::runtime_error("Unknown type: " + current_token.value);
}

Field MdbParser::parseColumnDefinition()
{
    Field field;
    std::string fieldName = parseIdentifier();

    field.attributes.push_back({"sql_column", fieldName});
    field.name = makeSafeIdentifier(fieldName);
    field.type = parseType();
    bool is_nullable = true;

    // Parse constraints
    while (true)
    {
        if (match(MdbTokenType::Not))
        {
            expect(MdbTokenType::Null);
            is_nullable = false;
        }
        else if (match(MdbTokenType::Null))
        {
            is_nullable = true;
        }
        else if (match(MdbTokenType::Primary))
        {
            expect(MdbTokenType::Key);
            field.attributes.push_back({"primary_key", "true"});
            is_nullable = false;
        }
        else
        {
            break;
        }
    }

    // Wrap in Optional if nullable
    if (is_nullable)
    {
        field.type = makeOptional(std::move(field.type));
    }

    return field;
}

Struct MdbParser::parseCreateTable()
{
    expect(MdbTokenType::Create);
    expect(MdbTokenType::Table);

    Struct table;

    auto tableName = parseIdentifier();

    table.attributes.push_back({"sql_table", tableName});
    table.name = makeSafeIdentifier(tableName);

    // Note: MDB schemas don't have namespaces, leave empty

    expect(MdbTokenType::LeftParen);

    while (!check(MdbTokenType::RightParen) && !check(MdbTokenType::Eof))
    {
        // Skip table-level constraints
        if (check(MdbTokenType::Primary))
        {
            while (!check(MdbTokenType::Comma) && !check(MdbTokenType::RightParen) &&
                   !check(MdbTokenType::Eof))
                advance();
            if (match(MdbTokenType::Comma))
                continue;
            break;
        }

        table.members.emplace_back(parseColumnDefinition());

        if (!match(MdbTokenType::Comma))
            break;
    }

    expect(MdbTokenType::RightParen);
    match(MdbTokenType::Semicolon);

    return table;
}


auto MdbParser::parseToAst(const std::string& src) -> bhw::Ast
{

    bhw::Ast ast;
    lexer.source = src;

    while (!check(MdbTokenType::Eof))
    {
        if (check(MdbTokenType::Create))
        {
            ast.nodes.emplace_back(parseCreateTable());
        }
        else
        {
            advance();
        }
    }

    return ast;
}
