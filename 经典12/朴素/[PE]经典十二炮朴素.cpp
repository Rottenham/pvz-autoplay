#include <filesystem>

#include "DanceCheat/DanceCheat.h"
#include "custom/lib.h"
#include "mod/mod.h"
#include "dsl/shorthand_240205.h"

/**** 挂机基础设定 ****/
const int FLAG_GOAL = 10000;
constexpr auto GAME_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\userdata\\game1_13.dat";
constexpr auto TEMP_DAT_PATH = "C:\\Users\\cresc\\Desktop\\temp\\tmp.dat";
const std::vector<Grid> COB_LIST = {{1, 5}, {2, 5}, {3, 7}, {4, 7}, {5, 5}, {6, 5}};
constexpr bool SKIP_TICK = true;
constexpr bool PAUSE_ON_FAIL = false;

/**** 收集数据 ****/
Logger<Console> logger;
auto start = std::chrono::high_resolution_clock::now();
int sl_count = 0;
int flag_count = 0;
Level level;
Stat total_level_stat("总"), N_level_stat("N"), N_pos_stat("N位置");

void on_fail();

void Script()
{
    if (flag_count > FLAG_GOAL) return;
    EnableModsScoped(DisableItemDrop);
    logger.SetLevel({LogLevel::DEBUG, LogLevel::ERROR, LogLevel::WARNING});
    SetInternalLogger(logger);
    SetReloadMode(ReloadMode::MAIN_UI_OR_FIGHT_UI);
    SelectCards({LILY, DOOM, ICE, COFFEE, PUFF, M_PUFF, SCAREDY, POT, 40, 41}, 0);
    
    auto zombie_type_list = GetZombieTypeList();
    if (zombie_type_list[DANCING_ZOMBIE]) {
        MaidCheats::Dancing();
        At(Time(20, 1109)) Do {MaidCheats::Stop(); };
    }
    DanceCheat(DanceCheatMode::FAST);

    if (SKIP_TICK)
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
        SetGameSpeed(5);
    Connect('F', [] { SetGameSpeed(5); });
    Connect('G', [] { SetGameSpeed(1); });

    OnWave(1_9, 11_19) At(341) PP();
    OnWave(10) {
        At(409) PP(9.2),
        At(830 - 299) Do {
            if (NowWave() == 10 && GetMainObject()->RefreshCountdown() > 200) {
                At(now + 299) N({{3, 9},{4, 9}});
                N_level_stat[to_string(level)]++;
                At(now + 100) Do {
                    auto ptrs = GetPlantPtrs({{3, 9}, {4, 9}}, DOOM);
                    if (ptrs[0] != nullptr) N_pos_stat["3-9"]++;
                    else if (ptrs[1] != nullptr) N_pos_stat["4-9"]++;
                    else {
                        N_pos_stat["failed"]++;
                        logger.Warning("并未放核!");
                    }
                };
            }
        },
    };
    OnWave(20) {
        At(0) I(1, 1),
        At(409) PP(9.2),
    };
    if (zombie_type_list[DANCING_ZOMBIE]) {
        C.SetCards({PUFF, M_PUFF, SCAREDY, POT});
        At(Time(10, 100)) C(300);
        At(Time(20, 100)) C(300);
    }

    for (auto wave : {9, 19, 20}) {
        for (int i = 1; i <= 3; i++) {
            At(Time(wave, 409 + i * 700 - 373)) Do {
                if (NowWave() == wave && GetMainObject()->RefreshCountdown() > 200) {
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
        logger.Debug("#, #\n#, #", N_level_stat, N_pos_stat,
                                   total_level_stat, get_time_estimate(flag_count, FLAG_GOAL, start));
    }
    if (flag_count % 100 == 0) {
        logger.Debug(set_random_seed());
    }
    logger.Debug("阳光: #, #, #f, SL次数: # #", GetMainObject()->Sun(), to_string(level), flag_count, sl_count, get_timestamp());
    level = get_level();
    if (!PAUSE_ON_FAIL)
        std::filesystem::copy(GAME_DAT_PATH, TEMP_DAT_PATH + std::to_string(flag_count % 10),
            std::filesystem::copy_options::overwrite_existing);
});

void on_fail()
{
    if (PAUSE_ON_FAIL) {
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