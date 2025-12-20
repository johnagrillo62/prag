// test.cpp
#include <gtest/gtest.h>

int main() {
    ::testing::RegisterTest(
        "Test", "Works", nullptr, nullptr,
        __FILE__, __LINE__,
        []() -> ::testing::Test* { return new ::testing::Test(); }
    );
    return 0;
}
