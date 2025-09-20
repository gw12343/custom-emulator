//
// Created by Gabe on 1/22/2025.
//
#include "ui.h"
#include <SDL.h>
#include <SDL_image.h>
#include <stdbool.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"

SDL_Window* win;
SDL_Renderer* renderer;
int running = 1;

int flags = 0;
float font_scale = 1;
u64 lastUITime = 0;
float scale = 0;
/* GUI */
struct nk_context* ctx;
struct nk_colorf bg;

float slider_value;

void (*onClock)(void);
void (*onReset)(void);
void (*onPlay)(void);
void (*onPause)(void);

bool isRunning(){
    return running;
}

void initUI(void (*clock)(void), void (*reset)(void), void (*play)(void), void (*pause)(void)) {
    lastUITime = SDL_GetTicks64();
    onClock = clock;
    onReset = reset;
    onPlay = play;
    onPause = pause;
    /* SDL setup */
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
    SDL_Init(SDL_INIT_VIDEO);

    win = SDL_CreateWindow("Emulator",
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);

    if (win == NULL)
    {
        SDL_Log("Error SDL_CreateWindow %s", SDL_GetError());
        exit(-1);
    }

    flags |= SDL_RENDERER_ACCELERATED;
    flags |= SDL_RENDERER_PRESENTVSYNC;


    renderer = SDL_CreateRenderer(win, -1, flags);

    if (renderer == NULL)
    {
        SDL_Log("Error SDL_CreateRenderer %s", SDL_GetError());
        exit(-1);
    }

    /* scale the renderer output for High-DPI displays */
    {
        int render_w, render_h;
        int window_w, window_h;
        float scale_x, scale_y;
        SDL_GetRendererOutputSize(renderer, &render_w, &render_h);
        SDL_GetWindowSize(win, &window_w, &window_h);
        scale_x = (float)(render_w) / (float)(window_w);
        scale_y = (float)(render_h) / (float)(window_h);
        SDL_RenderSetScale(renderer, scale_x, scale_y);
        font_scale = scale_y;
    }

    ctx = nk_sdl_init(win, renderer);
    {
        struct nk_font_atlas* atlas;
        struct nk_font_config config = nk_font_config(0);
        struct nk_font* font;

        nk_sdl_font_stash_begin(&atlas);
        font = nk_font_atlas_add_default(atlas, 13 * font_scale, &config);
        nk_sdl_font_stash_end();

        font->handle.height /= font_scale;
        nk_style_set_font(ctx, &font->handle);
    }

    bg.r = 0.10f, bg.g = 0.18f, bg.b = 0.24f, bg.a = 1.0f;
}
void handleInput(){
    /* Input */
    SDL_Event evt;
    nk_input_begin(ctx);
    while (SDL_PollEvent(&evt))
    {
        if (evt.type == SDL_QUIT) shutdownUI();
        nk_sdl_handle_event(&evt);
    }
    nk_sdl_handle_grab(); /* optional grabbing behavior */
    nk_input_end(ctx);
}
void shutdownUI() {
    nk_sdl_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
}

void drawUI(CPU *cpu) {

    if (nk_begin(ctx, "CPU State", nk_rect(10, 10, 300, 320), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "Registers", NK_TEXT_CENTERED);
        nk_labelf(ctx, NK_TEXT_CENTERED, "Bus: 0x%x (%u)", cpu->debug_bus, cpu->debug_bus);
        nk_labelf(ctx, NK_TEXT_CENTERED, "Z:%u L:%u E:%u G:%u C:%u",
                  cpu->flags[FLAG_Z],
                  cpu->flags[FLAG_L],
                  cpu->flags[FLAG_E],
                  cpu->flags[FLAG_G],
                  cpu->flags[FLAG_C]
        );

        if (cpu->halted) {
            struct nk_color red = nk_rgb(255, 0, 0);
            nk_layout_row_dynamic(ctx, 20, 1);
            nk_label_colored(ctx, "HALTED", NK_TEXT_CENTERED, red);
        }
        const char* registerNames[] = {
                "r1",
                "r2",
                "r3",
                "r4",
                "r5",
                "r6",
                "r7",
                "r8",
                "ins",
                "adr",
                "pc",
                "bp",
                "sp",
        };

        const u32 registerValues[] = {
                cpu->regs[0],
                cpu->regs[1],
                cpu->regs[2],
                cpu->regs[3],
                cpu->regs[4],
                cpu->regs[5],
                cpu->regs[6],
                cpu->regs[7],
                cpu->ins_reg,
                cpu->adr,
                cpu->pc,
                cpu->bp,
                cpu->sp
        };



        // Static layout for two centered columns
        nk_layout_row_static(ctx, 20, 130, 2); // 130px width for each column, 2 columns
        for (int i = 0; i < 14; i += 2)
        {
            // Display two registers per row

            if(i+1 < 8){
                nk_labelf(ctx, NK_TEXT_CENTERED, "%s: 0x%X (%u)", registerNames[i], registerValues[i], registerValues[i]);
            }else{
                nk_labelf(ctx, NK_TEXT_CENTERED, "%s: 0x%X", registerNames[i], registerValues[i]);
            }
            if (i + 1 < 13)
            {
                if(i+1 < 8){
                    nk_labelf(ctx, NK_TEXT_CENTERED, "%s: 0x%X (%u)", registerNames[i+1], registerValues[i+1], registerValues[i+1]);
                }else{
                    nk_labelf(ctx, NK_TEXT_CENTERED, "%s: 0x%X", registerNames[i+1], registerValues[i+1]);

                }
            }
            else
            {
                nk_label(ctx, " ", NK_TEXT_CENTERED);
            }
        }
    }
    nk_end(ctx);

    static int visible_rows = 16;
    static int bytes_per_row = 8;
    static int scroll_position = 0x0000;


    static uint16_t mem_start = 0; // Start of the memory view

    if (nk_begin(ctx, "Memory View", nk_rect(320, 390, 870, 400), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 320, 1);
        if (nk_group_begin(ctx, "MemoryGroup", NK_WINDOW_BORDER))
        {
            for (int i = mem_start; i < mem_start + (bytes_per_row * visible_rows); i += bytes_per_row)
            {
                nk_layout_row_static(ctx, 15, 750, 1);
                nk_labelf(ctx, NK_TEXT_LEFT, "0x%04X:              %08X %08X %08X %08X  %08X %08X %08X %08X",
                          i,
                          cpu->program[i], cpu->program[i + 1], cpu->program[i + 2], cpu->program[i + 3],
                              cpu->program[i + 4], cpu->program[i + 5], cpu->program[i + 6], cpu->program[i + 7]);


                if(i / bytes_per_row == cpu->pc / bytes_per_row) {
                    struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
                    struct nk_rect rect = nk_layout_widget_bounds(ctx);
                    rect.w=60;
                    //rect.x += 145;
                    rect.x += 145;
                    int n = cpu->pc % bytes_per_row;
                    if(n < 4){
                        rect.x += 63 * n;
                    }else{
                        rect.x += 70 + 63 * (n-1);
                    }
                    nk_fill_rect(canvas, rect, 2, nk_rgba(60, 20, 220, 35));
                }
                if(i / bytes_per_row == cpu->adr / bytes_per_row) {
                    struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);
                    struct nk_rect rect = nk_layout_widget_bounds(ctx);
                    rect.w=60;
                    //rect.x += 145;
                    rect.x += 145;
                    int n = cpu->adr % bytes_per_row;
                    if(n < 4){
                        rect.x += 63 * n;
                    }else{
                        rect.x += 70 + 63 * (n-1);
                    }
                    nk_fill_rect(canvas, rect, 2, nk_rgba(235, 204, 52, 35));
                }

            }
            nk_group_end(ctx);
        }

        nk_layout_row_dynamic(ctx, 25, 2);
        if (nk_button_label(ctx, "Scroll Up")) mem_start -= (bytes_per_row * visible_rows);
        if (nk_button_label(ctx, "Scroll Down")) mem_start += (bytes_per_row * visible_rows);
    }
    nk_end(ctx);

    if (nk_begin(ctx, "Controls", nk_rect(10, 340, 300, 140), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 25, 3);
        if (nk_button_label(ctx, "Step"))
        {
            if(onClock != NULL) onClock();
            mem_start = (cpu->adr / (bytes_per_row * visible_rows))*(bytes_per_row * visible_rows);
        }
        if (nk_button_label(ctx, "Run"))
        {
            if(onPlay != NULL) onPlay();
        }
        if (nk_button_label(ctx, "Pause"))
        {
            if(onPause != NULL) onPause();
        }

        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_button_label(ctx, "Reset"))
        {
            if(onReset != NULL) onReset();
        }

        nk_layout_row_dynamic(ctx, 25, 1);

        nk_slider_float(ctx, 0.0f, &slider_value, 1500.0f, 0.25f); // Slider with steps of 0.25

    }
    nk_end(ctx);

    const char* b = "";

    if (nk_begin(ctx, "Keyboard Output", nk_rect(320, 10, 870, 370), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 100, 1);
        nk_label_wrap(ctx, cpu->output);
    }
    nk_end(ctx);

    
    if (nk_begin(ctx, "Instruction Viewer", nk_rect(10, 490, 300, 300), NK_WINDOW_BORDER | NK_WINDOW_TITLE))
    {
        nk_layout_row_dynamic(ctx, 25, 1);
        char name[50];
        snprintf(name, sizeof(name), "Instructions (step %d)", cpu->mc_counter);
        nk_label(ctx, name, NK_TEXT_CENTERED);

        nk_layout_row_dynamic(ctx, 20, 1);

        int current_address = cpu->pc;
        int start_address = current_address - 4;
        int end_address = current_address + 4;

        for (int addr = start_address; addr <= end_address; addr++)
        {
            if (addr < 0 || addr >= 0x10000)
            { // Out of bounds, skip
                nk_labelf(ctx, NK_TEXT_LEFT, "0x%04X: -- -- --", addr);
                continue;
            }

            uint32_t ins = cpu->program[addr];

            uint8_t opcode = (ins >> 24) & 0xFF;
            uint8_t a = (ins >> 20) & 0xF;
            uint8_t b = (ins >> 16) & 0xF;
            uint8_t lit = ins & 0xFFFF;

            if (addr == current_address)
            {
                nk_labelf(ctx, NK_TEXT_LEFT, "** 0x%04X: %02X %01X %01X %04X **", addr, opcode, a, b, lit);
                struct nk_command_buffer* canvas = nk_window_get_canvas(ctx);
                struct nk_rect rect = nk_layout_widget_bounds(ctx);
                nk_fill_rect(canvas, rect, 0, nk_rgba(60, 20, 220, 35)); // Dark blue background
            }
            else
            {
                nk_labelf(ctx, NK_TEXT_LEFT, "0x%04X: %02X %01X %01X %04X", addr, opcode, a, b, lit);
            }
        }
    }
    nk_end(ctx);
}

void endFrame() {
    SDL_SetRenderDrawColor(renderer, bg.r * 255, bg.g * 255, bg.b * 255, bg.a * 255);
    SDL_RenderClear(renderer);

    nk_sdl_render(NK_ANTI_ALIASING_ON);

    SDL_RenderPresent(renderer);
}

float getSliderPosition() {
    return slider_value;
}
