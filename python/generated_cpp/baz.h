struct Baz {
    std::vector<int> numbers;

    std::vector<int> getNumbers() const { return numbers; }
    void setNumbers(const std::vector<int>& value) { numbers = value; }
};