#include "nested-3-anon-var.h"

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>

#include </mnt/c/Users/johna/source/repos/meta/meta.h>

#include "nested-3-anon-var.meta"

int main()
{
    std::cout << "*******\n";
    struct A a;
    a.x = 1;
    a.b.y = 2;
    a.b.c.z = 3;

    std::cout << meta::toString(a) << "\n";
    std::cout << meta::toJson(a) << "\n";
    std::cout << meta::toYaml(a) << "\n";

    std::cout << "*******\n";
    a.x = 10;
    a.b.y = 20;
    a.b.c.z = 30;

    std::cout << meta::toString(a) << "\n";
    std::cout << meta::toJson(a) << "\n\n";
    std::cout << meta::toYaml(a) << "\n";
}
