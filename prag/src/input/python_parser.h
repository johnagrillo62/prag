#pragma once

#include <memory>
#include <string>

#include "ast.h"
#include "ast_parser.h"
#include "parser_registry2.h"

namespace bhw
{
class PythonParserImpl;

class PythonParser : public bhw::AstParser
    //, public AutoRegisterParser<PythonParser>
{
    /*
  public:
    static std::vector<std::string> extensions()
    {
        return {"python"};
    }
    */
  private:
    std::unique_ptr<PythonParserImpl> impl;

  public:
    virtual ~PythonParser();

    auto parseToAst(const std::string& src) -> bhw::Ast override;
    auto getLang() -> bhw::Language override
    {
        return Language::Python;
    }
};
} // namespace bhw
