#include <iostream>

struct XY
{
    int64_t x_;
    int64_t y_;
};

#include "meta.h"
#include "demo09.meta"

int main()
{
    XY p(1, 2);
    std::cout << meta::toString(p) << "\n";
}
