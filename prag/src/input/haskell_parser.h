
// haskell_parser.h - Haskell type parser with proper lexer/parser
#pragma once
#include <cctype>
#include <stdexcept>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"

namespace bhw
{

class HaskellLexer
{
  public:
    enum class TokenType
    {
        DATA,
        TYPE,
        NEWTYPE,
        MODULE,
        WHERE,
        IMPORT,
        QUALIFIED,
        AS,
        DERIVING,
        LBRACE,
        RBRACE,
        LPAREN,
        RPAREN,
        LBRACKET,
        RBRACKET,
        PIPE,
        COMMA,
        DOUBLECOLON,
        EQUALS,
        ID,
        PRAGMA,
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

            // Comments: --
            if (pos + 1 < source.size() && source[pos] == '-' && source[pos + 1] == '-')
            {
                while (pos < source.size() && source[pos] != '\n')
                    pos++;
                continue;
            }

            // Block comments: {- ... -}
            if (pos + 1 < source.size() && source[pos] == '{' && source[pos + 1] == '-')
            {
                pos += 2;
                int depth = 1;
                while (pos + 1 < source.size() && depth > 0)
                {
                    if (source[pos] == '{' && source[pos + 1] == '-')
                    {
                        depth++;
                        pos += 2;
                    }
                    else if (source[pos] == '-' && source[pos + 1] == '}')
                    {
                        depth--;
                        pos += 2;
                    }
                    else
                    {
                        if (source[pos] == '\n')
                        {
                            line++;
                            col = 1;
                        }
                        pos++;
                    }
                }
                continue;
            }

            // Pragma: {-# ... #-}
            if (pos + 2 < source.size() && source[pos] == '{' && source[pos + 1] == '-' &&
                source[pos + 2] == '#')
            {
                size_t start = pos;
                while (pos + 2 < source.size())
                {
                    if (source[pos] == '#' && source[pos + 1] == '-' && source[pos + 2] == '}')
                    {
                        pos += 3;
                        break;
                    }
                    pos++;
                }
                tokens.push_back({TokenType::PRAGMA, source.substr(start, pos - start), line, col});
                continue;
            }

            // Double colon ::
            if (pos + 1 < source.size() && source[pos] == ':' && source[pos + 1] == ':')
            {
                tokens.push_back({TokenType::DOUBLECOLON, "::", line, col});
                pos += 2;
                col += 2;
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
            if (source[pos] == '|')
            {
                tokens.push_back({TokenType::PIPE, "|", line, col});
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
            if (source[pos] == '=')
            {
                tokens.push_back({TokenType::EQUALS, "=", line, col});
                pos++;
                col++;
                continue;
            }

            // Identifiers and keywords (can start with uppercase or lowercase)
            if (std::isalpha(source[pos]) || source[pos] == '_')
            {
                size_t start = pos;
                int startCol = col;

                while (pos < source.size() && (std::isalnum(source[pos]) || source[pos] == '_' ||
                                               source[pos] == '\'' || source[pos] == '.'))
                {
                    pos++;
                    col++;
                }

                std::string word = source.substr(start, pos - start);
                TokenType type = TokenType::ID;

                if (word == "data")
                    type = TokenType::DATA;
                else if (word == "type")
                    type = TokenType::TYPE;
                else if (word == "newtype")
                    type = TokenType::NEWTYPE;
                else if (word == "module")
                    type = TokenType::MODULE;
                else if (word == "where")
                    type = TokenType::WHERE;
                else if (word == "import")
                    type = TokenType::IMPORT;
                else if (word == "qualified")
                    type = TokenType::QUALIFIED;
                else if (word == "as")
                    type = TokenType::AS;
                else if (word == "deriving")
                    type = TokenType::DERIVING;

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

class HaskellParser : public AstParser, public AutoRegisterParser<HaskellParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"hs"};
    }
  public:

    Ast parseToAst(const std::string& src) override
    {
        HaskellLexer lexer;
        tokens = lexer.tokenize(src);
        pos = 0;

        Ast ast;
        ast.srcName = "haskell";

        parseFile(ast);

        return ast;
    }

    auto getLang() -> bhw::Language override
    {
        return Language::Haskell;
    }

  private:
    std::string src;
    std::vector<HaskellLexer::Token> tokens;
    size_t pos;

    bool isAtEnd() const
    {
        return peek().type == HaskellLexer::TokenType::EOF_TOKEN;
    }

    HaskellLexer::Token peek() const
    {
        if (pos >= tokens.size())
            return tokens.back();
        return tokens[pos];
    }

    HaskellLexer::Token previous() const
    {
        if (pos == 0)
            return tokens[0];
        return tokens[pos - 1];
    }

    HaskellLexer::Token advance()
    {
        if (!isAtEnd())
            pos++;
        return previous();
    }

    bool match(HaskellLexer::TokenType type)
    {
        return peek().type == type;
    }

    bool consume(HaskellLexer::TokenType type, const std::string& errMsg)
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
            // Skip pragmas
            if (match(HaskellLexer::TokenType::PRAGMA))
            {
                advance();
                continue;
            }

            // Skip module declarations
            if (match(HaskellLexer::TokenType::MODULE))
            {
                advance();
                // Skip until 'where'
                while (!match(HaskellLexer::TokenType::WHERE) && !isAtEnd())
                    advance();
                if (match(HaskellLexer::TokenType::WHERE))
                    advance();
                continue;
            }

            // Skip imports
            if (match(HaskellLexer::TokenType::IMPORT))
            {
                advance();
                // Skip entire import line
                while (!isAtEnd())
                {
                    auto token = advance();
                    // Import ends at next data/type/newtype/import or EOF
                    if (match(HaskellLexer::TokenType::DATA) ||
                        match(HaskellLexer::TokenType::TYPE) ||
                        match(HaskellLexer::TokenType::NEWTYPE) ||
                        match(HaskellLexer::TokenType::IMPORT))
                        break;
                }
                continue;
            }

            // Parse data declarations
            if (match(HaskellLexer::TokenType::DATA))
            {
                parseData(ast);
                continue;
            }

            // Parse type aliases
            if (match(HaskellLexer::TokenType::TYPE))
            {
                parseTypeAlias(ast);
                continue;
            }

            // Parse newtype
            if (match(HaskellLexer::TokenType::NEWTYPE))
            {
                parseNewtype(ast);
                continue;
            }

            // Skip unknown tokens
            if (!isAtEnd())
                advance();
        }
    }

    void parseData(Ast& ast)
    {
        consume(HaskellLexer::TokenType::DATA, "Expected 'data'");

        if (!match(HaskellLexer::TokenType::ID))
            throw std::runtime_error("Expected type name");

        std::string typeName = advance().value;

        consume(HaskellLexer::TokenType::EQUALS, "Expected '='");

        // Check if it's a record type (has braces)
        // Look ahead to see if we have a record
        size_t savedPos = pos;
        bool isRecord = false;

        // Skip constructor name if present
        if (match(HaskellLexer::TokenType::ID))
            advance();

        if (match(HaskellLexer::TokenType::LBRACE))
            isRecord = true;

        pos = savedPos; // Restore position

        if (isRecord)
        {
            parseRecord(typeName, ast);
        }
        else
        {
            parseADT(typeName, ast);
        }

        // Skip deriving clause
        if (match(HaskellLexer::TokenType::DERIVING))
        {
            advance();
            if (match(HaskellLexer::TokenType::LPAREN))
            {
                advance();
                while (!match(HaskellLexer::TokenType::RPAREN) && !isAtEnd())
                    advance();
                if (match(HaskellLexer::TokenType::RPAREN))
                    advance();
            }
            else if (match(HaskellLexer::TokenType::ID))
            {
                advance(); // Single deriving class
            }
        }
    }

    void parseRecord(const std::string& typeName, Ast& ast)
    {
        Struct s;
        s.name = typeName;

        // Skip constructor name (usually same as type name)
        if (match(HaskellLexer::TokenType::ID))
            advance();

        consume(HaskellLexer::TokenType::LBRACE, "Expected '{'");

        while (!match(HaskellLexer::TokenType::RBRACE) && !isAtEnd())
        {
            if (!match(HaskellLexer::TokenType::ID))
                break;

            std::string fieldName = advance().value;

            consume(HaskellLexer::TokenType::DOUBLECOLON, "Expected '::'");

            std::unique_ptr<Type> fieldType = parseType();

            Field f;
            f.name = fieldName;
            f.type = std::move(fieldType);
            s.members.push_back(std::move(f));

            if (match(HaskellLexer::TokenType::COMMA))
                advance();
        }

        consume(HaskellLexer::TokenType::RBRACE, "Expected '}'");

        ast.nodes.push_back(std::move(s));
    }

    void parseADT(const std::string& typeName, Ast& ast)
    {
        std::vector<std::pair<std::string, std::unique_ptr<Type>>> constructors;

        // Parse first constructor
        if (!match(HaskellLexer::TokenType::ID))
            return;

        std::string conName = advance().value;
        std::unique_ptr<Type> conType = nullptr;

        // Check if constructor has a type argument
        if (match(HaskellLexer::TokenType::ID) || match(HaskellLexer::TokenType::LBRACKET) ||
            match(HaskellLexer::TokenType::LPAREN))
        {
            conType = parseType();
        }

        constructors.push_back({conName, std::move(conType)});

        // Parse remaining constructors
        while (match(HaskellLexer::TokenType::PIPE) && !isAtEnd())
        {
            advance(); // consume |

            if (!match(HaskellLexer::TokenType::ID))
                break;

            std::string cn = advance().value;
            std::unique_ptr<Type> ct = nullptr;

            if (match(HaskellLexer::TokenType::ID) || match(HaskellLexer::TokenType::LBRACKET) ||
                match(HaskellLexer::TokenType::LPAREN))
            {
                ct = parseType();
            }

            constructors.push_back({cn, std::move(ct)});
        }

        // Check if it's an enum (no types) or ADT
        bool isEnum = true;
        for (const auto& [name, type] : constructors)
        {
            if (type != nullptr)
            {
                isEnum = false;
                break;
            }
        }

        if (isEnum)
        {
            Enum e;
            e.name = typeName;
            e.scoped = true;

            int num = 0;
            for (const auto& [name, type] : constructors)
            {
                EnumValue ev;
                ev.name = name;
                ev.number = num++;
                e.values.push_back(std::move(ev));
            }

            ast.nodes.push_back(std::move(e));
        }
        else
        {
            Oneof o;
            o.name = typeName;

            for (auto& [name, type] : constructors)
            {
                OneofField of;
                of.name = name;
                if (type)
                    of.type = std::move(type);
                else
                {
                    SimpleType st;
                    st.reifiedType = ReifiedTypeId::Unknown;
                    of.type = std::make_unique<Type>(std::move(st));
                }
                o.fields.push_back(std::move(of));
            }

            ast.nodes.push_back(std::move(o));
        }
    }

    void parseTypeAlias(Ast& ast)
    {
        // Skip type aliases for now
        advance(); // consume 'type'
        while (!isAtEnd() && !match(HaskellLexer::TokenType::DATA) &&
               !match(HaskellLexer::TokenType::TYPE) && !match(HaskellLexer::TokenType::NEWTYPE))
        {
            advance();
        }
    }

    void parseNewtype(Ast& ast)
    {
        consume(HaskellLexer::TokenType::NEWTYPE, "Expected 'newtype'");

        if (!match(HaskellLexer::TokenType::ID))
            throw std::runtime_error("Expected type name");

        Struct s;
        s.name = advance().value;

        consume(HaskellLexer::TokenType::EQUALS, "Expected '='");

        // Skip constructor name
        if (match(HaskellLexer::TokenType::ID))
            advance();

        // Parse wrapped type
        std::unique_ptr<Type> wrappedType = parseType();

        Field f;
        f.name = "value";
        f.type = std::move(wrappedType);
        s.members.push_back(std::move(f));

        ast.nodes.push_back(std::move(s));

        // Skip deriving
        if (match(HaskellLexer::TokenType::DERIVING))
        {
            advance();
            if (match(HaskellLexer::TokenType::LPAREN))
            {
                advance();
                while (!match(HaskellLexer::TokenType::RPAREN) && !isAtEnd())
                    advance();
                if (match(HaskellLexer::TokenType::RPAREN))
                    advance();
            }
            else if (match(HaskellLexer::TokenType::ID))
            {
                advance();
            }
        }
    }

    std::unique_ptr<Type> parseType()
    {
        // Handle list: [Type]
        if (match(HaskellLexer::TokenType::LBRACKET))
        {
            advance();
            std::unique_ptr<Type> elemType = parseType();
            consume(HaskellLexer::TokenType::RBRACKET, "Expected ']'");

            GenericType gt;
            gt.reifiedType = ReifiedTypeId::List;
            gt.args.push_back(std::move(elemType));
            return std::make_unique<Type>(std::move(gt));
        }

        // Handle parenthesized types or tuples
        if (match(HaskellLexer::TokenType::LPAREN))
        {
            advance();
            std::unique_ptr<Type> innerType = parseType();
            consume(HaskellLexer::TokenType::RPAREN, "Expected ')'");
            return innerType;
        }

        if (!match(HaskellLexer::TokenType::ID))
            throw std::runtime_error("Expected type");

        std::string typeName = advance().value;

        // Handle Maybe
        if (typeName == "Maybe")
        {
            std::unique_ptr<Type> innerType = parseType();
            GenericType gt;
            gt.reifiedType = ReifiedTypeId::Optional;
            gt.args.push_back(std::move(innerType));
            return std::make_unique<Type>(std::move(gt));
        }

        // Handle Map.Map
        if (typeName == "Map.Map")
        {
            std::unique_ptr<Type> keyType = parseType();
            std::unique_ptr<Type> valueType = parseType();
            GenericType gt;
            gt.reifiedType = ReifiedTypeId::Map;
            gt.args.push_back(std::move(keyType));
            gt.args.push_back(std::move(valueType));
            return std::make_unique<Type>(std::move(gt));
        }

        // Handle Set.Set
        if (typeName == "Set.Set")
        {
            std::unique_ptr<Type> elemType = parseType();
            GenericType gt;
            gt.reifiedType = ReifiedTypeId::Set;
            gt.args.push_back(std::move(elemType));
            return std::make_unique<Type>(std::move(gt));
        }

        // Simple type or struct ref
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
        if (t == "Bool")
            return ReifiedTypeId::Bool;
        if (t == "Int" || t == "Int32")
            return ReifiedTypeId::Int32;
        if (t == "Int8")
            return ReifiedTypeId::Int8;
        if (t == "Int16")
            return ReifiedTypeId::Int16;
        if (t == "Int64")
            return ReifiedTypeId::Int64;
        if (t == "Word8")
            return ReifiedTypeId::UInt8;
        if (t == "Word16")
            return ReifiedTypeId::UInt16;
        if (t == "Word32")
            return ReifiedTypeId::UInt32;
        if (t == "Word64")
            return ReifiedTypeId::UInt64;
        if (t == "Float")
            return ReifiedTypeId::Float32;
        if (t == "Double")
            return ReifiedTypeId::Float64;
        if (t == "String" || t == "Text")
            return ReifiedTypeId::String;
        if (t == "Char")
            return ReifiedTypeId::Char;
        if (t == "ByteString")
            return ReifiedTypeId::Bytes;
        if (t == "()")
            return ReifiedTypeId::Unknown;
        return ReifiedTypeId::StructRefType;
    }
};

} // namespace bhw
