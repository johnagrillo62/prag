#include <iostream>

#include "../meta/meta_field.h"
#include "../meta/meta_lang.h"

struct ComplexRow {
    uint64_t id;
    std::string name;
    std::vector<int> scores;           // Simple vector
    std::vector<std::string> tags;     // Vector of strings

    //  has to be hand wired into .meta
    //  clang does not create nested templares in hdr files

    std::vector<std::vector<int>> matrix;        // 2D matrix

    std::vector<std::vector<std::string>> categories; // Nested string arrays
};

#include "demo-all.meta"

template <Language... Langs> void reflect_all_languages()
{
    ((std::cout << meta::reflect<ComplexRow, Langs>() << "\n"), ...);
}

int main()
{
    // Usage
    reflect_all_languages<Language::CPP,
                          Language::JAVA,
                          Language::PYTHON,
                          Language::TYPESCRIPT,
                          Language::RUST,
                          Language::GO,
                          Language::CSHARP,
                          Language::KOTLIN,
                          Language::SWIFT,
                          Language::JAVASCRIPT,
                          Language::PHP,
                          Language::RUBY,
                          Language::SCALA,
                          Language::DART,
                          Language::LUA,
                          Language::PERL,
                          Language::HASKELL,
                          Language::ELIXIR,
                          Language::CLOJURE,
                          Language::FSHARP,
                          Language::VB_NET,
                          Language::OBJECTIVEC,
                          Language::R,
                          Language::MATLAB,
                          Language::JULIA>();
}
