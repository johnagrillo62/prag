// protobuf_parser.h
#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"


namespace bhw
{
enum class ProtoTokenType
{
    // Keywords
    Syntax,
    Package,
    Import,
    Message,
    Enum,
    Service,
    Rpc,
    Returns,
    Repeated,
    Optional,
    Required,
    Map,
    Oneof,
    Stream,

    // Scalar Types
    Double,
    Float,
    Int32,
    Int64,
    Uint32,
    Uint64,
    Sint32,
    Sint64,
    Fixed32,
    Fixed64,
    Sfixed32,
    Sfixed64,
    Bool,
    String,
    Bytes,

    // Symbols
    LBrace,
    RBrace,
    LParen,
    RParen,
    LAngle,
    RAngle,
    Semicolon,
    Equals,
    Comma,
    Dot,

    // Literals
    Identifier,
    Number,
    StringLiteral,

    Eof,
    Unknown
};

struct ProtoToken
{
    ProtoTokenType type;
    std::string value;
    size_t line;
    size_t column;
};

class ProtoLexer
{
  public:
    ProtoToken nextToken();
    std::string source;
  
  private:

    size_t pos = 0;
    size_t line = 1;
    size_t column = 1;

    char current() const;
    char peek(size_t offset = 1) const;
    void advance();
    void skipWhitespace();
    void skipComment();
    ProtoToken makeToken(ProtoTokenType type, const std::string& value);
    ProtoToken readNumber();
    ProtoToken readString();
    ProtoToken readIdentifier();
};

class ProtoBufParser : public AstParser, public AutoRegisterParser<ProtoBufParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"proto"};
    }
  public:
    ~ProtoBufParser() override = default;
    bhw::Ast parseToAst(const std::string& src) override;
    auto getLang() -> bhw::Language override
    {
        return Language::ProtoBuf;
    }

  private:
    ProtoLexer lexer;
    ProtoToken current_token;
    std::vector<std::string> current_package;

    // TsToken navigation
    void advance();
    bool match(ProtoTokenType type) const;
    bool expect(ProtoTokenType type);

    // Type parsing
    std::unique_ptr<Type> parseScalarType(ProtoTokenType type);
    std::unique_ptr<Type> parseType();

    // Component parsing
    Field parseField();
    Enum parseEnum();
    Oneof parseOneof();
    Service parseService();
    Struct parseMessage();
};
} // namespace bhw
