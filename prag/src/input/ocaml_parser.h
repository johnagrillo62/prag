// ocaml_parser_proper.h - Proper OCaml parser with lexer
#pragma once
#include <stdexcept>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "languages.h"

namespace bhw
{

class OCamlLexer
{
  public:
    enum class TokenType
    {
        MODULE,
        STRUCT,
        END,
        TYPE,
        EQUALS,
        LBRACE,
        RBRACE,
        COLON,
        SEMICOLON,
        PIPE,
        OF,
        LPAREN,
        RPAREN,
        COMMA,
        DOT,
        LANGLE,
        RANGLE,
        ID,
        COMMENT,
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

            // Comments: (* ... *)
            if (pos + 1 < source.size() && source[pos] == '(' && source[pos + 1] == '*')
            {
                int depth = 1;
                pos += 2;
                col += 2;

                while (pos < source.size() && depth > 0)
                {
                    if (pos + 1 < source.size())
                    {
                        if (source[pos] == '(' && source[pos + 1] == '*')
                        {
                            depth++;
                            pos += 2;
                            col += 2;
                            continue;
                        }
                        if (source[pos] == '*' && source[pos + 1] == ')')
                        {
                            depth--;
                            pos += 2;
                            col += 2;
                            continue;
                        }
                    }
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
                }
                continue;
            }

            // Single char tokens
            if (source[pos] == '=')
            {
                tokens.push_back({TokenType::EQUALS, "=", line, col});
                pos++;
                col++;
                continue;
            }
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
            if (source[pos] == ':')
            {
                tokens.push_back({TokenType::COLON, ":", line, col});
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
            if (source[pos] == '|')
            {
                tokens.push_back({TokenType::PIPE, "|", line, col});
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

            // Identifiers and keywords
            if (std::isalpha(source[pos]) || source[pos] == '_')
            {
                size_t start = pos;
                int startCol = col;

                while (pos < source.size() &&
                       (std::isalnum(source[pos]) || source[pos] == '_' || source[pos] == '\''))
                {
                    pos++;
                    col++;
                }

                std::string word = source.substr(start, pos - start);
                TokenType type = TokenType::ID;

                if (word == "module")
                    type = TokenType::MODULE;
                else if (word == "struct")
                    type = TokenType::STRUCT;
                else if (word == "end")
                    type = TokenType::END;
                else if (word == "type")
                    type = TokenType::TYPE;
                else if (word == "of")
                    type = TokenType::OF;

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

class OCamlParser : public AstParser, public AutoRegisterParser<OCamlParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"ml"};
    }
 
  public:
    Ast parseToAst(const std::string& src) override
    {
        OCamlLexer lexer;
        tokens = lexer.tokenize(src);
        pos = 0;

        while (!isAtEnd())
        {
            if (match(OCamlLexer::TokenType::MODULE))
            {
                Namespace ns = parseModule();
                nodes.push_back(std::move(ns));
            }
            else if (match(OCamlLexer::TokenType::TYPE))
            {
                parseTopLevelType();
            }
            else
            {
                advance();
            }
        }

        Ast ast;
        ast.srcName = "ocaml";

        // Preserve source order by using the nodes vector directly
        ast.nodes = std::move(nodes);

        return ast;
    }

    auto getLang() -> bhw::Language override
    {
        return Language::OCaml;
    }

  private:
    std::string src;
    std::vector<OCamlLexer::Token> tokens;
    size_t pos;

    // Single ordered vector to preserve declaration order
    std::vector<AstRootNode> nodes;
    std::string currentNamespace;

    bool isAtEnd() const
    {
        return peek().type == OCamlLexer::TokenType::EOF_TOKEN;
    }

    OCamlLexer::Token peek() const
    {
        if (pos >= tokens.size())
            return tokens.back();
        return tokens[pos];
    }

    OCamlLexer::Token previous() const
    {
        if (pos == 0)
            return tokens[0];
        return tokens[pos - 1];
    }

    OCamlLexer::Token advance()
    {
        if (!isAtEnd())
            pos++;
        return previous();
    }

    bool match(OCamlLexer::TokenType type)
    {
        return peek().type == type;
    }

    bool consume(OCamlLexer::TokenType type, const std::string& errMsg)
    {
        if (match(type))
        {
            advance();
            return true;
        }
        throw std::runtime_error(errMsg + " at line " + std::to_string(peek().line));
    }

    Namespace parseModule()
    {
        consume(OCamlLexer::TokenType::MODULE, "Expected 'module'");

        if (!match(OCamlLexer::TokenType::ID))
            throw std::runtime_error("Expected module name");

        std::string moduleName = advance().value;
        // Capitalize first letter for namespace
        if (!moduleName.empty())
            moduleName[0] = std::toupper(moduleName[0]);

        consume(OCamlLexer::TokenType::EQUALS, "Expected '=' after module name");
        consume(OCamlLexer::TokenType::STRUCT, "Expected 'struct'");

        Namespace ns;
        ns.name = moduleName;

        std::string savedNs = currentNamespace;
        currentNamespace = moduleName;

        // Parse module body until 'end'
        while (!match(OCamlLexer::TokenType::END) && !isAtEnd())
        {
            if (match(OCamlLexer::TokenType::TYPE))
            {
                parseTypeIntoNamespace(ns);
            }
            else if (match(OCamlLexer::TokenType::MODULE))
            {
                // Parse nested module and add it to THIS namespace
                Namespace nestedNs = parseModule();
                ns.nodes.push_back(std::move(nestedNs));
            }
            else
            {
                advance();
            }
        }

        consume(OCamlLexer::TokenType::END, "Expected 'end'");

        currentNamespace = savedNs;
        return ns;
    }

    void parseTopLevelType()
    {
        consume(OCamlLexer::TokenType::TYPE, "Expected 'type'");

        if (!match(OCamlLexer::TokenType::ID))
            throw std::runtime_error("Expected type name");

        std::string typeName = advance().value;

        consume(OCamlLexer::TokenType::EQUALS, "Expected '='");

        // Check if it's a record
        if (match(OCamlLexer::TokenType::LBRACE))
        {
            Struct s = parseRecord(typeName);
            nodes.push_back(std::move(s));
        }
        else
        {
            // It's a variant or enum
            parseVariantOrEnum(typeName);
        }
    }

    void parseTypeIntoNamespace(Namespace& ns)
    {
        consume(OCamlLexer::TokenType::TYPE, "Expected 'type'");

        if (!match(OCamlLexer::TokenType::ID))
            throw std::runtime_error("Expected type name");

        std::string typeName = advance().value;

        consume(OCamlLexer::TokenType::EQUALS, "Expected '='");

        if (match(OCamlLexer::TokenType::LBRACE))
        {
            Struct s = parseRecord(typeName);
            ns.nodes.push_back(std::move(s));
        }
        else
        {
            parseVariantOrEnumIntoNamespace(typeName, ns);
        }
    }

    Struct parseRecord(const std::string& name)
    {
        Struct s;
        s.name = name;
        if (!currentNamespace.empty())
            s.namespaces.push_back(currentNamespace);

        consume(OCamlLexer::TokenType::LBRACE, "Expected '{'");

        while (!match(OCamlLexer::TokenType::RBRACE) && !isAtEnd())
        {
            if (!match(OCamlLexer::TokenType::ID))
                break;

            std::string fieldName = advance().value;

            consume(OCamlLexer::TokenType::COLON, "Expected ':' after field name");

            std::unique_ptr<Type> fieldType = parseType();

            Field f;
            f.name = fieldName;
            f.type = std::move(fieldType);
            s.members.push_back(std::move(f));

            if (match(OCamlLexer::TokenType::SEMICOLON))
                advance();
        }

        consume(OCamlLexer::TokenType::RBRACE, "Expected '}'");

        return s;
    }

    void parseVariantOrEnum(const std::string& name)
    {
        std::vector<std::pair<std::string, std::unique_ptr<Type>>> cases;

        // Parse first case
        if (match(OCamlLexer::TokenType::PIPE))
            advance();

        if (!match(OCamlLexer::TokenType::ID))
            return;

        std::string caseName = advance().value;
        std::unique_ptr<Type> caseType = nullptr;

        if (match(OCamlLexer::TokenType::OF))
        {
            advance();
            caseType = parseType();
        }

        cases.push_back({caseName, std::move(caseType)});

        // Parse remaining cases
        while (match(OCamlLexer::TokenType::PIPE) && !isAtEnd())
        {
            advance(); // consume |

            if (!match(OCamlLexer::TokenType::ID))
                break;

            std::string cn = advance().value;
            std::unique_ptr<Type> ct = nullptr;

            if (match(OCamlLexer::TokenType::OF))
            {
                advance();
                ct = parseType();
            }

            cases.push_back({cn, std::move(ct)});
        }

        // Check if all cases have no types (enum)
        bool isEnum = true;
        for (const auto& [cn, ct] : cases)
        {
            if (ct != nullptr)
            {
                isEnum = false;
                break;
            }
        }

        if (isEnum)
        {
            Enum e;
            e.name = name;
            e.scoped = true;
            if (!currentNamespace.empty())
                e.namespaces.push_back(currentNamespace);

            int num = 0;
            for (const auto& [cn, ct] : cases)
            {
                EnumValue ev;
                ev.name = cn;
                ev.number = num++;
                e.values.push_back(std::move(ev));
            }
            nodes.push_back(std::move(e));
        }
        else
        {
            Oneof o;
            o.name = name;
            for (auto& [cn, ct] : cases)
            {
                OneofField of;
                of.name = cn;
                if (ct)
                    of.type = std::move(ct);
                else
                {
                    SimpleType st;
                    st.reifiedType = ReifiedTypeId::Unknown;
                    of.type = std::make_unique<Type>(std::move(st));
                }
                o.fields.push_back(std::move(of));
            }
            nodes.push_back(std::move(o));
        }
    }

    void parseVariantOrEnumIntoNamespace(const std::string& name, Namespace& ns)
    {
        std::vector<std::pair<std::string, std::unique_ptr<Type>>> cases;

        if (match(OCamlLexer::TokenType::PIPE))
            advance();

        if (!match(OCamlLexer::TokenType::ID))
            return;

        std::string caseName = advance().value;
        std::unique_ptr<Type> caseType = nullptr;

        if (match(OCamlLexer::TokenType::OF))
        {
            advance();
            caseType = parseType();
        }

        cases.push_back({caseName, std::move(caseType)});

        while (match(OCamlLexer::TokenType::PIPE) && !isAtEnd())
        {
            advance();

            if (!match(OCamlLexer::TokenType::ID))
                break;

            std::string cn = advance().value;
            std::unique_ptr<Type> ct = nullptr;

            if (match(OCamlLexer::TokenType::OF))
            {
                advance();
                ct = parseType();
            }

            cases.push_back({cn, std::move(ct)});
        }

        bool isEnum = true;
        for (const auto& [cn, ct] : cases)
        {
            if (ct != nullptr)
            {
                isEnum = false;
                break;
            }
        }

        if (isEnum)
        {
            Enum e;
            e.name = name;
            e.scoped = true;
            if (!currentNamespace.empty())
                e.namespaces.push_back(currentNamespace);

            int num = 0;
            for (const auto& [cn, ct] : cases)
            {
                EnumValue ev;
                ev.name = cn;
                ev.number = num++;
                e.values.push_back(std::move(ev));
            }
            ns.nodes.push_back(std::move(e));
        }
        else
        {
            Oneof o;
            o.name = name;
            for (auto& [cn, ct] : cases)
            {
                OneofField of;
                of.name = cn;
                if (ct)
                    of.type = std::move(ct);
                else
                {
                    SimpleType st;
                    st.reifiedType = ReifiedTypeId::Unknown;
                    of.type = std::make_unique<Type>(std::move(st));
                }
                o.fields.push_back(std::move(of));
            }
            ns.nodes.push_back(std::move(o));
        }
    }

    std::unique_ptr<Type> parseType()
    {
        // Handle <anonymous> struct references
        if (match(OCamlLexer::TokenType::LANGLE))
        {
            advance(); // consume

            if (!match(OCamlLexer::TokenType::ID))
                throw std::runtime_error("Expected identifier after <");

            std::string name = advance().value;

            consume(OCamlLexer::TokenType::RANGLE, "Expected > after anonymous type name");

            // Create a StructRefType with the <name> format preserved
            StructRefType ref;
            ref.srcTypeString = "<" + name + ">";
            ref.reifiedType = ReifiedTypeId::StructRefType;
            return std::make_unique<Type>(std::move(ref));
        }

        // Handle parenthesized types: (type1, type2) Constructor.t
        if (match(OCamlLexer::TokenType::LPAREN))
        {
            advance(); // consume (

            // Parse first type argument
            std::unique_ptr<Type> keyType = parseType();

            consume(OCamlLexer::TokenType::COMMA, "Expected ',' in type constructor args");

            // Parse second type argument
            std::unique_ptr<Type> valueType = parseType();

            consume(OCamlLexer::TokenType::RPAREN, "Expected ')' after type constructor args");

            // Now expect the type constructor (e.g., "Map")
            if (!match(OCamlLexer::TokenType::ID))
                throw std::runtime_error("Expected type constructor after parenthesized args");

            std::string constructor = advance().value;

            // Skip ".t" if present
            if (match(OCamlLexer::TokenType::DOT))
            {
                advance(); // consume .
                if (match(OCamlLexer::TokenType::ID) && peek().value == "t")
                {
                    advance(); // consume t
                }
            }

            GenericType gt;
            gt.reifiedType = ReifiedTypeId::Map;
            gt.args.push_back(std::move(keyType));
            gt.args.push_back(std::move(valueType));

            return std::make_unique<Type>(std::move(gt));
        }

        // Handle simple types and postfix constructors
        if (!match(OCamlLexer::TokenType::ID))
            throw std::runtime_error("Expected type");

        std::string typeName = advance().value;

        // Start with base type
        std::unique_ptr<Type> currentType;
        ReifiedTypeId rid = mapToReified(typeName);
        if (rid == ReifiedTypeId::StructRefType)
        {
            StructRefType ref;
            ref.srcTypeString = typeName;
            ref.reifiedType = ReifiedTypeId::StructRefType;
            currentType = std::make_unique<Type>(std::move(ref));
        }
        else
        {
            SimpleType st;
            st.srcTypeString = typeName;
            st.reifiedType = rid;
            currentType = std::make_unique<Type>(std::move(st));
        }

        // Keep wrapping in type constructors as long as we see them
        while (match(OCamlLexer::TokenType::ID))
        {
            std::string constructor = peek().value;

            if (constructor == "list")
            {
                advance();
                GenericType gt;
                gt.reifiedType = ReifiedTypeId::List;
                gt.args.push_back(std::move(currentType));
                currentType = std::make_unique<Type>(std::move(gt));
            }
            else if (constructor == "option")
            {
                advance();
                GenericType gt;
                gt.reifiedType = ReifiedTypeId::Optional;
                gt.args.push_back(std::move(currentType));
                currentType = std::make_unique<Type>(std::move(gt));
            }
            else if (constructor == "array")
            {
                advance();
                GenericType gt;
                gt.reifiedType = ReifiedTypeId::Array;
                gt.args.push_back(std::move(currentType));
                currentType = std::make_unique<Type>(std::move(gt));
            }
            else
            {
                // Not a type constructor, stop
                break;
            }
        }

        return currentType;
    }

    ReifiedTypeId mapToReified(const std::string& t)
    {
        if (t == "bool")
            return ReifiedTypeId::Bool;
        if (t == "int")
            return ReifiedTypeId::Int32;
        if (t == "int64")
            return ReifiedTypeId::Int64;
        if (t == "float")
            return ReifiedTypeId::Float64;
        if (t == "string")
            return ReifiedTypeId::String;
        if (t == "char")
            return ReifiedTypeId::Char;
        if (t == "bytes")
            return ReifiedTypeId::Bytes;
        if (t == "unit")
            return ReifiedTypeId::Unknown;
        return ReifiedTypeId::StructRefType;
    }
};

} // namespace bhw

