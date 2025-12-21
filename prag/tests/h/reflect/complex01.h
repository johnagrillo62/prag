#include <map>
#include <string>
#include <vector>

struct Complex01
{
    int id;
    std::string string;
    std::map<std::string, std::map<std::string, std::string>> stringStringMap;
};
