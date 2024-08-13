#include <filesystem>

#include "dsl/shorthand_240205.h"
#include "custom/lib.h"
#include "mod/mod.h"
#include "DanceCheat/DanceCheat.h"

/**** 挂机基础设定 ****/
constexpr auto INIT_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\game1_13.dat";
constexpr auto GAME_DAT_PATH = "C:\\ProgramData\\PopCap Games\\PlantsVsZombies\\userdata\\game1_13.dat";
constexpr auto TEMP_DAT_PATH = "C:\\Users\\cresc\\Desktop\\temp\\tmp.dat";
const std::vector<Grid> SAFE_COB_LIST = {{1, 5}, {2, 5}, {5, 5}, {6, 5}, {3, 7}, {4, 7}};
const std::vector<Grid> UNSAFE_COB_LIST = {{1, 1}, {2, 1}, {5, 1}, {6, 1}};
constexpr AutoplayMode MODE = AutoplayMode::DEMO;
constexpr int FLAG_GOAL = MODE == AutoplayMode::DEMO ? 50 : 10000;
const std::vector<std::pair<int, int>> FIX_COB_THRES_BY_SUN = {{0, 160}, {4000, 260}};

/**** 收集数据 ****/
Logger<Console> logger;
auto start = std::chrono::high_resolution_clock::now();
int sl_count = 0;
int flag_count = 0;
int giga_count = 0;
std::vector<int> init_cob_hp(UNSAFE_COB_LIST.size(), 0);
bool stop_skip_tick = false;
bool reload = false;
Level level;
bool* zombie_type_list;
Stat total_level_stat("总"), cob_fix_stat("换炮"), cob_fix_count("换炮数"),
     giga_level_hp_loss("红关炮损"), non_giga_level_hp_loss("非红关炮损"), ash_stat("灰烬");
Record state_record("状态");

void pause();
void sl();

Coroutine fix_cob() {
    std::vector<std::pair<int, Grid>> cobs_to_fix = {};
    auto hp_thres = 0;
    for (auto [sun, thres] : FIX_COB_THRES_BY_SUN)
        if (GetMainObject()->Sun() >= sun)
            hp_thres = thres;
    for (auto [p, i] : GetPlantPtrsI(UNSAFE_COB_LIST, COB)) {
        if (p->Hp() <= hp_thres) {
            cobs_to_fix.push_back({p->Hp(), {p->Row() + 1, p->Col() + 1}});
        }
    }
    std::sort(cobs_to_fix.begin(), cobs_to_fix.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    if (level != Level::Fast && cobs_to_fix.size() > 1)
        cobs_to_fix.erase(cobs_to_fix.begin() + 1, cobs_to_fix.end());

    if (!cobs_to_fix.empty()) {
        co_await Time(0, 2);
        for (auto [_, cob] : cobs_to_fix)
            cobManager.EraseFromList({{cob}});
        for (auto [hp, pos] : cobs_to_fix) {
            co_await [=] { return GetMainObject()->SeedArray()[GetCardIndex(COB)].Cd() == 0 && IsSeedUsable(KERNEL) && GetMainObject()->Sun() >= 2000; };
            auto [row, col] = pos;
            logger.Warning("更换#-#炮 (HP=#)", row, col, hp);
            cob_fix_stat[std::to_string(hp)]++;
            cob_fix_count++;

            auto new_cob_list = cobManager.GetList();
            new_cob_list.push_back(pos);

            RemovePlant(row, col);
            At(now) Card(KERNEL_PULT, row, col + 1);
            At(now + 751) {
                Card({KERNEL_PULT, COB}, row, col),
                Do { cobManager.SetList(new_cob_list); },
            };
        }
    }
}

bool remaining_zombie_exist() {
    for (auto& z : aliveZombieFilter) {
        if (z.Abscissa() > 400)
            return true;
    }
    return false;
}

bool threatening_giga_exist() {
    for (auto& z : aliveZombieFilter) {
        if (z.Type() == GIGA && z.Hp() > 1800)
            return true;
    }
    return false;
}

std::unordered_map<std::string, Timeline> states;

Timeline Transition(int wavelength, std::string delayed, std::string activated, std::string no_garg, std::string finish, bool ignore_wavelength_warning = false) {
    return At(wavelength - 200) Do {
        int current_wave = NowWave();
        while (NowTime(current_wave) != wavelength - 200)
            current_wave--;
        if (!ignore_wavelength_warning && NowTime(current_wave + 1) != INT_MIN) {
            int actual_wavelength = NowTime(current_wave) - NowTime(current_wave + 1);
            if (actual_wavelength != wavelength) {
                logger.Warning("第 # 波提前刷新，预期波长 >=#cs, 实际波长 #cs", current_wave, wavelength, actual_wavelength);
            }
        }

        const std::string* next_state;
        std::string intent;
        if (NowTime(current_wave + 1) == INT_MIN) {
            next_state = &delayed;
            intent = "延迟";
        }
        else if (current_wave % 10 == 8) {
            next_state = &finish;
            intent = "收尾";
        }
        else if (!no_garg.empty() && level == Level::Variable && giga_count >= 50 && !threatening_giga_exist()) {
            next_state = &no_garg;
            intent = "转快";
        }
        else {
            next_state = &activated;
            intent = "激活";
        }

        if (states.contains(*next_state)) {
            if (next_state == &delayed) {
                OnWave(current_wave) states[*next_state];
                state_record.add(std::format("w{} {}", current_wave, *next_state));
            }
            else {
                OnWave(current_wave + 1) states[*next_state];
                state_record.add(std::format("w{} {}", current_wave + 1, *next_state));
            }
        } else {
            logger.Error("状态转移失败: #->#", intent, *next_state);
        }
    };
}

Coroutine finish() {
    while (true) {
        co_await [=] { return cobManager.GetUsableList().size() >= 2; };
        if (!remaining_zombie_exist())
            co_return;
        cobManager.Fire({{2, 9}, {5, 9}});
        co_await NowDelayTime(376);
    }
}

TickRunner cob_hp_drawer;
void draw_cob_hp()
{
    for (auto& p : GetPlantPtrs(UNSAFE_COB_LIST, COB)) {
        if (p != nullptr)
            painter.Draw(Text(std::format("HP: {}", p->Hp()), p->Abscissa(), p->Ordinate()));
    }
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
    SelectCards({KERNEL_PULT, COB, ICE, COFFEE, CHERRY, LILY, DOOM}, MODE == AutoplayMode::DEMO ? 8 : 0);
    DanceCheat(DanceCheatMode::FAST);

    giga_count = 0;
    state_record.clear();
    CoLaunch(fix_cob);

    // 有巨人节奏: PPDD | PP | PP
    states["巨1"] = {
        At(291) PP() & DD<110>(9),
        Transition(601, "", "巨2", "快", "PP收尾"),
    };
    states["巨2"] = {
        At(341) PP(),
        Transition(601, "巨2延", "巨3", "快", "PP收尾"),
    };
    states["巨2延"] = {
        At(1002) PP(),
        Transition(1202, "", "巨1", "快", "PPDD收尾"),
    };
    states["巨3"] = {
        At(341) PP(),
        Transition(601, "巨3延", "巨1", "快", "PPDD收尾"),
    };
    states["巨3延"] = {
        At(1002) PP() & DD<110>(9),
        Transition(1202, "", "巨2", "快", "PP收尾"),
    };
    states["巨w10"] = {
        At(341) PP(),
        At(532 - 373) Do {
            if (cobManager.GetUsableList().size() >= 2)
                cobManager.Fire({{2, 9}, {5, 9}});
            else {
                At(now + 373) N({{3, 9}, {4, 9}});
                ash_stat["w10核"]++;
            }
        },
        Transition(732, "", "巨2", "快", "PP收尾", true),
    };

    // 快速关节奏: PP
    states["快"] = {
        At(341) PP(),
        Transition(601, "快延1", "快", "", "PP收尾"),
    };
    states["快延1"] = {
        At(1002) PP(),
        Transition(1202, "快延2", "快", "", "PP收尾"),
    };
    states["快w10"] = {
        At(341) PP(),
        At(532) PP(),
        Transition(732, "", "快", "", "PP收尾", true),  
    };

    // 收尾
    states["PP收尾"] = {
        At(338) PP() & Do { CoLaunch(finish); },
    };
    states["PPDD收尾"] = {
        At(291) PP(),
        At(401) DD<110>(9) & Do { CoLaunch(finish); },
    };

    if (level == Level::Fast) {
        OnWave(1) states["快"];
        OnWave(10) states["快w10"];
    } else {
        OnWave(1) states["巨3"];
        OnWave(10) states["巨w10"];
    }

    OnWave(20) {
        At(248) P(4, 7.5),
        At(338 - 373) Do {
            if (zombie_type_list[GIGA] && cobManager.GetUsableList().size() >= 4)
                At(now + 373) PP() & DD<110>(9);
            else if (cobManager.GetUsableList().size() >= 2)
                cobManager.Fire({{2, 9}, {5, 9}});
            else {
                cobManager.Fire(2, 9);
                At(now + 373) A(5, 9);
                ash_stat["w20樱"]++;
            }
        }
    };

    if (zombie_type_list[BUNGEE_ZOMBIE]) {
        OnWave(10) At(600) I(1, 7);
        OnWave(20) {
            At(675) I(1, 7),
            At(1583 - 373) Do { CoLaunch(finish); },
        };
    } else {
        OnWave(20) At(829 - 373) Do { CoLaunch(finish); };
    }

    if (MODE != AutoplayMode::SKIPTICK) {
        SetGameSpeed(5);
        cob_hp_drawer.Start(draw_cob_hp);
    }
    else
        SkipTick([=] {
            if (stop_skip_tick) return false;
            for (auto [p, i] : GetPlantPtrsI(SAFE_COB_LIST, COB)) {
                if (p == nullptr || p->Hp() < p->HpMax()) {
                    logger.Error("#炮受损", SAFE_COB_LIST[i]);
                    pause();
                    return false;
                }
            }
            for (auto [p, i] : GetPlantPtrsI(UNSAFE_COB_LIST, COB)) {
                if (p == nullptr && Contains(cobManager.GetList(), UNSAFE_COB_LIST[i])) {
                    logger.Error("#炮被啃光", UNSAFE_COB_LIST[i]);
                    sl();
                    return false;
                }
            }
            return true;
        });
    Connect('F', [] { SetGameSpeed(5); });
    Connect('G', [] { SetGameSpeed(1); });

    OnWave(1_20) At(0) Do {
        for (auto& z : aliveZombieFilter) {
            if (z.Type() == GIGA && z.ExistTime() == 0)
                giga_count++;
        }
    };
    OnWave(1) At(-599) Do {
        for (auto [p, i] : GetPlantPtrsI(UNSAFE_COB_LIST, COB)) {
            init_cob_hp[i] = p == nullptr ? 0 : p->Hp();
        }
    };
    OnWave(21) At(-499) Do {
        for (auto [p, i] : GetPlantPtrsI(UNSAFE_COB_LIST, COB)) {
            int curr_hp = p == nullptr ? 0 : p->Hp();
            if (curr_hp < init_cob_hp[i]) {
                auto hp_loss = std::to_string(init_cob_hp[i] - curr_hp);
                if (zombie_type_list[GIGA])
                    giga_level_hp_loss[hp_loss]++;
                else
                    non_giga_level_hp_loss[hp_loss]++;
            }
        }
    };
}

OnAfterInject({
    if (MODE != AutoplayMode::DEMO && std::filesystem::exists(INIT_DAT_PATH))
        std::filesystem::copy(INIT_DAT_PATH, GAME_DAT_PATH, std::filesystem::copy_options::overwrite_existing);
    EnterGame();
});

OnBeforeScript({
    if (MODE != AutoplayMode::DEMO)
        std::filesystem::copy(GAME_DAT_PATH, TEMP_DAT_PATH + std::to_string(flag_count % 10), std::filesystem::copy_options::overwrite_existing);
    if (flag_count > 0 && flag_count % 100 == 0) {
        auto time_estimate = get_time_estimate(flag_count, FLAG_GOAL, start);
        auto cob_fix_frequency = cob_fix_count.val() == 0 ? "?" : std::format("{:.1f}", static_cast<double>(flag_count) / cob_fix_count.val());
        logger.Debug("#, #, 每#f换炮, #\n#, #\n#, #", cob_fix_stat, cob_fix_count, cob_fix_frequency, ash_stat,
                                                     giga_level_hp_loss, non_giga_level_hp_loss,
                                                     total_level_stat, time_estimate);
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
    logger.Debug("起始炮HP=[#]", Concat(init_cob_hp));
    std::array<Zombie*, 2> zombies{};
    for (auto& z : aliveZombieFilter) {
        int idx = z.Abscissa() <= 400 ? 0 : 1;
        if (zombies[idx] == nullptr || z.Abscissa() + z.AttackAbscissa() < zombies[idx] ->Abscissa() + zombies[idx] ->AttackAbscissa())
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
