# :space_invader: Chip-8-Emu :space_invader:
## Simple CHIP-8 emulator written in C++

### Usage
Run:

`./chip8 [filename] [-d/--debug] [-r[width]]`

First argument always has to be a file path/name. Optional arguments are:

`-d / --debug`

Debug mode: execute instructions step by step by pressing right arrow. You can also output registers values by pressing right control.

`-r[width]`

Choose one of the custom resolutions: **640**x320, **1280**x640, **1920**x960 and **2560**x1280. _(default: 1280x640)_

### Prerequisites
-[SDL 2](https://www.libsdl.org/) library

-make

### Setup
Clone repo:

`git clone https://github.com/kajtekajtek/Chip-8-Emu.git`

Compile:

`make` _(Unix)_

### Additional resources
https://tobiasvl.github.io/blog/write-a-chip-8-emulator/

https://github.com/mattmikolay/chip-8/wiki/CHIP%E2%80%908-Technical-Reference

https://github.com/Timendus/chip8-test-suite

![ibm](https://github.com/kajtekajtek/Chip-8-Emu/assets/129865118/34501ecf-7125-46e6-8a20-6e57038a0c21)

![corax](https://github.com/kajtekajtek/Chip-8-Emu/assets/129865118/5f2df3e7-ce6d-44ac-aeb7-3ad698069ac8)

![tetris](https://github.com/kajtekajtek/Chip-8-Emu/assets/129865118/9fafabda-f738-42a9-b423-80a415525514)
