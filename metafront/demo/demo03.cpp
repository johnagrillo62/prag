#include <iostream>
#include <map>
#include <string>
#include <vector>
struct Car
{
    std::string make;
    std::string model;
    int16_t year;
    bool electric;
    int32_t miles;
    std::vector<std::string> repairs;
    std::map<std::string, std::string> service;
};

#include "../meta/meta_txt.h"
#include "demo03.meta"

int main()
{
    Car car;
    car.make = "ford";
    car.model = "taurus";
    car.year = 2025;
    car.miles = 100000;
    car.electric = true;
    car.repairs = {"fender", "motor"};

    car.service = {{
                       "Mon",
                       "Oil",
                   },
                   {"Tues", "More Oil"},
                   {"Wed", "Window Washer"}};

    toString(car);
}
