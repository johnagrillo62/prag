#pragma once

#include <memory>
#include <string>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"


namespace bhw
{

enum class MdbTokenType
{
    // Literals
    Identifier,
    Number,
    String,

    // Keywords
    Create,
    Table,
    Int,
    Integer,
    Bigint,
    Smallint,
    Tinyint,
    Varchar,
    Text,
    Char,
    Decimal,
    Float,
    Double,
    Real,
    Date,
    Time,
    DateTime,
    Timestamp,
    Boolean,
    Bool,
    Binary,
    Varbinary,
    Blob,
    Null,
    Not,
    Primary,
    Key,

    // MDB-specific types
    Long,
    Single,
    Byte,

    // Symbols
    LeftParen,
    RightParen,
    LeftBracket,
    RightBracket,
    Comma,
    Semicolon,

    // Special
    Eof,
    Unknown
};

struct MdbToken
{
    MdbTokenType type;
    std::string value;
    size_t line;
    size_t column;
};

class MdbLexer
{
  public:
    MdbToken nextToken();

    std::string source;
    size_t pos = 0;
    size_t line = 1;
    size_t column = 1;

    char current() const;
    char peek(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipComment();

    MdbToken makeToken(MdbTokenType type, const std::string& value);
    MdbToken readNumber();
    MdbToken readString();
    MdbToken readIdentifier();
    MdbToken readBracketedIdentifier();
};

class MdbParser : public AstParser, public AutoRegisterParser<MdbParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"mdb"};
    }
    
  public:
    auto parseToAst(const std::string& src) -> Ast override;
    auto getLang() -> Language override
    {
        return Language::MDB;
    }

  private:
    MdbLexer lexer;
    MdbToken current_token;

    void advance();
    bool match(MdbTokenType type);
    bool check(MdbTokenType type) const;
    MdbToken expect(MdbTokenType type);

    std::string parseIdentifier();
    std::unique_ptr<Type> parseType();
    Field parseColumnDefinition();
    Struct parseCreateTable();

    // Efficient type factory methods
    std::unique_ptr<Type> makeType(ReifiedTypeId ct);
    std::unique_ptr<Type> makeOptional(std::unique_ptr<Type> inner);
};
} // namespace bhw
