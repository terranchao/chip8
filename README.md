
# A CHIP-8 Interpreter

## Introduction

[CHIP_8](https://en.wikipedia.org/wiki/CHIP-8) is a *low-level programming
language* that was developed back in the 1970s. It was first used as a game
development medium for microcomputers, but it remains popular with programmer
hobbyists and retro-game enthusiasts to this day.

A CHIP-8 program must run on a software platform that is capable of interpreting
the CHIP-8 programming language, which comes in the form of a simple instruction
set. Such a platform could be thought of as an "emulator", but since an emulator
is technically meant to mimic a specific hardware model — and since CHIP-8 does
not describe any physical device — let us refer to this platform as a **virtual
machine** or simply an **interpreter**, which is what this project implements.

## Environment

- This project was written in C and is built using CMake.

- This project was developed on
[WSL (Windows Subsystem for Linux)](https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux)
Version 2. Here is some version information from the time of this writing:

    ```bash
    $ wsl.exe -l -v
      NAME            STATE           VERSION
    * Ubuntu-22.04    Running         2
    $
    $ wsl.exe --version
    WSL version: 1.1.6.0
    Kernel version: 5.15.90.1
    WSLg version: 1.0.50
    MSRDC version: 1.2.3770
    Direct3D version: 1.608.2-61064218
    DXCore version: 10.0.25131.1002-220531-1700.rs-onecore-base2-hyp
    Windows version: 10.0.19045.2728
    $
    $ uname -a
    Linux 5.15.90.1-microsoft-standard-WSL2 #1 SMP Fri Jan 27 02:56:13 UTC 2023 x86_64 x86_64 x86_64 GNU/Linux
    ```

- User I/O is handled by the
[SDL development library](https://www.libsdl.org/). Newer versions of WSL
automatically come with [WSLg](https://github.com/microsoft/wslg),
which allows GUI applications in WSL (like those based on SDL) to run directly
on Windows, no X server required.

## Usage

**Note:** At the moment, these instructions are catered to Linux/WSL users. WSL
users must make sure their version of WSL is new enough to support WSLg
features.

1. Install dependencies

    On Debian systems:
    ```bash
    sudo apt install build-essential cmake libsdl2-dev
    ```

2. Download

    ```bash
    git clone git@github.com:terranchao/chip8.git
    ```

3. Build

    ```bash
    cd chip8

    # CMake sequence
    mkdir build     && \
    pushd build     && \
        cmake ..    && \
        make        && \
        cp chip8 .. && \
    popd            && \
    rm -rf build
    ```

4. Run

    ```
    $ ./chip8

    [USAGE] ./chip8 scale delay ROM

    Arguments:
        scale - window size multiplier (range: [1, 120])
        delay - instruction cycle delay amount (range: [0, 10])
        ROM   - CHIP-8 program filename

    The recommended strategy for finding the right `scale` and `delay` values is
    to start them small, and slowly increase them until "it feels right".
    A `scale` in the range of [10, 30] should do well for most users.
    The `delay` argument is to be used for tweaking the animation speed of a
    CHIP-8 program. Without an added delay, most modern processors will cause
    programs to run too quickly.

    ```

    There is a known WSLg-SDL compatibility
    [issue](https://github.com/microsoft/wslg/issues/715) that causes a
    segmentation fault. If a segmentation fault occurs, try exporting the
    following environment variable and retrying. This must be done once per
    shell instance.
    ```bash
    $ export LIBGL_ALWAYS_SOFTWARE=1
    ```

5. Input

    Once the application is up and running, the user is able to provide keyboard
    input to the interpreter using the following virtual key mapping:
    ```
    QWERTY      ->      CHIP-8
    
    1 2 3 4             1 2 3 C
    Q W E R             4 5 6 D
    A S D F             7 8 9 E
    Z X C V             A 0 B F
    ```

## Testing

### ROMs

https://github.com/Timendus/chip8-test-suite

This interpreter follows as closely as possible the "quirks" of the original
CHIP-8 variant that ran on the
[COSMAC VIP](https://en.wikipedia.org/wiki/COSMAC_VIP).

### Games

### Unit testing

TODO

## References

Many excellent guides were referenced for conceptual accuracy:

- https://austinmorlan.com/posts/chip8_emulator/
- http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
- https://github.com/mattmikolay/chip-8/wiki
- https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
- https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
- https://laurencescotford.com/chip-8-on-the-cosmac-vip-index/
- https://chip-8.github.io/links/
