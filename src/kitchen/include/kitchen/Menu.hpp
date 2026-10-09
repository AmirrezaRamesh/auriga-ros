#ifndef MENU_HPP_
#define MENU_HPP_

#include <iostream>
#include <vector>

struct Food
{
public:
    Food() {};
    Food(std::string name_, int prepare, int remaining_)
    {
        name = name_;
        preparationTime = prepare;
        remaining = remaining_;
    }
    std::string name;
    int preparationTime;
    int remaining;
};

std::vector<Food> menu = {
    Food("pizza", 15, 2),
    Food("burger", 5, 3),
    Food("pasta", 7, 3),
};

#endif