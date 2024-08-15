#pragma once

#include <cstdlib>
#include <filesystem>

#include "macro.h"

namespace SharedInternal {

std::pair<int, int> get_plant_def(int col, PlantType type)
{
    int x = 40 + (col - 1) * 80;
    switch (type) {
    case PUMPKIN:
        return {x + 20, x + 80};
    case TALL_NUT:
        return {x + 30, x + 70};
    case COB_CANNON:
        return {x + 20, x + 120};
    default:
        return {x + 30, x + 50};
    }
}

float hammer_rate(Zombie* zombie)
{
    auto animation_code = zombie->MRef<uint16_t>(0x118);
    auto animation_array = GetPvzBase()->AnimationMain()->AnimationOffset()->AnimationArray();
    auto circulation_rate = animation_array[animation_code].CirculationRate();
    return circulation_rate - 0.644f;
}

};

template <typename T>
std::string Concat(const T& container, const std::string& separator = ",")
{
    std::ostringstream oss;
    bool first = true;
    for (const auto& elem : container) {
        if (!first) {
            oss << separator;
        } else {
            first = false;
        }
        oss << std::format("{}", elem);
    }
    return oss.str();
}

template <typename T>
bool Contains(const std::vector<T>& container, const T& elem)
{
    return std::find(container.begin(), container.end(), elem) != container.end();
}

void notify(const std::string& msg = ".")
{
    if (std::filesystem::exists("notify.bat"))
        _wsystem(std::format(L".\\notify.bat \"{}\"", StrToWstr(msg)).c_str());
}