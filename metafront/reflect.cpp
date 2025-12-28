#include <string>

#include "meta.h"


//@tableName("reflect")
struct Reflect
{
  //@columnName("x")
  int x;
  unsigned y;
  std::string string;
  std::vector<std::string> strings;
};


#include "reflect.meta"

int main()
{
  Reflect reflect;
  reflect.x = 100;
  reflect.y = 100;
  reflect.string = "reflect";
  reflect.strings = {"reflect", "reflect",  "reflect", "reflect"};
  std::cout << meta::toString(reflect) << "\n";
}


