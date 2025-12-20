// example.thrift - Apache Thrift IDL
namespace cpp example

struct User {
  1: required i32 id
  2: required string name
  3: optional string email
  4: list<string> tags
}

enum Status {
  ACTIVE = 1,
  INACTIVE = 2,
  BANNED = 3
}

service UserService {
  User getUser(1: i32 id)
  void createUser(1: User user)
}
