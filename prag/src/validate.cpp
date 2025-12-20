#include <iostream>

#include "enums.h"
#include "languages.h"
#include "reified.h"
#include "schema.h"

using namespace schema;
using FR = FieldRequirement;

int main()
{
    auto lang = "Cpp26";
    auto schema = schema::Schema::create(
        StringField("task", FR::Required),
        EnumField("language", FR::Required, std::type_identity<bhw::Language>{}),
        EnumField("reified", FR::Required, std::type_identity<bhw::ReifiedTypeId>{}));

    YAMLNode config;
    config.set("task", "mytask");
    config.set("language", lang);
    config.set("reified", "Int32");

    auto result6 = schema.validate(config);
    if (!result6.errors.empty())
    {
        std::cout << "Validation failed:\n";
        for (const auto& err : result6.errors)
        {
            std::cout << "  " << err.path << ": " << err.message << "\n";
        }
        return 1;
    }

    auto ennum = bhw::to_enum<bhw::Language>(lang);
    std::cout << "  " << ennum.value() << "\n";

    return 0;
}