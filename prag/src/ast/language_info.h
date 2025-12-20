#pragma once
#include <string>
#include "reified.h"
#include "languages.h"

namespace bhw
{
struct TypeInfo
{
    std::string type_name;
    std::string default_value;
};

struct NamingConventions
{
    std::string struct_name;     // PascalCase, etc.
    std::string field_name;      // snake_case, camelCase, etc.
    std::string constant;        // UPPER_SNAKE_CASE, etc.
    std::string namespace_style; // snake_case, etc.
    std::string file_name;       // snake_case, kebab-case, etc.
};

struct LanguageInfo
{
    std::string file_ext;
    std::string comment_style;
    std::map<ReifiedTypeId, TypeInfo> type_map;
    NamingConventions naming;
};

const std::map<Language, LanguageInfo>& getRegistry();
}
