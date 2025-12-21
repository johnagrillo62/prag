
#include "../meta/meta.h"
#include "../meta/json.h"
#include "../meta/txt.h"
#include "../meta/yaml.h"

struct Attrs
{

};


struct A {
  int x;
  int y;

  constexpr static const auto fields = std::make_tuple(
   meta::Field< ::A, &::A::x>("id"),
   meta::Field< ::A, &::A::y, Attrs>("Y", Attrs{}));
};


int main()
{
}



  






