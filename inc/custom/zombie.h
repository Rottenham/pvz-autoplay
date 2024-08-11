#pragma once

#include "macro.h"
#include "shared.h"

Zombie* zombie_exist(std::function<bool(const Zombie&)> predicate)
{
    for (auto& z : aliveZombieFilter) {
        if (predicate(z)) {
            return &z;
        }
    }
    return nullptr;
}

// [0, 0.644], if no imminent hammer return 0, otherwise return min circulation rate until hammer
float get_imminent_hammer_rate(Grid pos, PlantType type = PEASHOOTER)
{
    float min_hammer_rate = 999.0f;
    auto plant_def = SharedInternal::get_plant_def(pos.col, type);
    for (auto& z : aliveZombieFilter) {
        if (RangeIn(z.Type(), {GARG, GIGA}) && z.IsHammering() && z.Row() + 1 == pos.row) {
            auto rate = SharedInternal::hammer_rate(&z);
            if (rate < 0) {
                std::pair<int, int> zombie_atk = {static_cast<int>(z.Abscissa()) - 30, static_cast<int>(z.Abscissa()) + 59};
                if (std::max(zombie_atk.first, plant_def.first) <= std::min(zombie_atk.second, plant_def.second))
                    min_hammer_rate = std::min(min_hammer_rate, -rate);
            }
        }
    }
    return min_hammer_rate == 999.0f ? 0.0f : min_hammer_rate;
}

void kill_all_zombies()
{
    for (auto& z : aliveZombieFilter) {
        z.State() = 3;
    }
}