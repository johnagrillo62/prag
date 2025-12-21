#include <string>
#include <variant>
struct ContactInfo
{
    std::variant<std::string, int, float> contact;
};
