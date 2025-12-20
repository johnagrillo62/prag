#include "typescript_parser.h"

#include <cctype>
#include <iostream>
#include <stdexcept>

#include "ast.h"

using namespace bhw;

void TypeScriptParser::advance(TypeScriptLexer& lexer, TsToken& current_token)
{
    current_token = lexer.nextToken();
}

void TypeScriptParser::expect(TypeScriptTokenType type,
                              TypeScriptLexer& lexer,
                              TsToken& current_token)
{
    if (current_token.type != type)
    {
        // Calculate the position 100 characters before lexer.pos
        size_t backPos = (lexer.pos >= 100)
                             ? lexer.pos - 100
                             : 0; // Ensure it doesn't go before the start of the string

        // Extract the substring from backPos to lexer.pos
        std::cerr << "******* Back 100 chars: " << backPos << " " << lexer.pos << "\n";
        std::cerr << lexer.source.substr(backPos, lexer.pos - backPos)
                  << " ?? "; // Extract from backPos to lexer.pos
        size_t nextCharsEnd = std::min(
            lexer.pos + 100, lexer.source.length()); // Ensure we don't go beyond the string length
        std::cerr << lexer.source.substr(
            lexer.pos, nextCharsEnd - lexer.pos); // Print from lexer.pos to nextCharsEnd

        throw std::runtime_error("Unexpected token: expected different type");
    }
    advance(lexer, current_token);
}

std::unique_ptr<Type> TypeScriptParser::parseType(TypeScriptLexer& lexer, TsToken& current_token)
{
    // Handle union types: string | number | null
    std::vector<std::unique_ptr<Type>> union_types;

    do
    {
        // Skip pipe if not first type in union
        if (!union_types.empty() && current_token.type == TypeScriptTokenType::PIPE)
        {
            advance(lexer, current_token);
        }

        std::unique_ptr<Type> base_type = parseSingleType(lexer, current_token);
        union_types.emplace_back(std::move(base_type));

    } while (current_token.type == TypeScriptTokenType::PIPE);

    // If single type, return it
    if (union_types.size() == 1)
    {
        return std::move(union_types[0]);
    }

    // Multiple types - create union (for now, just return first type)
    // TODO: Properly represent union types in AST
    return std::move(union_types[0]);
}

std::unique_ptr<Type> TypeScriptParser::parseSingleType(TypeScriptLexer& lexer,
                                                        TsToken& current_token)
{
    std::string type_name = current_token.value;
    advance(lexer, current_token);

    // Check for generics: Array<T>, Map<K, V>
    if (current_token.type == TypeScriptTokenType::LT)
    {
        advance(lexer, current_token); // consume

        // Only handle known generic types
        if (type_name == "Array")
        {
            GenericType generic_type;
            generic_type.reifiedType = ReifiedTypeId::List;

            // Parse type argument
            generic_type.args.emplace_back(parseType(lexer, current_token));

            expect(TypeScriptTokenType::GT, lexer, current_token); // consume >

            return std::make_unique<Type>(std::move(generic_type));
        }
        else if (type_name == "Map")
        {
            GenericType generic_type;
            generic_type.reifiedType = ReifiedTypeId::Map;

            // Parse key type
            generic_type.args.emplace_back(parseType(lexer, current_token));

            if (current_token.type == TypeScriptTokenType::COMMA)
            {
                advance(lexer, current_token);
                // Parse value type
                generic_type.args.emplace_back(parseType(lexer, current_token));
            }

            expect(TypeScriptTokenType::GT, lexer, current_token); // consume >

            return std::make_unique<Type>(std::move(generic_type));
        }
        else
        {
            // Unknown generic type - skip generic args and treat as simple type
            int depth = 1;
            while (depth > 0 && current_token.type != TypeScriptTokenType::END_OF_FILE)
            {
                if (current_token.type == TypeScriptTokenType::LT)
                    depth++;
                if (current_token.type == TypeScriptTokenType::GT)
                    depth--;
                advance(lexer, current_token);
            }

            SimpleType simple_type;
            simple_type.srcTypeString = type_name;
            return std::make_unique<Type>(simple_type);
        }
    }

    // Check for array syntax: string[]
    if (current_token.type == TypeScriptTokenType::LBRACKET)
    {
        advance(lexer, current_token);                               // consume [
        expect(TypeScriptTokenType::RBRACKET, lexer, current_token); // consume ]

        GenericType generic_type;
        generic_type.reifiedType = ReifiedTypeId::List;

        // Map TypeScript types to canonical types
        ReifiedTypeId element_type = ReifiedTypeId::Unknown;
        if (type_name == "string")
            element_type = ReifiedTypeId::String;
        else if (type_name == "number")
            element_type = ReifiedTypeId::Int32;
        else if (type_name == "boolean")
            element_type = ReifiedTypeId::Bool;

        SimpleType simple_type;
        simple_type.reifiedType = element_type;
        simple_type.srcTypeString = type_name;
        generic_type.args.emplace_back(std::make_unique<Type>(simple_type));

        return std::make_unique<Type>(std::move(generic_type));
    }

    // Simple type - map to canonical if possible
    ReifiedTypeId canonical = ReifiedTypeId::Unknown;
    if (type_name == "string")
        canonical = ReifiedTypeId::String;
    else if (type_name == "number")
        canonical = ReifiedTypeId::Int32;
    else if (type_name == "boolean")
        canonical = ReifiedTypeId::Bool;
    else if (type_name == "any")
        canonical = ReifiedTypeId::Unknown;

    SimpleType simple_type;
    simple_type.reifiedType = canonical;

    return std::make_unique<Type>(simple_type);
}

Struct TypeScriptParser::parseInterface(TypeScriptLexer& lexer, TsToken& current_token)
{
    expect(TypeScriptTokenType::INTERFACE, lexer, current_token);

    Struct entity;
    entity.name = current_token.value;
    expect(TypeScriptTokenType::IDENTIFIER, lexer, current_token);

    expect(TypeScriptTokenType::LBRACE, lexer, current_token);

    while (current_token.type != TypeScriptTokenType::RBRACE &&
           current_token.type != TypeScriptTokenType::END_OF_FILE)
    {
        Field f;
        f.name = current_token.value;
        if (current_token.type == TypeScriptTokenType::TYPE)
        {
            expect(TypeScriptTokenType::TYPE, lexer, current_token);
        }
        else
        {
            expect(TypeScriptTokenType::IDENTIFIER, lexer, current_token);
        }

        // Check for optional marker '?'
        bool is_optional = false;
        if (current_token.type == TypeScriptTokenType::QUESTION)
        {
            is_optional = true;
            advance(lexer, current_token);
        }

        expect(TypeScriptTokenType::COLON, lexer, current_token);

        // Parse the type (handles arrays, generics, unions, nested)
        auto field_type = parseType(lexer, current_token);

        // Wrap in optional if needed
        if (is_optional)
        {
            PointerType ptr_type;
            ptr_type.pointee = std::move(field_type);
            f.type = std::make_unique<Type>(std::move(ptr_type));
        }
        else
        {
            f.type = std::move(field_type);
        }

        expect(TypeScriptTokenType::SEMICOLON, lexer, current_token);
        entity.members.emplace_back(std::move(f));
    }

    expect(TypeScriptTokenType::RBRACE, lexer, current_token);
    return entity;
}

Enum TypeScriptParser::parseEnum(TypeScriptLexer& lexer, TsToken& current_token)
{
    expect(TypeScriptTokenType::ENUM, lexer, current_token);

    Enum enum_type;
    enum_type.name = current_token.value;
    expect(TypeScriptTokenType::IDENTIFIER, lexer, current_token);
    expect(TypeScriptTokenType::LBRACE, lexer, current_token);

    int auto_value = 0;

    while (current_token.type != TypeScriptTokenType::RBRACE &&
           current_token.type != TypeScriptTokenType::END_OF_FILE)
    {
        EnumValue ev;
        ev.name = current_token.value;
        expect(TypeScriptTokenType::IDENTIFIER, lexer, current_token);

        // Check for explicit value
        if (current_token.type == TypeScriptTokenType::EQUALS)
        {
            advance(lexer, current_token);

            if (current_token.type == TypeScriptTokenType::NUMBER_LITERAL)
            {
                ev.number = std::stoi(current_token.value);
                auto_value = ev.number + 1;
                advance(lexer, current_token);
            }
            else if (current_token.type == TypeScriptTokenType::STRING_LITERAL)
            {
                // String enum value
                ev.number = auto_value++;
                advance(lexer, current_token);
            }
        }
        else
        {
            ev.number = auto_value++;
        }

        enum_type.values.emplace_back(std::move(ev));

        // Handle optional comma
        if (current_token.type == TypeScriptTokenType::COMMA)
        {
            advance(lexer, current_token);
        }
    }

    expect(TypeScriptTokenType::RBRACE, lexer, current_token);
    return enum_type;
}

auto TypeScriptParser::parseToAst(const std::string& src) -> bhw::Ast
{
    TypeScriptLexer lexer(src);
    TsToken current_token = lexer.nextToken();
    bhw::Ast ast;
    // Main parsing loop
    while (current_token.type != TypeScriptTokenType::END_OF_FILE)
    {
        if (current_token.type == TypeScriptTokenType::INTERFACE)
        {
            ast.nodes.emplace_back(parseInterface(lexer, current_token));
        }
        else if (current_token.type == TypeScriptTokenType::ENUM)
        {
            ast.nodes.emplace_back(parseEnum(lexer, current_token));
        }
        else
        {
            advance(lexer, current_token);
        }
    }

    return ast;
}
