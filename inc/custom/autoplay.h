#pragma once

#include <cassert>
#include <chrono>
#include <sstream>

#include "macro.h"
#include "mod.h"

namespace AutoplayInternal {

std::random_device rd;
std::mt19937 gen(rd());

uint32_t get_random_seed()
{
    std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
    return dist(gen);
}

std::pair<int, int> get_plant_def(int col, PlantType type)
{
    int x = 40 + (col - 1) * 80;
    switch (type) {
    case PUMPKIN:
        return {x + 20, x + 80};
    case TALL_NUT:
        return {x + 30, x + 70};
    case COB_CANNON:
        return {x + 20, x + 120};
    default:
        return {x + 30, x + 50};
    }
}

float hammer_rate(Zombie* zombie)
{
    auto animation_code = zombie->MRef<uint16_t>(0x118);
    auto animation_array = GetPvzBase()->AnimationMain()->AnimationOffset()->AnimationArray();
    auto circulation_rate = animation_array[animation_code].CirculationRate();
    return circulation_rate - 0.644f;
}

};

enum class Level {
    Slow,
    Variable,
    White,
    Fast,
};

std::string to_string(Level level)
{
    switch (level) {
    case Level::Slow:
        return "慢";
    case Level::Variable:
        return "变";
    case Level::White:
        return "白";
    case Level::Fast:
        return "快";
    default:
        assert(false && "unreachable");
    }
}

Level get_level()
{
    auto type_list = GetZombieTypeList();
    if (type_list[GIGA] && type_list[GARG])
        return Level::Slow;
    if (type_list[GIGA] && !type_list[GARG])
        return Level::Variable;
    if (!type_list[GIGA] && type_list[GARG])
        return Level::White;
    return Level::Fast;
}

class Stat {
public:
    explicit Stat(const std::string& name)
        : name(name)
    {
    }

    int& operator[](const std::string& key)
    {
        return stats[key];
    }

    const int& operator[](const std::string& key) const
    {
        return stats.at(key);
    }

    Stat operator++(int)
    {
        Stat temp = *this;
        stats[""]++;
        return temp;
    }

    std::string to_string() const
    {
        std::ostringstream oss;
        oss << name << "={";
        bool first = true;
        for (const auto& [key, value] : stats) {
            if (first)
                first = false;
            else
                oss << ", ";
            if (key != "")
                oss << key << ": ";
            oss << value;
        }
        oss << "}";
        return oss.str();
    }

private:
    std::map<std::string, int> stats;
    std::string name;
};

template <>
struct std::formatter<Stat> : std::formatter<string> {
    template <typename FormatContext>
    auto format(const Stat& stat, FormatContext& ctx)
    {
        return std::formatter<std::string>::format(stat.to_string(), ctx);
    }
};

class Record {
public:
    explicit Record(const std::string& name, size_t max_record_num)
        : name(name)
        , max_record_num(max_record_num)
    {
    }

    void add(std::string record)
    {
        if (records.size() == max_record_num) {
            records.pop_front();
        }
        records.push_back(record);
    }

    std::string to_string() const
    {
        std::ostringstream oss;
        oss << name << "=[";
        bool first = true;
        for (const auto& record : records) {
            if (first)
                first = false;
            else
                oss << ", ";
            oss << record;
        }
        oss << "]";
        return oss.str();
    }

private:
    std::deque<std::string> records;
    std::string name;
    size_t max_record_num;
};

template <>
struct std::formatter<Record> : std::formatter<string> {
    template <typename FormatContext>
    auto format(const Record& stat, FormatContext& ctx)
    {
        return std::formatter<std::string>::format(stat.to_string(), ctx);
    }
};

std::string set_random_seed()
{
    auto seed = AutoplayInternal::get_random_seed();
    set_seed_and_update(seed);
    std::ostringstream oss;
    oss << "出怪种子: 0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << seed;
    return oss.str();
}

std::string get_timestamp()
{
    auto now = std::time(nullptr);
    std::tm localTime;
    localtime_s(&localTime, &now);
    return std::format("[{:02}:{:02}]", localTime.tm_hour, localTime.tm_min);
}

std::string get_time_estimate(int completed_flag, int total_flag, std::chrono::time_point<std::chrono::high_resolution_clock> start)
{
    std::chrono::duration<double> elapsed_duration = std::chrono::high_resolution_clock::now() - start;
    double elapsed_seconds = elapsed_duration.count();
    double total_seconds = elapsed_seconds / completed_flag * total_flag;
    double remaining_seconds = total_seconds - elapsed_seconds;

    auto finish_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + std::chrono::seconds(static_cast<int>(remaining_seconds)));
    std::tm finish_time;
    localtime_s(&finish_time, &finish_time_t);

    std::ostringstream oss;
    oss << "预计 " << std::put_time(&finish_time, "%H:%M") << " 完成 " << total_flag / 1000.0 << "k flags "
        << "(共 " << static_cast<int>(total_seconds / 60) << " min)";
    return oss.str();
}

std::string get_prev_wave_stat(int prev_wave_num)
{
    std::ostringstream oss;
    int w = NowWave();
    oss << std::format("w{} 至{}cs", w, NowTime(w));
    for (int i = 1; i < prev_wave_num; i++) {
        oss << "\n";
        w--;
        if (w < 1)
            break;
        oss << std::format("w{} 共{}cs", w, NowTime(w) - NowTime(w + 1));
    }
    return oss.str();
}

// [0, 0.644], if no imminent hammer return 0, otherwise return min circulation rate until hammer
float get_imminent_hammer_rate(Grid pos, PlantType type = PEASHOOTER)
{
    float min_hammer_rate = 999.0f;
    auto plant_def = AutoplayInternal::get_plant_def(pos.col, type);
    for (auto& z : aliveZombieFilter) {
        if (RangeIn(z.Type(), {GARG, GIGA}) && z.IsHammering() && z.Row() + 1 == pos.row) {
            auto rate = AutoplayInternal::hammer_rate(&z);
            if (rate < 0) {
                std::pair<int, int> zombie_atk = {static_cast<int>(z.Abscissa()) - 30, static_cast<int>(z.Abscissa()) + 59};
                if (std::max(zombie_atk.first, plant_def.first) <= std::min(zombie_atk.second, plant_def.second))
                    min_hammer_rate = std::min(min_hammer_rate, -rate);
            }
        }
    }
    return min_hammer_rate == 999.0f ? 0.0f : min_hammer_rate;
}