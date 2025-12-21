#include <cstdint>
struct A
{
  int64_t x;
  struct B
  {
    int64_t y;
    struct C
    {
      int64_t z;
    } c;
  } b ;
} a;


/*
  A a;
  a.x = 0;
  a.b.c.z = 0;
  
  A::B b;
  b.y = 0;
  b.c.z = 0;
  
  A::B::C c;
  c.z = 0;
*/
