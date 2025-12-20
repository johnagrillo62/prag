
#include <iostream>
#include <vector>
struct ComplexRow
{
    int64_t id;
    std::string name;
    std::vector<int64_t> scores;
    std::vector<std::string> tags;
    std::vector<std::vector<int64_t>> matrix;

    std::vector<std::vector<std::string>> categories;
};

#include "../meta/meta_field.h"
#include "../meta/meta_json.h"
#include "demo07.meta"

int main()
{

    std::vector<ComplexRow> data = {

        {1,
         "Fred",
         {89, 78, 100},
         {"helper", "volunteer"},
         {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}},
         {{"history", "social studies"}, {"gym", "cooking"}}},

        {2,
         "Helper",
         {89, 78, 100},
         {"athlete", "teacher"},
         {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}},
         {{"fitnes", "weights"}, {"sowing", "cooking"}}}

    };
    std::cout << meta::json::serialize(data) << "\n";
}
