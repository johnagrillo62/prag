// csharp_parser.h - C# type parser with proper lexer/parser
#pragma once
#include <cctype>
#include <stdexcept>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"

namespace bhw
{

class CSharpLexer
{
  public:
    enum class TokenType
    {
        NAMESPACE,
        CLASS,
        STRUCT,
        RECORD,
        ENUM,
        INTERFACE,
        PUBLIC,
        PRIVATE,
        PROTECTED,
        INTERNAL,
        STATIC,
        READONLY,
        CONST,
        ABSTRACT,
        SEALED,
        PARTIAL,
        GET,
        SET,
        USING,
        LBRACE,
        RBRACE,
        LPAREN,
        RPAREN,
        LBRACKET,
        RBRACKET,
        LANGLE,
        RANGLE,
        SEMICOLON,
        COLON,
        COMMA,
        DOT,
        EQUALS,
        QUESTION,
        ID,
        NUMBER,
        STRING,
        EOF_TOKEN
    };

    struct Token
    {
        TokenType type;
        std::string value;
        int line;
        int col;
    };

    std::vector<Token> tokenize(const std::string& source)
    {
        std::vector<Token> tokens;
        size_t pos = 0;
        int line = 1;
        int col = 1;

        while (pos < source.size())
        {
            // Skip whitespace
            if (std::isspace(source[pos]))
            {
                if (source[pos] == '\n')
                {
                    line++;
                    col = 1;
                }
                else
                {
                    col++;
                }
                pos++;
                continue;
            }

            // Comments: //
            if (pos + 1 < source.size() && source[pos] == '/' && source[pos + 1] == '/')
            {
                while (pos < source.size() && source[pos] != '\n')
                    pos++;
                continue;
            }

            // Comments: /* ... */
            if (pos + 1 < source.size() && source[pos] == '/' && source[pos + 1] == '*')
            {
                pos += 2;
                while (pos + 1 < source.size())
                {
                    if (source[pos] == '*' && source[pos + 1] == '/')
                    {
                        pos += 2;
                        break;
                    }
                    if (source[pos] == '\n')
                    {
                        line++;
                        col = 1;
                    }
                    pos++;
                }
                continue;
            }

            // String literals
            if (source[pos] == '"')
            {
                size_t start = pos;
                pos++;
                while (pos < source.size() && source[pos] != '"')
                {
                    if (source[pos] == '\\')
                        pos++; // Skip escaped char
                    pos++;
                }
                if (pos < source.size())
                    pos++; // Skip closing "
                tokens.push_back({TokenType::STRING, source.substr(start, pos - start), line, col});
                continue;
            }

            // Single char tokens
            if (source[pos] == '{')
            {
                tokens.push_back({TokenType::LBRACE, "{", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == '}')
            {
                tokens.push_back({TokenType::RBRACE, "}", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == '(')
            {
                tokens.push_back({TokenType::LPAREN, "(", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == ')')
            {
                tokens.push_back({TokenType::RPAREN, ")", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == '[')
            {
                tokens.push_back({TokenType::LBRACKET, "[", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == ']')
            {
                tokens.push_back({TokenType::RBRACKET, "]", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == '<')
            {
                tokens.push_back({TokenType::LANGLE, "<", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == '>')
            {
                tokens.push_back({TokenType::RANGLE, ">", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == ';')
            {
                tokens.push_back({TokenType::SEMICOLON, ";", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == ':')
            {
                tokens.push_back({TokenType::COLON, ":", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == ',')
            {
                tokens.push_back({TokenType::COMMA, ",", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == '.')
            {
                tokens.push_back({TokenType::DOT, ".", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == '=')
            {
                tokens.push_back({TokenType::EQUALS, "=", line, col});
                pos++;
                col++;
                continue;
            }
            if (source[pos] == '?')
            {
                tokens.push_back({TokenType::QUESTION, "?", line, col});
                pos++;
                col++;
                continue;
            }

            // Numbers
            if (std::isdigit(source[pos]))
            {
                size_t start = pos;
                while (pos < source.size() && std::isdigit(source[pos]))
                {
                    pos++;
                    col++;
                }
                tokens.push_back({TokenType::NUMBER, source.substr(start, pos - start), line, col});
                continue;
            }

            // Identifiers and keywords
            if (std::isalpha(source[pos]) || source[pos] == '_')
            {
                size_t start = pos;
                int startCol = col;

                while (pos < source.size() && (std::isalnum(source[pos]) || source[pos] == '_'))
                {
                    pos++;
                    col++;
                }

                std::string word = source.substr(start, pos - start);
                TokenType type = TokenType::ID;

                if (word == "namespace")
                    type = TokenType::NAMESPACE;
                else if (word == "class")
                    type = TokenType::CLASS;
                else if (word == "struct")
                    type = TokenType::STRUCT;
                else if (word == "record")
                    type = TokenType::RECORD;
                else if (word == "enum")
                    type = TokenType::ENUM;
                else if (word == "interface")
                    type = TokenType::INTERFACE;
                else if (word == "public")
                    type = TokenType::PUBLIC;
                else if (word == "private")
                    type = TokenType::PRIVATE;
                else if (word == "protected")
                    type = TokenType::PROTECTED;
                else if (word == "internal")
                    type = TokenType::INTERNAL;
                else if (word == "static")
                    type = TokenType::STATIC;
                else if (word == "readonly")
                    type = TokenType::READONLY;
                else if (word == "const")
                    type = TokenType::CONST;
                else if (word == "abstract")
                    type = TokenType::ABSTRACT;
                else if (word == "sealed")
                    type = TokenType::SEALED;
                else if (word == "partial")
                    type = TokenType::PARTIAL;
                else if (word == "get")
                    type = TokenType::GET;
                else if (word == "set")
                    type = TokenType::SET;
                else if (word == "using")
                    type = TokenType::USING;

                tokens.push_back({type, word, line, startCol});
                continue;
            }

            // Unknown character - skip it
            pos++;
            col++;
        }

        tokens.push_back({TokenType::EOF_TOKEN, "", line, col});
        return tokens;
    }
};

class CSharpParser : public AstParser, public AutoRegisterParser<CSharpParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"cs"};
    }
  public:

    Ast parseToAst(const std::string& src) override
    {
        CSharpLexer lexer;
        tokens = lexer.tokenize(src);
        pos = 0;

        Ast ast;
        ast.srcName = "csharp";

        parseFile(ast);

        return ast;
    }

    auto getLang() -> bhw::Language override
    {
        return Language::CSharp;
    }

  private:
    std::string src;
    std::vector<CSharpLexer::Token> tokens;
    size_t pos;

    bool isAtEnd() const
    {
        return peek().type == CSharpLexer::TokenType::EOF_TOKEN;
    }

    CSharpLexer::Token peek() const
    {
        if (pos >= tokens.size())
            return tokens.back();
        return tokens[pos];
    }

    CSharpLexer::Token previous() const
    {
        if (pos == 0)
            return tokens[0];
        return tokens[pos - 1];
    }

    CSharpLexer::Token advance()
    {
        if (!isAtEnd())
            pos++;
        return previous();
    }

    bool match(CSharpLexer::TokenType type)
    {
        return peek().type == type;
    }

    bool consume(CSharpLexer::TokenType type, const std::string& errMsg)
    {
        if (match(type))
        {
            advance();
            return true;
        }
        throw std::runtime_error(errMsg + " at line " + std::to_string(peek().line));
    }

    void parseFile(Ast& ast)
    {
        while (!isAtEnd())
        {
            // Skip using directives
            if (match(CSharpLexer::TokenType::USING))
            {
                advance();
                // Skip until semicolon
                while (!match(CSharpLexer::TokenType::SEMICOLON) && !isAtEnd())
                    advance();
                if (match(CSharpLexer::TokenType::SEMICOLON))
                    advance();
                continue;
            }

            // Parse namespace
            if (match(CSharpLexer::TokenType::NAMESPACE))
            {
                parseNamespace(ast);
                continue;
            }

            // Parse top-level declarations
            parseDeclaration(ast, {});
        }
    }

    void parseNamespace(Ast& ast)
    {
        consume(CSharpLexer::TokenType::NAMESPACE, "Expected 'namespace'");

        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected namespace name");

        std::string nsName = advance().value;

        // Handle dotted namespace names
        std::vector<std::string> nsPath;
        nsPath.push_back(nsName);

        while (match(CSharpLexer::TokenType::DOT))
        {
            advance(); // consume .
            if (!match(CSharpLexer::TokenType::ID))
                throw std::runtime_error("Expected identifier after '.'");
            nsPath.push_back(advance().value);
        }

        consume(CSharpLexer::TokenType::LBRACE, "Expected '{'");

        // Create Namespace node
        Namespace ns;
        ns.name = nsName;

        // Parse namespace body
        while (!match(CSharpLexer::TokenType::RBRACE) && !isAtEnd())
        {
            if (match(CSharpLexer::TokenType::NAMESPACE))
            {
                // Nested namespace - parse recursively into this namespace
                parseNestedNamespace(ns, nsPath);
            }
            else
            {
                parseDeclarationIntoNamespace(ns, nsPath);
            }
        }

        consume(CSharpLexer::TokenType::RBRACE, "Expected '}'");

        // Add namespace to AST
        ast.nodes.push_back(std::move(ns));
    }

    void parseNestedNamespace(Namespace& parentNs, const std::vector<std::string>& parentPath)
    {
        consume(CSharpLexer::TokenType::NAMESPACE, "Expected 'namespace'");

        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected namespace name");

        std::string nsName = advance().value;

        std::vector<std::string> nsPath = parentPath;
        nsPath.push_back(nsName);

        consume(CSharpLexer::TokenType::LBRACE, "Expected '{'");

        Namespace ns;
        ns.name = nsName;

        while (!match(CSharpLexer::TokenType::RBRACE) && !isAtEnd())
        {
            if (match(CSharpLexer::TokenType::NAMESPACE))
            {
                parseNestedNamespace(ns, nsPath);
            }
            else
            {
                parseDeclarationIntoNamespace(ns, nsPath);
            }
        }

        consume(CSharpLexer::TokenType::RBRACE, "Expected '}'");

        parentNs.nodes.push_back(std::move(ns));
    }

    void parseDeclaration(Ast& ast, const std::vector<std::string>& nsPath)
    {
        // Capture modifiers
        bool hasAbstract = false;
        while (match(CSharpLexer::TokenType::PUBLIC) || match(CSharpLexer::TokenType::PRIVATE) ||
               match(CSharpLexer::TokenType::PROTECTED) ||
               match(CSharpLexer::TokenType::INTERNAL) || match(CSharpLexer::TokenType::STATIC) ||
               match(CSharpLexer::TokenType::READONLY) || match(CSharpLexer::TokenType::CONST) ||
               match(CSharpLexer::TokenType::ABSTRACT) || match(CSharpLexer::TokenType::SEALED) ||
               match(CSharpLexer::TokenType::PARTIAL))
        {
            if (match(CSharpLexer::TokenType::ABSTRACT))
                hasAbstract = true;
            advance();
        }

        if (match(CSharpLexer::TokenType::CLASS) || match(CSharpLexer::TokenType::STRUCT))
        {
            parseClass(ast, nsPath, hasAbstract);
        }
        else if (match(CSharpLexer::TokenType::RECORD))
        {
            parseRecord(ast, nsPath, hasAbstract);
        }
        else if (match(CSharpLexer::TokenType::ENUM))
        {
            parseEnum(ast, nsPath);
        }
        else if (match(CSharpLexer::TokenType::INTERFACE))
        {
            // Skip interfaces for now
            advance();
            skipToEndOfBlock();
        }
        else if (!isAtEnd())
        {
            advance(); // Skip unknown token
        }
    }

    void parseDeclarationIntoNamespace(Namespace& ns, const std::vector<std::string>& nsPath)
    {
        // Capture modifiers
        bool hasAbstract = false;
        while (match(CSharpLexer::TokenType::PUBLIC) || match(CSharpLexer::TokenType::PRIVATE) ||
               match(CSharpLexer::TokenType::PROTECTED) ||
               match(CSharpLexer::TokenType::INTERNAL) || match(CSharpLexer::TokenType::STATIC) ||
               match(CSharpLexer::TokenType::READONLY) || match(CSharpLexer::TokenType::CONST) ||
               match(CSharpLexer::TokenType::ABSTRACT) || match(CSharpLexer::TokenType::SEALED) ||
               match(CSharpLexer::TokenType::PARTIAL))
        {
            if (match(CSharpLexer::TokenType::ABSTRACT))
                hasAbstract = true;
            advance();
        }

        if (match(CSharpLexer::TokenType::CLASS) || match(CSharpLexer::TokenType::STRUCT))
        {
            parseClassIntoNamespace(ns, nsPath, hasAbstract);
        }
        else if (match(CSharpLexer::TokenType::RECORD))
        {
            parseRecordIntoNamespace(ns, nsPath, hasAbstract);
        }
        else if (match(CSharpLexer::TokenType::ENUM))
        {
            parseEnumIntoNamespace(ns, nsPath);
        }
        else if (match(CSharpLexer::TokenType::INTERFACE))
        {
            // Skip interfaces for now
            advance();
            skipToEndOfBlock();
        }
        else if (!isAtEnd())
        {
            advance(); // Skip unknown token
        }
    }

    void parseClass(Ast& ast, const std::vector<std::string>& nsPath, bool hasAbstract = false)
    {
        advance(); // consume class/struct

        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected class name");

        Struct s;
        s.name = advance().value;
        s.namespaces = nsPath;

        // Set record and abstract flags
        s.isRecord = false;
        s.isAbstract = hasAbstract;

        // Parse inheritance/implements clause
        if (match(CSharpLexer::TokenType::COLON))
        {
            advance();
            // Get the first type (base class)
            if (match(CSharpLexer::TokenType::ID))
            {
                s.baseType = advance().value;
                // Skip any generic parameters or interfaces
                while (!match(CSharpLexer::TokenType::LBRACE) &&
                       !match(CSharpLexer::TokenType::SEMICOLON) && !isAtEnd())
                    advance();
            }
        }

        consume(CSharpLexer::TokenType::LBRACE, "Expected '{'");

        // Parse class body
        while (!match(CSharpLexer::TokenType::RBRACE) && !isAtEnd())
        {
            // Skip modifiers
            bool isPublic = false;
            while (
                match(CSharpLexer::TokenType::PUBLIC) || match(CSharpLexer::TokenType::PRIVATE) ||
                match(CSharpLexer::TokenType::PROTECTED) ||
                match(CSharpLexer::TokenType::INTERNAL) || match(CSharpLexer::TokenType::STATIC) ||
                match(CSharpLexer::TokenType::READONLY) || match(CSharpLexer::TokenType::CONST))
            {
                if (match(CSharpLexer::TokenType::PUBLIC))
                    isPublic = true;
                advance();
            }

            // Parse field/property
            if (match(CSharpLexer::TokenType::ID))
            {
                std::unique_ptr<Type> fieldType = parseType();

                if (!match(CSharpLexer::TokenType::ID))
                {
                    // Not a field, skip
                    skipToEndOfStatement();
                    continue;
                }

                std::string fieldName = advance().value;

                // Check for property { get; set; } or field ;
                if (match(CSharpLexer::TokenType::LBRACE))
                {
                    // Property
                    advance(); // consume {
                    while (!match(CSharpLexer::TokenType::RBRACE) && !isAtEnd())
                        advance();
                    if (match(CSharpLexer::TokenType::RBRACE))
                        advance();

                    if (isPublic)
                    {
                        Field f;
                        f.name = fieldName;
                        f.type = std::move(fieldType);
                        s.members.push_back(std::move(f));
                    }
                }
                else if (match(CSharpLexer::TokenType::SEMICOLON))
                {
                    // Field
                    advance();

                    if (isPublic)
                    {
                        Field f;
                        f.name = fieldName;
                        f.type = std::move(fieldType);
                        s.members.push_back(std::move(f));
                    }
                }
                else
                {
                    skipToEndOfStatement();
                }
            }
            else
            {
                // Method, nested type, etc - skip
                skipToEndOfBlock();
            }
        }

        consume(CSharpLexer::TokenType::RBRACE, "Expected '}'");

        ast.nodes.push_back(std::move(s));
    }

    void parseClassIntoNamespace(Namespace& ns,
                                 const std::vector<std::string>& nsPath,
                                 bool hasAbstract = false)
    {
        advance(); // consume class/struct

        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected class name");

        Struct s;
        s.name = advance().value;
        s.namespaces = nsPath;

        // Set record and abstract flags
        s.isRecord = false;
        s.isAbstract = hasAbstract;

        // Parse inheritance/implements clause
        if (match(CSharpLexer::TokenType::COLON))
        {
            advance();
            // Get the first type (base class)
            if (match(CSharpLexer::TokenType::ID))
            {
                s.baseType = advance().value;
                // Skip any generic parameters or interfaces
                while (!match(CSharpLexer::TokenType::LBRACE) &&
                       !match(CSharpLexer::TokenType::SEMICOLON) && !isAtEnd())
                    advance();
            }
        }

        consume(CSharpLexer::TokenType::LBRACE, "Expected '{'");

        // Parse class body
        while (!match(CSharpLexer::TokenType::RBRACE) && !isAtEnd())
        {
            // Skip modifiers
            bool isPublic = false;
            while (
                match(CSharpLexer::TokenType::PUBLIC) || match(CSharpLexer::TokenType::PRIVATE) ||
                match(CSharpLexer::TokenType::PROTECTED) ||
                match(CSharpLexer::TokenType::INTERNAL) || match(CSharpLexer::TokenType::STATIC) ||
                match(CSharpLexer::TokenType::READONLY) || match(CSharpLexer::TokenType::CONST))
            {
                if (match(CSharpLexer::TokenType::PUBLIC))
                    isPublic = true;
                advance();
            }

            // Parse field/property
            if (match(CSharpLexer::TokenType::ID))
            {
                std::unique_ptr<Type> fieldType = parseType();

                if (!match(CSharpLexer::TokenType::ID))
                {
                    // Not a field, skip
                    skipToEndOfStatement();
                    continue;
                }

                std::string fieldName = advance().value;

                // Check for property { get; set; } or field ;
                if (match(CSharpLexer::TokenType::LBRACE))
                {
                    // Property
                    advance(); // consume {
                    while (!match(CSharpLexer::TokenType::RBRACE) && !isAtEnd())
                        advance();
                    if (match(CSharpLexer::TokenType::RBRACE))
                        advance();

                    if (isPublic)
                    {
                        Field f;
                        f.name = fieldName;
                        f.type = std::move(fieldType);
                        s.members.push_back(std::move(f));
                    }
                }
                else if (match(CSharpLexer::TokenType::SEMICOLON))
                {
                    // Field
                    advance();

                    if (isPublic)
                    {
                        Field f;
                        f.name = fieldName;
                        f.type = std::move(fieldType);
                        s.members.push_back(std::move(f));
                    }
                }
                else
                {
                    skipToEndOfStatement();
                }
            }
            else
            {
                // Method, nested type, etc - skip
                skipToEndOfBlock();
            }
        }

        consume(CSharpLexer::TokenType::RBRACE, "Expected '}'");

        ns.nodes.push_back(std::move(s));
    }

    void parseRecord(Ast& ast, const std::vector<std::string>& nsPath, bool hasAbstract = false)
    {
        advance(); // consume record

        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected record name");

        Struct s;
        s.name = advance().value;
        s.namespaces = nsPath;

        // Set record and abstract flags
        s.isRecord = true;
        s.isAbstract = hasAbstract;

        // Parse inheritance (: BaseType) if present
        if (match(CSharpLexer::TokenType::COLON))
        {
            advance();
            if (match(CSharpLexer::TokenType::ID))
            {
                s.baseType = advance().value;
            }
        }

        // Parse positional parameters
        if (match(CSharpLexer::TokenType::LPAREN))
        {
            advance(); // consume (

            while (!match(CSharpLexer::TokenType::RPAREN) && !isAtEnd())
            {
                std::unique_ptr<Type> paramType = parseType();

                if (!match(CSharpLexer::TokenType::ID))
                    break;

                std::string paramName = advance().value;

                Field f;
                f.name = paramName;
                f.type = std::move(paramType);
                s.members.push_back(std::move(f));

                if (match(CSharpLexer::TokenType::COMMA))
                    advance();
            }

            if (match(CSharpLexer::TokenType::RPAREN))
                advance();
        }

        // Parse inheritance after parameters (for primary constructor records)
        // Example: record Name(Type Value) : BaseType;
        if (match(CSharpLexer::TokenType::COLON))
        {
            advance();
            if (match(CSharpLexer::TokenType::ID))
            {
                s.baseType = advance().value;
            }
        }

        // Skip remaining tokens to semicolon or brace
        while (!match(CSharpLexer::TokenType::SEMICOLON) &&
               !match(CSharpLexer::TokenType::LBRACE) && !isAtEnd())
        {
            advance();
        }

        if (match(CSharpLexer::TokenType::SEMICOLON))
            advance();
        else if (match(CSharpLexer::TokenType::LBRACE))
            skipToEndOfBlock();

        ast.nodes.push_back(std::move(s));
    }

    void parseRecordIntoNamespace(Namespace& ns,
                                  const std::vector<std::string>& nsPath,
                                  bool hasAbstract = false)
    {
        advance(); // consume record

        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected record name");

        Struct s;
        s.name = advance().value;
        s.namespaces = nsPath;

        // Set record and abstract flags
        s.isRecord = true;
        s.isAbstract = hasAbstract;

        // Parse inheritance (: BaseType) if present
        if (match(CSharpLexer::TokenType::COLON))
        {
            advance();
            if (match(CSharpLexer::TokenType::ID))
            {
                s.baseType = advance().value;
            }
        }

        // Parse positional parameters
        if (match(CSharpLexer::TokenType::LPAREN))
        {
            advance(); // consume (

            while (!match(CSharpLexer::TokenType::RPAREN) && !isAtEnd())
            {
                std::unique_ptr<Type> paramType = parseType();

                if (!match(CSharpLexer::TokenType::ID))
                    break;

                std::string paramName = advance().value;

                Field f;
                f.name = paramName;
                f.type = std::move(paramType);
                s.members.push_back(std::move(f));

                if (match(CSharpLexer::TokenType::COMMA))
                    advance();
            }

            if (match(CSharpLexer::TokenType::RPAREN))
                advance();
        }

        // Parse inheritance after parameters (for primary constructor records)
        // Example: record Name(Type Value) : BaseType;
        if (match(CSharpLexer::TokenType::COLON))
        {
            advance();
            if (match(CSharpLexer::TokenType::ID))
            {
                s.baseType = advance().value;
            }
        }

        // Skip remaining tokens to semicolon or brace
        while (!match(CSharpLexer::TokenType::SEMICOLON) &&
               !match(CSharpLexer::TokenType::LBRACE) && !isAtEnd())
        {
            advance();
        }

        if (match(CSharpLexer::TokenType::SEMICOLON))
            advance();
        else if (match(CSharpLexer::TokenType::LBRACE))
            skipToEndOfBlock();

        ns.nodes.push_back(std::move(s));
    }

    void parseEnum(Ast& ast, const std::vector<std::string>& nsPath)
    {
        advance(); // consume enum

        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected enum name");

        Enum e;
        e.name = advance().value;
        e.namespaces = nsPath;
        e.scoped = true;

        consume(CSharpLexer::TokenType::LBRACE, "Expected '{'");

        int nextValue = 0;
        while (!match(CSharpLexer::TokenType::RBRACE) && !isAtEnd())
        {
            if (!match(CSharpLexer::TokenType::ID))
                break;

            EnumValue ev;
            ev.name = advance().value;

            if (match(CSharpLexer::TokenType::EQUALS))
            {
                advance();
                if (match(CSharpLexer::TokenType::NUMBER))
                {
                    ev.number = std::stoi(advance().value);
                    nextValue = ev.number + 1;
                }
            }
            else
            {
                ev.number = nextValue++;
            }

            e.values.push_back(std::move(ev));

            if (match(CSharpLexer::TokenType::COMMA))
                advance();
        }

        consume(CSharpLexer::TokenType::RBRACE, "Expected '}'");

        ast.nodes.push_back(std::move(e));
    }

    void parseEnumIntoNamespace(Namespace& ns, const std::vector<std::string>& nsPath)
    {
        advance(); // consume enum

        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected enum name");

        Enum e;
        e.name = advance().value;
        e.namespaces = nsPath;
        e.scoped = true;

        consume(CSharpLexer::TokenType::LBRACE, "Expected '{'");

        int nextValue = 0;
        while (!match(CSharpLexer::TokenType::RBRACE) && !isAtEnd())
        {
            if (!match(CSharpLexer::TokenType::ID))
                break;

            EnumValue ev;
            ev.name = advance().value;

            if (match(CSharpLexer::TokenType::EQUALS))
            {
                advance();
                if (match(CSharpLexer::TokenType::NUMBER))
                {
                    ev.number = std::stoi(advance().value);
                    nextValue = ev.number + 1;
                }
            }
            else
            {
                ev.number = nextValue++;
            }

            e.values.push_back(std::move(ev));

            if (match(CSharpLexer::TokenType::COMMA))
                advance();
        }

        consume(CSharpLexer::TokenType::RBRACE, "Expected '}'");

        ns.nodes.push_back(std::move(e));
    }

    std::unique_ptr<Type> parseType()
    {
        if (!match(CSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected type");

        std::string typeName = advance().value;

        // Handle generic types: List<T>, Dictionary<K,V>
        if (match(CSharpLexer::TokenType::LANGLE))
        {
            advance(); // consume

            std::vector<std::unique_ptr<Type>> args;
            args.push_back(parseType());

            while (match(CSharpLexer::TokenType::COMMA))
            {
                advance();
                args.push_back(parseType());
            }

            consume(CSharpLexer::TokenType::RANGLE, "Expected '>'");

            GenericType gt;
            if (typeName == "List")
                gt.reifiedType = ReifiedTypeId::List;
            else if (typeName == "Dictionary")
                gt.reifiedType = ReifiedTypeId::Map;
            else if (typeName == "HashSet")
                gt.reifiedType = ReifiedTypeId::Set;
            else
                gt.reifiedType = ReifiedTypeId::Unknown;

            gt.args = std::move(args);

            std::unique_ptr<Type> result = std::make_unique<Type>(std::move(gt));

            // Check for array: List<int>[]
            if (match(CSharpLexer::TokenType::LBRACKET))
            {
                advance();
                consume(CSharpLexer::TokenType::RBRACKET, "Expected ']'");

                GenericType arrayGt;
                arrayGt.reifiedType = ReifiedTypeId::List;
                arrayGt.args.push_back(std::move(result));
                result = std::make_unique<Type>(std::move(arrayGt));
            }

            // Check for nullable: List<int>?
            if (match(CSharpLexer::TokenType::QUESTION))
            {
                advance();

                GenericType optGt;
                optGt.reifiedType = ReifiedTypeId::Optional;
                optGt.args.push_back(std::move(result));
                result = std::make_unique<Type>(std::move(optGt));
            }

            return result;
        }

        // Handle arrays: int[]
        if (match(CSharpLexer::TokenType::LBRACKET))
        {
            advance();
            consume(CSharpLexer::TokenType::RBRACKET, "Expected ']'");

            // Special case: byte[] = Bytes (check for nullable byte[]?)
            if (typeName == "byte")
            {
                SimpleType st;
                st.srcTypeString = "byte[]";
                st.reifiedType = ReifiedTypeId::Bytes;
                std::unique_ptr<Type> result = std::make_unique<Type>(std::move(st));

                // Check for nullable: byte[]?
                if (match(CSharpLexer::TokenType::QUESTION))
                {
                    advance();

                    GenericType optGt;
                    optGt.reifiedType = ReifiedTypeId::Optional;
                    optGt.args.push_back(std::move(result));
                    result = std::make_unique<Type>(std::move(optGt));
                }

                return result;
            }

            // Regular arrays become List<T>
            ReifiedTypeId rid = mapToReified(typeName);
            std::unique_ptr<Type> baseType;

            if (rid == ReifiedTypeId::StructRefType)
            {
                StructRefType ref;
                ref.srcTypeString = typeName;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                baseType = std::make_unique<Type>(std::move(ref));
            }
            else
            {
                SimpleType st;
                st.srcTypeString = typeName;
                st.reifiedType = rid;
                baseType = std::make_unique<Type>(std::move(st));
            }

            GenericType gt;
            gt.reifiedType = ReifiedTypeId::List;
            gt.args.push_back(std::move(baseType));

            std::unique_ptr<Type> result = std::make_unique<Type>(std::move(gt));

            // Check for nullable: int[]?
            if (match(CSharpLexer::TokenType::QUESTION))
            {
                advance();

                GenericType optGt;
                optGt.reifiedType = ReifiedTypeId::Optional;
                optGt.args.push_back(std::move(result));
                result = std::make_unique<Type>(std::move(optGt));
            }

            return result;
        }

        // Handle nullable: int?
        if (match(CSharpLexer::TokenType::QUESTION))
        {
            advance();

            ReifiedTypeId rid = mapToReified(typeName);
            std::unique_ptr<Type> baseType;

            if (rid == ReifiedTypeId::StructRefType)
            {
                StructRefType ref;
                ref.srcTypeString = typeName;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                baseType = std::make_unique<Type>(std::move(ref));
            }
            else
            {
                SimpleType st;
                st.srcTypeString = typeName;
                st.reifiedType = rid;
                baseType = std::make_unique<Type>(std::move(st));
            }

            GenericType gt;
            gt.reifiedType = ReifiedTypeId::Optional;
            gt.args.push_back(std::move(baseType));

            return std::make_unique<Type>(std::move(gt));
        }

        // Simple type - check if it's a user-defined type or primitive
        ReifiedTypeId rid = mapToReified(typeName);
        if (rid == ReifiedTypeId::StructRefType)
        {
            StructRefType ref;
            ref.srcTypeString = typeName;
            ref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(ref));
        }

        SimpleType st;
        st.srcTypeString = typeName;
        st.reifiedType = rid;
        return std::make_unique<Type>(std::move(st));
    }

    ReifiedTypeId mapToReified(const std::string& t)
    {
        if (t == "bool")
            return ReifiedTypeId::Bool;
        if (t == "byte")
            return ReifiedTypeId::UInt8;
        if (t == "sbyte")
            return ReifiedTypeId::Int8;
        if (t == "short")
            return ReifiedTypeId::Int16;
        if (t == "ushort")
            return ReifiedTypeId::UInt16;
        if (t == "int")
            return ReifiedTypeId::Int32;
        if (t == "uint")
            return ReifiedTypeId::UInt32;
        if (t == "long")
            return ReifiedTypeId::Int64;
        if (t == "ulong")
            return ReifiedTypeId::UInt64;
        if (t == "float")
            return ReifiedTypeId::Float32;
        if (t == "double")
            return ReifiedTypeId::Float64;
        if (t == "string")
            return ReifiedTypeId::String;
        if (t == "char")
            return ReifiedTypeId::Char;
        if (t == "decimal")
            return ReifiedTypeId::Decimal;
        return ReifiedTypeId::StructRefType;
    }

    void skipToEndOfStatement()
    {
        while (!match(CSharpLexer::TokenType::SEMICOLON) && !isAtEnd())
            advance();
        if (match(CSharpLexer::TokenType::SEMICOLON))
            advance();
    }

    void skipToEndOfBlock()
    {
        if (match(CSharpLexer::TokenType::LBRACE))
        {
            advance();
            int depth = 1;
            while (depth > 0 && !isAtEnd())
            {
                if (match(CSharpLexer::TokenType::LBRACE))
                    depth++;
                else if (match(CSharpLexer::TokenType::RBRACE))
                    depth--;
                advance();
            }
        }
        else
        {
            skipToEndOfStatement();
        }
    }
};

} // namespace bhw