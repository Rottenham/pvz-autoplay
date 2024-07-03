#include <filesystem>

#include "avz.h"
#include "custom/lib.h"
#include "mod/mod.h"

/**** 挂机基础设定 ****/
constexpr int FLAG_GOAL = 10000;
constexpr auto GAME_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\userdata\\game1_13.dat";
constexpr auto TEMP_DAT_PATH = "C:\\Users\\cresc\\Desktop\\temp\\tmp.dat";
const std::vector<Grid> COB_LIST = {{1, 5}, {2, 5}, {3, 3}, {4, 3}, {5, 5}, {6, 5}};
const std::vector<Grid> GLOOM_LIST = {{3, 7}, {4, 7}};
constexpr bool SKIP_TICK = true;
constexpr bool PAUSE_ON_FAIL = false;

/**** 收集数据 ****/
Logger<Console> logger;
auto start = std::chrono::high_resolution_clock::now();
int sl_count = 0;
int flag_count = 0;
Level level;
Stat total_level_stat("总"), cob_hp_stat("炮HP"), gloom_fix_stat("补曾");

void on_fail();

TickRunner gloom_fume_fix_runner;
void fix_gloom_fume() {
    if (GetMainObject()->SeedArray()[GetCardIndex(GLOOM_SHROOM)].Cd() == 0) {
        for (auto [row, col] : GLOOM_LIST) {
            if (GetPlantPtr(row, col, GLOOM_SHROOM) == nullptr) {
                ACard({LILY, FUME_SHROOM, GLOOM_SHROOM, COFFEE, PUMPKIN}, row, col);
                logger.Warning("修补 #-# 曾.", row, col);
                gloom_fix_stat[std::format("{}-{}", row, col)]++;
                break;
            }
        }
    }
}

TickRunner smart_blover_runner;
void smart_blover()
{
    if (IsSeedUsable(BLOVER)) {
        for (auto& z : aAliveZombieFilter) {
            if (z.Type() == BALLOON_ZOMBIE && z.Abscissa() < 100) {
                ACard(BLOVER, 6, 1);
                return;
            }
        }
    }
}

void Script() {
    if (flag_count > FLAG_GOAL) return;
    EnableModsScoped(DisableItemDrop);
    logger.SetLevel({LogLevel::DEBUG, LogLevel::ERROR, LogLevel::WARNING});
    SetInternalLogger(logger);
    SetReloadMode(ReloadMode::MAIN_UI_OR_FIGHT_UI);
    SelectCards({PUMPKIN, LILY, FUME_SHROOM, GLOOM_SHROOM, COFFEE, BLOVER, 0, 1, 2, 3}, 0);
    
    plantFixer.Start(PUMPKIN, {}, 2666);
    MaidCheats::Dancing();
    gloom_fume_fix_runner.Start(fix_gloom_fume);
    if (GetZombieTypeList()[BALLOON_ZOMBIE])
        smart_blover_runner.Start(smart_blover);

    if (SKIP_TICK)
        SkipTick([] {
            auto ptrs = GetPlantPtrs(COB_LIST, COB);
            for (size_t i = 0; i < ptrs.size(); i++) {
                if (ptrs[i] == nullptr) {
                    logger.Warning("#-#炮丢失", COB_LIST[i].row, COB_LIST[i].col);
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

    Connect([] { return NowTime(1) % 869 == 296; },
        [] {aCobManager.Fire({{2, 8.15}, {5, 8.2375}}); });
}

OnAfterInject({
    EnterGame();
});

AOnEnterFight({
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
        logger.Debug("#\n#\n#, #", cob_hp_stat,
                                   gloom_fix_stat,
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
        ABackToMain(false);
        AEnterGame();
    }
}

OnExitFight({
    if (GetPvzBase()->GameUi() == AAsm::ZOMBIES_WON) {
        Zombie* zombie = nullptr;
        for(auto& z : aAliveZombieFilter) {
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