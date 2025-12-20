#include "python_parser.h"

#include <cctype>
#include <stdexcept>

#include "ast.h"
using namespace bhw;
// Token types for Python
enum class PythonTokenType
{
    DATACLASS,
    CLASS,
    DEF,
    IDENTIFIER,
    COLON,
    EQUALS,
    LBRACKET, // [
    RBRACKET, // ]
    LPAREN,   // (
    RPAREN,   // )
    LBRACE,   // { (actually we'll use indentation, but for simplicity)
    RBRACE,   // }
    COMMA,
    ARROW, // ->
    END_OF_FILE
};

struct PythonToken
{
    PythonTokenType type;
    std::string value;
};

class PythonLexer
{
  private:
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

            // Comments: #
            if (c == '#')
            {
                while (pos < source.length() && source[pos] != '\n')
                {
                    pos++;
                }
                continue;
            }

            break;
        }
    }

    PythonToken readIdentifier()
    {
        size_t start = pos;
        while (pos < source.length() && (std::isalnum(source[pos]) || source[pos] == '_'))
        {
            pos++;
        }
        std::string value = source.substr(start, pos - start);

        // Check keywords
        if (value == "dataclass")
            return {PythonTokenType::DATACLASS, value};
        if (value == "class")
            return {PythonTokenType::CLASS, value};
        if (value == "def")
            return {PythonTokenType::DEF, value};

        return {PythonTokenType::IDENTIFIER, value};
    }

  public:
    PythonLexer(const std::string& src) : source(src)
    {
    }

    PythonToken nextToken()
    {
        skipWhitespaceAndComments();

        if (pos >= source.length())
        {
            return {PythonTokenType::END_OF_FILE, ""};
        }

        char c = source[pos];

        // Single character tokens
        if (c == ':')
        {
            pos++;
            return {PythonTokenType::COLON, ":"};
        }
        if (c == '=')
        {
            pos++;
            return {PythonTokenType::EQUALS, "="};
        }
        if (c == '[')
        {
            pos++;
            return {PythonTokenType::LBRACKET, "["};
        }
        if (c == ']')
        {
            pos++;
            return {PythonTokenType::RBRACKET, "]"};
        }
        if (c == '(')
        {
            pos++;
            return {PythonTokenType::LPAREN, "("};
        }
        if (c == ')')
        {
            pos++;
            return {PythonTokenType::RPAREN, ")"};
        }
        if (c == '{')
        {
            pos++;
            return {PythonTokenType::LBRACE, "{"};
        }
        if (c == '}')
        {
            pos++;
            return {PythonTokenType::RBRACE, "}"};
        }
        if (c == ',')
        {
            pos++;
            return {PythonTokenType::COMMA, ","};
        }

        // Arrow ->
        if (c == '-' && pos + 1 < source.length() && source[pos + 1] == '>')
        {
            pos += 2;
            return {PythonTokenType::ARROW, "->"};
        }

        // Identifiers
        if (std::isalpha(c) || c == '_')
        {
            return readIdentifier();
        }

        // Skip unknown
        pos++;
        return nextToken();
    }
};

// AstParser implementation
class PythonParserImpl
{
  private:
    PythonLexer lexer;
    PythonToken current_token;

    void advance()
    {
        current_token = lexer.nextToken();
    }

    void expect(PythonTokenType type)
    {
        if (current_token.type != type)
        {
            throw std::runtime_error("Unexpected token in Python parser");
        }
        advance();
    }

    bool match(PythonTokenType type)
    {
        if (current_token.type == type)
        {
            advance();
            return true;
        }
        return false;
    }

    std::unique_ptr<Type> parseTypeAnnotation()
    {
        std::string type_name = current_token.value;
        expect(PythonTokenType::IDENTIFIER);

        // Check for List[T], Dict[K,V], Optional[T], etc.
        bool is_optional = false;
        bool is_list = false;
        bool is_dict = false;

        if (type_name == "Optional")
        {
            is_optional = true;
            expect(PythonTokenType::LBRACKET);
            type_name = current_token.value;
            expect(PythonTokenType::IDENTIFIER);
            expect(PythonTokenType::RBRACKET);
        }
        else if (type_name == "List")
        {
            is_list = true;
            expect(PythonTokenType::LBRACKET);

            // Parse element type
            auto element_type = parseTypeAnnotation();
            expect(PythonTokenType::RBRACKET);

            // Create GenericType for List
            GenericType generic_type;
            generic_type.containerRefiedType = CanonicalType::List;
            generic_type.args.emplace_back(std::move(element_type));

            return std::make_unique<Type>(std::move(generic_type));
        }
        else if (type_name == "Dict")
        {
            is_dict = true;
            expect(PythonTokenType::LBRACKET);

            // Parse key type
            auto key_type = parseTypeAnnotation();
            expect(PythonTokenType::COMMA);

            // Parse value type
            auto value_type = parseTypeAnnotation();
            expect(PythonTokenType::RBRACKET);

            // Create GenericType for Map
            GenericType generic_type;
            generic_type.containerRefiedType = CanonicalType::Map;
            generic_type.args.emplace_back(std::move(key_type));
            generic_type.args.emplace_back(std::move(value_type));

            return std::make_unique<Type>(std::move(generic_type));
        }

        // Map Python types to canonical types
        CanonicalType canonical = CanonicalType::Unknown;
        if (type_name == "int")
            canonical = CanonicalType::Int64;
        else if (type_name == "float")
            canonical = CanonicalType::Float64;
        else if (type_name == "str")
            canonical = CanonicalType::String;
        else if (type_name == "bool")
            canonical = CanonicalType::Bool;

        // Create SimpleType
        SimpleType simple_type;
        if (canonical != CanonicalType::Unknown)
        {
            simple_type.type = canonical;
        }
        else
        {
            simple_type.type = type_name;
        }
        simple_type.original_type_string = type_name;

        // Wrap in optional if needed
        if (is_optional)
        {
            PointerType ptr_type;
            ptr_type.pointee = std::make_unique<Type>(simple_type);
            return std::make_unique<Type>(std::move(ptr_type));
        }
        else
        {
            return std::make_unique<Type>(simple_type);
        }
    }

    Struct parseDataclass()
    {
        // Skip @dataclass decorator
        while (current_token.type != PythonTokenType::CLASS &&
               current_token.type != PythonTokenType::END_OF_FILE)
        {
            advance();
        }

        expect(PythonTokenType::CLASS);

        Struct entity;
        entity.name = current_token.value;
        expect(PythonTokenType::IDENTIFIER);

        // Skip inheritance and colons
        while (current_token.type != PythonTokenType::COLON &&
               current_token.type != PythonTokenType::END_OF_FILE)
        {
            advance();
        }
        expect(PythonTokenType::COLON);

        // Parse fields (simplified - assumes proper indentation)
        while (current_token.type == PythonTokenType::IDENTIFIER)
        {
            Field f;
            f.name = current_token.value;
            advance();

            if (current_token.type != PythonTokenType::COLON)
            {
                break; // Not a field annotation
            }
            expect(PythonTokenType::COLON);

            // Parse type annotation
            f.type = parseTypeAnnotation();

            // Check for default value
            if (match(PythonTokenType::EQUALS))
            {
                // Skip default value for now
                while (current_token.type != PythonTokenType::IDENTIFIER &&
                       current_token.type != PythonTokenType::END_OF_FILE &&
                       current_token.type != PythonTokenType::CLASS &&
                       current_token.type != PythonTokenType::DEF)
                {
                    advance();
                }
            }

            entity.members.emplace_back(std::move(f));
        }

        return entity;
    }

  public:
    PythonParserImpl(const std::string& source) : lexer(source)
    {
        advance();
    }

    std::vector<Struct> parse()
    {
        std::vector<Struct> entities;

        while (current_token.type != PythonTokenType::END_OF_FILE)
        {
            // Look for @dataclass decorator
            if (current_token.type == PythonTokenType::DATACLASS)
            {
                entities.emplace_back(parseDataclass());
            }
            else if (current_token.type == PythonTokenType::CLASS)
            {
                // Also parse class without decorator (might have type hints)
                entities.emplace_back(parseDataclass());
            }
            else
            {
                advance();
            }
        }

        return entities;
    }
};

// Public API
PythonParser::PythonParser(const std::string& source)
    : impl(std::make_unique<PythonParserImpl>(source))
{
}

PythonParser::~PythonParser() = default;

auto PythonParser::parseToAst() -> bhw::Ast
{
    bhw::Ast ast;
    return ast;
}
