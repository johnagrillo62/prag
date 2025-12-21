#pragma once

#include <string>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"

namespace bhw
{
enum class RustTokenType
{
    // Literals
    Eof,
    Identifier,
    Number,
    StringLiteral,
    CharLiteral,

    // Keywords
    Struct,
    Enum,
    Impl,
    Trait,
    Type,
    Fn,
    Pub,
    Mod,
    Use,
    Const,
    Static,
    Let,
    Mut,
    Ref,

    // Types
    I8,
    I16,
    I32,
    I64,
    I128,
    Isize,
    U8,
    U16,
    U32,
    U64,
    U128,
    Usize,
    F32,
    F64,
    Bool,
    Char,
    Str,
    String,
    Vec,
    Option,
    Result,
    Box,
    Rc,
    Arc,
    HashMap,
    HashSet,

    // Punctuation
    LBrace,      // {
    RBrace,      // }
    LParen,      // (
    RParen,      // )
    LBracket,    // [
    RBracket,    // ]
    LAngle,      //
    RAngle,      // >
    Semicolon,   // ;
    Colon,       // :
    DoubleColon, // ::
    Comma,       // ,
    Dot,         // .
    Equals,      // =
    Arrow,       // ->
    FatArrow,    // =>
    Ampersand,   // &
    Star,        // *
    Hash,        // #
    Exclamation, // !
    Question,    // ?

    Unknown
};

struct RustToken
{
    RustTokenType type{};
    std::string value{};
    size_t line{};
    size_t column{};
};

class RustLexer
{
  public:
    RustToken nextToken();

    std::string source;
    size_t pos = 0;
    size_t line = 1;
    size_t column = 1;

    char current() const;
    char peek(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipComment();

    RustToken makeToken(RustTokenType type, const std::string& value);
    RustToken readNumber();
    RustToken readString();
    RustToken readChar();
    RustToken readIdentifier();
    RustToken readRawString();
};

class RustParser : public bhw::AstParser, public AutoRegisterParser<RustParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"rs"};
    }
    
  public:
    auto parseToAst(const std::string& src) -> bhw::Ast override;
    auto getLang() -> bhw::Language override
    {
        return Language::Rust;
    }

  private:
    RustLexer lexer;
    RustToken current_token;
    std::vector<std::string> current_module;

    void advance();
    bool match(RustTokenType type) const;
    bool expect(RustTokenType type);

    std::unique_ptr<Type> parseScalarType(RustTokenType type);
    std::unique_ptr<Type> parseType();

    Field parseField();
    Struct parseStruct();
    Enum parseEnum();
    std::vector<Attribute> parseAttributes();

    void parseMod();
    void parseUse();
    void parseImpl();
};
} // namespace bhw
