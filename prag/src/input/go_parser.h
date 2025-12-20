
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "languages.h"
#include "parser_registry2.h"

namespace bhw
{
enum class GoTokenType
{
    // Keywords
    Package,
    Import,
    Type,
    Struct,
    Interface,
    Const,
    Var,
    Func,
    Map,
    Chan,

    // Literals
    Identifier,
    Number,
    String,

    // Symbols
    LBrace,   // {
    RBrace,   // }
    LParen,   // (
    RParen,   // )
    LBracket, // [
    RBracket, // ]
    Comma,    // ,
    Dot,      // .
    Star,     // *
    Equals,   // =

    Eof,
    Unknown
};

struct GoToken
{
    GoTokenType type;
    std::string value;
    size_t line;
    size_t column;
};

class GoLexer
{
  public:
    GoToken nextToken();

    std::string source;
    size_t pos = 0;

  private:
    char current() const;
    char peek(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();

    GoToken makeToken(GoTokenType type, const std::string& value);
    GoToken readNumber();
    GoToken readIdentifier();
    GoToken readString();
    GoToken readRawString();

    size_t line = 1;
    size_t column = 1;
};


class GoParser : public bhw::AstParser, public AutoRegisterParser<GoParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"go"};
    }
    
  public:
    ~GoParser() override = default;

    bhw::Ast parseToAst(const std::string& src) override;
    auto getLang() -> bhw::Language override
    {
        return Language::Go;
    }

  private:
    void advance();
    bool match(GoTokenType type) const;
    bool expect(GoTokenType type);

    std::string parseQualifiedName();
    void registerUserType(const std::string& type_name);
    std::unique_ptr<Type> resolveSimpleType(const std::string& type_name);
    std::unique_ptr<Type> parseType();
    Field parseField();
    Struct parseStructBody();
    Enum parseEnum(const std::string& enumName);

    GoLexer lexer;
    GoToken current_token;
    std::set<std::string> known_user_types_;
};
} // namespace bhw
