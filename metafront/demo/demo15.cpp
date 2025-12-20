#include <string>

class [[clang::annotate("table_name:User")]] User
{
  public:
    User(int id, std::string name) : id_(id), name_(name) {};

    int id_ [[clang::annotate("field_name:friend_id")]];
    std::string name_;
    std::string name2_ = "name2";
    std::string name3_ = "name3";
};

#include "../meta/meta_db.h"
#include "../meta/meta_field.h"
#include "demo15.meta"

int main()
{
    User user(1, "10");
    std::cout << db::createTable<User>() << "\n";
    std::cout << db::selectSQL<User>() << "\n";
    std::cout << db::insertSQL(user) << "\n";
}
