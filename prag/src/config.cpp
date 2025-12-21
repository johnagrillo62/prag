#include <iostream>

#include "language_info.h"
#include "languages.h"
#include "parser_registry2.h"

int main()
{

    std::cout << "All languages:\n";
    const auto& ins = bhw::ParserRegistry2::instance();
    for (auto lang : ins.listRegistered())
    {
        std::cerr << lang << "\n";
        
    }
    std::cerr << "\n";

    std::cout << "All languages Enum: ";
    bhw::LanguageEnum::forEach([](auto l) { std::cout << l << " "; });
    std::cout << "\n";

    const auto& langs = bhw::getRegistry();

    for (const auto& [lang, info] : langs)
    {
        std::cout << lang << "\n  " << info.comment_style << "\n";

        for (const auto& [type, config] : info.type_map)
        {
            std::cout << "  " << type << " {\"" << config.type_name << "\", "
                      << "\"" << config.default_value << "\"}\n";
        }
    }
    return 0;
}
