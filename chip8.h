#include <fstream>
#include <string>
#include <iostream>
#include <SDL.h>
#include <stdlib.h>
#include <time.h>

//macros for extracting nibbles from 4 digit hex numbers
#define FIRST_NIBBLE(instr) (instr >> 12)
#define SECOND_NIBBLE(instr) ((instr & 0x0F00) >> 8)
#define THIRD_NIBBLE(instr) ((instr & 0x00F0) >> 4)
#define FOURTH_NIBBLE(instr) (instr & 0x000F)
#define NNN(instr) (instr & 0x0FFF)
#define NN(instr, n) ((instr >> n) & 0x00FF)

//calculate pixel array index based on pixel coordinates
#define pixel_index(X, x_axis, Y, y_axis) (X+x_axis+(Y+y_axis)*64)

//4kb of memory
extern unsigned char storage[4096];

//program counter
extern unsigned char* pc;

//registers-------------
//16x8bit data registers
extern unsigned char registers[16];
//1x16bit address register (12bit used)
extern unsigned short I;
//2x8bit timer registers (decrementing at 60Hz)
extern unsigned char delay_timer;
extern unsigned char sound_timer;		//beep if >1

//stack
extern unsigned char* stack[16];
//stack counter
extern unsigned char sc;

//display variables
constexpr int window_width = 1280;
constexpr int window_height = 640;
constexpr int pixel_size = 10;
extern bool displayDraw;

//pixel buffer
extern unsigned char display_buffer[2048];

namespace Chip8 {
	//initialize 
	void init();
	//load program into memory
	void loadFile(const std::string& f); 
	//fetch instruction from memory
	unsigned short instructionFetch();
	//decode and execute
	void decodeAndExecute(const unsigned short& instr);
}

namespace Display {
	//update the dispay texture
	void update(SDL_Texture*);
	//redraw the display texture to the window
	void draw(SDL_Renderer*, SDL_Texture*);
	//DXYN - display sprite
	void drawSprite(const unsigned short&);
}
