#include "ast.h"
#include "cpp_walker.h"
#include "cpp_parser.h"
#include <sstream>
#include <iostream>

#include "registry_ast_walker.h"

std::string inputFile;

namespace
{
class CppEnumWalker : public bhw::RegistryAstWalker
{
  public:
    CppEnumWalker() : bhw::RegistryAstWalker(bhw::Language::Cpp26)
    {
    }
    std::string generateOneof(const bhw::Oneof& oneof, const bhw::WalkContext& ctx) override
    {
        return "";
    }
  protected:
  virtual std::string walkEnum(const bhw::Enum& e, const bhw::WalkContext& ctx)
  {
      std::stringstream str;
      std::string result;

      str << "// ============================================================================\n"
	  << "// AUTO-GENERATED FILE - DO NOT EDIT MANUALLY\n"
	  << "// ============================================================================\n"
	  << "// This file was generated " ;

      if (!inputFile.empty())
	{
	  str << "  from " << inputFile << " ";
	}
      str << "by the enum code generator.\n"
	  << "//\n"
	  << "// This provides automatic enum reflection for " << e.name << ":\n"
	  << "//   - " << e.name << "IdEnum::toString(value) -> string\n"
	  << "//   - " << e.name << "IdEnum::fromString(str) -> optional<" << e.name << ">\n"
	  << "//   - " << e.name << "IdEnum::forEach(fn) -> iterate all values\n"
	  << "//   - operator<< for streaming enums\n"
	  << "// ============================================================================\n"
	  << "\n"
	  << "inline constexpr std::array<std::pair<"
	  << e.name << ",const char*>, " << e.values.size() << ">" << " " << e.name << "Mapping {{\n";
      
      for (const auto& value : e.values)
      {
	str << "    {" << e.name << "::" << value.name << ", \"" << value.name << "\"},\n";
      }
      str << "}};\n";

      str << "using " << e.name << "Enum = EnumTraitsAuto<" << e.name << ", " << e.name << "Mapping>;\n";
      str << "template <> struct EnumMapping<" << e.name << "> { using Type = " << e.name << "Enum; };\n";
      return str.str();
      
    }
};
} // namespace



int main(int argc, char *argv[])
{
  inputFile = argv[1];
  std::string source = bhw::readFile(inputFile);
  bhw::CppParser parser;
  auto ast = parser.parseToAst(source);
  CppEnumWalker walker;
  std::cout << walker.walk(std::move(ast)) << "\n";
}

/*
// ==================== Example enums ====================
enum class Language : uint8_t { Avro, CSharp, Capnp, Cpp26, FSharp };
inline constexpr std::array<std::pair<Language,const char*>,5> languageMapping {{
    {Language::Avro, "Avro"},
    {Language::CSharp, "CSharp"},
    {Language::Capnp, "Capnp"},
    {Language::Cpp26, "Cpp26"},
    {Language::FSharp, "FSharp"}
}};



*/




