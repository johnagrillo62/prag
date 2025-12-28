#include "cpp_parser.h"

#include <algorithm>
#include <cctype>
#include <climits>
#include <map>
#include <stdexcept>
#include <string>

#include "ast.h"

using namespace bhw;
namespace
{
// STRING BOUNDARY #1: Input C++ type strings → Canonical enums
const std::map<std::string, bhw::ReifiedTypeId> CPP_TO_CANONICAL = {
    // Primitives
    {"bool", bhw::ReifiedTypeId::Bool},
    {"char", bhw::ReifiedTypeId::Char},
    {"signed char", bhw::ReifiedTypeId::Int8},
    {"unsigned char", bhw::ReifiedTypeId::UInt8},
    {"int8_t", bhw::ReifiedTypeId::Int8},
    {"uint8_t", bhw::ReifiedTypeId::UInt8},
    {"byte", bhw::ReifiedTypeId::UInt8},
    {"std::byte", bhw::ReifiedTypeId::UInt8},
    {"short", bhw::ReifiedTypeId::Int16},
    {"signed short", bhw::ReifiedTypeId::Int16},
    {"unsigned short", bhw::ReifiedTypeId::UInt16},
    {"int16_t", bhw::ReifiedTypeId::Int16},
    {"uint16_t", bhw::ReifiedTypeId::UInt16},
    {"int", bhw::ReifiedTypeId::Int32},
    {"signed", bhw::ReifiedTypeId::Int32},
    {"int32_t", bhw::ReifiedTypeId::Int32},
    {"signed int", bhw::ReifiedTypeId::Int32},
    {"unsigned", bhw::ReifiedTypeId::UInt32},
    {"uint32_t", bhw::ReifiedTypeId::UInt32},
    {"unsigned int", bhw::ReifiedTypeId::UInt32},
    {"long", bhw::ReifiedTypeId::Int64},
    {"signed long", bhw::ReifiedTypeId::Int64},
    {"unsigned long", bhw::ReifiedTypeId::UInt64},
    {"int64_t", bhw::ReifiedTypeId::Int64},
    {"uint64_t", bhw::ReifiedTypeId::UInt64},
    {"long long", bhw::ReifiedTypeId::Int64},
    {"signed long long", bhw::ReifiedTypeId::Int64},
    {"unsigned long long", bhw::ReifiedTypeId::UInt64},
    {"float", bhw::ReifiedTypeId::Float32},
    {"double", bhw::ReifiedTypeId::Float64},

    // String variations
    {"std::string", bhw::ReifiedTypeId::String},
    {"string", bhw::ReifiedTypeId::String},

    // Standard types
    {"std::chrono::system_clock::time_point", bhw::ReifiedTypeId::DateTime},
    {"std::chrono::year_month_day", bhw::ReifiedTypeId::Date},
    {"std::chrono::hh_mm_ss", bhw::ReifiedTypeId::Time},
    {"std::chrono::duration", bhw::ReifiedTypeId::Duration},
    {"std::array<uint8_t, 16>", bhw::ReifiedTypeId::UUID},

    // Containers
    {"std::vector", bhw::ReifiedTypeId::List},
    {"vector", bhw::ReifiedTypeId::List},
    {"std::map", bhw::ReifiedTypeId::Map},
    {"map", bhw::ReifiedTypeId::Map},
    {"std::set", bhw::ReifiedTypeId::Set},
    {"set", bhw::ReifiedTypeId::Set},
    {"std::unordered_map", bhw::ReifiedTypeId::UnorderedMap},
    {"unordered_map", bhw::ReifiedTypeId::UnorderedMap},
    {"std::unordered_set", bhw::ReifiedTypeId::UnorderedSet},
    {"unordered_set", bhw::ReifiedTypeId::UnorderedSet},
    {"std::optional", bhw::ReifiedTypeId::Optional},
    {"optional", bhw::ReifiedTypeId::Optional},
    {"std::tuple", bhw::ReifiedTypeId::Tuple},
    {"tuple", bhw::ReifiedTypeId::Tuple},
    {"std::variant", bhw::ReifiedTypeId::Variant},
    {"variant", bhw::ReifiedTypeId::Variant},
    {"std::monostate", bhw::ReifiedTypeId::Monostate}, 
    {"monostate", bhw::ReifiedTypeId::Monostate},     
    {"std::pair", bhw::ReifiedTypeId::Pair},
    {"pair", bhw::ReifiedTypeId::Pair},
    {"std::array", bhw::ReifiedTypeId::Array},
    {"array", bhw::ReifiedTypeId::Array},

    // Ownership
    {"std::unique_ptr", bhw::ReifiedTypeId::UniquePtr},
    {"unique_ptr", bhw::ReifiedTypeId::UniquePtr},
    {"std::shared_ptr", bhw::ReifiedTypeId::SharedPtr},
    {"shared_ptr", bhw::ReifiedTypeId::SharedPtr},
};
} // namespace

// ============================================================================
// CppLexer Implementation
// ============================================================================

char CppLexer::current() const
{
    return pos < source.size() ? source[pos] : '\0';
}

char CppLexer::peek(size_t offset) const
{
    return (pos + offset) < source.size() ? source[pos + offset] : '\0';
}

void CppLexer::advance()
{
    if (current() == '\n')
    {
        line++;
        column = 1;
    }
    else
    {
        column++;
    }
    pos++;
}

bool CppLexer::matchChar(char c)
{
    if (current() == c)
    {
        advance();
        return true;
    }
    return false;
}

bool CppLexer::matchString(const std::string& str)
{
    for (size_t i = 0; i < str.size(); i++)
    {
        if (peek(i) != str[i])
            return false;
    }
    for (size_t i = 0; i < str.size(); i++)
    {
        advance();
    }
    return true;
}

void CppLexer::skipWhitespace()
{
    while (std::isspace(current()))
        advance();
}

void CppLexer::skipLineComment()
{
    while (current() != '\n' && current() != '\r' && current() != '\0')
        advance();
    if (current() == '\r')
        advance();
    if (current() == '\n')
        advance();
}

void CppLexer::skipBlockComment()
{
    while (true)
    {
        if (current() == '\0')
            throw std::runtime_error("Unterminated block comment");

        if (current() == '*' && peek() == '/')
        {
            advance(); // *
            advance(); // /
            break;
        }
        advance();
    }
}

void CppLexer::skipCppAttribute()
{
    // [[ already consumed by caller
    int bracket_depth = 1; // We've seen one [[
    
    while (bracket_depth > 0 && current() != '\0')
    {
        if (current() == '[' && peek() == '[')
        {
            bracket_depth++;
            advance();
            advance();
        }
        else if (current() == ']' && peek() == ']')
        {
            bracket_depth--;
            advance();
            advance();
        }
        else
        {
            advance();
        }
    }
    
    if (bracket_depth > 0)
        throw std::runtime_error("Unterminated C++ attribute [[...]]");
}

CppToken CppLexer::makeToken(CppTokenType type, const std::string& value)
{
    return CppToken(type, value, line, column);
}

CppToken CppLexer::readNumber()
{
    std::string value;

    while (std::isdigit(current()) || current() == '.')
    {
        value += current();
        advance();
    }

    return makeToken(CppTokenType::Number, value);
}

CppToken CppLexer::readIdentifier()
{
    std::string value;

    while (std::isalnum(current()) || current() == '_')
    {
        value += current();
        advance();
    }

    // Check for keywords
    static const std::map<std::string, CppTokenType> keywords = {
        {"struct", CppTokenType::Struct},
        {"namespace", CppTokenType::Namespace},
        {"enum", CppTokenType::Enum},
        {"class", CppTokenType::Class},
        {"using", CppTokenType::Using},      // ← ADDED
        {"typedef", CppTokenType::Typedef},  // ← ADDED
    };

    auto it = keywords.find(value);
    if (it != keywords.end())
        return makeToken(it->second, value);

    return makeToken(CppTokenType::Identifier, value);
}

CppToken CppLexer::readAttribute()
{
    // @ already consumed by caller
    size_t start_line = line;
    size_t start_col = column;

    // Parse attribute name
    std::string name;
    while (std::isalnum(current()) || current() == '_')
    {
        name += current();
        advance();
    }

    if (name.empty())
    {
        throw std::runtime_error("Attribute name cannot be empty at line " +
                                 std::to_string(start_line) + ". No space allowed after @");
    }

    skipWhitespace();

    // Parse attribute value in parentheses
    std::string value;
    if (matchChar('('))
    {
        skipWhitespace();

        // Try to parse string literal first (with quotes)
        if (matchChar('"'))
        {
            while (current() != '"' && current() != '\0')
            {
                if (current() == '\\')
                {
                    advance();
                    value += current();
                    advance();
                }
                else
                {
                    value += current();
                    advance();
                }
            }
            if (!matchChar('"'))
                throw std::runtime_error("Unterminated string in attribute");
        }
        else
        {
            // No quotes - read identifier: alphanumeric, underscore, colon, dot
            while (std::isalnum(current()) || current() == '_' || current() == ':' || current() == '.')
            {
                value += current();
                advance();
            }
        }

        skipWhitespace();

        if (!matchChar(')'))
            throw std::runtime_error("Expected ')' after attribute value");
    }

    // Format: name=value or just name
    std::string attr_text = name;
    if (!value.empty())
        attr_text += "=" + value;

    return CppToken(CppTokenType::Attribute, attr_text, start_line, start_col);
}

CppToken CppLexer::nextToken()
{
    while (true)
    {
        skipWhitespace();

        if (current() == '/' && (peek() == '/' || peek() == '*'))
        {
            if (peek() == '/')
            {
                advance(); // first /
                advance(); // second /

                // Check for attribute
                while (current() == ' ' || current() == '\t')
                    advance();

                if (current() == '@')
                {
                    advance(); // consume @
                    return readAttribute();
                }
                else
                {
                    skipLineComment();
                }
            }
            else
            {
                advance(); // /
                advance(); // *
                skipBlockComment();
            }
            continue;
        }

        // Skip preprocessor directives
        if (current() == '#')
        {
            skipLineComment();
            continue;
        }

        break;
    }

    if (current() == '\0')
        return makeToken(CppTokenType::Eof, "");

    if (std::isdigit(current()))
        return readNumber();

    if (std::isalpha(current()) || current() == '_')
        return readIdentifier();

    // Skip C++ attributes [[...]]
    if (current() == '[' && peek() == '[')
    {
        advance(); // first [
        advance(); // second [
        skipCppAttribute();
        return nextToken(); // Get next real token
    }

    char ch = current();
    size_t start_line = line;
    size_t start_col = column;
    advance();

    // Check for :: (scope resolution)
    if (ch == ':' && current() == ':')
    {
        advance();
        return CppToken(CppTokenType::Colon, "::", start_line, start_col);
    }

    switch (ch)
    {
    case '{':
        return CppToken(CppTokenType::LBrace, "{", start_line, start_col);
    case '}':
        return CppToken(CppTokenType::RBrace, "}", start_line, start_col);
    case '<':
        return CppToken(CppTokenType::LAngle, "<", start_line, start_col);
    case '>':
        return CppToken(CppTokenType::RAngle, ">", start_line, start_col);
    case ';':
        return CppToken(CppTokenType::Semicolon, ";", start_line, start_col);
    case ',':
        return CppToken(CppTokenType::Comma, ",", start_line, start_col);
    case '*':
        return CppToken(CppTokenType::Star, "*", start_line, start_col);
    case '=':
        return CppToken(CppTokenType::Equals, "=", start_line, start_col);
    case ':': 
       return CppToken(CppTokenType::Colon, ":", start_line, start_col);
    default:
        return CppToken(CppTokenType::Unknown, std::string(1, ch), start_line, start_col);
    }
}

// ============================================================================
// CppParser Implementation
// ============================================================================


void CppParser::advance()
{
    current_token = lexer.nextToken();
}

bool CppParser::match(CppTokenType type) const
{
    return current_token.type == type;
}

bool CppParser::expect(CppTokenType type)
{
    if (!match(type))
        return false;
    advance();
    return true;
}

bool CppParser::peek_ahead_is_struct()
{
    size_t saved_pos = lexer.pos;
    CppToken saved_token = current_token;

    // Skip identifier (type name)
    if (match(CppTokenType::Identifier))
        advance();

    bool is_struct = match(CppTokenType::Struct);

    // Restore position
    lexer.pos = saved_pos;
    current_token = saved_token;

    return is_struct;
}

std::vector<Attribute> CppParser::collectPendingAttributes()
{
    auto attrs = std::move(pending_attributes_);
    pending_attributes_.clear();
    return attrs;
}

std::string CppParser::parseQualifiedName()
{
    std::string name = current_token.value;
    expect(CppTokenType::Identifier);

    while (match(CppTokenType::Colon))
    {
        advance();
        name += "::";
        name += current_token.value;
        expect(CppTokenType::Identifier);
    }

    return name;
}

void CppParser::registerUserType(const std::string& type_name)
{
    known_user_types_.insert(type_name);
}

bool CppParser::isKnownType(const std::string& type_name) const
{
    if (CPP_TO_CANONICAL.count(type_name))
        return true;

    if (known_user_types_.count(type_name))
        return true;

    return false;
}

std::unique_ptr<Type> CppParser::resolveType(const std::string& type_name)
{
    // Try canonical type first
    auto it = CPP_TO_CANONICAL.find(type_name);
    if (it != CPP_TO_CANONICAL.end())
    {
        SimpleType st;
        st.reifiedType = it->second;
        st.srcTypeString = type_name;
        return std::make_unique<Type>(st);
    }

    // Try user-defined type
    if (known_user_types_.count(type_name))
    {
        return std::make_unique<Type>(StructRefType{
            type_name,
            ReifiedTypeId::StructRefType,
        });
    }

    std::string error_msg = "Unknown type: '" + type_name + "' at line " + 
                        std::to_string(current_token.line);

    throw std::runtime_error(error_msg);
}

std::string CppParser::parseBuiltinType()
{
    std::vector<std::string> parts;

    // 1. Optional: unsigned/signed
    if (current_token.value == "unsigned" || current_token.value == "signed")
    {
        parts.emplace_back(current_token.value);
        advance();
    }

    // 2. Optional: short/long/long long
    if (current_token.value == "short")
    {
        parts.emplace_back("short");
        advance();
    }
    else if (current_token.value == "long")
    {
        parts.emplace_back("long");
        advance();

        if (current_token.value == "long")
        {
            parts.emplace_back("long");
            advance();
        }
    }

    // 3. Optional: int/char
    if (current_token.value == "int")
    {
        parts.emplace_back("int");
        advance();
    }
    else if (current_token.value == "char")
    {
        parts.emplace_back("char");
        advance();
    }

    // 4. Handle standalone keywords
    if (parts.empty())
    {
        if (current_token.value == "bool" || current_token.value == "float" ||
            current_token.value == "double")
        {
            parts.emplace_back(current_token.value);
            advance();
        }
    }

    // Build and return
    std::string result;
    for (size_t i = 0; i < parts.size(); ++i)
    {
        if (i > 0)
            result += " ";
        result += parts[i];
    }

    return result;
}

std::unique_ptr<Type> CppParser::parseType(size_t)
{
    // Try to parse builtin type first (multi-token)
    std::string type_name = parseBuiltinType();
    if (type_name.empty())
        type_name = parseQualifiedName();

    // Check if it's a generic type
    if (match(CppTokenType::LAngle))
    {
        advance(); // consume <

        auto it = CPP_TO_CANONICAL.find(type_name);
        if (it == CPP_TO_CANONICAL.end())
        {
            throw std::runtime_error("Unknown generic container: '" + type_name + "' at line " +
                                     std::to_string(current_token.line));
        }

        // Parse type arguments
        std::vector<std::unique_ptr<Type>> args;

        args.emplace_back(parseType());

        while (match(CppTokenType::Comma))
        {
            advance();
            args.emplace_back(parseType());
        }

        expect(CppTokenType::RAngle);

        auto gen = std::make_unique<Type>(GenericType{std::move(args), it->second});
        gen->srcType = type_name;

        return gen;
    }

    // Check for pointer
    if (match(CppTokenType::Star))
    {
        advance();
        auto pointee = resolveType(type_name);
        return std::make_unique<Type>(PointerType{std::move(pointee)});
    }

    // Simple type
    return resolveType(type_name);
}





Field CppParser::parseField()
{
    auto attrs = collectPendingAttributes();
    auto type = parseType();
    std::string name = current_token.value;
    expect(CppTokenType::Identifier);
    
    // Handle bitfield syntax
    // uint8_t input : 1;
    //               ^^^^ Skip this part
    if (match(CppTokenType::Colon) && current_token.value == ":")
    {
        advance();  // Skip the ':'
        
        // Skip the bitfield width (number or constant expression)
        if (match(CppTokenType::Number))
        {
            advance();
        }
        else if (match(CppTokenType::Identifier))
        {
            // Could be a constant like: width : MY_CONSTANT
            advance();
        }
        else
        {
            throw std::runtime_error(
                "Expected bitfield width (number or identifier) after ':' at line " +
                std::to_string(current_token.line)
            );
        }
    }
    
    expect(CppTokenType::Semicolon);
    
    return Field{name, std::move(type), std::move(attrs)};
}

Struct CppParser::parseNestedStruct()
{
    auto field_attrs = collectPendingAttributes();

    expect(CppTokenType::Struct);

    Struct nested;

    // Optional class name
    if (match(CppTokenType::Identifier))
    {
        nested.name = current_token.value;
        registerUserType(nested.name);
        advance();
    }
    else
    {
        nested.isAnonymous = true;
    }

    expect(CppTokenType::LBrace);

    // Recursively parse fields
    while (!match(CppTokenType::RBrace) && !match(CppTokenType::Eof))
    {
        // ⭐ NEW: Skip using/typedef declarations
        if (match(CppTokenType::Using) || match(CppTokenType::Typedef))
        {
            while (!match(CppTokenType::Semicolon) && !match(CppTokenType::Eof))
            {
                advance();
            }
            if (match(CppTokenType::Semicolon))
            {
                advance();
            }
            continue;
        }

        if (match(CppTokenType::Attribute))
        {
            std::string attr_str = current_token.value;
            advance();

            size_t eq_pos = attr_str.find('=');
            Attribute attr;
            if (eq_pos != std::string::npos)
            {
                attr.name = attr_str.substr(0, eq_pos);
                attr.value = attr_str.substr(eq_pos + 1);
            }
            else
            {
                attr.name = attr_str;
                attr.value = "";
            }
            pending_attributes_.emplace_back(attr);
        }
        else if (match(CppTokenType::Enum))
        {
            nested.members.emplace_back(parseEnum());
        }
        else if (match(CppTokenType::Struct))
        {
            nested.members.emplace_back(parseNestedStruct());
        }
        else
        {
            nested.members.emplace_back(parseField());
        }
    }

    expect(CppTokenType::RBrace);

    // Optional instance name
    if (match(CppTokenType::Identifier))
    {
        nested.variableName = current_token.value;
        advance();
    }

    expect(CppTokenType::Semicolon);
    return nested;
}


Enum CppParser::parseEnum()
{
    auto attrs = collectPendingAttributes();
    expect(CppTokenType::Enum);
    bool scoped = false;
    if (match(CppTokenType::Class))
    {
        scoped = true;
        advance();
    }
    std::string name = current_token.value;
    expect(CppTokenType::Identifier);
    registerUserType(name);
    // Check for underlying type specification (e.g., : uint8_t)
    std::string underlying_type;
    if (match(CppTokenType::Colon))
    {
        advance(); // consume ':'
        underlying_type = current_token.value;
        expect(CppTokenType::Identifier); // the underlying type (uint8_t, int, etc.)
    }
    
    expect(CppTokenType::LBrace);
    std::vector<EnumValue> values;
    int auto_value = 0;
    while (!match(CppTokenType::RBrace) && !match(CppTokenType::Eof))
    {
        if (match(CppTokenType::Attribute))
        {
            std::string attr_str = current_token.value;
            advance();
            size_t eq_pos = attr_str.find('=');
            Attribute attr;
            if (eq_pos != std::string::npos)
            {
                attr.name = attr_str.substr(0, eq_pos);
                attr.value = attr_str.substr(eq_pos + 1);
            }
            else
            {
                attr.name = attr_str;
                attr.value = "";
            }
            pending_attributes_.emplace_back(attr);
        }
        else if (match(CppTokenType::Identifier))
        {
            auto value_attrs = collectPendingAttributes();
            std::string value_name = current_token.value;
            expect(CppTokenType::Identifier);
            int value_number = auto_value;
            // Check for explicit value assignment
            if (match(CppTokenType::Equals))
            {
                advance();
                value_number = std::stoi(current_token.value);
                expect(CppTokenType::Number);
            }
            values.emplace_back(EnumValue{value_name, value_number, std::move(value_attrs)});
            auto_value = value_number + 1;
            // Optional comma
            if (match(CppTokenType::Comma))
                advance();
        }
        else
        {
            advance(); // Skip unexpected tokens
        }
    }
    expect(CppTokenType::RBrace);
    expect(CppTokenType::Semicolon);
    return Enum{.name = name,
                .namespaces = namespace_stack_,
                .values = std::move(values),
                .attributes = std::move(attrs),
                .scoped = scoped,
		.underlying_type = underlying_type
    };
}


Struct CppParser::parseStruct()
{
    auto attrs = collectPendingAttributes();

    expect(CppTokenType::Struct);
    std::string name = current_token.value;
    expect(CppTokenType::Identifier);

    // Handle forward declarations (struct Name;)
    if (match(CppTokenType::Semicolon))
    {
        advance(); // consume semicolon
        registerUserType(name); // Register the type name for later use
        // Return empty struct as placeholder
        return Struct{.name = name,
                      .namespaces = namespace_stack_,
                      .members = {},
                      .attributes = std::move(attrs)};
    }

    
    expect(CppTokenType::LBrace);

    std::vector<Field> fields;
    std::vector<StructMember> members;

    while (!match(CppTokenType::RBrace) && !match(CppTokenType::Eof))
    {
        if (match(CppTokenType::Using) || match(CppTokenType::Typedef))
        {
            // Skip until semicolon
            while (!match(CppTokenType::Semicolon) && !match(CppTokenType::Eof))
            {
                advance();
            }
            if (match(CppTokenType::Semicolon))
            {
                advance();
            }
            continue; // Skip to next member
        }

        if (match(CppTokenType::Attribute))
        {
            std::string attr_str = current_token.value;
            advance();

            size_t eq_pos = attr_str.find('=');
            Attribute attr;
            if (eq_pos != std::string::npos)
            {
                attr.name = attr_str.substr(0, eq_pos);
                attr.value = attr_str.substr(eq_pos + 1);
            }
            else
            {
                attr.name = attr_str;
                attr.value = "";
            }

            pending_attributes_.emplace_back(attr);
        }
        else if (match(CppTokenType::Struct))
        {
            members.emplace_back(parseNestedStruct());
        }
        else if (match(CppTokenType::Enum))
        {
            members.emplace_back(parseEnum());
        }
        else
        {
            members.emplace_back(parseField());
        }
    }

    expect(CppTokenType::RBrace);

    std::string varName;
    if (match(CppTokenType::Identifier))
    {
        varName = current_token.value;
        advance();
    }

    expect(CppTokenType::Semicolon);

    return Struct{.name = name,
                  .namespaces = namespace_stack_,
                  .members = std::move(members),
                  .attributes = std::move(attrs),
                  .variableName = varName};
}


bhw::Namespace CppParser::parseNamespace(size_t indent)
{
    expect(CppTokenType::Namespace);

    std::string name = parseQualifiedName();
    expect(CppTokenType::LBrace);

    bhw::Namespace ns;
    ns.name = name;
    namespace_stack_.emplace_back(name);
    ns.nodes = std::move(parseDeclarations(indent + 2));
    namespace_stack_.pop_back();

    expect(CppTokenType::RBrace);

    if (match(CppTokenType::Semicolon))
        advance();

    return ns;
}

std::vector<bhw::AstRootNode> CppParser::parseDeclarations(size_t indent)
{
    std::vector<bhw::AstRootNode> nodes;

    while (!match(CppTokenType::Eof) && !match(CppTokenType::RBrace))
    {
        // ⭐ NEW: Skip using/typedef at top level
        if (match(CppTokenType::Using) || match(CppTokenType::Typedef))
        {
            while (!match(CppTokenType::Semicolon) && !match(CppTokenType::Eof))
            {
                advance();
            }
            if (match(CppTokenType::Semicolon))
            {
                advance();
            }
            continue;
        }

        if (match(CppTokenType::Namespace))
        {
            nodes.emplace_back(parseNamespace(indent + 2));
        }
        else if (match(CppTokenType::Enum))
        {
            nodes.emplace_back(parseEnum());
        }
        else if (match(CppTokenType::Struct))
        {
            nodes.emplace_back(parseStruct());
        }
        else
        {
            advance(); // Skip unknown tokens
        }
    }

    return nodes;
}


void CppParser::collectStructNames()
{
    size_t saved_pos = lexer.pos;
    CppToken saved_token = current_token;
    size_t saved_line = lexer.line;

    while (!match(CppTokenType::Eof))
    {
        if (match(CppTokenType::Struct))
        {
            advance();
            if (match(CppTokenType::Identifier))
            {
                registerUserType(current_token.value);
            }
        }
        advance();
    }

    // Restore position
    lexer.pos = saved_pos;
    current_token = saved_token;
    lexer.line = saved_line;
}

bhw::Ast CppParser::parseToAst(const std::string& src)
{
    bhw::Ast ast;

    try
    {

        lexer.source = src;
        advance();

        // Register all known struct names first
        collectStructNames();

        // Parse everything
        while (!match(CppTokenType::Eof))
        {
            if (match(CppTokenType::Attribute))
            {
                std::string attr_str = current_token.value;
                advance();

                size_t eq_pos = attr_str.find('=');
                Attribute attr;
                if (eq_pos != std::string::npos)
                {
                    attr.name = attr_str.substr(0, eq_pos);
                    attr.value = attr_str.substr(eq_pos + 1);
                }
                else
                {
                    attr.name = attr_str;
                    attr.value = "";
                }

                pending_attributes_.emplace_back(attr);
            }
            else if (match(CppTokenType::Namespace))
            {
                bhw::Namespace ns = parseNamespace(0);
                ast.nodes.emplace_back(std::move(ns));
            }
            else if (match(CppTokenType::Enum))
            {
                ast.nodes.emplace_back(parseEnum());
            }
            else if (match(CppTokenType::Struct))
            {
                ast.nodes.emplace_back(parseStruct());
            }
            else
            {
                advance(); // Skip unknown tokens
            }
        }
    }
    catch (std::runtime_error e)
    {
        std::cerr << "Parse error " << e.what() << "\n";
    }
    return ast;
}
