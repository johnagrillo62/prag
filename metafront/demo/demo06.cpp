#include <iostream>
#include <vector>
struct Row
{
    int64_t field1;
    int64_t field2;
    int64_t field3;
    std::string field4;
    std::string field5;
    std::string field6;
};

#include "meta.h"
#include "meta_csv.h"
#include "demo06.meta"

int main()
{
    std::vector<Row> rows = {{
                                 1,
                                 2,
                                 3,
                                 "a",
                                 "xxxx",
                             },
                             {4, 5, 6, "b", "xxxx"},
                             {7, 8, 9, "c", "xxxx"},
                             {10, 11, 12, "d"},
                             {13, 14, 15, "e"},
                             {16, 17, 18, "f"},
                             {19, 20, 21, "g"},
                             {22, 23, 24, "h"},
                             {25, 26, 27, "i"},
                             {28, 29, 30, "j"}};

    std::cout << meta::toCSVWithHeader(rows);
}
