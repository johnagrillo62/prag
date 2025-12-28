
#include <iostream>

struct A
{
    int64_t intA;
    int64_t x;

};

#include "meta.h"
#include "demo10.meta"

int main()
{
    A a{};
    std::cerr << meta::toString(a) << "\n";
}
