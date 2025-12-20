
/**
 * @file meta_lang.h
 * @brief Cross-Language Type Mapping and Reflection System
 * @author John Grillo
 * @version 1.0
 *
 * Idea by John Grillo
 * Database of languages by Claude AI

 * A C++ template metaprogramming systestruct LanguageConfig<Language::PERL> {
    struct Types {
        static constexpr const char* int_val = "Int";
        static constexpr const char* int16_val = "Int";
        static constexpr const char* string_val = "Str";
        static constexpr const char* bool_val = "Bool";
        static constexpr const char* double_val = "Num";
        static constexpr const char* float_val = "Num";
    };

    struct Containers {
        static constexpr const char* vector_fmt = "ArrayRef[{}]";
        static constexpr const char* map_fmt = "HashRef[{}]";
        static constexpr const char* optional_fmt = "Maybe[{}]";
    };
};
 * Automatically maps C++ types
 * to equivalent types in different programming languages. Uses compile-time
 * reflection to generate language-specific type definitions from a single
 * C++ source of truth.
 *
 * Key Features:
 * - Single definition, multi-language output
 * - Compile-time type safety and validation
 * - Extensible language configuration system
 * - Support for primitives, containers, and optionals
 * - Zero runtime overhead (all template metaprogramming)
 *
 * Supported Languages:
 * C++, Java, Python, TypeScript, Rust, Go, C#, Kotlin, Swift, JavaScript,
 * PHP, Ruby, Scala, Dart, Lua, Perl, Haskell, Elixir, Clojure, F#,
 * VB.NET, Objective-C, R, MATLAB, Julia
 *
 * Usage Example:
 * @code
 * struct MyData {
 *     int id;
 *     std::string name;
 *     std::vector<double> values;
 * };
 *
 * // Generate type definitions for all languages
 * std::string cpp_def = meta::reflect<MyData, Language::CPP>();
 * std::string java_def = meta::reflect<MyData, Language::JAVA>();
 * std::string rust_def = meta::reflect<MyData, Language::RUST>();
 * @endcode
 *
 * Architecture:
 * - LanguageConfig<Lang>: Type mapping configurations per language
 * - CleanTypeMapper<T, Lang>: Template-based type resolution
 * - meta::reflect<T, Lang>(): Main reflection entry point
 *
 * @note Requires C++17 or later for constexpr if and fold expressions
 */

#pragma once
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

enum class Language
{
    CPP,
    JAVA,
    PYTHON,
    TYPESCRIPT,
    RUST,
    GO,
    CSHARP,
    KOTLIN,
    SWIFT,
    JAVASCRIPT,
    PHP,
    RUBY,
    SCALA,
    DART,
    LUA,
    PERL,
    HASKELL,
    ELIXIR,
    CLOJURE,
    FSHARP,
    VB_NET,
    OBJECTIVEC,
    R,
    MATLAB,
    JULIA
};

#include <unordered_map>

const std::unordered_map<Language, std::string> languageMap = {
    {Language::CPP, "C++"},
    {Language::JAVA, "Java"},
    {Language::PYTHON, "Python"},
    {Language::TYPESCRIPT, "TypeScript"},
    {Language::RUST, "Rust"},
    {Language::GO, "Go"},
    {Language::CSHARP, "C#"},
    {Language::KOTLIN, "Kotlin"},
    {Language::SWIFT, "Swift"},
    {Language::JAVASCRIPT, "JavaScript"},
    {Language::PHP, "PHP"},
    {Language::RUBY, "Ruby"},
    {Language::SCALA, "Scala"},
    {Language::DART, "Dart"},
    {Language::LUA, "Lua"},
    {Language::PERL, "Perl"},
    {Language::HASKELL, "Haskell"},
    {Language::ELIXIR, "Elixir"},
    {Language::CLOJURE, "Clojure"},
    {Language::FSHARP, "F#"},
    {Language::VB_NET, "VB.NET"},
    {Language::OBJECTIVEC, "Objective-C"},
    {Language::R, "R"},
    {Language::MATLAB, "MATLAB"},
    {Language::JULIA, "Julia"}};

std::string languageToString(Language lang)
{
    auto it = languageMap.find(lang);
    return (it != languageMap.end()) ? it->second : "Unknown";
}

// Group all type mappings for a language in one place
template <Language Lang> struct LanguageAttributes
{
    // Will be specialized for each language
};

// Convenience function
template <typename T, Language Lang> struct TypeMapper
{
    static std::string map()
    {
        using Attrs = LanguageAttributes<Lang>;

        if constexpr (std::is_same_v<T, int>)
        {
            return Attrs::int_type;
        }
        else if constexpr (std::is_same_v<T, int16_t>)
        {
            return Attrs::int16_type;
        }
        else if constexpr (std::is_same_v<T, int32_t>)
        {
            return Attrs::int32_type;
        }
        else if constexpr (std::is_same_v<T, int64_t>)
        {
            return Attrs::int64_type;
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            return Attrs::string_type;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            return Attrs::bool_type;
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            return Attrs::double_type;
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            return Attrs::float_type;
        }
        return "unknown";
    }
};

std::string format_string(const std::string& format, const std::string& replacement)
{
    std::string result = format;
    size_t pos = result.find("{}");
    if (pos != std::string::npos)
    {
        result.replace(pos, 2, replacement);
    }
    return result;
}

std::string format_string(const std::string& format,
                          const std::string& first,
                          const std::string& second)
{
    std::string result = format;
    size_t pos = result.find("{}");
    if (pos != std::string::npos)
    {
        result.replace(pos, 2, first);
        pos = result.find("{}");
        if (pos != std::string::npos)
        {
            result.replace(pos, 2, second);
        }
    }
    return result;
}


template <Language Lang> struct LanguageConfig
{
    // Will be specialized per language
};

template <> struct LanguageConfig<Language::CPP>
{
    struct Types
    {
        static constexpr const char* int_val = "int";
        static constexpr const char* int16_val = "int16_t";
        static constexpr const char* string_val = "std::string";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "double";
        static constexpr const char* float_val = "float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "std::vector<{}>";
        static constexpr const char* map_fmt = "std::map<{}, {}>";
        static constexpr const char* optional_fmt = "std::optional<{}>";
    };
};

template <> struct LanguageConfig<Language::JAVA>
{
    struct Types
    {
        static constexpr const char* int_val = "Integer";
        static constexpr const char* int16_val = "Short";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Boolean";
        static constexpr const char* double_val = "Double";
        static constexpr const char* float_val = "Float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "List<{}>";
        static constexpr const char* map_fmt = "Map<{}, {}>";
        static constexpr const char* optional_fmt = "Optional<{}>";
    };
};

// Ultra-clean TypeMapper using config injection
template <typename T, Language Lang> struct CleanTypeMapper
{
    static std::string map()
    {
        using Config = LanguageConfig<Lang>;

        if constexpr (std::is_same_v<T, int>)
            return Config::Types::int_val;
        else if constexpr (std::is_same_v<T, int16_t>)
            return Config::Types::int16_val;
        else if constexpr (std::is_same_v<T, std::string>)
            return Config::Types::string_val;
        else if constexpr (std::is_same_v<T, bool>)
            return Config::Types::bool_val;
        else if constexpr (std::is_same_v<T, double>)
            return Config::Types::double_val;
        else if constexpr (std::is_same_v<T, float>)
            return Config::Types::float_val;
        return "unknown";
    }
};

// Container specializations using config
template <typename T, Language Lang> struct CleanTypeMapper<std::vector<T>, Lang>
{
    static std::string map()
    {
        using Config = LanguageConfig<Lang>;
        std::string element_type = CleanTypeMapper<T, Lang>::map();
        return format_string(Config::Containers::vector_fmt, element_type);
    }
};

template <typename K, typename V, Language Lang> struct CleanTypeMapper<std::map<K, V>, Lang>
{
    static std::string map()
    {
        using Config = LanguageConfig<Lang>;
        std::string key_type = CleanTypeMapper<K, Lang>::map();
        std::string value_type = CleanTypeMapper<V, Lang>::map();
        return format_string(Config::Containers::map_fmt, key_type, value_type);
    }
};

// Just add the language config and you're done!
template <> struct LanguageConfig<Language::RUST>
{
    struct Types
    {
        static constexpr const char* int_val = "i32";
        static constexpr const char* int16_val = "i16";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "f64";
        static constexpr const char* float_val = "f32";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "Vec<{}>";
        static constexpr const char* map_fmt = "HashMap<{}, {}>";
        static constexpr const char* optional_fmt = "Option<{}>";
    };
};

template <> struct LanguageConfig<Language::TYPESCRIPT>
{
    struct Types
    {
        static constexpr const char* int_val = "number";
        static constexpr const char* int16_val = "number";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "boolean";
        static constexpr const char* double_val = "number";
        static constexpr const char* float_val = "number";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "Array<{}>";
        static constexpr const char* map_fmt = "Map<{}, {}>";
        static constexpr const char* optional_fmt = "{} | undefined";
    };
};

template <> struct LanguageConfig<Language::CSHARP>
{
    struct Types
    {
        static constexpr const char* int_val = "int";
        static constexpr const char* int16_val = "short";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "double";
        static constexpr const char* float_val = "float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "List<{}>";
        static constexpr const char* map_fmt = "Dictionary<{}, {}>";
        static constexpr const char* optional_fmt = "{}?";
    };
};

template <> struct LanguageConfig<Language::GO>
{
    struct Types
    {
        static constexpr const char* int_val = "int";
        static constexpr const char* int16_val = "int16";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "float64";
        static constexpr const char* float_val = "float32";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "[]{}";
        static constexpr const char* map_fmt = "map[{}]{}";
        static constexpr const char* optional_fmt = "*{}";
    };
};

// Python
template <> struct LanguageConfig<Language::PYTHON>
{
    struct Types
    {
        static constexpr const char* int_val = "int";
        static constexpr const char* int16_val = "int";
        static constexpr const char* string_val = "str";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "float";
        static constexpr const char* float_val = "float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "List[{}]";
        static constexpr const char* map_fmt = "Dict[{}, {}]";
        static constexpr const char* optional_fmt = "Optional[{}]";
    };
};

// Kotlin
template <> struct LanguageConfig<Language::KOTLIN>
{
    struct Types
    {
        static constexpr const char* int_val = "Int";
        static constexpr const char* int16_val = "Short";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Boolean";
        static constexpr const char* double_val = "Double";
        static constexpr const char* float_val = "Float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "List<{}>";
        static constexpr const char* map_fmt = "Map<{}, {}>";
        static constexpr const char* optional_fmt = "{}?";
    };
};

// Swift
template <> struct LanguageConfig<Language::SWIFT>
{
    struct Types
    {
        static constexpr const char* int_val = "Int";
        static constexpr const char* int16_val = "Int16";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Bool";
        static constexpr const char* double_val = "Double";
        static constexpr const char* float_val = "Float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "[{}]";
        static constexpr const char* map_fmt = "[{}: {}]";
        static constexpr const char* optional_fmt = "{}?";
    };
};

// JavaScript
template <> struct LanguageConfig<Language::JAVASCRIPT>
{
    struct Types
    {
        static constexpr const char* int_val = "number";
        static constexpr const char* int16_val = "number";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "boolean";
        static constexpr const char* double_val = "number";
        static constexpr const char* float_val = "number";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "Array<{}>";
        static constexpr const char* map_fmt = "Map<{}, {}>";
        static constexpr const char* optional_fmt = "{} | undefined";
    };
};

// PHP
template <> struct LanguageConfig<Language::PHP>
{
    struct Types
    {
        static constexpr const char* int_val = "int";
        static constexpr const char* int16_val = "int";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "float";
        static constexpr const char* float_val = "float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "array<{}>";
        static constexpr const char* map_fmt = "array<{}, {}>";
        static constexpr const char* optional_fmt = "?{}";
    };
};

// Ruby
template <> struct LanguageConfig<Language::RUBY>
{
    struct Types
    {
        static constexpr const char* int_val = "Integer";
        static constexpr const char* int16_val = "Integer";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Boolean";
        static constexpr const char* double_val = "Float";
        static constexpr const char* float_val = "Float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "Array[{}]";
        static constexpr const char* map_fmt = "Hash[{}, {}]";
        static constexpr const char* optional_fmt = "{} | nil";
    };
};

// Scala
template <> struct LanguageConfig<Language::SCALA>
{
    struct Types
    {
        static constexpr const char* int_val = "Int";
        static constexpr const char* int16_val = "Short";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Boolean";
        static constexpr const char* double_val = "Double";
        static constexpr const char* float_val = "Float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "List[{}]";
        static constexpr const char* map_fmt = "Map[{}, {}]";
        static constexpr const char* optional_fmt = "Option[{}]";
    };
};

// Dart
template <> struct LanguageConfig<Language::DART>
{
    struct Types
    {
        static constexpr const char* int_val = "int";
        static constexpr const char* int16_val = "int";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "double";
        static constexpr const char* float_val = "double";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "List<{}>";
        static constexpr const char* map_fmt = "Map<{}, {}>";
        static constexpr const char* optional_fmt = "{}?";
    };
};

// Haskell
template <> struct LanguageConfig<Language::HASKELL>
{
    struct Types
    {
        static constexpr const char* int_val = "Int";
        static constexpr const char* int16_val = "Int16";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Bool";
        static constexpr const char* double_val = "Double";
        static constexpr const char* float_val = "Float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "[{}]";
        static constexpr const char* map_fmt = "Map {} {}";
        static constexpr const char* optional_fmt = "Maybe {}";
    };
};

// F#
template <> struct LanguageConfig<Language::FSHARP>
{
    struct Types
    {
        static constexpr const char* int_val = "int";
        static constexpr const char* int16_val = "int16";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "double";
        static constexpr const char* float_val = "float32";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "List<{}>";
        static constexpr const char* map_fmt = "Map<{}, {}>";
        static constexpr const char* optional_fmt = "{} option";
    };
};

// Lua
template <> struct LanguageConfig<Language::LUA>
{
    struct Types
    {
        static constexpr const char* int_val = "number";
        static constexpr const char* int16_val = "number";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "boolean";
        static constexpr const char* double_val = "number";
        static constexpr const char* float_val = "number";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "table<{}>";
        static constexpr const char* map_fmt = "table<{}, {}>";
        static constexpr const char* optional_fmt = "{} | nil";
    };
};

// Objective-C
template <> struct LanguageConfig<Language::OBJECTIVEC>
{
    struct Types
    {
        static constexpr const char* int_val = "NSInteger";
        static constexpr const char* int16_val = "short";
        static constexpr const char* string_val = "NSString*";
        static constexpr const char* bool_val = "BOOL";
        static constexpr const char* double_val = "double";
        static constexpr const char* float_val = "float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "NSArray<{}>*";
        static constexpr const char* map_fmt = "NSDictionary<{}, {}>*";
        static constexpr const char* optional_fmt = "{} _Nullable";
    };
};

// Elixir
template <> struct LanguageConfig<Language::ELIXIR>
{
    struct Types
    {
        static constexpr const char* int_val = "integer()";
        static constexpr const char* int16_val = "integer()";
        static constexpr const char* string_val = "String.t()";
        static constexpr const char* bool_val = "boolean()";
        static constexpr const char* double_val = "float()";
        static constexpr const char* float_val = "float()";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "list({})";
        static constexpr const char* map_fmt = "map({}, {})";
        static constexpr const char* optional_fmt = "{} | nil";
    };
};

// R
template <> struct LanguageConfig<Language::R>
{
    struct Types
    {
        static constexpr const char* int_val = "integer";
        static constexpr const char* int16_val = "integer";
        static constexpr const char* string_val = "character";
        static constexpr const char* bool_val = "logical";
        static constexpr const char* double_val = "numeric";
        static constexpr const char* float_val = "numeric";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "vector({})";
        static constexpr const char* map_fmt = "list({}, {})";
        static constexpr const char* optional_fmt = "{} | NULL";
    };
};

// Julia
template <> struct LanguageConfig<Language::JULIA>
{
    struct Types
    {
        static constexpr const char* int_val = "Int";
        static constexpr const char* int16_val = "Int16";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Bool";
        static constexpr const char* double_val = "Float64";
        static constexpr const char* float_val = "Float32";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "Vector{{{}}}";
        static constexpr const char* map_fmt = "Dict{{{}, {}}}";
        static constexpr const char* optional_fmt = "Union{{{}, Nothing}}";
    };
};

template <> struct LanguageConfig<Language::PERL>
{
    struct Types
    {
        static constexpr const char* int_val = "int";
        static constexpr const char* int16_val = "int";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "bool";
        static constexpr const char* double_val = "number";
        static constexpr const char* float_val = "number";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "ArrayRef[{}]";
        static constexpr const char* map_fmt = "HashRef[{}, {}]";
        static constexpr const char* optional_fmt = "{} | undef";
    };
};

// Clojure
template <> struct LanguageConfig<Language::CLOJURE>
{
    struct Types
    {
        static constexpr const char* int_val = "Long";
        static constexpr const char* int16_val = "Short";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Boolean";
        static constexpr const char* double_val = "Double";
        static constexpr const char* float_val = "Float";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "[{}]";
        static constexpr const char* map_fmt = "{{{} {}}}";
        static constexpr const char* optional_fmt = "(or {} nil)";
    };
};

// VB.NET
template <> struct LanguageConfig<Language::VB_NET>
{
    struct Types
    {
        static constexpr const char* int_val = "Integer";
        static constexpr const char* int16_val = "Short";
        static constexpr const char* string_val = "String";
        static constexpr const char* bool_val = "Boolean";
        static constexpr const char* double_val = "Double";
        static constexpr const char* float_val = "Single";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "List(Of {})";
        static constexpr const char* map_fmt = "Dictionary(Of {}, {})";
        static constexpr const char* optional_fmt = "{}?";
    };
};

// MATLAB
template <> struct LanguageConfig<Language::MATLAB>
{
    struct Types
    {
        static constexpr const char* int_val = "int32";
        static constexpr const char* int16_val = "int16";
        static constexpr const char* string_val = "string";
        static constexpr const char* bool_val = "logical";
        static constexpr const char* double_val = "double";
        static constexpr const char* float_val = "single";
    };

    struct Containers
    {
        static constexpr const char* vector_fmt = "{}[]";
        static constexpr const char* map_fmt = "containers.Map({}, {})";
        static constexpr const char* optional_fmt = "{} | missing";
    };
};

template <typename T, Language Lang> std::string map_to_language()
{
    return CleanTypeMapper<T, Lang>::map();
}

namespace meta
{

template <typename ObjectType, Language Lang> std::string reflect()
{
    std::ostringstream os;
    auto& tpl = meta::MetaTuple<ObjectType>::fields;
    os << languageMap.at(Lang) << "\n";
    std::apply(
        [&](auto&&... fieldMeta) -> void
        {
            ((os << fieldMeta.fieldName << " "
                 << ", "
                 << map_to_language<typename std::decay_t<decltype(fieldMeta)>::FieldType, Lang>()
                 << "\n"),
             ...);
        },
        tpl);
    return os.str();
}
} // namespace meta
