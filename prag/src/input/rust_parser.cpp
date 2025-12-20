// rust_parser.cpp
#include "rust_parser.h"

#include <cctype>
#include <map>
#include <stdexcept>

// ============================================================================
// RustLexer Implementation
// ============================================================================
using namespace bhw;

char RustLexer::current() const
{
    return pos < source.size() ? source[pos] : '\0';
}

char RustLexer::peek(size_t offset) const
{
    return (pos + offset) < source.size() ? source[pos + offset] : '\0';
}

void RustLexer::advance()
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

void RustLexer::skipWhitespace()
{
    while (std::isspace(current()))
        advance();
}

void RustLexer::skipComment()
{
    if (current() == '/' && peek() == '/')
    {
        // Line comment
        while (current() != '\n' && current() != '\0')
            advance();
    }
    else if (current() == '/' && peek() == '*')
    {
        // Block comment (can be nested in Rust!)
        int depth = 1;
        advance(); // skip /
        advance(); // skip *

        while (depth > 0 && current() != '\0')
        {
            if (current() == '/' && peek() == '*')
            {
                depth++;
                advance();
                advance();
            }
            else if (current() == '*' && peek() == '/')
            {
                depth--;
                advance();
                advance();
            }
            else
            {
                advance();
            }
        }
    }
}

RustToken RustLexer::makeToken(RustTokenType type, const std::string& value)
{
    return RustToken{type, value, line, column};
}

RustToken RustLexer::readNumber()
{
    std::string value;

    // Handle hex, binary, octal
    if (current() == '0')
    {
        value += current();
        advance();

        if (current() == 'x' || current() == 'b' || current() == 'o')
        {
            value += current();
            advance();
        }
    }

    while (std::isalnum(current()) || current() == '_' || current() == '.')
    {
        value += current();
        advance();
    }

    return makeToken(RustTokenType::Number, value);
}

RustToken RustLexer::readString()
{
    std::string value;
    char quote = current();
    advance(); // skip opening "

    while (current() != quote && current() != '\0')
    {
        if (current() == '\\')
        {
            advance();
            if (current() != '\0')
            {
                value += current();
                advance();
            }
        }
        else
        {
            value += current();
            advance();
        }
    }

    if (current() == quote)
        advance(); // skip closing "

    return makeToken(RustTokenType::StringLiteral, value);
}

RustToken RustLexer::readRawString()
{
    std::string value;

    // r"..." or r#"..."# or r##"..."##
    advance(); // skip 'r'

    size_t hash_count = 0;
    while (current() == '#')
    {
        hash_count++;
        advance();
    }

    if (current() == '"')
        advance(); // skip opening "

    // Read until closing " followed by same number of #
    while (current() != '\0')
    {
        if (current() == '"')
        {
            bool matches = true;
            for (size_t i = 0; i < hash_count; i++)
            {
                if (peek(i + 1) != '#')
                {
                    matches = false;
                    break;
                }
            }

            if (matches)
            {
                advance(); // skip "
                for (size_t i = 0; i < hash_count; i++)
                    advance();
                break;
            }
        }

        value += current();
        advance();
    }

    return makeToken(RustTokenType::StringLiteral, value);
}

RustToken RustLexer::readChar()
{
    std::string value;
    advance(); // skip opening '

    if (current() == '\\')
    {
        advance();
        value += current();
        advance();
    }
    else if (current() != '\'')
    {
        value += current();
        advance();
    }

    if (current() == '\'')
        advance(); // skip closing '

    return makeToken(RustTokenType::CharLiteral, value);
}

RustToken RustLexer::readIdentifier()
{
    std::string value;
    while (std::isalnum(current()) || current() == '_')
    {
        value += current();
        advance();
    }

    // Check for keywords
    static const std::map<std::string, RustTokenType> keywords = {
        {"struct", RustTokenType::Struct},   {"enum", RustTokenType::Enum},
        {"impl", RustTokenType::Impl},       {"trait", RustTokenType::Trait},
        {"type", RustTokenType::Type},       {"fn", RustTokenType::Fn},
        {"pub", RustTokenType::Pub},         {"mod", RustTokenType::Mod},
        {"use", RustTokenType::Use},         {"const", RustTokenType::Const},
        {"static", RustTokenType::Static},   {"let", RustTokenType::Let},
        {"mut", RustTokenType::Mut},         {"ref", RustTokenType::Ref},
        {"i8", RustTokenType::I8},           {"i16", RustTokenType::I16},
        {"i32", RustTokenType::I32},         {"i64", RustTokenType::I64},
        {"i128", RustTokenType::I128},       {"isize", RustTokenType::Isize},
        {"u8", RustTokenType::U8},           {"u16", RustTokenType::U16},
        {"u32", RustTokenType::U32},         {"u64", RustTokenType::U64},
        {"u128", RustTokenType::U128},       {"usize", RustTokenType::Usize},
        {"f32", RustTokenType::F32},         {"f64", RustTokenType::F64},
        {"bool", RustTokenType::Bool},       {"char", RustTokenType::Char},
        {"str", RustTokenType::Str},         {"String", RustTokenType::String},
        {"Vec", RustTokenType::Vec},         {"Option", RustTokenType::Option},
        {"Result", RustTokenType::Result},   {"Box", RustTokenType::Box},
        {"Rc", RustTokenType::Rc},           {"Arc", RustTokenType::Arc},
        {"HashMap", RustTokenType::HashMap}, {"HashSet", RustTokenType::HashSet}};

    auto it = keywords.find(value);
    if (it != keywords.end())
        return makeToken(it->second, value);

    return makeToken(RustTokenType::Identifier, value);
}

RustToken RustLexer::nextToken()
{
    while (true)
    {
        skipWhitespace();

        if (current() == '/' && (peek() == '/' || peek() == '*'))
        {
            skipComment();
            continue;
        }
        break;
    }

    if (current() == '\0')
        return makeToken(RustTokenType::Eof, "");

    if (std::isdigit(current()))
        return readNumber();

    if (current() == '"')
        return readString();

    if (current() == 'r' && (peek() == '"' || peek() == '#'))
        return readRawString();

    if (current() == '\'')
        return readChar();

    if (std::isalpha(current()) || current() == '_')
        return readIdentifier();

    char ch = current();
    advance();

    // Multi-character operators
    if (ch == ':' && current() == ':')
    {
        advance();
        return makeToken(RustTokenType::DoubleColon, "::");
    }

    if (ch == '-' && current() == '>')
    {
        advance();
        return makeToken(RustTokenType::Arrow, "->");
    }

    if (ch == '=' && current() == '>')
    {
        advance();
        return makeToken(RustTokenType::FatArrow, "=>");
    }

    switch (ch)
    {
    case '{':
        return makeToken(RustTokenType::LBrace, "{");
    case '}':
        return makeToken(RustTokenType::RBrace, "}");
    case '(':
        return makeToken(RustTokenType::LParen, "(");
    case ')':
        return makeToken(RustTokenType::RParen, ")");
    case '[':
        return makeToken(RustTokenType::LBracket, "[");
    case ']':
        return makeToken(RustTokenType::RBracket, "]");
    case '<':
        return makeToken(RustTokenType::LAngle, "<");
    case '>':
        return makeToken(RustTokenType::RAngle, ">");
    case ';':
        return makeToken(RustTokenType::Semicolon, ";");
    case ':':
        return makeToken(RustTokenType::Colon, ":");
    case ',':
        return makeToken(RustTokenType::Comma, ",");
    case '.':
        return makeToken(RustTokenType::Dot, ".");
    case '=':
        return makeToken(RustTokenType::Equals, "=");
    case '&':
        return makeToken(RustTokenType::Ampersand, "&");
    case '*':
        return makeToken(RustTokenType::Star, "*");
    case '#':
        return makeToken(RustTokenType::Hash, "#");
    case '!':
        return makeToken(RustTokenType::Exclamation, "!");
    case '?':
        return makeToken(RustTokenType::Question, "?");
    default:
        return makeToken(RustTokenType::Unknown, std::string(1, ch));
    }
}

// ============================================================================
// RustParser Implementation
// ============================================================================

void RustParser::advance()
{
    current_token = lexer.nextToken();
}

bool RustParser::match(RustTokenType type) const
{
    return current_token.type == type;
}

bool RustParser::expect(RustTokenType type)
{
    if (!match(type))
        return false;
    advance();
    return true;
}

std::unique_ptr<Type> RustParser::parseScalarType(RustTokenType type)
{
    ReifiedTypeId refiedTypeId;
    switch (type)
    {
    case RustTokenType::I8:
        refiedTypeId = ReifiedTypeId::Int8;
        break;
    case RustTokenType::I16:
        refiedTypeId = ReifiedTypeId::Int16;
        break;
    case RustTokenType::I32:
        refiedTypeId = ReifiedTypeId::Int32;
        break;
    case RustTokenType::I64:
        refiedTypeId = ReifiedTypeId::Int64;
        break;
    case RustTokenType::I128:
        refiedTypeId = ReifiedTypeId::Int64;
        break; // Approximate
    case RustTokenType::Isize:
        refiedTypeId = ReifiedTypeId::Int64;
        break;
    case RustTokenType::U8:
        refiedTypeId = ReifiedTypeId::UInt8;
        break;
    case RustTokenType::U16:
        refiedTypeId = ReifiedTypeId::UInt16;
        break;
    case RustTokenType::U32:
        refiedTypeId = ReifiedTypeId::UInt32;
        break;
    case RustTokenType::U64:
        refiedTypeId = ReifiedTypeId::UInt64;
        break;
    case RustTokenType::U128:
        refiedTypeId = ReifiedTypeId::UInt64;
        break; // Approximate
    case RustTokenType::Usize:
        refiedTypeId = ReifiedTypeId::UInt64;
        break;
    case RustTokenType::F32:
        refiedTypeId = ReifiedTypeId::Float32;
        break;
    case RustTokenType::F64:
        refiedTypeId = ReifiedTypeId::Float64;
        break;
    case RustTokenType::Bool:
        refiedTypeId = ReifiedTypeId::Bool;
        break;
    case RustTokenType::Char:
        refiedTypeId = ReifiedTypeId::Char;
        break;
    case RustTokenType::Str:
        refiedTypeId = ReifiedTypeId::String;
        break;
    case RustTokenType::String:
        refiedTypeId = ReifiedTypeId::String;
        break;
    default:
        refiedTypeId = ReifiedTypeId::Int32;
        break;
    }

    SimpleType simple;
    simple.reifiedType = refiedTypeId;
    return std::make_unique<Type>(simple);
}
std::unique_ptr<Type> RustParser::parseType()
{
    using namespace bhw;

    // Handle references (&T, &mut T)
    if (match(RustTokenType::Ampersand))
    {
        advance();
        if (match(RustTokenType::Mut))
            advance();
        return parseType();
    }

    // Parse potential namespace path (e.g., std::collections::)
    std::string namespace_path;
    if (match(RustTokenType::Identifier))
    {
        namespace_path = current_token.value;
        advance();

        while (match(RustTokenType::DoubleColon))
        {
            advance();
            namespace_path += "::";

            if (match(RustTokenType::Identifier))
            {
                namespace_path += current_token.value;
                advance();
            }
            else
            {
                // Hit a keyword after :: - that's fine, check below
                break;
            }
        }
    }

    // Now check for container keywords (with or without namespace prefix)

    // Handle Vec<T>
    if (match(RustTokenType::Vec))
    {
        advance();
        expect(RustTokenType::LAngle);
        auto element_type = parseType();
        expect(RustTokenType::RAngle);

        GenericType list;
        list.reifiedType = ReifiedTypeId::List;
        list.args.emplace_back(std::move(element_type));
        return std::make_unique<Type>(std::move(list));
    }

    // Handle HashMap<K, V>
    if (match(RustTokenType::HashMap))
    {
        advance();
        expect(RustTokenType::LAngle);
        auto key_type = parseType();
        expect(RustTokenType::Comma);
        auto value_type = parseType();
        expect(RustTokenType::RAngle);

        GenericType map;
        map.reifiedType = ReifiedTypeId::Map;
        map.args.emplace_back(std::move(key_type));
        map.args.emplace_back(std::move(value_type));
        return std::make_unique<Type>(std::move(map));
    }

    // Handle HashSet<T>
    if (match(RustTokenType::HashSet))
    {
        advance();
        expect(RustTokenType::LAngle);
        auto element_type = parseType();
        expect(RustTokenType::RAngle);

        GenericType set;
        set.reifiedType = ReifiedTypeId::Set; // Assuming you have Set
        set.args.emplace_back(std::move(element_type));
        return std::make_unique<Type>(std::move(set));
    }

    // Handle Option<T>
    if (match(RustTokenType::Option))
    {
        advance();
        expect(RustTokenType::LAngle);
        auto inner_type = parseType();
        expect(RustTokenType::RAngle);

        GenericType optional;
        optional.reifiedType = ReifiedTypeId::Optional;
        optional.args.emplace_back(std::move(inner_type));
        return std::make_unique<Type>(std::move(optional));
    }

    // Handle Result<T, E>
    if (match(RustTokenType::Result))
    {
        advance();
        expect(RustTokenType::LAngle);
        auto ok_type = parseType();
        expect(RustTokenType::Comma);
        auto err_type = parseType();
        expect(RustTokenType::RAngle);

        GenericType variant;
        variant.reifiedType = ReifiedTypeId::Variant;
        variant.args.emplace_back(std::move(ok_type));
        variant.args.emplace_back(std::move(err_type));
        return std::make_unique<Type>(std::move(variant));
    }

    // Handle Box<T>
    if (match(RustTokenType::Box))
    {
        advance();
        expect(RustTokenType::LAngle);
        auto inner_type = parseType();
        expect(RustTokenType::RAngle);

        GenericType unique_ptr;
        unique_ptr.reifiedType = ReifiedTypeId::UniquePtr;
        unique_ptr.args.emplace_back(std::move(inner_type));
        return std::make_unique<Type>(std::move(unique_ptr));
    }

    // Handle Arc<T>
    if (match(RustTokenType::Arc))
    {
        advance();
        expect(RustTokenType::LAngle);
        auto inner_type = parseType();
        expect(RustTokenType::RAngle);

        GenericType shared_ptr;
        shared_ptr.reifiedType = ReifiedTypeId::SharedPtr;
        shared_ptr.args.emplace_back(std::move(inner_type));
        return std::make_unique<Type>(std::move(shared_ptr));
    }

    // Handle Rc<T>
    if (match(RustTokenType::Rc))
    {
        advance();
        expect(RustTokenType::LAngle);
        auto inner_type = parseType();
        expect(RustTokenType::RAngle);

        GenericType shared_ptr;
        shared_ptr.reifiedType = ReifiedTypeId::SharedPtr;
        shared_ptr.args.emplace_back(std::move(inner_type));
        return std::make_unique<Type>(std::move(shared_ptr));
    }

    // Handle String (could be std::string::String or just String)
    if (match(RustTokenType::String))
    {
        advance();
        SimpleType simple;
        simple.reifiedType = ReifiedTypeId::String;
        return std::make_unique<Type>(simple);
    }

    // Handle scalar types (i8, i16, u32, f64, bool, etc.)
    if (current_token.type >= RustTokenType::I8 && current_token.type <= RustTokenType::Str)
    {
        auto type = parseScalarType(current_token.type);
        advance();
        return type;
    }

    // If we collected a namespace path but didn't match a keyword,
    // it's a user-defined type (possibly with generics)
    if (!namespace_path.empty() && match(RustTokenType::Identifier))
    {
        namespace_path += current_token.value;
        advance();

        // Handle generic parameters on user types
        if (match(RustTokenType::LAngle))
        {
            advance();
            // Skip generics for now
            int depth = 1;
            while (depth > 0 && !match(RustTokenType::Eof))
            {
                if (match(RustTokenType::LAngle))
                    depth++;
                else if (match(RustTokenType::RAngle))
                    depth--;
                advance();
            }
        }
        SimpleType simple;
        simple.srcTypeString = namespace_path;
        return std::make_unique<Type>(simple);
    }

    // Plain user-defined type (no namespace, already consumed if identifier)
    if (!namespace_path.empty())
    {
        StructRefType type;
        type.srcTypeString = namespace_path;
        type.reifiedType = ReifiedTypeId::StructRefType;
        return std::make_unique<Type>(type);
    }

    // Identifier without namespace (already handled above, but just in case)
    if (match(RustTokenType::Identifier))
    {
        std::string type_name = current_token.value;
        advance();

        // Handle generic parameters
        if (match(RustTokenType::LAngle))
        {
            advance();
            int depth = 1;
            while (depth > 0 && !match(RustTokenType::Eof))
            {
                if (match(RustTokenType::LAngle))
                    depth++;
                else if (match(RustTokenType::RAngle))
                    depth--;
                advance();
            }
        }

        SimpleType simple;
        simple.srcTypeString = type_name;
        return std::make_unique<Type>(simple);
    }

    return nullptr;
}
std::vector<Attribute> RustParser::parseAttributes()
{
    std::vector<Attribute> attributes;

    // Parse #[...] or #![...]
    while (match(RustTokenType::Hash))
    {
        advance();

        // bool inner = false;
        if (match(RustTokenType::Exclamation))
        {
            ////inner = true;
            advance();
        }

        if (!expect(RustTokenType::LBracket))
            break;

        // Parse attribute content
        std::string attr_name;
        std::string attr_value;

        if (match(RustTokenType::Identifier))
        {
            attr_name = current_token.value;
            advance();

            // Handle derive(...) or other function-like attributes
            if (match(RustTokenType::LParen))
            {
                advance();

                // Collect comma-separated identifiers
                while (!match(RustTokenType::RParen) && !match(RustTokenType::Eof))
                {
                    if (match(RustTokenType::Identifier))
                    {
                        if (!attr_value.empty())
                            attr_value += ",";
                        attr_value += current_token.value;
                        advance();
                    }
                    else if (match(RustTokenType::Comma))
                    {
                        advance();
                    }
                    else
                    {
                        advance();
                    }
                }

                expect(RustTokenType::RParen);
            }
            // Handle key = value attributes
            else if (match(RustTokenType::Equals))
            {
                advance();
                if (match(RustTokenType::StringLiteral) || match(RustTokenType::Identifier))
                {
                    attr_value = current_token.value;
                    advance();
                }
            }
        }

        expect(RustTokenType::RBracket);

        Attribute attr;
        attr.name = attr_name;
        attr.value = attr_value;
        attributes.emplace_back(attr);
    }

    return attributes;
}

Field RustParser::parseField()
{
    Field field;

    // Parse field attributes (#[serde(...)])
    field.attributes = parseAttributes();

    // Visibility (pub)
    if (match(RustTokenType::Pub))
    {
        Attribute vis_attr;
        vis_attr.name = "visibility";
        vis_attr.value = "public";
        field.attributes.emplace_back(vis_attr);
        advance();
    }

    // Field name - can be identifier OR keyword used as identifier
    if (match(RustTokenType::Identifier))
    {
        field.name = current_token.value;
        advance();
    }
    // Allow scalar type keywords as field names (i8, u32, bool, etc.)
    else if (current_token.type >= RustTokenType::I8 && current_token.type <= RustTokenType::Str)
    {
        field.name = current_token.value;
        advance();
    }
    // Allow other Rust keywords as field names (type, mod, etc.)
    else if (match(RustTokenType::Type) || match(RustTokenType::Trait) ||
             match(RustTokenType::Impl) || match(RustTokenType::Fn) || match(RustTokenType::Mod) ||
             match(RustTokenType::Use) || match(RustTokenType::Const) ||
             match(RustTokenType::Static) || match(RustTokenType::Struct) ||
             match(RustTokenType::Enum))
    {
        field.name = current_token.value;
        advance();
    }

    // Colon
    expect(RustTokenType::Colon);

    // Type
    field.type = parseType();

    // Comma (optional at end)
    if (match(RustTokenType::Comma))
        advance();

    return field;
}

Struct RustParser::parseStruct()
{
    Struct result;

    // Parse struct attributes
    result.attributes = parseAttributes();

    // Visibility
    if (match(RustTokenType::Pub))
    {
        Attribute vis_attr;
        vis_attr.name = "visibility";
        vis_attr.value = "public";
        result.attributes.emplace_back(vis_attr);
        advance();
    }

    expect(RustTokenType::Struct);

    if (match(RustTokenType::Identifier))
    {
        result.name = current_token.value;
        advance();
    }

    result.namespaces = current_module;

    // Handle generic parameters
    if (match(RustTokenType::LAngle))
    {
        advance();
        int depth = 1;
        while (depth > 0 && !match(RustTokenType::Eof))
        {
            if (match(RustTokenType::LAngle))
            {
            }
            else if (match(RustTokenType::RAngle))
            {
                depth--;
            }
            advance();
        }
    }

    expect(RustTokenType::LBrace);

    while (!match(RustTokenType::RBrace) && !match(RustTokenType::Eof))
    {
        // Save position to detect if we're stuck
        size_t start_pos = lexer.pos;

        Field field = parseField();

        if (field.type)
        {
            result.members.emplace_back(std::move(field));
        }
        else
        {
            // Field parsing failed - skip to next comma or closing brace
            // to avoid infinite loop
            while (!match(RustTokenType::Comma) && !match(RustTokenType::RBrace) &&
                   !match(RustTokenType::Eof))
            {
                advance();
            }

            // Skip the comma if present
            if (match(RustTokenType::Comma))
                advance();
        }

        // Safety check: if we didn't advance at all, force advance
        if (lexer.pos == start_pos)
        {
            advance(); // Prevent infinite loop on stuck tokens
        }
    }

    expect(RustTokenType::RBrace);

    return result;
}

Enum RustParser::parseEnum()
{
    Enum result;

    // Parse enum attributes
    auto attributes = parseAttributes();
    for (const auto& attr : attributes)
        result.attributes.emplace_back(attr);

    // Visibility
    if (match(RustTokenType::Pub))
        advance();

    expect(RustTokenType::Enum);

    if (match(RustTokenType::Identifier))
    {
        result.name = current_token.value;
        advance();
    }

    result.namespaces = current_module;

    // Handle generic parameters
    if (match(RustTokenType::LAngle))
    {
        advance();
        int depth = 1;
        while (depth > 0 && !match(RustTokenType::Eof))
        {
            if (match(RustTokenType::LAngle))
                depth++;
            else if (match(RustTokenType::RAngle))
                depth--;
            advance();
        }
    }

    expect(RustTokenType::LBrace);

    int value = 0;
    while (!match(RustTokenType::RBrace) && !match(RustTokenType::Eof))
    {
        if (match(RustTokenType::Identifier))
        {
            EnumValue ev;
            ev.name = current_token.value;
            ev.number = value++;
            advance();

            // Handle enum variants with data
            if (match(RustTokenType::LParen))
            {
                advance(); // consume '('

                // Parse the type inside parentheses
                auto variant_type = parseType();

                if (variant_type)
                {
                    ev.type = std::move(variant_type);
                }

                // Skip any additional types (for multi-field tuples)
                while (!match(RustTokenType::RParen) && !match(RustTokenType::Eof))
                {
                    if (match(RustTokenType::Comma))
                        advance();
                    else
                        advance();
                }

                expect(RustTokenType::RParen);
            }
            else if (match(RustTokenType::LBrace))
            {
                // Struct variant - skip for now
                advance();
                int depth = 1;
                while (depth > 0 && !match(RustTokenType::Eof))
                {
                    if (match(RustTokenType::LBrace))
                        depth++;
                    else if (match(RustTokenType::RBrace))
                        depth--;
                    advance();
                }
            }

            // Handle explicit discriminant (= value)
            if (match(RustTokenType::Equals))
            {
                advance();
                if (match(RustTokenType::Number))
                {
                    ev.number = std::stoi(current_token.value);
                    value = ev.number + 1;
                    advance();
                }
            }

            result.values.emplace_back(std::move(ev));

            if (match(RustTokenType::Comma))
                advance();
        }
        else
        {
            advance();
        }
    }

    expect(RustTokenType::RBrace);

    return result;
}

void RustParser::parseMod()
{
    expect(RustTokenType::Mod);

    if (match(RustTokenType::Identifier))
    {
        current_module.emplace_back(current_token.value);
        advance();
    }

    if (match(RustTokenType::LBrace))
    {
        advance(); // consume the '{'
        // Module contents will be parsed by the main loop in parseToAst()
        // DON'T skip! DON'T pop current_module here!
    }
    else if (match(RustTokenType::Semicolon))
    {
        // External module: pub mod demo;
        advance();
        if (!current_module.empty())
            current_module.pop_back();
    }
}

void RustParser::parseUse()
{
    expect(RustTokenType::Use);

    // Skip use statements
    while (!match(RustTokenType::Semicolon) && !match(RustTokenType::Eof))
        advance();

    if (match(RustTokenType::Semicolon))
        advance();
}

void RustParser::parseImpl()
{
    expect(RustTokenType::Impl);

    // Skip impl blocks
    while (!match(RustTokenType::LBrace) && !match(RustTokenType::Eof))
        advance();

    if (match(RustTokenType::LBrace))
    {
        advance();
        int depth = 1;
        while (depth > 0 && !match(RustTokenType::Eof))
        {
            if (match(RustTokenType::LBrace))
                depth++;
            else if (match(RustTokenType::RBrace))
                depth--;
            advance();
        }
    }
}

struct RustParseResult
{
    std::vector<Struct> structs;
    std::vector<Enum> enums;
    std::vector<Service> services;
};

auto RustParser::parseToAst(const std::string& src) -> bhw::Ast
{
    bhw::Ast ast;
    lexer.source = src;
    advance();


    // Stack to track where to add nodes (ast.nodes or namespace.nodes)
    std::vector<std::vector<bhw::AstRootNode>*> node_stack;
    node_stack.push_back(&ast.nodes);

    while (!match(RustTokenType::Eof))
    {
        // Handle module closing brace
        if (match(RustTokenType::RBrace))
        {
            if (!current_module.empty())
            {
                current_module.pop_back();
                node_stack.pop_back(); // Pop from stack
            }
            advance();
            continue;
        }

        // Current target for adding nodes
        auto* current_nodes = node_stack.back();

        // Parse attributes at top level
        if (match(RustTokenType::Hash))
        {
            auto attributes = parseAttributes();

            // Attributes apply to next item
            if (match(RustTokenType::Pub))
                advance();

            if (match(RustTokenType::Struct))
            {
                Struct s = parseStruct();
                s.namespaces.clear(); // Clear - using Namespace nodes instead
                s.attributes.insert(s.attributes.begin(), attributes.begin(), attributes.end());
                current_nodes->emplace_back(std::move(s));
            }
            else if (match(RustTokenType::Enum))
            {
                Enum e = parseEnum();
                e.namespaces.clear(); // Clear - using Namespace nodes instead
                e.attributes.insert(e.attributes.begin(), attributes.begin(), attributes.end());
                current_nodes->emplace_back(std::move(e));
            }
            else if (match(RustTokenType::Mod))
            {
                // Create Namespace node
                advance(); // consume mod
                if (match(RustTokenType::Identifier))
                {
                    Namespace ns;
                    ns.name = current_token.value;
                    current_module.push_back(ns.name);
                    advance();

                    if (match(RustTokenType::LBrace))
                    {
                        advance(); // consume {
                        current_nodes->emplace_back(std::move(ns));
                        auto& ns_node = std::get<Namespace>(current_nodes->back());
                        node_stack.push_back(&ns_node.nodes);
                    }
                }
            }
            else
            {
                // Attributes for something else - skip
                advance();
            }
        }
        else if (match(RustTokenType::Pub))
        {
            advance();

            if (match(RustTokenType::Struct))
            {
                Struct s = parseStruct();
                s.namespaces.clear(); // Clear - using Namespace nodes instead
                current_nodes->emplace_back(std::move(s));
            }
            else if (match(RustTokenType::Enum))
            {
                Enum e = parseEnum();
                e.namespaces.clear(); // Clear - using Namespace nodes instead
                current_nodes->emplace_back(std::move(e));
            }
            else if (match(RustTokenType::Mod))
            {
                // Create Namespace node
                if (match(RustTokenType::Identifier))
                {
                    Namespace ns;
                    ns.name = current_token.value;
                    current_module.push_back(ns.name);
                    advance();

                    if (match(RustTokenType::LBrace))
                    {
                        advance();
                        current_nodes->emplace_back(std::move(ns));
                        auto& ns_node = std::get<Namespace>(current_nodes->back());
                        node_stack.push_back(&ns_node.nodes);
                    }
                    else if (match(RustTokenType::Semicolon))
                    {
                        // External module
                        advance();
                        current_module.pop_back();
                    }
                }
            }
            else if (match(RustTokenType::Fn))
            {
                // Skip function
                advance();
                while (!match(RustTokenType::LBrace) && !match(RustTokenType::Eof))
                    advance();

                if (match(RustTokenType::LBrace))
                {
                    advance();
                    int depth = 1;
                    while (depth > 0 && !match(RustTokenType::Eof))
                    {
                        if (match(RustTokenType::LBrace))
                            depth++;
                        else if (match(RustTokenType::RBrace))
                            depth--;
                        advance();
                    }
                }
            }
            else
            {
                advance();
            }
        }
        else if (match(RustTokenType::Struct))
        {
            Struct s = parseStruct();
            s.namespaces.clear(); // Clear - using Namespace nodes instead
            current_nodes->emplace_back(std::move(s));
        }
        else if (match(RustTokenType::Enum))
        {
            Enum e = parseEnum();
            e.namespaces.clear(); // Clear - using Namespace nodes instead
            current_nodes->emplace_back(std::move(e));
        }
        else if (match(RustTokenType::Mod))
        {
            // Non-pub mod
            advance(); // consume mod
            if (match(RustTokenType::Identifier))
            {
                Namespace ns;
                ns.name = current_token.value;
                current_module.push_back(ns.name);
                advance();

                if (match(RustTokenType::LBrace))
                {
                    advance(); // consume {
                    current_nodes->emplace_back(std::move(ns));
                    auto& ns_node = std::get<Namespace>(current_nodes->back());
                    node_stack.push_back(&ns_node.nodes);
                }
                else if (match(RustTokenType::Semicolon))
                {
                    // External module
                    advance();
                    current_module.pop_back();
                }
            }
        }
        else if (match(RustTokenType::Use))
        {
            parseUse();
        }
        else if (match(RustTokenType::Impl))
        {
            parseImpl();
        }
        else if (match(RustTokenType::Fn))
        {
            // Skip standalone function
            advance();
            while (!match(RustTokenType::LBrace) && !match(RustTokenType::Eof))
                advance();

            if (match(RustTokenType::LBrace))
            {
                advance();
                int depth = 1;
                while (depth > 0 && !match(RustTokenType::Eof))
                {
                    if (match(RustTokenType::LBrace))
                        depth++;
                    else if (match(RustTokenType::RBrace))
                        depth--;
                    advance();
                }
            }
        }
        else if (match(RustTokenType::Const) || match(RustTokenType::Static))
        {
            // Skip constants/statics
            while (!match(RustTokenType::Semicolon) && !match(RustTokenType::Eof))
                advance();
            if (match(RustTokenType::Semicolon))
                advance();
        }
        else
        {
            advance();
        }
    }

    return ast;
}
