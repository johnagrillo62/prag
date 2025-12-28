#include <vector>
#include <string>
#include <map>

#include "meta.h"


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

#include "demo14.meta"


int main()
{
    demo::Priv p(1, 2);
    std::cout << meta::toString(p) << "\n";
}
