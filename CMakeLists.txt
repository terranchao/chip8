cmake_minimum_required(VERSION 3.22.1)

project(chip8 LANGUAGES C)

add_compile_definitions(CHIP8)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

add_executable(chip8
    chip8.c
    io.c
    load.c
    main.c
)

target_link_libraries(chip8
    -lSDL2
)

target_compile_options(chip8
PRIVATE
    -fstack-protector-all
    -Wall
    -Wextra
    -Wpedantic
)
