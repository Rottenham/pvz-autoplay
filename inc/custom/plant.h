#pragma once

#include "macro.h"

#define plant_filter(...) (AliveFilter<Plant>([=](Plant* p) { return __VA_ARGS__; }))
#define plant_exist(...) (!plant_filter(__VA_ARGS__).Empty())
#define plant_count(...) (plant_filter(__VA_ARGS__).Count())

std::vector<std::pair<Plant*, size_t>> GetPlantPtrsI(const std::vector<AGrid>& lst, int type)
{
    auto indices = GetPlantPtrs(lst, type);
    std::vector<std::pair<Plant*, size_t>> res;
    for (size_t i = 0; i < indices.size(); i++)
        res.push_back({indices[i], i});
    return res;
}