#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"

namespace bhw
{
enum class ThriftTokenType
{
    // Keywords
    Namespace,
    Include,
    Struct,
    Enum,
    Service,
    Exception,
    Typedef,
    Const,
    Required,
    Optional,
    Oneway,

    // Types
    Bool,
    Byte,
    I8,
    I16,
    I32,
    I64,
    Double,
    String,
    Binary,
    List,
    Set,
    Map,

    // Symbols
    LBrace,    // {
    RBrace,    // }
    LParen,    // (
    RParen,    // )
    LT,        //
    GT,        // >
    Comma,     // ,
    Semicolon, // ;
    Colon,     // :
    Equals,    // =

    // Other
    Identifier,
    Number,
    StringLiteral,
    EndOfFile
};

struct ThriftToken
{
    ThriftTokenType type;
    std::string value;
};

class ThriftParser : public bhw::AstParser, public AutoRegisterParser<ThriftParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"thrift"};
    }
    
  public:

    auto parseToAst(const std::string& src) -> bhw::Ast override;
    auto getLang() -> bhw::Language override
    {
        return Language::Thrift;
    }

    std::vector<Enum> parseEnums();
    std::vector<Service> parseServices();

  private:
    std::string source;
    size_t pos = 0;
    ThriftToken current_token;

    // Lexer methods
    void skipWhitespaceAndComments();
    ThriftToken nextToken();
    ThriftToken readIdentifier();
    ThriftToken readNumber();
    ThriftToken readStringLiteral();
    void advance();
    bool match(ThriftTokenType type);
    void expect(ThriftTokenType type);
    std::string parseIdentifier();

    // AstParser methods
    std::unique_ptr<Type> parseType();
    Field parseField();
    Struct parseStruct();
    Enum parseEnum();
    Service parseService();
};
} // namespace bhw