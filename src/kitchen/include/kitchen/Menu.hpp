#ifndef MENU_HPP_
#define MENU_HPP_

#include <iostream>
#include <vector>

struct Food
{
public:
    Food() {};
    Food(std::string name_, int prepare, int remaining_)
        : name(name_), preparationTime(prepare), remaining(remaining_)
    {
    }
    std::string name;
    int preparationTime;
    int remaining;
};

extern std::vector<Food> menu;

#endif