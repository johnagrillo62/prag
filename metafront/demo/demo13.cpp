#include <iostream>

class XY
{
  public:
    XY(int x, int y) : x_(x), y_(y)
    {
    }

    int getX()
    {
        return x_;
    }
    void setX(int x)
    {
        x_ = x;
    }

  private:
    int64_t x_ __attribute__((annotate("private-is-private")));
    int64_t y_;
};

#include "meta.h"
#include "demo13.meta"

int main()
{
    XY p(1, 2);
    std::cerr << meta::toString(p) << "\n";
}
