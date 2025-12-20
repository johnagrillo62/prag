namespace demo
{
struct MyClass
{
    int x;
    int y;
};

struct Complex
{
    int id;
    std::string string;
    std::vector<MyClass> vector;
    std::map<std::string, std::map<std::string, std::string>> stringStringMap;
};
}

int main()
{
    demo::Complex c{
        42,
        "Hello World",
        {
            {1, 10},
            {2, 20},
            {3, 30}
        },
        {
            {"users", {
                {"alice", "alice@example.com"},
                {"bob", "bob@example.com"},
                {"charlie", "charlie@example.com"}
            }},
            {"config", {
                {"theme", "dark"},
                {"language", "en"},
                {"timezone", "UTC"}
            }},
            {"metadata", {
                {"version", "1.0.0"},
                {"author", "John"},
                {"license", "MIT"}
            }}
        }
    };

    std::cout << "JSON:\n" << meta::toJson(c) << "\n\n";
    std::cout << "YAML:\n" << meta::toYaml(c) << "\n\n";
    std::cout << "String:\n" << meta::toString(c) << "\n";

    return 0;
}
