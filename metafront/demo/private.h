#pragma once
namespace demo
{
class Priv
{
  public:
    Priv(int x, int y) : x_(x), y_(y)
    {
    }

  private:
    int x_ __attribute__((annotate("private-is-private")));
    int y_;

  public:
    std::vector<std::string> repairs_;
    std::map<std::string, std::string> service_;
};
} // namespace demo
