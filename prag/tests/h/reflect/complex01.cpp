
#include "complex01.h"

#include </mnt/c/Users/johna/source/repos/metah/meta.h>

#include "complex01.meta"

int main()
{
    //clang-format off
    Complex01 c{42,
                "Hello World",
                {{"users",
                  {{"alice", "alice@example.com"},
                   {"bob", "bob@example.com"},
                   {"charlie", "charlie@example.com"}}},
                 {"config", {{"theme", "dark"}, {"language", "en"}, {"timezone", "UTC"}}},
                 {"metadata", {{"version", "1.0.0"}, {"author", "John"}, {"license", "MIT"}}}}};
    //clang-format on

    std::cout << "JSON:\n" << meta::toJson(c) << "\n\n";
    std::cout << "YAML:\n" << meta::toYaml(c) << "\n\n";
    std::cout << "String:\n" << meta::toString(c) << "\n";

    return 0;
}
