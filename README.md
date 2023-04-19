
# A CHIP-8 Interpreter

## Introduction

[CHIP-8](https://en.wikipedia.org/wiki/CHIP-8) is a low-level programming
language that was developed back in the 1970s. It was first used as a game
development medium for the microcomputers of its day, but it remains popular
with programmer hobbyists and retro-game enthusiasts to this day.

A CHIP-8 program must run on a software platform that is capable of interpreting
the CHIP-8 programming language, which comes in the form of a simple instruction
set. Such a platform could be thought of as an "emulator", but since an emulator
is technically meant to mimic a specific hardware model — and since CHIP-8 does
not describe any physical device — let us refer to this platform as a **virtual
machine** or simply an **interpreter**, which is what this project implements.

This CHIP-8 interpreter aims to follow the original conventions of the language,
used by CHIP-8 programs that ran on the
[COSMAC VIP](https://en.wikipedia.org/wiki/COSMAC_VIP).

## Features

### Display and Sound

Demo: Breakout (1997)

https://user-images.githubusercontent.com/5587757/232962506-a63e0332-f7ac-4fa0-bd85-3aaa520bdab4.mp4

### Color customization (Optional)

Before the application window opens, the user has a chance to customize the
display with their own color codes in the command line.

Example: McDonald's

```
Enter 'y' for colors: y
Background > da291c
Background color set to #DA291C
Foreground > ffcc00
Foreground color set to #FFCC00
```

### User Input

The user is able to provide keyboard input to the interpreter using a virtual
key mapping:

```
QWERTY      ->      CHIP-8

1 2 3 4             1 2 3 C
Q W E R             4 5 6 D
A S D F             7 8 9 E
Z X C V             A 0 B F
```

### Additional User Interface

- <kbd>-</kbd>/<kbd>=</kbd> - Change window size
- <kbd>Esc</kbd> - Quit interpreter
- <kbd>Space</kbd> - Pause program
- <kbd>Backspace</kbd> - Restart program (request is toggleable during pause)

### Multithreading

The decision for multithreading is in an attempt to simulate operation that is
closer in concept to that of the COSMAC VIP.

- The COSMAC VIP comes equipped with a
[physical keypad](https://laurencescotford.com/chip-8-on-the-cosmac-vip-keyboard-input/),
so the interpreter runs a dedicated I/O thread that is able to detect user
input outside of the main program thread, just as a keyboard is able to do.
- The COSMAC VIP's
[interrupt system](https://laurencescotford.com/chip-8-on-the-cosmac-vip-interrupts/)
is responsible for refreshing the display and decrementing the system timers at
a rate of 60Hz. The interpreter implements this system by spawning a dedicated
timer thread that performs these tasks at the required frequency with precision,
also separate from the main program thread.

## Development Notes

- Written in C, built with CMake (GCC)

- Developed on
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

1. **Install dependencies**

    On Debian systems
    ```bash
    sudo apt update
    sudo apt install build-essential cmake libsdl2-dev
    ```

2. **Download**

    ```bash
    git clone https://github.com/terranchao/chip8.git
    ```

3. **Build**

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

4. **Run**

    The user should provide a `ROM` argument to the interpreter. This argument
    should be the filename of a valid CHIP-8 program file. CHIP-8 program files
    come in binary format, and usually have designated .ch8 file extensions.
    ```bash
    ./chip8 ROM
    ```

    There is a known WSLg-SDL compatibility
    [issue](https://github.com/microsoft/wslg/issues/715) that causes a
    segmentation fault. If a segmentation fault occurs, try running the
    following and retrying. This must be done once per shell instance.
    ```bash
    $ export LIBGL_ALWAYS_SOFTWARE=1
    ```

## Testing

### ROMs

[<img src="https://user-images.githubusercontent.com/5587757/232945003-606aac05-af2a-4eef-9443-4f2ca1690981.png"/>](https://github.com/Timendus/chip8-test-suite)

### Games

https://github.com/JohnEarnest/chip8Archive/tree/master/roms

### Unit testing

TODO

## References

Many excellent guides were referenced for this project:

- https://austinmorlan.com/posts/chip8_emulator/
- http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
- https://github.com/mattmikolay/chip-8/wiki
- https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/
- https://tobiasvl.github.io/blog/write-a-chip-8-emulator/
- https://laurencescotford.com/chip-8-on-the-cosmac-vip-index/
- https://chip-8.github.io/links/
