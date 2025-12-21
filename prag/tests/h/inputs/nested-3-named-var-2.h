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
    } C;
  } B ;
} A;


/*
  A.x = 10;
  A.B.y = 10;

  struct A a;
  a.x = 10;
  a.B.y = 10;

  struct A::B b;
  b.C.z = 10;

  struct A::B::C c;
  c.z = 10;
*/
