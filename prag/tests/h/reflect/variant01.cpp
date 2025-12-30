#include </mnt/c/Users/johna/source/repos/metah/meta.h>

#include "variant01.h"

#include "variant01.meta"

int main()
{
    ContactInfo info;

    std::cout << "json\n";
    std::cout << meta::toJson(info) << "\n";

    std::cout << "json\n";
    info.contact = 100;
    std::cout << meta::toJson(info) << "\n";

    info.contact = 100.0f;
    std::cout << "json\n";
    std::cout << meta::toJson(info) << "\n";
    std::cout << "string\n";
    std::cout << meta::toString(info) << "\n";
    std::cout << "yaml\n";
    std::cout << meta::toYaml(info) << "\n";

    info.contact = "hllo";
    std::cout << meta::toYaml(info) << "\n";
    std::cout << meta::toString(info) << "\n";
}
