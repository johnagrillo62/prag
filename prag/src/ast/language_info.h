#pragma once
#include <string>

#include "languages.h"
#include "reified.h"

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

enum class FlatteningPolicy
{
    Preserve, // Keep nested types as-is
    Flatten   // Move them to top-level / lift
};

enum class AnonymousPolicy
{
    Preserve, // inline / anonymous (C++ only)
    Rename,   // inline / named
};
struct FlatteningPolicySet
{
    FlatteningPolicy structs;  // Structs / records
    AnonymousPolicy anonymous; // anoymnous structs C++ only
    FlatteningPolicy enums;    // Simple enums
    FlatteningPolicy oneofs;   // Oneofs / union / sum types
    FlatteningPolicy variants; // Variant types / union-like constructs

    bool needsFlattening() const
    {
        return structs == FlatteningPolicy::Flatten || enums == FlatteningPolicy::Flatten ||
               oneofs == FlatteningPolicy::Flatten || variants == FlatteningPolicy::Flatten;
    }
};

struct LanguageInfo
{
    std::string file_ext;
    std::string comment_style;
    FlatteningPolicySet flattening;
    std::map<ReifiedTypeId, TypeInfo> type_map;
    NamingConventions naming;
};

const std::map<Language, LanguageInfo>& getRegistry();
} // namespace bhw
