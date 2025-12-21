#pragma once

#include <memory>
#include <string>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"

namespace bhw
{
}
enum class GraphQLTokenType
{
    // Keywords
    Type,
    Interface,
    Enum,
    Input,
    Query,
    Mutation,
    Subscription,

    // Types
    Int,
    Float,
    String,
    Boolean,
    ID,

    // Symbols
    LBrace,      // {
    RBrace,      // }
    LBracket,    // [
    RBracket,    // ]
    LParen,      // (
    RParen,      // )
    Colon,       // :
    Exclamation, // !
    Pipe,        // |
    Ampersand,   // &
    At,          // @

    // Other
    Identifier,
    EndOfFile
};
namespace bhw
{

struct GraphQLToken
{
    GraphQLTokenType type;
    std::string value;
};

class GraphQLParser : public AstParser, public AutoRegisterParser<GraphQLParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"gpl", "graphql"};
    }
  public:
    auto parseToAst(const std::string& src) -> Ast override;
    auto getLang() -> Language override
    {
        return Language::GraphQl;
    }

  private:
    std::string source;
    size_t pos = 0;
    GraphQLToken current_token;

    // Lexer methods
    void skipWhitespaceAndComments();
    GraphQLToken nextToken();
    GraphQLToken readIdentifier();
    void advance();
    bool match(GraphQLTokenType type);
    void expect(GraphQLTokenType type);

    // AstParser methods
    std::unique_ptr<Type> parseType();
    Field parseField();
    Struct parseTypeDefinition();
    Enum parseEnumDefinition();
};
} // namespace bhw
