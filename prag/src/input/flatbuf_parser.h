// flatbuf_parser.h
#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"


namespace bhw
{

enum class FlatBufTokenType
{
    // Keywords
    Namespace,
    Table,
    Struct,
    Enum,
    Union,
    Root_Type,
    File_Identifier,

    // Types
    Bool,
    Byte,
    UByte,
    Short,
    UShort,
    Int,
    UInt,
    Float,
    Long,
    ULong,
    Double,
    String,

    // Symbols
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Colon,
    Semicolon,
    Comma,
    Equal,

    Identifier,
    IntLiteral,
    StringLiteral,
    Comment,
    EndOfFile,
    Unknown
};

struct FlatBufToken
{
    FlatBufTokenType type{};
    std::string value{};
    int line{};
    int column{};
};

class FlatBufLexer
{
  public:
    explicit FlatBufLexer(std::string_view source);
    std::vector<FlatBufToken> tokenize();

  private:
    void skipWhitespace();
    void skipComment();
    FlatBufToken readIdentifierOrKeyword();
    FlatBufToken readNumber();
    FlatBufToken readString();

    char peek(int offset = 0) const;
    char advance();
    bool isAtEnd() const;

    std::string_view source_;
    size_t pos_ = 0;
    int line_ = 1;
    int column_ = 1;
};

class FlatBufParser : public bhw::AstParser, public AutoRegisterParser<FlatBufParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"fbs"};
    }
  public:
    auto parseToAst(const std::string& src) -> bhw::Ast override;
    auto getLang() -> bhw::Language override
    {
        return Language::FlatBuf;
    }

  private:
    const FlatBufToken& peek(int offset = 0) const;
    const FlatBufToken& advance();
    bool match(FlatBufTokenType type);
    bool check(FlatBufTokenType type) const;
    void expect(FlatBufTokenType type, const std::string& message);

    std::string parseNamespace();
    Struct parseTable();
    Struct parseStruct();
    Struct parseEnum();
    Field parseField();
    std::unique_ptr<Type> parseType();

    ReifiedTypeId mapFlatBufType(FlatBufTokenType type);

    std::vector<FlatBufToken> tokens_;
    size_t pos_ = 0;
    std::string current_namespace_;
};
} // namespace bhw
