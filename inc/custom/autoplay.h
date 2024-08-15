#pragma once

#include <cassert>
#include <chrono>
#include <sstream>

#include "macro.h"
#include "mod.h"
#include "shared.h"

namespace AutoplayInternal {

std::random_device rd;
std::mt19937 gen(rd());

uint32_t get_random_seed()
{
    std::uniform_int_distribution<uint32_t> dist(0, UINT32_MAX);
    return dist(gen);
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
        throw "unreachable";
    }
}

template <>
struct std::formatter<Level> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(Level level, FormatContext& ctx) const
    {
        return std::formatter<std::string>::format(to_string(level), ctx);
    }
};

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

enum class AutoplayMode {
    DEMO,
    CHECK,
    SKIPTICK
};

class Stat {
public:
    explicit Stat(const std::string& name)
        : name(name)
    {
    }

    template <typename T>
    int& operator[](const T& key)
    {
        return stats[std::format("{}", key)];
    }

    template <typename T>
    const int& operator[](const T& key) const
    {
        return stats.at(std::format("{}", key));
    }

    Stat operator++(int)
    {
        Stat temp = *this;
        stats[""]++;
        return temp;
    }

    std::string to_string() const
    {
        std::vector<std::string> elems;
        for (const auto& [key, value] : stats)
            elems.push_back(std::format("{}: {}", key, value));
        return std::format("{}={{{}}}", name, Concat(elems, ", "));
    }

    int val() const
    {
        auto it = stats.find("");
        return (it != stats.end()) ? it->second : 0;
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
    explicit Record(const std::string& name, int max_record_num = -1)
        : name(name)
        , max_record_num(max_record_num)
    {
    }

    template <typename T>
    void add(const T& record)
    {
        if (records.size() == max_record_num) {
            records.pop_front();
        }
        records.push_back(std::format("{}", record));
    }

    void clear()
    {
        records.clear();
    }

    std::string to_string() const
    {
        return std::format("{}=[{}]", name, Concat(records, ", "));
    }

private:
    std::deque<std::string> records;
    std::string name;
    int max_record_num;
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
    std::vector<std::string> outputs;
    int w = NowWave();
    outputs.push_back(std::format("w{} 至{}cs", w, NowTime(w)));
    for (int i = 1; i < prev_wave_num; i++) {
        w--;
        if (w < 1)
            break;
        outputs.push_back(std::format("w{} 共{}cs", w, NowTime(w) - NowTime(w + 1)));
    }
    std::reverse(outputs.begin(), outputs.end());
    return Concat(outputs, "\n");
}

Time get_readable_time(Time time)
{
    while (time.time < 0) {
        time.wave--;
        time.time += NowTime(time.wave) - NowTime(time.wave + 1);
    }
    return time;
}