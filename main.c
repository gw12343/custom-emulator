#include "ui.h"
#include "SDL.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>


const u32 constantVals[] = {0x0, 0x1, 0xFFFFFFFF, 0x0};

void load_hex(const char filename[], u32* data);
u16 microcode_address(int counter, u8 instruction);
void cycle(CPU* pcpu);
void resetCPU(CPU* pcpu);

const char* get_load_name(int num){
    switch (num) {
        case 0: return "(alu)";
        case 1: return "(r1)";
        case 2: return "(r2)";
        case 3: return "(r3)";
        case 4: return "(r4)";
        case 5: return "(r5)";
        case 6: return "(r6)";
        case 7: return "(r7)";
        case 8: return "(r8)";
        case 9: return "(ram)";
        case 10: return "(ins)";
        case 11: return "(adr)";
        case 12: return "(pc)";
        case 13: return "(?\?)";
        case 14: return "(bp)";
        case 15: return "(sp)";
        default: return "(invalid)";
    }
}
const char* get_store_name(int num){
    switch (num) {
        case 0: return "(?\?)";
        case 1: return "(r1)";
        case 2: return "(r2)";
        case 3: return "(r3)";
        case 4: return "(r4)";
        case 5: return "(r5)";
        case 6: return "(r6)";
        case 7: return "(r7)";
        case 8: return "(r8)";
        case 9: return "(ram)";
        case 10: return "(ins)";
        case 11: return "(adr)";
        case 12: return "(pc)";
        case 13: return "(?\?)";
        case 14: return "(bp)";
        case 15: return "(sp)";
        default: return "(invalid)";
    }
}

CPU* cpu;

void c(void){
    cycle(cpu);
}

void reset(){
    resetCPU(cpu);
}

bool simulating = 0;
u64 lastTime;

void pause(){
    simulating = 0;
}


void play(){
    simulating = 1;
    lastTime = SDL_GetTicks();
}


int SDL_main(int argc, char* argv[])
{
    cpu = (CPU*)malloc(sizeof(CPU));
    resetCPU(cpu);

    cycle(cpu);
    initUI(c, reset, play, pause);
    lastTime = SDL_GetTicks();
    while (isRunning())
    {
        handleInput();

        if(simulating) {

            // Calculate # of updates desired since last frame
            // based on elapsed time div by target clocks per second
            u64 dt = SDL_GetTicks() - lastTime;
            u64 prd = (u64) ((1.0 / getSliderPosition()) * 1000);

            // Prevent divide by zero for very high cps, just limit to 20
            if (prd == 0) {
                for (int i = 0; i < 20; i++) {
                    c();
                }
                lastTime += dt;
            } else {
                if (dt / prd > 0) {
                    for (int i = 0; i < dt / prd; i++) {
                        c();
                    }
                    lastTime += dt;
                }
            }
        }


        drawUI(cpu);

        endFrame();
    }

    shutdownUI();
    free(cpu);
    return 0;
}

u32 rotate_left(u32 value, u32 shift) {
    shift %= 32;
    return (value << shift) | (value >> (32 - shift));
}

u32 rotate_right(u32 value, u32 shift) {
    shift %= 32;
    return (value >> shift) | (value << (32 - shift));
}

u32 executeALU(CPU* pcpu, u8 op, u32 a, u32 b, bool updateFlags) {
    bool greater = false;
    bool less = false;
    bool equal = false;
    bool carry = false;
    //printf("ALU OP %u\n", op);
    u32 val = 0;

    if(op == 1){
        if (b > 0 && a > INT_MAX - b){
            carry = true;
        }
        val = (u32)(a + b);
        //printf("%u + %u = %u", a, b, val);
    }
    else if(op == 2){
        if (b > 0 && a < INT_MAX + b){
            carry = true;
        }
        val = (u32)(a - b);
    }
    else if(op == 3){
        val = (u32)(a * b);
    }
    else if(op == 4){
        val = (u32)(-a);
    }
    else if(op == 5){
        val = (u32)(a << b);
    }
    else if(op == 6 || op == 7){
        val = (u32)(a >> b);
    }
    else if(op == 8){
        val = (u32)(rotate_left(a, b));
    }
    else if(op == 9){
        val = (u32)(rotate_right(a, b));
    }
    else if(op == 10){
        greater = a > b;
        equal = a == b;
        less = a < b;
    }
    else if(op == 11){
        val = (u32)(a & b);
    }
    else if(op == 12){
        val = (u32)(a | b);
    }
    else if(op == 13){
        val = (u32)(~a);
    }
    else if(op == 14){
        val = (u32)(a ^ b);
    }
    else if(op == 15){
        val = (u32)(~(a & b));
    }

    if(updateFlags) {
        bool *flags = pcpu->flags;
        if (op != 10) {
            flags[FLAG_Z] = val == 0;
        } else {
            flags[FLAG_L] = less;
            flags[FLAG_E] = equal;
            flags[FLAG_G] = greater;
        }
        if(op == 1 || op == 2) {
            flags[FLAG_C] = carry;
        }
    }

    return val;
}

u32 getRegisterValue(CPU* pcpu, int reg, u16 insLit, u32 constVal, u32 op, u32 a, u32 b, bool updateFlags){
    if(pcpu == NULL) return 0;
    if(reg == 0) return executeALU(pcpu, op, a, b, updateFlags);
    if(reg > 0 && reg <=8){
        return pcpu->regs[reg - 1];
    }
    if(reg == 9) return pcpu->program[pcpu->adr];
    if(reg == 10) return insLit;
    if(reg == 11) return pcpu->adr;
    if(reg == 12) return pcpu->pc;
    if(reg == 13) return constVal;
    if(reg == 14) return pcpu->bp;
    if(reg == 15) return pcpu->sp;
    return 0;
}

void setRegisterValue(CPU* pcpu, int reg, u32 value){
    if(pcpu == NULL) return;
    if(reg > 0 && reg <=8){
        pcpu->regs[reg - 1] = value;
    }else if(reg == 9){
        pcpu->program[pcpu->adr] = value; //TODO mem map
        if(pcpu->adr == 0x6000){
            //printf("wrote: %c", (char)value);
            *(pcpu->outptr++) = (char)value;
            *(pcpu->outptr) = '\0';
        }
    }else if(reg == 10){
        pcpu->ins_reg = value;
    }else if(reg == 11){
        pcpu->adr = (u16)value;
    }else if(reg == 12){
        pcpu->pc = value;
    }else if(reg == 14){
        pcpu->bp = value;
    }else if(reg == 15){
        pcpu->sp = value;
    }
}

u32 aluMux(CPU* pcpu, u32 reg, u32 constVal){
    if(reg == 0){
        return 0;
    }
    if(reg <= 8){
        return pcpu->regs[reg - 1];
    }
    if(reg == 9){
        return pcpu->program[pcpu->adr];
    }
    if(reg == 10){
        return pcpu->ins_reg & 0xFFFF;
    }
    if(reg == 11){
        return pcpu->adr;
    }
    if(reg == 12){
        return pcpu->pc;
    }
    if(reg == 13){
        return constVal;
    }
    if(reg == 14){
        return pcpu->bp;
    }
    if(reg == 15){
        return pcpu->bp;
    }
    return 0;
}

void cycle(CPU* pcpu) {
    if(!pcpu || pcpu->halted) return;
    u16 mc_adr = microcode_address(pcpu->mc_counter, pcpu->ins_reg >> 24);
    u32 mc = pcpu->microcode[mc_adr];

    u8 registerIn = mc & 0xF;
    u8 registerOut = (mc >> 4) & 0xF;
    u32 constantVal = constantVals[(mc >> 8) & 0x3];
    u8 aluOp = (mc >> 10) & 0xF;
    bool pcCount = (mc >> 14) & 1;
    bool mcEnd = (mc >> 15) & 1;
    bool spCount = (mc >> 16) & 1;
    bool insAOut = (mc >> 17) & 1;
    bool insBOut = (mc >> 18) & 1;
    bool insAIn = (mc >> 19) & 1;
    bool insBIn = (mc >> 20) & 1;
    bool spDecrement = (mc >> 21) & 1;
    bool updateFlags = (mc >> 22) & 1;
    bool halt = (mc >> 23) & 1;
    u8 flagCondition = (mc >> 24) & 0b111;
    bool insLitB = (mc >> 27) & 1;
    bool constB = (mc >> 28) & 1;

    if(halt){
        printf("======= CPU HALTED =======");
        pcpu->halted = true;
        return;
    }

    u32 a = aluMux(pcpu, (pcpu->ins_reg >> 20) & 0xF, constantVal);
    u32 b;

    if(constB){
        b = constantVal;
    }else if(insLitB){
        b = pcpu->ins_reg & 0xFFFF;
    } else {
        b = aluMux(pcpu, (pcpu->ins_reg >> 16) & 0xF, constantVal);
    }

    if(insAIn){
        registerIn = (pcpu->ins_reg >> 20) & 0xF;
    }else if(insBIn){
        registerIn = (pcpu->ins_reg >> 16) & 0xF;
    }

    if(insAOut){
        registerOut = (pcpu->ins_reg >> 20) & 0xF;
    }else if(insBOut){
        registerOut = (pcpu->ins_reg >> 16) & 0xF;
    }

/*
    printf("\nINS %x (step %d)\n", pcpu->ins_reg >> 24, pcpu->mc_counter);
    {
        printf("        register out: %u %s\n", registerOut, get_load_name(registerOut));
        printf("        register in: %u %s\n", registerIn, get_store_name(registerIn));
        printf("        constant val: %u \n", constantVal);
        printf("        alu op: %u \n", aluOp);
        printf("        pc count: %s \n", pcCount ? "true" : "false");
        printf("        mc end: %s \n", mcEnd ? "true" : "false");
        printf("        sp count: %s \n", spCount ? "true" : "false");
        printf("        ins a out: %s \n", insAOut ? "true" : "false");
        printf("        ins b out: %s \n", insBOut ? "true" : "false");
        printf("        ins a in: %s \n", insAIn ? "true" : "false");
        printf("        ins b in: %s \n", insBIn ? "true" : "false");
        printf("        sp dec: %s \n", spDecrement ? "true" : "false");
        printf("        update flags: %s \n", updateFlags ? "true" : "false");
        printf("        halt: %s \n", halt ? "true" : "false");
        printf("        flagCondition: %u \n", flagCondition);
        printf("        use ins lit B: %u \n", insLitB);
        printf("        use const b: %u \n", constB);
    }
*/



    u32 bus = getRegisterValue(pcpu, registerOut, (pcpu->ins_reg & 0xFFFF), constantVal, aluOp, a, b, updateFlags);

    pcpu->debug_bus = bus;
    setRegisterValue(pcpu, registerIn, bus);
    //printf("    bus: %x\n", bus);
    //printf("    ins: %x\n\n\n", pcpu->ins_reg);

    pcpu->mc_counter++;
    if(pcCount){
        pcpu->pc++;
    }

    bool flagConditionFailed = false;
    switch (flagCondition) {
        case 1:
            flagConditionFailed = pcpu->flags[FLAG_Z];
            break;
        case 2:
            flagConditionFailed = !pcpu->flags[FLAG_Z];
            break;
        case 3:
            flagConditionFailed = !pcpu->flags[FLAG_L];
            break;
        case 4:
            flagConditionFailed = !pcpu->flags[FLAG_G];
            break;
        case 5:
            flagConditionFailed = !pcpu->flags[FLAG_E];
            break;
        case 6:
            flagConditionFailed = pcpu->flags[FLAG_E];
            break;
        case 7:
            flagConditionFailed = !pcpu->flags[FLAG_C];
            break;
        default:
            flagConditionFailed = false;
    }

    if(mcEnd || flagConditionFailed){
        pcpu->mc_counter = 0;
        return;
    }
    if(spCount){
        pcpu->sp += spDecrement ? -1 : 1;
    }


}

u16 microcode_address(int counter, u8 instruction) {
    return (counter & 0xF) + ((instruction & 0xFF) << 4);
}

void load_hex(const char filename[], u32* data) {
    FILE *file = NULL;
    char buffer[72];
    file = fopen(filename, "r");
    if(file == NULL){
        return;
    }

    int n = 0;
    fgets(buffer, 100, file);
    while(fgets(buffer, 100, file)) {
        char* ptr = (char *) buffer;
        for(int i = 0; i < 8; i ++){
            u32 val = (u32)strtoll(ptr, NULL, 16);
            data[n++] = val;
            ptr += 9;
        }
    }
}

void resetCPU(CPU* pcpu) {
    if(pcpu == NULL)  return;
    pcpu->ins_reg = 0;
    pcpu->pc = 0;
    pcpu->bp = 0;
    pcpu->sp = 0;
    pcpu->adr = 0;
    pcpu->output[0] = '\0';
    pcpu->outptr = &pcpu->output[0];
    pcpu->flags[FLAG_Z] = false;
    pcpu->flags[FLAG_L] = false;
    pcpu->flags[FLAG_E] = false;
    pcpu->flags[FLAG_G] = false;
    pcpu->flags[FLAG_C] = false;
    memset(pcpu->regs, 0, sizeof pcpu->regs);
    pcpu->halted = false;
    pcpu->mc_counter = 0;
    load_hex("program.rom", pcpu->program);
    load_hex("microcode.rom", pcpu->microcode);
}

