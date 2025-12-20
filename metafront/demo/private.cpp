#include "private.h"

#include <functional>
#include <iostream>

#include "../meta/meta_field.h"
#include "../meta/meta_txt.h"
#include "private.meta"

int main()
{
    demo::Priv p(1, 2);
    toString(p);
}
