#include <fstream>
#include <string>
#include <iostream>
#include <SDL.h>
#include <stdlib.h>
#include <time.h>
#include <stdexcept>
#include <vector>

///macros for extracting nibbles from 4 digit hex numbers
#define FIRST_NIBBLE(instr) (instr >> 12)
#define SECOND_NIBBLE(instr) ((instr & 0x0F00) >> 8)
#define THIRD_NIBBLE(instr) ((instr & 0x00F0) >> 4)
#define FOURTH_NIBBLE(instr) (instr & 0x000F)
#define NNN(instr) (instr & 0x0FFF)
#define NN(instr, n) ((instr >> n) & 0x00FF)

//calculate pixel array index based on pixel coordinates
#define pixel_index(X, x_axis, Y, y_axis) (X+x_axis+(Y+y_axis)*64)

//key bindings
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

extern bool isRunning;

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

//display variables (przeniesc do namespaceu i osobnego pliku)
constexpr int window_width = 1280;
constexpr int window_height = 640;
constexpr int pixel_size = 10;
extern bool displayDraw;
extern SDL_Window* window;
extern SDL_Renderer* renderer;
extern SDL_Texture* texture;
//pixel buffer
extern unsigned char display_buffer[2048];

//font
extern unsigned char font[90];

namespace Chip8 {
	//initialize 
	void init();
	//load program into memory
	void loadFile(const std::string& f); 
	//fetch instruction from memory
	unsigned short instructionFetch();
	//decode and execute
	void decodeAndExecute(const unsigned short& instr);
	//main program loop
	void loop();
	//callback function for SDL_AddTimer() (decrement timer registers by 1 every 17ms(60Hz))
	uint32_t timerCallback(uint32_t, void*);
}

namespace Display {
	//update the dispay texture
	void update(SDL_Texture*);
	//redraw the display texture to the window
	void draw(SDL_Renderer*, SDL_Texture*);
	//DXYN - display sprite
	void drawSprite(const unsigned short&);
}

namespace Keyboard {
	//convert hexadecimal CHIP-8 keyboard digit to SDL keyboard input scancode values
	constexpr unsigned char scancodes[16] = { KEY_0, KEY_1, KEY_2, KEY_3,
						   KEY_4, KEY_5, KEY_6, KEY_7,
						   KEY_8, KEY_9, KEY_A, KEY_B,
					           KEY_C, KEY_D, KEY_E, KEY_F };
	//array of states of keys (currently pressed or not)
	extern bool state[322];
}

namespace Options {
	//parse command line arguments
	void parse(int argc, char* argv[]);
	//debug mode
	extern bool debug;
	//file to open
	extern std::string filename;
}
