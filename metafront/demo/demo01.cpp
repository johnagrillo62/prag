#include <iostream>
#include <string>
struct Car
{
    std::string maker;
    std::string model;
    unsigned short year;
    bool electric;
    unsigned int howmanymiles;
};

#include "../meta/meta_field.h"
#include "../meta/meta_txt.h"
#include "demo01.meta"

int main()
{
    Car car;
    car.maker = "dodge";
    car.model = "caravan";
    car.electric = false;
    car.year = 2020;
    car.howmanymiles = 100000;
    toString(car);
}
