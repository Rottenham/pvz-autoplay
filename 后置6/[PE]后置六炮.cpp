#include <filesystem>

#include "custom/lib.h"
#include "mod/mod.h"
#include "dsl/shorthand_240205.h"

/**** 挂机基础设定 ****/
constexpr int FLAG_GOAL = 5000;
constexpr auto GAME_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\userdata\\game1_13.dat";
constexpr auto TEMP_DAT_PATH = "C:\\Users\\cresc\\Desktop\\temp\\tmp.dat";
const std::vector<Grid> COB_LIST = {{2, 4}, {3, 3}, {4, 3}, {5, 4}};
const std::vector<Grid> PUMPKIN_LIST = {{4, 6}, {3, 6}, {4, 7}, {3, 7}, {1, 5}, {6, 5}, {1, 4}, {6, 4}, {4, 5}, {3, 5}};
const std::vector<Grid> GLOOM_LIST = {{4, 7}, {3, 7}, {4, 6}, {3, 6}};
const std::vector<Grid> FUME_LIST = {{1, 5}, {6, 5}, {1, 4}, {6, 4}};
constexpr int SHORE_ROW_PUFF_COL = 6;
const std::vector<Grid> PUFF_LIST = {{1, 6}, {6, 6}, {2, SHORE_ROW_PUFF_COL}, {5, SHORE_ROW_PUFF_COL}};
constexpr bool SKIP_TICK = true;
constexpr bool PAUSE_ON_FAIL = true;

/**** 收集数据 ****/
Logger<Console> logger;
auto start = std::chrono::high_resolution_clock::now();
int sl_count = 0;
int flag_count = 0;
Level level;
Stat total_level_stat("总"), cob_hp_stat("炮HP"), gloom_fix_stat("补曾"), fume_fix_stat("补大喷"), shovel_puff_stat("铲小喷");
Record PP_record("PP生效", 3);
bool* zombie_type_list;
PlantFixer sunflower_planter;

void on_fail();

bool not_in_cd(const std::vector<PlantType>& plant_types) {
    for (auto plant_type : plant_types) {
        if (GetMainObject()->SeedArray()[GetCardIndex(plant_type)].Cd() != 0)
            return false; 
    }
    return true;
}

TickRunner fix_gloom_runner;
void fix_gloom() {
    auto ptrs = GetPlantPtrs(GLOOM_LIST, GLOOM_SHROOM);
    for (size_t i = 0; i < GLOOM_LIST.size(); i++) {
        auto [row, col] = GLOOM_LIST[i];
        if (ptrs[i] == nullptr) {
            std::vector<PlantType> cards;
            if (row == 3 || row == 4) cards = {LILY, FUME_SHROOM, GLOOM_SHROOM, COFFEE};
            else cards = {FUME_SHROOM, GLOOM_SHROOM, COFFEE};
            if (not_in_cd(cards)) {
                ACard(cards, row, col);
                logger.Warning("修补 #-# 曾.", row, col);
                gloom_fix_stat[std::format("{}-{}", row, col)]++;
                gloom_fix_stat["total"]++;
            }
            return;
        }
    }
    ptrs = GetPlantPtrs(FUME_LIST, FUME_SHROOM);
    for (size_t i = 0; i < FUME_LIST.size(); i++) {
        auto [row, col] = FUME_LIST[i];
        if (ptrs[i] == nullptr) {
            std::vector<PlantType> cards = {FUME_SHROOM, COFFEE};
            if (not_in_cd(cards)) {
                ACard(cards, row, col);
                logger.Warning("修补 #-# 喷.", row, col);
                fume_fix_stat[std::format("{}-{}", row, col)]++;
            }
            return;
        }
    }
}

TickRunner smart_blover_runner;
void smart_blover()
{
    if (IsSeedUsable(BLOVER)) {
        for (auto& z : aliveZombieFilter) {
            if (z.Type() == BALLOON_ZOMBIE && z.Abscissa() < 0) {
                ACard(BLOVER, 6, 1);
                return;
            }
        }
    }
}

bool can_jump(float PV_x, int puff_col) {
    return PV_x - 29 <= 40 + (puff_col - 1) * 80 + 50 + 4 + 10; // +10 is for safety
}

std::unordered_set<int> get_avoid_PV_rows() {
    if (!zombie_type_list[POLE_VAULTING_ZOMBIE]) return {};
    std::unordered_set<int> rows;
    for (auto& z : aliveZombieFilter) {
        if (z.Type() == POLE_VAULTING_ZOMBIE && RangeIn(z.Row() + 1, {2, 5}) &&
            can_jump(z.Abscissa(), SHORE_ROW_PUFF_COL)) {
            rows.insert(z.Row() + 1);
        }
    }
    return rows;
}

TickRunner fodder_runner;
void fodder() {
    auto avoid_PV_rows = get_avoid_PV_rows();
    auto ptrs = GetPlantPtrs(PUFF_LIST, PUFF);
    for (size_t i = 0; i < ptrs.size(); i++) {
        auto ptr = ptrs[i];
        auto [row, col] = PUFF_LIST[i];
        if (avoid_PV_rows.count(row)) {
            if (ptr != nullptr) {
                AShovel(row, col);
                shovel_puff_stat[std::format("{}-{}", row, col)]++;
            }
        } else if (ptr == nullptr && (zombie_type_list[FOOTBALL_ZOMBIE] || IsZombieExist(GIGA))) {
            if (IsSeedUsable(PUFF)) {
                ACard(PUFF, row, col);
                return;
            } else if (IsSeedUsable(M_PUFF)) {
                ACard(M_PUFF, row, col);
                return;
            }
        }
    }
}

void remove_ladder() {
    for (auto& pi : alivePlaceItemFilter) {
        if (pi.Type() == 3) {
            for (auto& fume_pos : FUME_LIST) {
                if (pi.Row() + 1 == fume_pos.row && pi.Col() + 1 == fume_pos.col) {
                    AShovel(fume_pos.row, fume_pos.col, PUMPKIN);
                }
            }
        }
    }
}

TickRunner fix_pumpkin_runner;
void fix_pumpkin(int hp_thres) {
    auto ptrs = GetPlantPtrs(PUMPKIN_LIST, PUMPKIN);
    for (size_t i = 0; i < ptrs.size(); i++) {
        auto ptr = ptrs[i];
        auto [row, col] = PUMPKIN_LIST[i];
        auto rate = get_imminent_hammer_rate(PUMPKIN_LIST[i], PUMPKIN);
        if (rate > 0.0f && rate < 0.05f) {
            if (ptr != nullptr) AShovel(row, col, PUMPKIN);
        } else if (ptr == nullptr || ptr->Hp() <= hp_thres) {
            if (IsSeedUsable(PUMPKIN)) {
                if (ptr != nullptr) AShovel(row, col, PUMPKIN);
                ACard(PUMPKIN, row, col);
            }
        }
    }
}

float get_cob_col() {
    float cob_col = zombie_type_list[GARG] ? 6.9375f : 7.5f;
    for (auto& z : aliveZombieFilter) {
        if (z.Type() == JACK_IN_THE_BOX_ZOMBIE && RangeIn(z.Row() + 1, {1, 6})) {
            float jack_x_in_373cs = z.Abscissa() - (z.SlowCountdown() == 0 ? 132.0f : (132.0f / 2.0f));
            cob_col = std::min(cob_col, (jack_x_in_373cs + 192.0f) / 80.0f);
        } else if (z.Type() == GIGA && z.Hp() > 600 && RangeIn(z.Row() + 1, {1, 6})) {
            float giga_x_in_373cs = z.Abscissa() - (z.SlowCountdown() == 0 ? 73.0f : (73.0f / 2.0f));
            cob_col = std::min(cob_col, (giga_x_in_373cs + 223.0f) / 80.0f);
        }
    }
    return cob_col;
}

void fire_PP() {
    auto cob_col = get_cob_col();
    cobManager.Fire({{2, cob_col}, {5, cob_col}});
    PP_record.add(std::format("w{} {}cs {}列", NowWave(), NowTime(NowWave()) + 373, cob_col));
}

void finish(int wave, int time) {
    At(Time(wave, time)) Do {
        if (NowWave() == wave && GetMainObject()->RefreshCountdown() > 200) {
            for (auto& z : aliveZombieFilter) {
                if (z.Abscissa() >= 320) {
                    fire_PP();
                    finish(wave, time + 1159);
                    break;
                }
            }
        }
    };
}

Coroutine fire_next_PP() {
    while (true) {
        int wave = NowWave();
        int time = NowTime(wave);
        co_await [=] { return NowTime(wave) >= time + 1159; };
        if (NowTime(wave + 1) > -100) { // -100 + 373 + 1159 = 1432, 仍能保证晚爆丑不炸
            co_await [=] { return NowTime(wave + 1) >= 1000 - 200 - 373; };
        }
        fire_PP();
        if (NowWave() == 9 || NowWave() == 19) {
            finish(NowWave(), NowTime(NowWave()) + 1159);
            break;
        }
    }
}

void Script() {
    if (flag_count > FLAG_GOAL) return;
    EnableModsScoped(DisableItemDrop);
    logger.SetLevel({LogLevel::DEBUG, LogLevel::ERROR, LogLevel::WARNING});
    SetInternalLogger(logger);
    SetReloadMode(ReloadMode::MAIN_UI_OR_FIGHT_UI);
    SelectCards({PUMPKIN, LILY, FUME_SHROOM, GLOOM_SHROOM, COFFEE, BLOVER, ICE, PUFF, M_PUFF, SUNFLOWER}, 0);

    remove_ladder();
    fix_pumpkin_runner.Start([=] { fix_pumpkin(2666); });
    fix_gloom_runner.Start(fix_gloom);
    fodder_runner.Start(fodder);

    zombie_type_list = GetZombieTypeList();
    if (zombie_type_list[BALLOON_ZOMBIE])
        smart_blover_runner.Start(smart_blover);
    int huge_wave_PP_time = 1432;
    if (zombie_type_list[BUNGEE_ZOMBIE]) {
        OnWave(10, 20) {
            At(400 - 300 - 751) Do { fix_gloom_runner.Pause(); }, 
            At(400) I(6, 1),
            At(400) Do { fix_gloom_runner.GoOn(); }, 
        };
        huge_wave_PP_time += 300;
    }
    if (GetMainObject()->Sun() < 8000 && !zombie_type_list[DIGGER_ZOMBIE]) {
        sunflower_planter.Start(SUNFLOWER, {{1, 1}, {2, 1}, {5, 1}});
    }
    if (zombie_type_list[DANCING_ZOMBIE]) {
        MaidCheats::Dancing();
        At(Time(20, 0)) Do { MaidCheats::Stop(); };
    }

    if (SKIP_TICK)
        SkipTick([] {
            auto ptrs = GetPlantPtrs(COB_LIST, COB);
            for (size_t i = 0; i < ptrs.size(); i++) {
                if (ptrs[i] == nullptr) {
                    logger.Warning("#-#炮丢失", COB_LIST[i].row, COB_LIST[i].col);
                    on_fail();
                    return false;
                }
                if (ptrs[i]->Hp() < ptrs[i]->HpMax()) {
                    logger.Warning("#-#炮损伤", COB_LIST[i].row, COB_LIST[i].col);
                    on_fail();
                    return false;
                }
            }
            return true;
        });
    else
        SetGameSpeed(5);
    Connect('F', [] { SetGameSpeed(5); });
    Connect('G', [] { SetGameSpeed(1); });

    OnWave(1) {
        At(1159 - 200 - 373) Do {
            fire_PP();
            CoLaunch(fire_next_PP);
        }
    };
    OnWave(10) {
        At(huge_wave_PP_time - 373) Do {
            fire_PP();
            CoLaunch(fire_next_PP);
        }
    };
    OnWave(20) Do {
        finish(20, huge_wave_PP_time - 373);
    };
}

OnAfterInject({
    EnterGame();
});

OnEnterFight({
    flag_count += 2;
    total_level_stat[to_string(level)]++;
});

OnBeforeScript({
    if (flag_count > 0 && flag_count % 100 == 0) {
        auto ptrs = GetPlantPtrs(COB_LIST, COB);
        for (size_t i = 0; i < ptrs.size(); i++) {
            auto [row, col] = COB_LIST[i];
            cob_hp_stat[std::format("{}-{}", row, col)] = ptrs[i]->Hp();
        }
        logger.Debug("#\n#, #\n#, #", cob_hp_stat,
                                      gloom_fix_stat, fume_fix_stat,
                                      total_level_stat, get_time_estimate(flag_count, FLAG_GOAL, start));
    }
    if (flag_count % 100 == 0) {
        logger.Debug(set_random_seed());
    }
    level = get_level();
    logger.Debug("阳光: #, #, #f, SL次数: # #", GetMainObject()->Sun(), to_string(level), flag_count, sl_count, get_timestamp());
    if (!PAUSE_ON_FAIL)
        std::filesystem::copy(GAME_DAT_PATH, TEMP_DAT_PATH + std::to_string(flag_count % 10),
            std::filesystem::copy_options::overwrite_existing);
});

void on_fail()
{
    logger.Debug(get_prev_wave_stat(3));
    logger.Debug("#", PP_record);
    std::ostringstream oss;
    for (auto& r : get_avoid_PV_rows()) oss << r << ",";
    logger.Debug("躲撑杆行: #", oss.str());
    if (PAUSE_ON_FAIL) {
        logger.Error("游戏暂停.");
        SetAdvancedPause(true);
    } else {
        BackToMain(false);
        EnterGame();
    }
}

OnExitFight({
    if (GetPvzBase()->GameUi() == AAsm::ZOMBIES_WON) {
        Zombie* zombie = nullptr;
        for(auto& z : aliveZombieFilter) {
            if (zombie == nullptr || z.Abscissa() < zombie->Abscissa()) {
                zombie = &z;
            }
        }
        logger.Error("种类为 # 的僵尸进家 (x=#), 僵尸获胜", zombie->Type(), zombie->Abscissa());
        on_fail();
    }
    if (GetPvzBase()->GameUi() != AAsm::LEVEL_INTRO && !PAUSE_ON_FAIL) {
        sl_count++;
        std::filesystem::copy(TEMP_DAT_PATH + std::to_string(flag_count % 10),
            GAME_DAT_PATH, std::filesystem::copy_options::overwrite_existing);
        flag_count = std::clamp(flag_count - 10, 0, INT_MAX);
    }
});