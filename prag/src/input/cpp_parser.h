#pragma once
#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"


namespace bhw
{

struct CPPTypeMapping
{
    std::string_view name;
    ReifiedTypeId id;
};

// All mappings in a constexpr array
constexpr std::array<CPPTypeMapping, 60> cpp_to_canonical{
    {// Primitives
     {"bool", ReifiedTypeId::Bool},
     {"char", ReifiedTypeId::Char},
     {"signed char", ReifiedTypeId::Int8},
     {"unsigned char", ReifiedTypeId::UInt8},
     {"int8_t", ReifiedTypeId::Int8},
     {"uint8_t", ReifiedTypeId::UInt8},
     {"byte", ReifiedTypeId::UInt8},
     {"std::byte", ReifiedTypeId::UInt8},
     {"short", ReifiedTypeId::Int16},

     {"unsigned short", ReifiedTypeId::UInt16},
     {"int", ReifiedTypeId::Int32},
     {"signed", ReifiedTypeId::Int32},
     {"int32_t", ReifiedTypeId::Int32},
     {"signed int", ReifiedTypeId::Int32},
     {"unsigned", ReifiedTypeId::UInt32},
     {"uint32_t", ReifiedTypeId::UInt32},
     {"unsigned int", ReifiedTypeId::UInt32},
     {"long", ReifiedTypeId::Int64},
     {"signed long", ReifiedTypeId::Int64},
     {"unsigned long", ReifiedTypeId::UInt64},
     {"int64_t", ReifiedTypeId::Int64},
     {"uint64_t", ReifiedTypeId::UInt64},
     {"long long", ReifiedTypeId::Int64},
     {"signed long long", ReifiedTypeId::Int64},
     {"unsigned long long", ReifiedTypeId::UInt64},
     {"float", ReifiedTypeId::Float32},
     {"double", ReifiedTypeId::Float64},
     {"std::string", ReifiedTypeId::String},
     {"string", ReifiedTypeId::String},
     {"std::chrono::system_clock::time_point", ReifiedTypeId::DateTime},
     {"std::chrono::year_month_day", ReifiedTypeId::Date},
     {"std::chrono::hh_mm_ss", ReifiedTypeId::Time},
     {"std::chrono::duration", ReifiedTypeId::Duration},
     {"std::array<uint8_t, 16>", ReifiedTypeId::UUID},
     {"std::vector", ReifiedTypeId::List},
     {"vector", ReifiedTypeId::List},
     {"std::map", ReifiedTypeId::Map},
     {"map", ReifiedTypeId::Map},
     {"std::set", ReifiedTypeId::Set},
     {"set", ReifiedTypeId::Set},
     {"std::unordered_map", ReifiedTypeId::UnorderedMap},
     {"unordered_map", ReifiedTypeId::UnorderedMap},
     {"std::unordered_set", ReifiedTypeId::UnorderedSet},
     {"unordered_set", ReifiedTypeId::UnorderedSet},
     {"std::optional", ReifiedTypeId::Optional},
     {"optional", ReifiedTypeId::Optional},
     {"std::tuple", ReifiedTypeId::Tuple},
     {"tuple", ReifiedTypeId::Tuple},
     {"std::variant", ReifiedTypeId::Variant},
     {"variant", ReifiedTypeId::Variant},
     {"std::monostate", bhw::ReifiedTypeId::Monostate},
     {"monostate", bhw::ReifiedTypeId::Monostate},
     {"std::pair", ReifiedTypeId::Pair},
     {"pair", ReifiedTypeId::Pair},
     {"std::array", ReifiedTypeId::Array},
     {"array", ReifiedTypeId::Array},
     {"std::unique_ptr", ReifiedTypeId::UniquePtr},
     {"unique_ptr", ReifiedTypeId::UniquePtr},
     {"std::shared_ptr", ReifiedTypeId::SharedPtr},
     {"shared_ptr", ReifiedTypeId::SharedPtr}}};

inline auto cppToCanonicalMap() -> const std::map<std::string, ReifiedTypeId>&
{
    static const std::pair<const char*, ReifiedTypeId> mappings[] = {
        {"bool", ReifiedTypeId::Bool},
        {"char", ReifiedTypeId::Char},
        {"signed char", ReifiedTypeId::Int8},
        {"unsigned char", ReifiedTypeId::UInt8},
        {"int8_t", ReifiedTypeId::Int8},
        {"uint8_t", ReifiedTypeId::UInt8},
        {"byte", ReifiedTypeId::UInt8},
        {"std::byte", ReifiedTypeId::UInt8},
        {"short", ReifiedTypeId::Int16},
        {"unsigned short", ReifiedTypeId::UInt16},
        {"int16_t", ReifiedTypeId::Int16},
        {"uint16_t", ReifiedTypeId::UInt16},
        {"int", ReifiedTypeId::Int32},
        {"signed", ReifiedTypeId::Int32},
        {"int32_t", ReifiedTypeId::Int32},
        {"signed int", ReifiedTypeId::Int32},
        {"unsigned", ReifiedTypeId::UInt32},
        {"uint32_t", ReifiedTypeId::UInt32},
        {"unsigned int", ReifiedTypeId::UInt32},
        {"long", ReifiedTypeId::Int64},
        {"signed long", ReifiedTypeId::Int64},
        {"unsigned long", ReifiedTypeId::UInt64},
        {"int64_t", ReifiedTypeId::Int64},
        {"uint64_t", ReifiedTypeId::UInt64},
        {"long long", ReifiedTypeId::Int64},
        {"signed long long", ReifiedTypeId::Int64},
        {"unsigned long long", ReifiedTypeId::UInt64},
        {"float", ReifiedTypeId::Float32},
        {"double", ReifiedTypeId::Float64},

        {"std::string", ReifiedTypeId::String},
        {"string", ReifiedTypeId::String},

        {"std::chrono::system_clock::time_point", ReifiedTypeId::DateTime},
        {"std::chrono::year_month_day", ReifiedTypeId::Date},
        {"std::chrono::hh_mm_ss", ReifiedTypeId::Time},
        {"std::chrono::duration", ReifiedTypeId::Duration},
        {"std::array<uint8_t, 16>", ReifiedTypeId::UUID},

        {"std::vector", ReifiedTypeId::List},
        {"vector", ReifiedTypeId::List},
        {"std::map", ReifiedTypeId::Map},
        {"map", ReifiedTypeId::Map},
        {"std::set", ReifiedTypeId::Set},
        {"set", ReifiedTypeId::Set},
        {"std::unordered_map", ReifiedTypeId::UnorderedMap},
        {"unordered_map", ReifiedTypeId::UnorderedMap},
        {"std::unordered_set", ReifiedTypeId::UnorderedSet},
        {"unordered_set", ReifiedTypeId::UnorderedSet},
        {"std::optional", ReifiedTypeId::Optional},
        {"optional", ReifiedTypeId::Optional},
        {"std::tuple", ReifiedTypeId::Tuple},
        {"tuple", ReifiedTypeId::Tuple},
        {"std::variant", ReifiedTypeId::Variant},
        {"variant", ReifiedTypeId::Variant},
        {"std::monostate", ReifiedTypeId::Monostate},
        {"monostate", ReifiedTypeId::Monostate},  
        {"std::pair", ReifiedTypeId::Pair},
        {"pair", ReifiedTypeId::Pair},
        {"std::array", ReifiedTypeId::Array},
        {"array", ReifiedTypeId::Array},

        {"std::unique_ptr", ReifiedTypeId::UniquePtr},
        {"unique_ptr", ReifiedTypeId::UniquePtr},
        {"std::shared_ptr", ReifiedTypeId::SharedPtr},
        {"shared_ptr", ReifiedTypeId::SharedPtr},
    };

    static const std::map<std::string, ReifiedTypeId> m(std::begin(mappings), std::end(mappings));

    return m;
}
enum class CppTokenType : uint8_t
{
    Eof,
    Identifier,
    Number,

    // Keywords
    Struct,
    Enum,
    Class,
    Namespace,
    Using,
    Typedef,
    // Symbols
    LBrace,    // {
    RBrace,    // }
    LAngle,    // <
    RAngle,    // >
    Semicolon, // ;
    Comma,     // ,
    Colon,     // ::
    Star,      // *
    Equals,    // =
   
    // Special
    Attribute, // //@ ...
    Unknown
};

struct CppToken
{
    // Explicit constructor initializing all members
    explicit CppToken(CppTokenType t = {}, std::string v = {}, size_t l = 0, size_t c = 0)
        : type(t),
          value(std::move(v)),
          line(l),
          column(c)
    {
    }
    CppTokenType type{};
    std::string value{};
    size_t line{};
    size_t column{};
};

// Parse result containing all parsed structures
struct CppParseResult
{
    std::vector<Struct> structs;
    std::vector<Enum> enums;
    Ast ast;
};

class CppLexer
{
  public:
    CppToken nextToken();

    // Need public access for parser lookahead
    size_t pos = 0;
    std::string source;

  private:
    size_t line = 1;
    size_t column = 1;

    char current() const;
    char peek(size_t offset = 1) const;
    void advance();

    void skipWhitespace();
    void skipLineComment();
    void skipBlockComment();
    void skipCppAttribute();  // Skip [[...]] C++ attributes

    CppToken makeToken(CppTokenType type, const std::string& value);
    CppToken readNumber();
    CppToken readIdentifier();
    CppToken readAttribute();

    bool matchChar(char c);
    bool matchString(const std::string& str);

    friend class CppParser;
};

class CppParser : public AstParser, public AutoRegisterParser<CppParser>
{
  public:
    static std::vector<std::string> extensions()
    {
        return {"h", "cpp"};
    }

  public:
    auto parseToAst(const std::string& src) -> bhw::Ast override;

    auto getLang() -> bhw::Language override
    {
        return Language::Cpp26;
    }

  private:
    CppLexer lexer;
    CppToken current_token;
    std::vector<std::string> namespace_stack_;
    std::vector<Attribute> pending_attributes_;
    std::set<std::string> known_user_types_;

    // Store parsed items
    std::vector<Struct> structs;
    std::vector<Enum> enums;

    void advance();
    bool match(CppTokenType type) const;
    bool expect(CppTokenType type);
    bool peek_ahead_is_struct();

    // Attribute handling
    std::vector<Attribute> collectPendingAttributes();

    // Name parsing
    std::string parseQualifiedName();

    // Type parsing and validation
    std::unique_ptr<Type> parseType(size_t level = 0);
    std::unique_ptr<Type> resolveType(const std::string& type_name);
    std::string suggestSimilarType(const std::string& typo) const;
    std::string parseBuiltinType();

    // User type registration
    void registerUserType(const std::string& type_name);
    bool isKnownType(const std::string& type_name) const;

    // Struct/field parsing
    Field parseField();
    Struct parseStruct();
    Struct parseNestedStruct();
    Enum parseEnum();

    // Namespace parsing
    bhw::Namespace parseNamespace(size_t indent = 0);
    std::vector<bhw::AstRootNode> parseDeclarations(size_t indent = 0);

    // Two-pass support
    void collectStructNames();
};
} // namespace bhw


