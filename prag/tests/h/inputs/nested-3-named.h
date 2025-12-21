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
    };
  };
};
