#include <fstream>
#include <iomanip>
#include <string>
#include <iostream>
#include <SDL.h>
#include <stdlib.h>
#include <time.h>
#include <stdexcept>
#include <vector>

// macros for extracting nibbles from 4 digit hex numbers
#define FIRST_NIBBLE(instr) (instr >> 12)
#define SECOND_NIBBLE(instr) ((instr & 0x0F00) >> 8)
#define THIRD_NIBBLE(instr) ((instr & 0x00F0) >> 4)
#define FOURTH_NIBBLE(instr) (instr & 0x000F)
#define NNN(instr) (instr & 0x0FFF)
#define NN(instr, n) ((instr >> n) & 0x00FF)

// calculate pixel buffer index based on coordinates
#define PIXEL_INDEX(X, x_offset, Y, y_offset) (X+x_offset+(Y+y_offset)*64)

// key bindings
#define ESCAPE SDL_SCANCODE_ESCAPE
#define KEY_0 SDL_SCANCODE_X
#define KEY_1 SDL_SCANCODE_1
#define KEY_2 SDL_SCANCODE_2
#define KEY_3 SDL_SCANCODE_3
#define KEY_4 SDL_SCANCODE_Q
#define KEY_5 SDL_SCANCODE_W
#define KEY_6 SDL_SCANCODE_E
#define KEY_7 SDL_SCANCODE_A
#define KEY_8 SDL_SCANCODE_S
#define KEY_9 SDL_SCANCODE_D
#define KEY_A SDL_SCANCODE_Z
#define KEY_B SDL_SCANCODE_C
#define KEY_C SDL_SCANCODE_4
#define KEY_D SDL_SCANCODE_R
#define KEY_E SDL_SCANCODE_F
#define KEY_F SDL_SCANCODE_V

// debug mode options
#define ADVANCE 0b00000001
#define SHOW_REGISTERS 0b00000010

extern bool isRunning;

// 4kb of memory
extern unsigned char storage[4096];

// program counter
extern unsigned char* pc;

// 16x8bit data registers
extern unsigned char registers[16];
// 1x16bit address register (12bit used)
extern unsigned short I;
// 2x8bit timer registers (decrementing at 60Hz)
extern unsigned char delay_timer;
extern unsigned char sound_timer;		//beep if >1

// stack
extern unsigned char* stack[16];
// stack counter
extern unsigned char sc;

// font
extern unsigned char font[90];

namespace Chip8 {
	// initialize 
	void init();
	// load program into memory
	void loadFile(const std::string& f); 
	// fetch instruction from memory
	unsigned short instructionFetch();
	// decode and execute
	void decodeAndExecute(const unsigned short& instr);
	// main program loop
	void loop();
	// debug mode program loop
	void loopDebug(uint8_t&);
	// increment program counter
	void incrementPC(const int&);
	// callback function for SDL_AddTimer() (decrement timer registers by 1 every 17ms(60Hz))
	uint32_t timerCallback(uint32_t, void*);
}

namespace Display {
	// display constants
	extern int width;
	extern int height;
	constexpr int pixel_size = 10;

	// flag indicating wheter display should be redrawn before next instruction
	extern bool redraw;

	// SDL variables
	extern SDL_Window* window;
	extern SDL_Renderer* renderer;
	extern SDL_Texture* texture;

	// pixel buffer
	extern unsigned char buffer[2048];

	// update the dispay texture
	void update(SDL_Texture*);
	// redraw the display texture to the display
	void draw(SDL_Renderer*, SDL_Texture*);
	// DXYN - draw sprite
	void drawSprite(const unsigned short&);
}

namespace Keyboard {
	// convert hexadecimal CHIP-8 keyboard digit to SDL keyboard input scancode values
	constexpr unsigned char scancodes[16] = { KEY_0, KEY_1, KEY_2, KEY_3,
						   KEY_4, KEY_5, KEY_6, KEY_7,
						   KEY_8, KEY_9, KEY_A, KEY_B,
					           KEY_C, KEY_D, KEY_E, KEY_F };
	// array of states of keys (pressed or not)
	extern bool state[322];
}

namespace Options {
	// parse command line arguments
	void parse(int argc, char* argv[]);
	// debug mode flag
	extern bool debug;
	// path to the file to open
	extern std::string filename;
}
