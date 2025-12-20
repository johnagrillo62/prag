#pragma once
namespace demo
{
class Getter
{
  public:
    Getter(int x, int y) : x_(x), y_(y)
    {
    }

    int getX()
    {
        return x_;
    }
    void setx(int x)
    {
        x_ = x;
    }

  private:
    int x_ __attribute__((annotate("private-is-private")));
    int y_;
};
} // namespace demo
