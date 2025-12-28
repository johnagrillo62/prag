#include <string>

class [[clang::annotate("TableName:USER")]] User
{
  public:

    int id_ [[clang::annotate("SqlColumn:friend_id")]];
    std::string name_;
    std::string name2_ = "name2";
    std::string name3_ = "name3";
};
#include "meta.h"
#include "meta_db.h"
#include "demo11.meta"

int main()
{
    User user(1, "10");
    std::cout << meta::createTable<User>() << "\n";
    std::cout << meta::selectSQL<User>() << "\n";
    std::cout << meta::insertSQL(user) << "\n";
}
