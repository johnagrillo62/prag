#include <functional>
#include <iostream>
#include <map>

struct Priv
{
  int64_t x_;
  int64_t y_;
  
  std::vector<std::string> repairs_;
  std::map<std::string, std::string> service_;
};

#include "meta.h"
#include "demo08.meta"

int main()
{
    Priv p(1, 2);
    std::cerr << meta::toString(p) << "\n";
}
