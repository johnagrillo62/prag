
#include <iostream>

struct A
{
    int64_t intA;
    struct 
    {
        int64_t x;
    } b ;
};

#include "meta.h"
#include "demo14.meta"

int main()
{
    A a{};
    std::cerr << meta::toString(a) << "\n"; 
}
