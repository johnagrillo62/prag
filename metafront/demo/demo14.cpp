
#include <iostream>

class A
{
  public:
    A();
    int64_t intA;
    class B
    {
      public:
        B();
        int64_t x;
    };
};

#include "../meta/meta_field.h"
#include "../meta/meta_txt.h"
#include "demo14.meta"

int main()
{
    A a{};
    toString(a);
}
