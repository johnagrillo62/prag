#include <iostream>

struct XY
{
    int64_t x_;
    int64_t y_;
};

#include "../meta/meta_field.h"
#include "../meta/meta_txt.h"
#include "demo09.meta"

int main()
{
    XY p(1, 2);
    toString(p);
}
