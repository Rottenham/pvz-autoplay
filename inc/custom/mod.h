#pragma once

#include "avz_pvz_struct.h"
#include "libavz.h"
#include "macro.h"

void set_level(int level)
{
    GetMainObject()->MPtr<PvzStruct>(0x160)->MRef<int>(0x6c) = level;
}

void update_zombies_preview()
{
    // kill preview zombies
    __asm__ __volatile__(
        "movl 0x6a9ec0, %%ebx;"
        "movl 0x768(%%ebx), %%ebx;"
        "movl $0x40df70, %%eax;"
        "calll *%%eax;"
        :
        :
        : "ebx", "eax", "memory");
    GetMainObject()->SelectCardUi_m()->IsCreatZombie() = false;
}

void original_spawn()
{
    auto zombie_type_list = GetMainObject()->ZombieTypeList();
    for (int i = 0; i < 33; i++)
        zombie_type_list[i] = false;

    // update zombies type
    __asm__ __volatile__(
        "movl 0x6a9ec0, %%esi;"
        "movl 0x768(%%esi), %%esi;"
        "movl 0x160(%%esi), %%esi;"
        "calll *%[func];"
        :
        : [func] "r"(0x00425840)
        : "esi", "memory");

    AAsm::PickZombieWaves();
    update_zombies_preview();
}

void set_seed_and_update(uint32_t seed)
{
    GetMainObject()->MRef<uint32_t>(0x561c) = seed;
    original_spawn();
}