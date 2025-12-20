#pragma once
#include <memory>
#include <string>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"

namespace bhw
{
// Forward declarations
enum class TypeScriptTokenType : uint8_t
{
    INTERFACE,
    TYPE,
    ENUM,
    IDENTIFIER,
    LBRACE,    // {
    RBRACE,    // }
    SEMICOLON, // ;
    COLON,     // :
    QUESTION,  // ?
    COMMA,     // ,
    LT,        //
    GT,        // >
    LBRACKET,  // [
    RBRACKET,  // ]
    PIPE,      // |
    AMPERSAND, // &
    EQUALS,    // =
    STRING_LITERAL,
    NUMBER_LITERAL,
    END_OF_FILE
};

struct TsToken
{
    TypeScriptTokenType type;
    std::string value;
};

// TsToken types for TypeScript
class TypeScriptLexer
{
  public:
    TypeScriptLexer(const std::string& src) : source(src)
    {
    }

    TsToken nextToken()
    {
        skipWhitespaceAndComments();

        if (pos >= source.length())
        {
            return {TypeScriptTokenType::END_OF_FILE, ""};
        }

        char c = source[pos];

        // Single character tokens
        if (c == '{')
        {
            pos++;
            return {TypeScriptTokenType::LBRACE, "{"};
        }
        if (c == '}')
        {
            pos++;
            return {TypeScriptTokenType::RBRACE, "}"};
        }
        if (c == ';')
        {
            pos++;
            return {TypeScriptTokenType::SEMICOLON, ";"};
        }
        if (c == ':')
        {
            pos++;
            return {TypeScriptTokenType::COLON, ":"};
        }
        if (c == '?')
        {
            pos++;
            return {TypeScriptTokenType::QUESTION, "?"};
        }
        if (c == ',')
        {
            pos++;
            return {TypeScriptTokenType::COMMA, ","};
        }
        if (c == '<')
        {
            pos++;
            return {TypeScriptTokenType::LT, "<"};
        }
        if (c == '>')
        {
            pos++;
            return {TypeScriptTokenType::GT, ">"};
        }
        if (c == '[')
        {
            pos++;
            return {TypeScriptTokenType::LBRACKET, "["};
        }
        if (c == ']')
        {
            pos++;
            return {TypeScriptTokenType::RBRACKET, "]"};
        }
        if (c == '|')
        {
            pos++;
            return {TypeScriptTokenType::PIPE, "|"};
        }
        if (c == '&')
        {
            pos++;
            return {TypeScriptTokenType::AMPERSAND, "&"};
        }
        if (c == '=')
        {
            pos++;
            return {TypeScriptTokenType::EQUALS, "="};
        }

        // String literals
        if (c == '"' || c == '\'')
        {
            return readString(c);
        }

        // Numbers
        if (std::isdigit(c))
        {
            return readNumber();
        }

        // Identifiers and keywords
        if (std::isalpha(c) || c == '_')
        {
            return readIdentifier();
        }

        // Skip unknown characters
        pos++;
        return nextToken();
    }

    std::string source;
    size_t pos = 0;

    void skipWhitespaceAndComments()
    {
        while (pos < source.length())
        {
            char c = source[pos];

            if (std::isspace(c))
            {
                pos++;
                continue;
            }

            // Line comments //
            if (c == '/' && pos + 1 < source.length() && source[pos + 1] == '/')
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

    TsToken readIdentifier()
    {
        size_t start = pos;
        while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_'))
        {
            pos++;
        }
        std::string value = source.substr(start, pos - start);

        if (value == "interface")
            return {TypeScriptTokenType::INTERFACE, value};
        if (value == "type")
            return {TypeScriptTokenType::TYPE, value};
        if (value == "enum")
            return {TypeScriptTokenType::ENUM, value};

        return {TypeScriptTokenType::IDENTIFIER, value};
    }

    TsToken readString(char quote)
    {
        pos++; // Skip opening quote
        size_t start = pos;
        while (pos < source.length() && source[pos] != quote)
        {
            if (source[pos] == '\\')
                pos++; // Skip escaped character
            pos++;
        }
        std::string value = source.substr(start, pos - start);
        if (pos < source.length())
            pos++; // Skip closing quote
        return {TypeScriptTokenType::STRING_LITERAL, value};
    }

    TsToken readNumber()
    {
        size_t start = pos;
        while (pos < source.length() && (std::isdigit(source[pos]) || source[pos] == '.'))
        {
            pos++;
        }
        std::string value = source.substr(start, pos - start);
        return {TypeScriptTokenType::NUMBER_LITERAL, value};
    }
};

class TypeScriptParser : public bhw::AstParser, public AutoRegisterParser<TypeScriptParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"ts"};
    }
    
  public:
    auto parseToAst(const std::string& src) -> bhw::Ast override;
    auto getLang() -> bhw::Language override
    {
        return Language::Typescript;
    }

  private:

    void advance(TypeScriptLexer& lexer, TsToken& current_token);
    void expect(TypeScriptTokenType type, TypeScriptLexer& lexer, TsToken& current_token);

    std::unique_ptr<Type> parseType(TypeScriptLexer& lexer, TsToken& current_token);
    std::unique_ptr<Type> parseSingleType(TypeScriptLexer& lexer, TsToken& current_token);

    Struct parseInterface(TypeScriptLexer& lexer, TsToken& current_token);
    Enum parseEnum(TypeScriptLexer& lexer, TsToken& current_token);
};
} // namespace bhw
