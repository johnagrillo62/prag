# example.capnp - Cap'n Proto Schema Example
@0xdbb9ad1f14bf0b36;

struct User {
    id @0 : Int32;
    name @1 : Text;
    email @2 : Text;
    tags @3 : List(Text);
}

enum Status {
    active @0;
    inactive @1;
    pending @2;
}

struct Post {
    id @0 : Int32;
    title @1 : Text;
    author @2 : User;
}
