#pragma once

#include "macro.h"

Plant* plant_exist(std::function<bool(const Plant&)> predicate)
{
    for (auto& p : alivePlantFilter) {
        if (predicate(p)) {
            return &p;
        }
    }
    return nullptr;
}