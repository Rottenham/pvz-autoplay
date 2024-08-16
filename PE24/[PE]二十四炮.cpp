#include <filesystem>

#include "dsl/shorthand_240205.h"
#include "custom/lib.h"
#include "mod/mod.h"
#include "DanceCheat/DanceCheat.h"

/**** 挂机基础设定 ****/
constexpr auto INIT_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\game1_13.dat";
constexpr auto GAME_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\userdata\\game1_13.dat";
constexpr auto TEMP_DAT_PATH = "C:\\Users\\cresc\\Desktop\\temp\\tmp.dat";
const std::vector<Grid> COB_LIST = {{1, 7}, {2, 7}, {3, 7}, {4, 7}, {5, 7}, {6, 7}};
constexpr AutoplayMode MODE = AutoplayMode::SKIPTICK;
constexpr int FLAG_GOAL = MODE == AutoplayMode::DEMO ? 50 : 10000;

/**** 收集数据 ****/
Logger<Console> logger;
auto start = std::chrono::high_resolution_clock::now();
int sl_count = 0;
int flag_count = 0;
int giga_count = 0;
bool stop_skip_tick = false;
bool reload = false;
Level level;
bool* zombie_type_list;
Stat total_level_stat("总"), ash_stat("灰烬");
Record state_record("状态");

void pause();
void sl();

std::unordered_map<std::string, Timeline> states;

Timeline Transition(int wavelength, std::string delayed, std::string activated, std::string no_giga, std::string finish, bool ignore_wavelength_warning = false) {
    return At(wavelength - 200) Do {
        int current_wave = NowWave();
        if (current_wave % 10 == 9)
            return;
        while (NowTime(current_wave) != wavelength - 200)
            current_wave--;
        if (!ignore_wavelength_warning && NowTime(current_wave + 1) != INT_MIN) {
            int actual_wavelength = NowTime(current_wave) - NowTime(current_wave + 1);
            if (actual_wavelength != wavelength) {
                logger.Warning("第 # 波提前刷新，预期波长 >=#cs, 实际波长 #cs", current_wave, wavelength, actual_wavelength);
            }
        }

        std::vector<std::pair<std::string, std::string>> choices = {{"激活", activated}, {"延迟", delayed}, {"收尾", finish}, {"转白", no_giga}};
        int idx = 0;
        if (NowTime(current_wave + 1) == INT_MIN)
            idx = 1;
        else if (current_wave % 10 == 8)
            idx = 2;
        else if (!no_giga.empty() && giga_count >= 50 && !zombie_exist(z->Type() == GIGA && z->Hp() > 1800))
            idx = 3;

        auto next_state = choices[idx].second;
        if (states.contains(next_state)) {
            if (idx == 1) {
                state_record.add(std::format("w{} {}", current_wave, next_state));
                OnWave(current_wave) states[next_state];
            }
            else {
                state_record.add(std::format("w{} {}", current_wave + 1, next_state));
                OnWave(current_wave + 1) states[next_state];
            }
        } else {
            logger.Error("状态转移失败: #->#", choices[idx].first, next_state);
        }
    };
}

Timeline finish(const std::string& sequence, int phase, int total_phase) {
    return At(-200) CoDo {
        int wave = NowWave(); // 8, 18
        while (true) {
            std::string intent = "";
            if (NowWave(true) < wave + 2)
                intent = std::format("w{}尚未刷新", wave + 2);
            else if (zombie_exist(z->AtWave() + 1 <= wave + 1 && z->Type() == GIGA))
                intent = "残留红";
            else
                break;
            At(now + 200) states[std::format("{}{}", sequence, phase)];
            state_record.add(std::format("{} w{} {}{}({})", get_readable_time(now), wave + 1, sequence, phase, intent));
            if (++phase > total_phase) phase -= total_phase;
            co_await (now + 601);
        }
    };
}

void Script()
{
    if (flag_count > FLAG_GOAL) {
        if (MODE == AutoplayMode::SKIPTICK)
            notify("测试完成");
        return;
    }
    EnableModsScoped(DisableItemDrop);
    logger.SetLevel({LogLevel::DEBUG, LogLevel::ERROR, LogLevel::WARNING});
    SetInternalLogger(logger);
    SetReloadMode(ReloadMode::MAIN_UI_OR_FIGHT_UI);
    SelectCards({ICE, M_ICE, COFFEE, PUMPKIN, PUFF, SUN, SCAREDY, POT, CHERRY}, MODE == AutoplayMode::DEMO ? 8 : 0);
    DanceCheat(DanceCheatMode::FAST);
    iceFiller.Start({{3, 9}, {4, 9}});
    plantFixer.Start(PUMPKIN, {{3, 9}, {4, 9}}, 1333);

    giga_count = 0;
    state_record.clear();
    
    // 红白节奏: PPSSDD | PPDD | PPI3
    states["红白1"] = {
        At(288) P(2255, 9) & DD<110>(9),
        Transition(601, "", "红白2", "", "红白2收尾"),
    };
    states["红白2"] = {
        At(359) PP() & DD<107>(7.8),
        Transition(601, "红白2延", "红白3", "", "红白3收尾"),
    };
    states["红白2延"] = {
        At(777) C(10, {1256, 9}),
        At(1002) PP() & 106_cs + I(),
        Transition(1202, "", "红白1", "无红1", "红白1收尾"),
    };
    states["红白3"] = {
        At(291) PP() & 106_cs + I(),
        Transition(601, "红白3延", "红白1", "无红1", "红白1收尾"),
    };
    states["红白3延"] = {
        At(1002) P(2255, 9) & DD<110>(9),
        Transition(1202, "", "红白2", "", "红白2收尾"),
    };
    states["红白1收尾"] = finish("红白", 1, 3);
    states["红白2收尾"] = finish("红白", 2, 3);
    states["红白3收尾"] = {
        At(291) PP() & 106_cs + I(),
        At(601 + 288 - 373) Do {
            if (NowWave(true) % 10 == 9)
                At(now + 373) P(2255, 9) & DD<110>(9);
            else
                At(now + 373) P(2255, 9);
        },
        At(601 * 2 + 359 - 373) Do {
            if (NowWave(true) % 10 == 9)
                At(now + 373) PP();
        }
    };
    states["红白w10"] = {
        At(295) P(2, 9) & D<100>(2, 9) & D<100+110>(1, 9),
        At(288) P(55, 9) & D<110>(5, 9),
        Transition(601, "", "红白2", "", "红白2收尾"),
    };

    // 红节奏: PSD/P | P/PSD * 3
    auto variable_P = [](bool upper) -> Timeline {
        int cob_row = upper ? 2 : 5;
        int fodder_row = upper ? 12 : 56;
        if (zombie_type_list[POLE_VAULTING_ZOMBIE] && zombie_type_list[DANCING_ZOMBIE])
            return {
                At(169) C.TriggerBy(DANCING_ZOMBIE)(70, {fodder_row, 9}),
                At(359) P(cob_row, 9),
            };
        else if (zombie_type_list[DANCING_ZOMBIE])
            return At(318) P(cob_row, 9);
        else
            return At(359) P(cob_row, 9);
    };
    states["红1"] = {
        At(288) P(22, 9) & 110_cs + P(1, 8.6),
        variable_P(false),
        Transition(601, "红1延", "红2", "无红1", "红2收尾"),
    };
    states["红1延"] = {
        At(1002) P(255, 9) & 110_cs + P(5, 8.6),
        At(777) C(10, {56, 9}),
        Transition(1202, "", "红1", "无红1", "红1收尾"),
    };
    states["红2"] = {
        variable_P(true),
        At(288) P(55, 9) & 110_cs + P(5, 8.6),
        Transition(601, "红2延", "红1", "无红1", "红1收尾"),
    };
    states["红2延"] = {
        At(1002) P(225, 9) & 110_cs + P(1, 8.6),
        At(777) C(10, {12, 9}),
        Transition(1202, "", "红2", "无红1", "红2收尾"),
    };
    states["红1收尾"] = finish("红", 1, 2);
    states["红2收尾"] = finish("红", 2, 2);
    states["红w10"] = {
        At(205) C(70, {12, 9}),
        At(395) P(2, 9),
        At(288) P(55, 9) & D<110>(5, 8.6),
        Transition(601, "红2延", "红1", "无红1", "红1收尾"),
    };

    // 无红节奏: PPDD * 6
    states["无红1"] = {
        At(298) PP() & 100_cs + PP(),
        Transition(601, "无红1延", "无红1", "", "无红1收尾"),
    };
    states["无红1延"] = {
        At(1002) PP(),
        Transition(1202, "", "无红1", "", "无红1收尾"),
    };
    states["无红1收尾"] = {
        At(298) PP() & 100_cs + PP(),
        At(601 + 298 - 373) Do {
            if (NowWave(true) % 10 == 9)
                At(now + 373) PP();
        },
    };

    if (level == Level::Slow) {
        OnWave(1) states["红白1"];
        OnWave(10) states["红白w10"];
    } else if (level == Level::Variable) {
        OnWave(1) states["红1"];
        OnWave(10) states["红w10"];
    } else {
        OnWave(1, 10) states["无红1"];
    }

    OnWave(20) {
        At(248) P(4, 7.5),
        At(338) PP() & 100_cs + PP(),
        At(338 + 100 + 110 - 373) Do {
            if (cobManager.GetUsableList().size() >= 2)
                At(now + 373) DD(9);
            else {
                At(now + 373) P(1, 9) & A(5, 9);
                ash_stat["w20樱桃"]++;
            }
        },
        At(1000 - 373) Do {
            if (IsZombieExist())
                At(now + 373) PP();
        },
    };

    if (MODE == AutoplayMode::SKIPTICK || (MODE == AutoplayMode::CHECK && !zombie_type_list[GIGA]))
        SkipTick([=] {
            if (stop_skip_tick) return false;
            for (auto [p, i] : GetPlantPtrsI(COB_LIST, COB)) {
                if (p == nullptr || p->Hp() < p->HpMax()) {
                    logger.Error("#炮受损", COB_LIST[i]);
                    pause();
                    return false;
                }
            }
            return true;
        });
    else
        SetGameSpeed(5);
    Connect('F', [] { SetGameSpeed(5); });
    Connect('G', [] { SetGameSpeed(1); });

    OnWave(1_20) At(0) Do { giga_count += zombie_count(z->Type() == GIGA && z->ExistTime() == 0); };
}

OnAfterInject({
    if (std::filesystem::exists(INIT_DAT_PATH))
        std::filesystem::copy(INIT_DAT_PATH, GAME_DAT_PATH, std::filesystem::copy_options::overwrite_existing);
    EnterGame();
});

OnBeforeScript({
    if (MODE != AutoplayMode::DEMO)
        std::filesystem::copy(GAME_DAT_PATH, TEMP_DAT_PATH + std::to_string(flag_count % 10), std::filesystem::copy_options::overwrite_existing);
    if (flag_count > 0 && flag_count % 100 == 0) {
        auto time_estimate = get_time_estimate(flag_count, FLAG_GOAL, start);
        logger.Debug("#\n#, #", ash_stat, total_level_stat, time_estimate);
        if (MODE == AutoplayMode::SKIPTICK && flag_count % 2000 == 0)
            notify(std::format("{}f, SL次数: {}, {}", flag_count, sl_count, time_estimate));
    }
    if (flag_count % 100 == 0) {
        if (MODE == AutoplayMode::DEMO)
            set_seed_and_update(0x2486C3D9);
        else
            logger.Debug(set_random_seed());
    }
    level = get_level();
    zombie_type_list = GetZombieTypeList();
    logger.Debug("阳光: #, ##, #f, SL次数: # #", GetMainObject()->Sun(), level, zombie_type_list[BUNGEE_ZOMBIE] ? "*" : "",
                                                flag_count, sl_count, get_timestamp());
    flag_count += 2;
    total_level_stat[to_string(level)]++;
});

void print_fail_stats() {
    logger.Debug(get_prev_wave_stat(6));
    logger.Debug("#", state_record);
    std::array<Zombie*, 2> zombies{};
    for (auto& z : aliveZombieFilter) {
        int idx = z.Abscissa() <= 400 ? 0 : 1;
        if (zombies[idx] == nullptr || z.Abscissa() + z.AttackAbscissa() < zombies[idx]->Abscissa() + zombies[idx]->AttackAbscissa())
            zombies[idx] = &z;
    }
    int idx = 0;
    for (auto str : {"x<=400", "x>400"}) {
        auto& z = zombies[idx];
        if (z == nullptr)
            logger.Debug("无#的僵尸", str);
        else
            logger.Debug("#的最左僵尸: w## (#路, x=#, 已存在时间=#)", str, z->StandState() + 1, ZOMBIE_NAME[z->Type()], z->Row() + 1, z->Abscissa(), z->ExistTime());
        idx++;
    }
    for (auto& [p, recover_time] : cobManager.GetRecoverList()) {
        if (p == nullptr || recover_time <= 0) continue;
        logger.Debug("#-#炮于 # 生效", p->Row() + 1, p->Col() + 1, get_readable_time(now - (3475 - recover_time) + 373));
    }  
}

void pause() {
    if (MODE == AutoplayMode::SKIPTICK)
        notify("测试中止");
    print_fail_stats();
    stop_skip_tick = true;
    SetAdvancedPause(true);
}

void sl() {
    print_fail_stats();
    reload = true;
    BackToMain(false);
    EnterGame();
}

OnExitFight({
    if (GetPvzBase()->GameUi() == AAsm::ZOMBIES_WON) {
        logger.Error("僵尸获胜");
        sl();
    }
    if (GetPvzBase()->GameUi() != AAsm::LEVEL_INTRO && reload) {
        sl_count++;
        std::filesystem::copy(TEMP_DAT_PATH + std::to_string(flag_count % 10), GAME_DAT_PATH, std::filesystem::copy_options::overwrite_existing);
        flag_count = std::clamp(flag_count - 10, 0, INT_MAX);
    }
    reload = false;
});
