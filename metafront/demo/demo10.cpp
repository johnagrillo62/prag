
#include <iostream>

struct A
{
    int64_t intA;
    int64_t x;

};

#include "../meta/meta_field.h"
#include "../meta/meta_txt.h"
#include "demo10.meta"

int main()
{
    A a{};
    toString(a);
}
