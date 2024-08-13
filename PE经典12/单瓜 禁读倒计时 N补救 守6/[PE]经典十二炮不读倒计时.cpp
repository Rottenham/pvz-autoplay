#include <filesystem>

#include "DanceCheat/DanceCheat.h"
#include "custom/lib.h"
#include "mod/mod.h"
#include "dsl/shorthand_240205.h"

/**** 挂机基础设定 ****/
constexpr auto GAME_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\userdata\\game1_13.dat";
constexpr auto TEMP_DAT_PATH = "C:\\Users\\cresc\\Desktop\\temp\\tmp.dat";
const std::vector<Grid> COB_LIST = {{1, 5}, {2, 5}, {3, 7}, {4, 7}, {5, 5}, {6, 5}};
constexpr bool SKIP_TICK = true;
constexpr bool PAUSE_ON_FAIL = false;
constexpr bool DEMO = false;
constexpr int FLAG_GOAL = DEMO ? 50 : 10000;

/**** 收集数据 ****/
Logger<Console> logger;
auto start = std::chrono::high_resolution_clock::now();
int sl_count = 0;
int flag_count = 0;
Level level;
Stat total_level_stat("总"), N_level_stat("N"), N_pos_stat("N位置"),
    delay_PP_stat("延迟PP");

void on_fail();

void smart_cherry(int wave) {
    std::array<int, 6> count = {};
    for (auto& z : aliveZombieFilter) {
        if (z.AtWave() + 1 == wave) {
            if (z.Type() == GIGA) count[z.Row()] += 18;
            else if (z.Type() == GARG) count[z.Row()] += 12;
        }
    }
    int best_row = (count[0] + count[1] > count[4] + count[5]) ? 2 : 5;
    At(now + 100) A(best_row, 9);
}

Coroutine handle_inactivated()
{
    int wave = NowWave();
    co_await [=] { return NowWave() > wave || NowTime(wave) >= 1110; };
    if (NowWave() > wave) {
        At(Time(NowWave(), 341)) N({{3, 9}, {4, 9}});
        logger.Warning("第 # 波延迟至 #cs 时刷新, 补核.", wave, NowTime(wave));
        N_level_stat[to_string(level)]++;
        At(Time(NowWave(), 341 - 10)) Do {
            auto ptrs = GetPlantPtrs({{3, 9}, {4, 9}}, DOOM);
            if (ptrs[0] != nullptr) N_pos_stat["3-9"]++;
            else if (ptrs[1] != nullptr) N_pos_stat["4-9"]++;
            else {
                N_pos_stat["failed"]++;
                logger.Error("并未放核!");
            }
        };
    } else {
        cobManager.Fire({{2, 8.75}, {5, 8.75}});
        delay_PP_stat++;
        co_await [=] { return NowWave() > wave; };
        logger.Warning("第 # 波延迟至 #cs 时刷新, 于 1100cs 预判下一波PP.", wave, NowTime(wave));
    }
}

TickRunner smart_blover_runner;
void smart_blover()
{
    if (IsSeedUsable(BLOVER)) {
        for (auto& z : aliveZombieFilter) {
            if (z.Type() == BALLOON_ZOMBIE && z.Abscissa() < 100) {
                ACard(BLOVER, 6, 1);
                return;
            }
        }
    }
}

void Script()
{
    if (flag_count >= FLAG_GOAL) return;
    EnableModsScoped(DisableItemDrop);
    logger.SetLevel({LogLevel::DEBUG, LogLevel::ERROR, LogLevel::WARNING});
    SetInternalLogger(logger);
    SetReloadMode(ReloadMode::MAIN_UI_OR_FIGHT_UI);
    SelectCards({LILY, DOOM, ICE, COFFEE, CHERRY, BLOVER}, DEMO ? 8 : 0);
    
    auto zombie_type_list = GetZombieTypeList();
    if (zombie_type_list[DANCING_ZOMBIE]) {
        MaidCheats::Dancing();
    } else {
        MaidCheats::Stop();
    }
    if (zombie_type_list[BALLOON_ZOMBIE]) {
        smart_blover_runner.Start(smart_blover);
    }
    DanceCheat(DanceCheatMode::FAST);

    if (SKIP_TICK && !DEMO)
        SkipTick([] {
            auto ptrs = GetPlantPtrs(COB_LIST, COB);
            for (size_t i = 0; i < ptrs.size(); i++) {
                auto ptr = ptrs[i];
                auto [row, col] = COB_LIST[i];
                if (ptr == nullptr) {
                    logger.Warning("#-#炮丢失", row, col);
                    on_fail();
                    return false;
                }
                if (ptr->Hp() < ptr->HpMax()) {
                    logger.Warning("#-#炮损伤", row, col);
                    on_fail();
                    return false;
                }
            }
            return true;
        });
    else
        SetGameSpeed(DEMO ? 4 : 5);
    Connect('F', [] { SetGameSpeed(5); });
    Connect('G', [] { SetGameSpeed(1); });

    At(Time(1, 341)) PP();
    At(Time(10, 341)) PP();
    for (int w = 1; w <= 18; w++) {
        if (w == 9) continue;
        At(Time(w, 601 + 341)) PP(8.75);
        At(Time(w, 781)) Do {
            if (NowWave() == w) CoLaunch(handle_inactivated);
        };
    }
    OnWave(20) {
        At(0) I(1, 1),
        At(409) PP(9.2),
    };
    if (zombie_type_list[GIGA] || zombie_type_list[GARG]) {
        At(Time(10, 401 - 100)) Do {
            smart_cherry(10);
        };
    }

    for (auto wave : {9, 19, 20}) {
        for (int i = 1; i <= 3; i++) {
            At(Time(wave, 409 + i * 700 - 373)) Do {
                if (NowWave() == wave) {
                    for (auto& zombie : aliveZombieFilter) {
                        if (zombie.Abscissa() > 400) {
                            At(now + 373) PP<RECOVER_FIRE>(8.5);
                            break;
                        }
                    }
                }
            };
        }
    }
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
        logger.Debug("#, #, #\n#, #", N_level_stat, N_pos_stat, delay_PP_stat,
                                      total_level_stat, get_time_estimate(flag_count, FLAG_GOAL, start));
    }
    if (flag_count % 100 == 0) {
        if (DEMO) set_seed_and_update(0x2486C3D9);
        else logger.Debug(set_random_seed());
    }
    logger.Debug("阳光: #, #, #f, SL次数: # #", GetMainObject()->Sun(), to_string(level), flag_count, sl_count, get_timestamp());
    level = get_level();
    if (!PAUSE_ON_FAIL && !DEMO)
        std::filesystem::copy(GAME_DAT_PATH, TEMP_DAT_PATH + std::to_string(flag_count % 10),
            std::filesystem::copy_options::overwrite_existing);
});

void on_fail()
{
    if (PAUSE_ON_FAIL || DEMO) {
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
    if (GetPvzBase()->GameUi() != AAsm::LEVEL_INTRO && !PAUSE_ON_FAIL && !DEMO) {
        sl_count++;
        std::filesystem::copy(TEMP_DAT_PATH + std::to_string(flag_count % 10),
            GAME_DAT_PATH, std::filesystem::copy_options::overwrite_existing);
        flag_count = std::clamp(flag_count - 10, 0, INT_MAX);
    }
});