#pragma once
#include "ast.h"
#include "languages.h"
#include "string.h"

namespace bhw
{
class AstParser
{
  public:
    AstParser() = default;
    virtual ~AstParser() = default;
    virtual auto getLang() -> bhw::Language = 0;
    virtual auto parseToAst(const std::string& src) -> bhw::Ast = 0;
};
} // namespace bhw
