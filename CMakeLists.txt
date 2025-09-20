cmake_minimum_required(VERSION 3.28)
project(CPU32Emulator C)

set(CMAKE_C_STANDARD 11)

add_executable(CPU32Emulator main.c
        ui.h
        ui.c
        cpu.h)

target_link_directories(CPU32Emulator
        PRIVATE ${PROJECT_SOURCE_DIR}/lib/SDL2-2.30.6/lib/x64)

target_link_libraries(CPU32Emulator
        ${PROJECT_SOURCE_DIR}/lib/SDL2-2.30.6/lib/x64/SDL2.lib
        ${PROJECT_SOURCE_DIR}/lib/SDL2-2.30.6/lib/x64/SDL2main.lib)

target_include_directories(CPU32Emulator
        PRIVATE
        ${PROJECT_SOURCE_DIR}/lib/Nuklear
        ${PROJECT_SOURCE_DIR}/lib/SDL2-2.30.6/include
        ${PROJECT_SOURCE_DIR}/lib/SDL2_image-2.8.2/include
)