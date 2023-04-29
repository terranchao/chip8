cmake_minimum_required(VERSION 3.22.1)

project(chip8 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_compile_definitions(COSMAC_VIP)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
endif()

file(GLOB SOURCES *.c)
list(FILTER SOURCES EXCLUDE REGEX .*unused.*)
add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME}
    -lSDL2
    -lm
    -lncurses
)

target_compile_options(${PROJECT_NAME}
PRIVATE
    -fstack-protector-all
    -Wall
    -Werror
    -Wextra
    -Wpedantic
)
