#include <iostream>
#include <map>
#include <vector>
struct Car
{
    std::string make;
    std::string model;
    int16_t year;

    //
    // limitation of AST parsing(Reality Check)
    //
    //@CSV_COLUMN("01")
    std::map<std::string, std::vector<std::string>> service;
};

#include "../meta/meta_txt.h"
#include "../meta/meta_yaml.h"
#include "demo04.meta"

int main()
{
    Car car;
    car.make = "ford";
    car.model = "taurus";
    car.year = 2025;

    car.service = {
        {"Mon", {"Oil", "Tires", "Window", "Wheel"}},
        {"Tues", {"Hood"}},
    };

    toString(car);
}
