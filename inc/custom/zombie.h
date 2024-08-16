#pragma once

#include "macro.h"
#include "shared.h"

#define zombie_filter(...) (AliveFilter<Zombie>([&](Zombie* z) { return __VA_ARGS__; }))
#define zombie_exist(...) (!zombie_filter(__VA_ARGS__).Empty())
#define zombie_count(...) (zombie_filter(__VA_ARGS__).Count())

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

const std::array<std::string, 33> ZOMBIE_NAME = {
    "普僵",
    "旗帜",
    "路障",
    "撑杆",
    "铁桶",
    "读报",
    "铁门",
    "橄榄",
    "舞王",
    "伴舞",
    "鸭子",
    "潜水",
    "冰车",
    "雪橇",
    "海豚",
    "小丑",
    "气球",
    "矿工",
    "跳跳",
    "雪人",
    "蹦极",
    "扶梯",
    "投篮",
    "白眼",
    "小鬼",
    "僵博",
    "豌豆僵尸",
    "坚果僵尸",
    "辣椒僵尸",
    "机枪僵尸",
    "窝瓜僵尸",
    "高坚果僵尸",
    "红眼",
};
