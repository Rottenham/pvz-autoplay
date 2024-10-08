#pragma once

#include "macro.h"
#include "shared.h"

#define plant_filter(...) (AliveFilter<Plant>([&](Plant* p) { return __VA_ARGS__; }))
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

const std::array<std::string, 89> PLANT_NAME = {
    "豌豆射手",
    "向日葵",
    "樱桃炸弹",
    "坚果",
    "土豆地雷",
    "寒冰射手",
    "大嘴花",
    "双重射手",
    "小喷菇",
    "阳光菇",
    "大喷菇",
    "墓碑吞噬者",
    "魅惑菇",
    "胆小菇",
    "寒冰菇",
    "毁灭菇",
    "荷叶",
    "倭瓜",
    "三发射手",
    "缠绕海藻",
    "火爆辣椒",
    "地刺",
    "火炬树桩",
    "高坚果",
    "水兵菇",
    "路灯花",
    "仙人掌",
    "三叶草",
    "裂荚射手",
    "杨桃",
    "南瓜头",
    "磁力菇",
    "卷心菜投手",
    "花盆",
    "玉米投手",
    "咖啡豆",
    "大蒜",
    "叶子保护伞",
    "金盏花",
    "西瓜投手",
    "机枪射手",
    "双子向日葵",
    "忧郁菇",
    "香蒲",
    "冰西瓜投手",
    "吸金磁",
    "地刺王",
    "玉米加农炮",
    "模仿者",
    "模仿豌豆射手",
    "模仿向日葵",
    "模仿樱桃炸弹",
    "模仿坚果",
    "模仿土豆地雷",
    "模仿寒冰射手",
    "模仿大嘴花",
    "模仿双重射手",
    "模仿小喷菇",
    "模仿阳光菇",
    "模仿大喷菇",
    "模仿墓碑吞噬者",
    "模仿魅惑菇",
    "模仿胆小菇",
    "模仿寒冰菇",
    "模仿毁灭菇",
    "模仿荷叶",
    "模仿倭瓜",
    "模仿三发射手",
    "模仿缠绕海藻",
    "模仿火爆辣椒",
    "模仿地刺",
    "模仿火炬树桩",
    "模仿高坚果",
    "模仿水兵菇",
    "模仿路灯花",
    "模仿仙人掌",
    "模仿三叶草",
    "模仿裂荚射手",
    "模仿杨桃",
    "模仿南瓜头",
    "模仿磁力菇",
    "模仿卷心菜投手",
    "模仿花盆",
    "模仿玉米投手",
    "模仿咖啡豆",
    "模仿大蒜",
    "模仿叶子保护伞",
    "模仿金盏花",
    "模仿西瓜投手",
};
