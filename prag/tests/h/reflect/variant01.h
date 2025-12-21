#include <variant>
#include <string>
struct ContactInfo
{
  std::variant<std::string, int, float> contact;
};


