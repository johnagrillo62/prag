#include <iostream>
#include <string>
#include <vector>
struct Car
{
    std::string make;
    std::string model;
    std::vector<std::string> repairs;
    int64_t miles;
};

#include "../meta/meta_field.h"
#include "../meta/meta_txt.h"
#include "demo02.meta"

int main()
{
    Car car;
    car.miles = 100;
    car.make = "ford";
    car.model = "taurus";
    car.repairs = {"motor"};
    toString(car);
}
