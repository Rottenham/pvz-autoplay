#pragma once

#include "macro.h"
#include "shared.h"

namespace CardInternal {

std::unordered_set<int> get_invalid_plants(const std::string& func_name)
{
    switch (GetMainObject()->Scene()) {
    case 0:  // DE
    case 7:  // 禅境花园
    case 9:  // 智慧树
    case 10: // D6E
        return {GRAVE_BUSTER, LILY_PAD, TANGLE_KELP, SEA_SHROOM, CATTAIL};
    case 1:  // NE
    case 11: // N6E
        return {LILY_PAD, TANGLE_KELP, SEA_SHROOM, COFFEE_BEAN, CATTAIL};
    case 6: // 蘑菇园
        return {GRAVE_BUSTER, LILY_PAD, TANGLE_KELP, SEA_SHROOM, COFFEE_BEAN, CATTAIL};
    case 2: // PE
        return {GRAVE_BUSTER};
    case 3: // FE
    case 8: // 水族馆
        return {GRAVE_BUSTER, COFFEE_BEAN};
    case 4: // RE
        return {GRAVE_BUSTER, LILY_PAD, TANGLE_KELP, SPIKEWEED, SEA_SHROOM, CATTAIL, SPIKEROCK};
    case 5: // ME
        return {GRAVE_BUSTER, LILY_PAD, TANGLE_KELP, SPIKEWEED, SEA_SHROOM, COFFEE_BEAN, CATTAIL, SPIKEROCK};
    default:
        GetInternalLogger()->Error("# 无法识别的场地代号: #", func_name, GetMainObject()->Scene());
        return {};
    }
}

void fill_cards(std::vector<int>& cards)
{
    static const std::vector<std::pair<int, int>> UPGRADES = {
        {REPEATER, GATLING_PEA},
        {SUNFLOWER, TWIN_SUNFLOWER},
        {FUME_SHROOM, GLOOM_SHROOM},
        {LILY, CATTAIL},
        {MELON_PULT, WINTER_MELON},
        {MAGNET_SHROOM, GOLD_MAGNET},
        {SPIKE, SPIKEROCK},
        {KERNEL, COB},
    };
    for (auto [original_plant, upgraded_plant] : UPGRADES) {
        if (!Contains(cards, original_plant) && !Contains(cards, upgraded_plant)) {
            cards.push_back(upgraded_plant);
            if (cards.size() >= 10)
                return;
        }
    }
    for (auto invalid_plant : CardInternal::get_invalid_plants("SelectCards")) {
        if (!Contains(cards, invalid_plant)) {
            cards.push_back(invalid_plant);
            if (cards.size() >= 10)
                return;
        }
    }
    int filler_card = 0;
    while (cards.size() < 10) {
        while (Contains(cards, filler_card))
            filler_card++;
        cards.push_back(filler_card);
    }
}

};

bool NotInCD(PlantType type)
{
    return GetMainObject()->SeedArray()[GetCardIndex(type)].Cd() == 0;
}

void SelectCards(std::vector<int> cards = {}, int select_interval = 17)
{
    CardInternal::fill_cards(cards);
    ASelectCards(cards, select_interval);
}