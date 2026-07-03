//
// Created by Gabe on 1/22/2025.
//

#ifndef CPU32EMULATOR_CPU_H
#define CPU32EMULATOR_CPU_H

#include "stdint.h"
#include "stdbool.h"

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

#define FLAG_Z 0
#define FLAG_L 1
#define FLAG_E 2
#define FLAG_G 3
#define FLAG_C 4

typedef struct cpu {
    u32 ins_reg;
    u32 regs[8];
    u32 pc;
    u32 bp;
    u32 sp;
    u32 adr;
    u32 cycles;
    bool halted;
    bool flags[5];
    u32 program[65536];
    u32 ram_out;
    u32 microcode[65536];
    int mc_counter;
    char* outptr;
    char output[256];
    u32 debug_bus;
} CPU;

#endif //CPU32EMULATOR_CPU_H
