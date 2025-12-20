// fsharp_parser.h - F# type parser with proper lexer/parser
#pragma once
#include <cctype>
#include <stdexcept>
#include <vector>

#include "ast.h"
#include "parser_registry2.h"

namespace bhw
{

class FSharpLexer
{
  public:
    enum class TokenType
    {
        NAMESPACE,
        MODULE,
        TYPE,
        OPEN,
        OF,
        AND,
        LBRACE,
        RBRACE,
        LPAREN,
        RPAREN,
        LBRACKET,
        RBRACKET,
        LANGLE,
        RANGLE,
        PIPE,
        COLON,
        SEMICOLON,
        COMMA,
        EQUALS,
        ID,
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

            // Comments: (* ... *)
            if (pos + 1 < source.size() && source[pos] == '(' && source[pos + 1] == '*')
            {
                pos += 2;
                int depth = 1;
                while (pos + 1 < source.size() && depth > 0)
                {
                    if (source[pos] == '(' && source[pos + 1] == '*')
                    {
                        depth++;
                        pos += 2;
                    }
                    else if (source[pos] == '*' && source[pos + 1] == ')')
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
            if (source[pos] == '|')
            {
                tokens.push_back({TokenType::PIPE, "|", line, col});
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

                if (word == "namespace")
                    type = TokenType::NAMESPACE;
                else if (word == "module")
                    type = TokenType::MODULE;
                else if (word == "type")
                    type = TokenType::TYPE;
                else if (word == "open")
                    type = TokenType::OPEN;
                else if (word == "of")
                    type = TokenType::OF;
                else if (word == "and")
                    type = TokenType::AND;

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

class FSharpParser : public AstParser, public AutoRegisterParser<FSharpParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"fs"};
    }
  public:
    Ast parseToAst(const std::string& src) override
    {
        FSharpLexer lexer;
        tokens = lexer.tokenize(src);
        pos = 0;

        Ast ast;
        ast.srcName = "fsharp";

        parseFile(ast, {});

        return ast;
    }

    auto getLang() -> bhw::Language override
    {
        return Language::FSharp;
    }

  private:
    std::string src;
    std::vector<FSharpLexer::Token> tokens;
    size_t pos;

    bool isAtEnd() const
    {
        return peek().type == FSharpLexer::TokenType::EOF_TOKEN;
    }

    FSharpLexer::Token peek() const
    {
        if (pos >= tokens.size())
            return tokens.back();
        return tokens[pos];
    }

    FSharpLexer::Token previous() const
    {
        if (pos == 0)
            return tokens[0];
        return tokens[pos - 1];
    }

    FSharpLexer::Token advance()
    {
        if (!isAtEnd())
            pos++;
        return previous();
    }

    bool match(FSharpLexer::TokenType type)
    {
        return peek().type == type;
    }

    bool consume(FSharpLexer::TokenType type, const std::string& errMsg)
    {
        if (match(type))
        {
            advance();
            return true;
        }
        throw std::runtime_error(errMsg + " at line " + std::to_string(peek().line));
    }

    std::string trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos)
            return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    void parseFile(Ast& ast, std::vector<std::string> nsPath)
    {
        while (!isAtEnd())
        {
            // Parse namespace
            if (match(FSharpLexer::TokenType::NAMESPACE))
            {
                parseNamespace(ast);
                continue;
            }

            // Skip open statements
            if (match(FSharpLexer::TokenType::OPEN))
            {
                advance();
                // Skip until next keyword or newline equivalent
                while (!isAtEnd() && !match(FSharpLexer::TokenType::NAMESPACE) &&
                       !match(FSharpLexer::TokenType::MODULE) &&
                       !match(FSharpLexer::TokenType::TYPE))
                {
                    advance();
                }
                continue;
            }

            // Parse module
            if (match(FSharpLexer::TokenType::MODULE))
            {
                parseModule(ast, nsPath);
                continue;
            }

            // Parse top-level type
            if (match(FSharpLexer::TokenType::TYPE))
            {
                parseType(ast, nsPath);
                continue;
            }

            // Skip unknown tokens
            if (!isAtEnd())
                advance();
        }
    }

    void parseNamespace(Ast& ast)
    {
        consume(FSharpLexer::TokenType::NAMESPACE, "Expected 'namespace'");

        std::vector<std::string> nsPath;

        if (!match(FSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected namespace name");

        std::string nsName = advance().value;
        nsPath.push_back(nsName);

        // Create a Namespace node to collect types
        Namespace ns;
        ns.name = nsName;

        // Skip open statements
        while (match(FSharpLexer::TokenType::OPEN))
        {
            advance();
            // Skip until next keyword
            while (!isAtEnd() && !match(FSharpLexer::TokenType::NAMESPACE) &&
                   !match(FSharpLexer::TokenType::MODULE) && !match(FSharpLexer::TokenType::TYPE))
            {
                advance();
            }
        }

        // Parse types and nested modules inside namespace
        while (!isAtEnd() && !match(FSharpLexer::TokenType::NAMESPACE))
        {
            if (match(FSharpLexer::TokenType::TYPE))
            {
                parseTypeIntoNamespace(ns, nsPath);
            }
            else if (match(FSharpLexer::TokenType::MODULE))
            {
                parseModuleIntoNamespace(ns, nsPath);
            }
            else if (!isAtEnd())
            {
                advance();
            }
        }

        // Add namespace to AST
        ast.nodes.push_back(std::move(ns));
    }

    void parseModule(Ast& ast, std::vector<std::string> parentNsPath)
    {
        consume(FSharpLexer::TokenType::MODULE, "Expected 'module'");

        if (!match(FSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected module name");

        std::string moduleName = advance().value;

        consume(FSharpLexer::TokenType::EQUALS, "Expected '=' after module name");

        // Create a Namespace node for the module
        Namespace ns;
        ns.name = moduleName;

        // Create temporary namespace path for nested types
        std::vector<std::string> nsPath = parentNsPath;
        nsPath.push_back(moduleName);

        // Parse module contents (types and nested modules)
        while (!isAtEnd() && !match(FSharpLexer::TokenType::MODULE) &&
               !match(FSharpLexer::TokenType::NAMESPACE))
        {
            if (match(FSharpLexer::TokenType::TYPE))
            {
                parseTypeIntoNamespace(ns, nsPath);
            }
            else if (match(FSharpLexer::TokenType::MODULE))
            {
                parseModuleIntoNamespace(ns, nsPath);
            }
            else if (!isAtEnd())
            {
                advance();
            }
        }

        // Add namespace to main AST
        ast.nodes.push_back(std::move(ns));
    }

    void parseModuleIntoNamespace(Namespace& parentNs, std::vector<std::string> parentNsPath)
    {
        consume(FSharpLexer::TokenType::MODULE, "Expected 'module'");

        if (!match(FSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected module name");

        std::string moduleName = advance().value;

        consume(FSharpLexer::TokenType::EQUALS, "Expected '=' after module name");

        // Create a Namespace node for the module
        Namespace ns;
        ns.name = moduleName;

        // Create temporary namespace path for nested types
        std::vector<std::string> nsPath = parentNsPath;
        nsPath.push_back(moduleName);

        // Parse module contents (types and nested modules)
        while (!isAtEnd() && !match(FSharpLexer::TokenType::MODULE) &&
               !match(FSharpLexer::TokenType::NAMESPACE))
        {
            if (match(FSharpLexer::TokenType::TYPE))
            {
                parseTypeIntoNamespace(ns, nsPath);
            }
            else if (match(FSharpLexer::TokenType::MODULE))
            {
                parseModuleIntoNamespace(ns, nsPath);
            }
            else if (!isAtEnd())
            {
                advance();
            }
        }

        // Add nested namespace to parent
        parentNs.nodes.push_back(std::move(ns));
    }

    void parseType(Ast& ast, const std::vector<std::string>& nsPath)
    {
        consume(FSharpLexer::TokenType::TYPE, "Expected 'type'");

        if (!match(FSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected type name");

        std::string typeName = advance().value;

        consume(FSharpLexer::TokenType::EQUALS, "Expected '='");

        // Check what kind of type this is
        if (match(FSharpLexer::TokenType::LBRACE))
        {
            parseRecord(typeName, nsPath, ast);
        }
        else if (match(FSharpLexer::TokenType::PIPE))
        {
            parseUnion(typeName, nsPath, ast);
        }
        else
        {
            // Could be type alias or other - skip for now
            skipToNextType();
        }
    }

    void parseTypeIntoNamespace(Namespace& ns, const std::vector<std::string>& nsPath)
    {
        consume(FSharpLexer::TokenType::TYPE, "Expected 'type'");

        if (!match(FSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected type name");

        std::string typeName = advance().value;

        consume(FSharpLexer::TokenType::EQUALS, "Expected '='");

        // Check what kind of type this is
        if (match(FSharpLexer::TokenType::LBRACE))
        {
            parseRecordIntoNamespace(typeName, nsPath, ns);
        }
        else if (match(FSharpLexer::TokenType::PIPE))
        {
            parseUnionIntoNamespace(typeName, nsPath, ns);
        }
        else
        {
            // Could be type alias or other - skip for now
            skipToNextType();
        }
    }

    void parseRecord(const std::string& typeName, const std::vector<std::string>& nsPath, Ast& ast)
    {
        consume(FSharpLexer::TokenType::LBRACE, "Expected '{'");

        Struct s;
        s.name = typeName;
        s.namespaces = nsPath;

        while (!match(FSharpLexer::TokenType::RBRACE) && !isAtEnd())
        {
            if (!match(FSharpLexer::TokenType::ID))
                break;

            std::string fieldName = advance().value;

            consume(FSharpLexer::TokenType::COLON, "Expected ':' after field name");

            std::unique_ptr<Type> fieldType = parseTypeExpr();

            Field f;
            f.name = fieldName;
            f.type = std::move(fieldType);
            s.members.push_back(std::move(f));

            // Optional semicolon or newline
            if (match(FSharpLexer::TokenType::SEMICOLON))
                advance();
        }

        consume(FSharpLexer::TokenType::RBRACE, "Expected '}'");

        ast.nodes.push_back(std::move(s));
    }

    void parseRecordIntoNamespace(const std::string& typeName,
                                  const std::vector<std::string>& nsPath,
                                  Namespace& ns)
    {
        consume(FSharpLexer::TokenType::LBRACE, "Expected '{'");

        Struct s;
        s.name = typeName;
        s.namespaces = nsPath;

        while (!match(FSharpLexer::TokenType::RBRACE) && !isAtEnd())
        {
            if (!match(FSharpLexer::TokenType::ID))
                break;

            std::string fieldName = advance().value;

            consume(FSharpLexer::TokenType::COLON, "Expected ':' after field name");

            std::unique_ptr<Type> fieldType = parseTypeExpr();

            Field f;
            f.name = fieldName;
            f.type = std::move(fieldType);
            s.members.push_back(std::move(f));

            // Optional semicolon or newline
            if (match(FSharpLexer::TokenType::SEMICOLON))
                advance();
        }

        consume(FSharpLexer::TokenType::RBRACE, "Expected '}'");

        ns.nodes.push_back(std::move(s));
    }
    void parseUnion(const std::string& typeName, const std::vector<std::string>& nsPath, Ast& ast)
    {
        std::vector<std::pair<std::string, std::unique_ptr<Type>>> cases;

        // First case (might have leading pipe or not)
        if (match(FSharpLexer::TokenType::PIPE))
            advance();

        if (!match(FSharpLexer::TokenType::ID))
            return;

        std::string caseName = advance().value;
        std::unique_ptr<Type> caseType = nullptr;

        // Handle enum with explicit value: | Case = number
        if (match(FSharpLexer::TokenType::EQUALS))
        {
            advance(); // consume =
            // Skip the number (we don't need it, enum values are auto-numbered)
            if (match(FSharpLexer::TokenType::ID)) // numbers are tokenized as IDs
                advance();
        }
        else if (match(FSharpLexer::TokenType::OF))
        {
            advance();
            caseType = parseTypeExpr();
        }

        cases.push_back({caseName, std::move(caseType)});

        // Remaining cases
        while (match(FSharpLexer::TokenType::PIPE) && !isAtEnd())
        {
            advance(); // consume |

            if (!match(FSharpLexer::TokenType::ID))
                break;

            std::string cn = advance().value;
            std::unique_ptr<Type> ct = nullptr;

            // Handle enum with explicit value: | Case = number
            if (match(FSharpLexer::TokenType::EQUALS))
            {
                advance(); // consume =
                // Skip the number
                if (match(FSharpLexer::TokenType::ID))
                    advance();
            }
            else if (match(FSharpLexer::TokenType::OF))
            {
                advance();
                ct = parseTypeExpr();
            }

            cases.push_back({cn, std::move(ct)});
        }

        // Rest of the method stays the same...
        // Check if it's an enum (no types) or union
        bool isEnum = true;
        for (const auto& [name, type] : cases)
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
            e.namespaces = nsPath;
            e.scoped = true;

            int num = 0;
            for (const auto& [name, type] : cases)
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

            for (auto& [name, type] : cases)
            {
                OneofField of;
                of.name = name;
                if (type)
                    of.type = std::move(type);
                else
                {
                    SimpleType st;
                    st.srcTypeString = "unit";
                    st.reifiedType = ReifiedTypeId::Unknown;
                    of.type = std::make_unique<Type>(std::move(st));
                }
                o.fields.push_back(std::move(of));
            }

            ast.nodes.push_back(std::move(o));
        }
    }

    void parseUnionIntoNamespace(const std::string& typeName,
                                 const std::vector<std::string>& nsPath,
                                 Namespace& ns)
    {
        std::vector<std::pair<std::string, std::unique_ptr<Type>>> cases;

        // First case (might have leading pipe or not)
        if (match(FSharpLexer::TokenType::PIPE))
            advance();

        if (!match(FSharpLexer::TokenType::ID))
            return;

        std::string caseName = advance().value;
        std::unique_ptr<Type> caseType = nullptr;

        // Handle enum with explicit value: | Case = number
        if (match(FSharpLexer::TokenType::EQUALS))
        {
            advance(); // consume =
            // Skip the number (we don't need it, enum values are auto-numbered)
            if (match(FSharpLexer::TokenType::ID)) // numbers are tokenized as IDs
                advance();
        }
        else if (match(FSharpLexer::TokenType::OF))
        {
            advance();
            caseType = parseTypeExpr();
        }

        cases.push_back({caseName, std::move(caseType)});

        // Remaining cases
        while (match(FSharpLexer::TokenType::PIPE) && !isAtEnd())
        {
            advance(); // consume |

            if (!match(FSharpLexer::TokenType::ID))
                break;

            std::string cn = advance().value;
            std::unique_ptr<Type> ct = nullptr;

            // Handle enum with explicit value: | Case = number
            if (match(FSharpLexer::TokenType::EQUALS))
            {
                advance(); // consume =
                // Skip the number
                if (match(FSharpLexer::TokenType::ID))
                    advance();
            }
            else if (match(FSharpLexer::TokenType::OF))
            {
                advance();
                ct = parseTypeExpr();
            }

            cases.push_back({cn, std::move(ct)});
        }

        // Check if it's an enum (no types) or union
        bool isEnum = true;
        for (const auto& [name, type] : cases)
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
            e.namespaces = nsPath;
            e.scoped = true;

            int num = 0;
            for (const auto& [name, type] : cases)
            {
                EnumValue ev;
                ev.name = name;
                ev.number = num++;
                e.values.push_back(std::move(ev));
            }

            ns.nodes.push_back(std::move(e));
        }
        else
        {
            Oneof o;
            o.name = typeName;

            for (auto& [name, type] : cases)
            {
                OneofField of;
                of.name = name;
                if (type)
                    of.type = std::move(type);
                else
                {
                    SimpleType st;
                    st.srcTypeString = "unit";
                    st.reifiedType = ReifiedTypeId::Unknown;
                    of.type = std::make_unique<Type>(std::move(st));
                }
                o.fields.push_back(std::move(of));
            }

            ns.nodes.push_back(std::move(o));
        }
    }

    std::unique_ptr<Type> parseTypeExpr()
    {
        if (!match(FSharpLexer::TokenType::ID))
            throw std::runtime_error("Expected type");

        std::string typeName = advance().value;

        // Check for generic/composite types by looking ahead
        if (match(FSharpLexer::TokenType::ID))
        {
            std::string nextToken = peek().value;

            // Handle "int list" -> List[int]
            if (nextToken == "list")
            {
                advance(); // consume "list"

                ReifiedTypeId rid = mapToReified(typeName);
                std::unique_ptr<Type> elemType;

                if (rid == ReifiedTypeId::StructRefType)
                {
                    StructRefType ref;
                    ref.srcTypeString = typeName;
                    ref.reifiedType = ReifiedTypeId::StructRefType;
                    elemType = std::make_unique<Type>(std::move(ref));
                }
                else
                {
                    SimpleType st;
                    st.srcTypeString = typeName;
                    st.reifiedType = rid;
                    elemType = std::make_unique<Type>(std::move(st));
                }

                GenericType gt;
                gt.reifiedType = ReifiedTypeId::List;
                gt.args.push_back(std::move(elemType));

                std::unique_ptr<Type> result = std::make_unique<Type>(std::move(gt));

                // Check for nested lists: "int list list"
                while (match(FSharpLexer::TokenType::ID) && peek().value == "list")
                {
                    advance(); // consume "list"
                    GenericType outerGt;
                    outerGt.reifiedType = ReifiedTypeId::List;
                    outerGt.args.push_back(std::move(result));
                    result = std::make_unique<Type>(std::move(outerGt));
                }

                // Check for option after list: "int list option"
                if (match(FSharpLexer::TokenType::ID) && peek().value == "option")
                {
                    advance(); // consume "option"
                    GenericType optGt;
                    optGt.reifiedType = ReifiedTypeId::Optional;
                    optGt.args.push_back(std::move(result));
                    result = std::make_unique<Type>(std::move(optGt));
                }

                return result;
            }
            // Handle "int option" -> Optional[int]
            else if (nextToken == "option")
            {
                advance(); // consume "option"

                ReifiedTypeId rid = mapToReified(typeName);
                std::unique_ptr<Type> innerType;

                if (rid == ReifiedTypeId::StructRefType)
                {
                    StructRefType ref;
                    ref.srcTypeString = typeName;
                    ref.reifiedType = ReifiedTypeId::StructRefType;
                    innerType = std::make_unique<Type>(std::move(ref));
                }
                else
                {
                    SimpleType st;
                    st.srcTypeString = typeName;
                    st.reifiedType = rid;
                    innerType = std::make_unique<Type>(std::move(st));
                }

                GenericType gt;
                gt.reifiedType = ReifiedTypeId::Optional;
                gt.args.push_back(std::move(innerType));

                return std::make_unique<Type>(std::move(gt));
            }
            // Handle "int array" -> List[int]
            else if (nextToken == "array")
            {
                advance(); // consume "array"

                ReifiedTypeId rid = mapToReified(typeName);
                std::unique_ptr<Type> elemType;

                if (rid == ReifiedTypeId::StructRefType)
                {
                    StructRefType ref;
                    ref.srcTypeString = typeName;
                    ref.reifiedType = ReifiedTypeId::StructRefType;
                    elemType = std::make_unique<Type>(std::move(ref));
                }
                else
                {
                    SimpleType st;
                    st.srcTypeString = typeName;
                    st.reifiedType = rid;
                    elemType = std::make_unique<Type>(std::move(st));
                }

                GenericType gt;
                gt.reifiedType = ReifiedTypeId::List;
                gt.args.push_back(std::move(elemType));

                std::unique_ptr<Type> result = std::make_unique<Type>(std::move(gt));

                // Check for option after array: "int array option"
                if (match(FSharpLexer::TokenType::ID) && peek().value == "option")
                {
                    advance(); // consume "option"
                    GenericType optGt;
                    optGt.reifiedType = ReifiedTypeId::Optional;
                    optGt.args.push_back(std::move(result));
                    result = std::make_unique<Type>(std::move(optGt));
                }

                return result;
            }
        }

        // Handle array syntax: int[]
        if (match(FSharpLexer::TokenType::LBRACKET))
        {
            advance();
            consume(FSharpLexer::TokenType::RBRACKET, "Expected ']'");

            ReifiedTypeId rid = mapToReified(typeName);
            std::unique_ptr<Type> elemType;

            if (rid == ReifiedTypeId::StructRefType)
            {
                StructRefType ref;
                ref.srcTypeString = typeName;
                ref.reifiedType = ReifiedTypeId::StructRefType;
                elemType = std::make_unique<Type>(std::move(ref));
            }
            else
            {
                SimpleType st;
                st.srcTypeString = typeName;
                st.reifiedType = rid;
                elemType = std::make_unique<Type>(std::move(st));
            }

            GenericType gt;
            gt.reifiedType = ReifiedTypeId::Array; // Use Array for [] syntax
            gt.args.push_back(std::move(elemType));

            std::unique_ptr<Type> result = std::make_unique<Type>(std::move(gt));

            // Check for option after array: "int[] option"
            if (match(FSharpLexer::TokenType::ID) && peek().value == "option")
            {
                advance(); // consume "option"
                GenericType optGt;
                optGt.reifiedType = ReifiedTypeId::Optional;
                optGt.args.push_back(std::move(result));
                result = std::make_unique<Type>(std::move(optGt));
            }

            return result;
        }

        // Handle angle bracket generics: Map<K, V>
        if (match(FSharpLexer::TokenType::LANGLE))
        {
            advance(); // consume <

            std::vector<std::unique_ptr<Type>> args;

            // Parse first type argument
            args.push_back(parseTypeExpr());

            // Parse remaining type arguments
            while (match(FSharpLexer::TokenType::COMMA))
            {
                advance(); // consume ,
                args.push_back(parseTypeExpr());
            }

            consume(FSharpLexer::TokenType::RANGLE, "Expected '>' after generic type arguments");

            // Determine the reified type based on the generic name
            ReifiedTypeId genericType = ReifiedTypeId::Unknown;
            if (typeName == "Map")
                genericType = ReifiedTypeId::Map;
            else if (typeName == "Set")
                genericType = ReifiedTypeId::Set;
            else if (typeName == "List")
                genericType = ReifiedTypeId::List;
            else if (typeName == "Option")
                genericType = ReifiedTypeId::Optional;
            else if (typeName == "option")
                genericType = ReifiedTypeId::Optional;

            GenericType gt;
            gt.reifiedType = genericType;
            gt.args = std::move(args);

            return std::make_unique<Type>(std::move(gt));
        }

        // Simple type
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
        if (t == "sbyte" || t == "int8")
            return ReifiedTypeId::Int8;
        if (t == "byte" || t == "uint8")
            return ReifiedTypeId::UInt8;
        if (t == "int16")
            return ReifiedTypeId::Int16;
        if (t == "uint16")
            return ReifiedTypeId::UInt16;
        if (t == "int" || t == "int32")
            return ReifiedTypeId::Int32;
        if (t == "uint32")
            return ReifiedTypeId::UInt32;
        if (t == "int64")
            return ReifiedTypeId::Int64;
        if (t == "uint64")
            return ReifiedTypeId::UInt64;
        if (t == "float32" || t == "single")
            return ReifiedTypeId::Float32;
        if (t == "float" || t == "double")
            return ReifiedTypeId::Float64;
        if (t == "string")
            return ReifiedTypeId::String;
        if (t == "char")
            return ReifiedTypeId::Char;
        if (t == "decimal")
            return ReifiedTypeId::Decimal;
        if (t == "obj" || t == "object")
            return ReifiedTypeId::Unknown;
        return ReifiedTypeId::StructRefType;
    }

    void skipToNextType()
    {
        while (!isAtEnd() && !match(FSharpLexer::TokenType::TYPE))
            advance();
    }
};

} // namespace bhw