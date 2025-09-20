//
// Created by Gabe on 1/22/2025.
//

#ifndef CPU32EMULATOR_UI_H
#define CPU32EMULATOR_UI_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include "cpu.h"


#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800


bool isRunning();

void initUI(void (*clock)(void), void (*reset)(void), void (*play)(void), void (*pause)(void));
void drawUI(CPU* cpu);

float getSliderPosition();
void handleInput();
void endFrame();
void shutdownUI();



#endif //CPU32EMULATOR_UI_H
