#include "chip8.h"

//4 Kb of memory
unsigned char storage[4096];

//program counter - starts at memory location 0x200 (512 decimal)
unsigned char* pc = &storage[0x200];

//registers
//16x8bit data registers
unsigned char registers[16];
//1x16bit addres register (12bit used)
unsigned short I = 0;
//timer registers (decrementing at 60Hz)
unsigned char delay_timer = 0;
unsigned char sound_timer = 0;		//beep if >1

//stack (16x entries)
unsigned char* stack[16] = { nullptr, nullptr, nullptr, nullptr,
				nullptr, nullptr, nullptr, nullptr,
				nullptr, nullptr, nullptr, nullptr,
				nullptr, nullptr, nullptr, nullptr };
//stack counter
unsigned char sc = 0;

//display variables
bool displayDraw = true;

//pixel buffer
unsigned char display_buffer[2048];

//---Chip8-----------------------------------------------------------------

void Chip8::init()
{
	//initialize registers
	for(int i=0; i < 16; ++i) registers[i] = 0;

	//initialize stack
	for (int i=0; i < 16; ++i) stack[i] = 0;

	//initialize display buffer
	for (int i = 0; i < 2048; ++i) display_buffer[i] = 0;
}

void Chip8::loadFile(const std::string& f)
{
	std::ifstream ifs {f,std::ios_base::binary};
	if(!ifs) {
		std::cout << "Error: can't open file " << f << '\n';
		return;
	}
	//point program counter to memory location 0x200
	pc = &storage[0x200];
	//load bytes from file to memory adress 0x200 to 0xEAO
	while(ifs && pc < &storage[0xEA0]) {
		ifs.read(reinterpret_cast<char*>(pc),1);
		++pc;
	}
	pc = &storage[0x200];
}

unsigned short Chip8::instructionFetch()
{
	//read two bytes from memory
	unsigned char byte_1 = *pc;
	unsigned char byte_2 = pc[1];
	//increment program counter by two
	pc+=2;
	//combine 2 bytes into one 16 bit instruction
	unsigned short instruction = (byte_1 << 8) | byte_2;	
	//std::cout << "Fetched instruction " << std::hex << instruction 
	//	<< std::noshowbase << '\n';
	//std::cout.unsetf(std::ios::hex);
	return instruction;
}

void Chip8::decodeAndExecute(const unsigned short& instr)
{
	switch(FIRST_NIBBLE(instr)) {
	case 0x0:
		switch(FOURTH_NIBBLE(instr)) {
		//00EE - return from subroutine
		case 0xE: {
			//pop last address from the stack and set PC to it
			pc = stack[sc-1];
			stack[sc-1] = nullptr;
			--sc;
			break;	
		}
		//00E0 - Clear screen
		case 0x0: {
			for (int i = 0; i < 2048; ++i) display_buffer[i] = 0;
			displayDraw = true;
			break;
		}
		}
		break;
	//1NNN - Jump
	case 0x1:
		pc = &storage[NNN(instr)];
		break;
	//2NNN - call subroutine
	case 0x2:
		//push current PC to the stack for the subroutine to return to 
		stack[sc] = pc;
		//set PC to NNN
		pc = &storage[NNN(instr)];
		//increase stack pointer
		++sc;
		break;
	//3XNN - skip one instruction if VX is equal to NN
	case 0x3:
		if (registers[SECOND_NIBBLE(instr)] == NN(instr,0))
			pc+=2;
		break;
	//4XNN - skip one instruction if VX is not equal to NN
	case 0x4:
		if (registers[SECOND_NIBBLE(instr)] != NN(instr,0))
			pc+=2;
		break;
	//5XY0 - skip one instruction if VX == VY
	case 0x5:
		if (registers[SECOND_NIBBLE(instr)] == registers[THIRD_NIBBLE(instr)])
			pc+=2;
		break;
	//6XNN - Set
	case 0x6:
		registers[SECOND_NIBBLE(instr)] = NN(instr,0);
		//std::cout << SECOND_NIBBLE(instr) << " <- " << NN(instr,0) << '\n';
		break;
	//7XNN - Add
	case 0x7:
		registers[SECOND_NIBBLE(instr)] += NN(instr,0);
		break;
	//logical and arithmetic instructions
	case 0x8:
		switch (FOURTH_NIBBLE(instr)) {
		//8XY0 - VX is set to the value of VY
		case 0x0:
			registers[SECOND_NIBBLE(instr)] = registers[THIRD_NIBBLE(instr)];
			break;
		//8XY1 - VX is set to VX OR VY
		case 0x1:
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] | registers[THIRD_NIBBLE(instr)];
			break;
		//8XY2 - VX is set to VX AND VY
		case 0x2:
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] & registers[THIRD_NIBBLE(instr)];
			break;
		//8XY3 - VX is set to VX XOR VY
		case 0x3:
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] ^ registers[THIRD_NIBBLE(instr)];
			break;
		//8XY4 - VX is set to VX + VY (VF = 1 if overflow)
		case 0x4:
			if (registers[SECOND_NIBBLE(instr)] + registers[THIRD_NIBBLE(instr)] > 0xFF)
				registers[0xF] = 1;
			else
				registers[0xF] = 0;
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] + registers[THIRD_NIBBLE(instr)];
			break;
		//8XY5 - VX is set to VX - VY
		case 0x5:
			if (registers[SECOND_NIBBLE(instr)] >= registers[THIRD_NIBBLE(instr)])
				registers[0xF] = 1;
			else if (registers[SECOND_NIBBLE(instr)] < registers[THIRD_NIBBLE(instr)])
				registers[0xF] = 0;
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] - registers[THIRD_NIBBLE(instr)];
			break;
		//8XY6 - load VY in to VX and shift 1 to the right (VF = shifted out bit)
		case 0x6:
			registers[SECOND_NIBBLE(instr)] = registers[THIRD_NIBBLE(instr)] >> 1;
			registers[0xF] = registers[THIRD_NIBBLE(instr)] & 0b00000001;
			break;
		//8XY7 - VX is set to VY - VX
		case 0x7:
			if (registers[THIRD_NIBBLE(instr)] >= registers[SECOND_NIBBLE(instr)])
				registers[0xF] = 1;
			else if (registers[THIRD_NIBBLE(instr)] < registers[SECOND_NIBBLE(instr)])
				registers[0xF] = 0;
			registers[SECOND_NIBBLE(instr)] = 
				registers[THIRD_NIBBLE(instr)] - registers[SECOND_NIBBLE(instr)];
			break;
		//8XY6 - load VY in to VX and shift 1 to the left (VF = shifted out bit)
		case 0xE:
			registers[SECOND_NIBBLE(instr)] = registers[THIRD_NIBBLE(instr)] << 1;
			registers[0xF] = registers[THIRD_NIBBLE(instr)] & 0b00000001;
			break;
		}
		break;
	//9XY0 - skip one instruction if VX != VY
	case 0x9:
		if (registers[SECOND_NIBBLE(instr)] != registers[THIRD_NIBBLE(instr)])
			pc+=2;
		break;
	//ANNN - Set index
	case 0xA:
		I = NNN(instr);
		break;
	case 0xB:
		break;
	case 0xC:
		break;
	//DXYN - Display sprite
	case 0xD:
		Display::drawSprite(instr);
		break;
	case 0xE:
		break;
	case 0xF:
		break;
	default:
		std::cout << "Error: couldn't decode instruction" << '\n';
		break;
	}
}

//---Display---------------------------------------------------------------

//update the display texture
void Display::update(SDL_Texture* texture)
{
	uint32_t pixels[2048];
	for (int i = 0; i < 2048; i++) 
		pixels[i] = display_buffer[i] ? 0xFFFFFFFF : 0x00000000;
	SDL_UpdateTexture(texture,NULL,pixels,64 * sizeof(uint32_t));
}

//redraw the display texture to the window
void Display::draw(SDL_Renderer* renderer, SDL_Texture* texture)
{
	//all rendering operations will be performed on buffer texture
	SDL_SetRenderTarget(renderer,texture);
	Display::update(texture);
	SDL_SetRenderTarget(renderer,NULL);
	//copy the texture to the window
	SDL_RenderCopy(renderer,texture,NULL,NULL);
	//update the window with the latest rendering operations
	SDL_RenderPresent(renderer);
}

//DXYN - display sprite
void Display::drawSprite(const unsigned short& instr)
{
	//location stored in registers specified by X,Y
	unsigned char X = registers[SECOND_NIBBLE(instr)] % 64;
	unsigned char Y = registers[THIRD_NIBBLE(instr)] % 32;
	//sprite starts at memory address stored in I register
	unsigned char* sprite = &storage[I];
	//set flag register to 0
	registers[0xF] = 0;
	//for every row of sprite
	for (int y_axis = 0; y_axis < FOURTH_NIBBLE(instr); ++y_axis) {
		//for every bit of row
		for (int x_axis = 0; x_axis < 8; ++x_axis) {
			if (pixel_index(X, x_axis, Y, y_axis) < 2048 &&
			((X+x_axis) < (64+64*Y+y_axis))) {
				bool current_bit = (*sprite >> 7-x_axis) & 0b00000001;
				//if both current bit and pixel on the screen are ON
				if (display_buffer[pixel_index(X, x_axis, Y, y_axis)]
				&& current_bit) {
					registers[0xF] = 1;
				}
				//XOR bit with currently drawn pixel
				display_buffer[pixel_index(X, x_axis, Y, y_axis)]
					^= current_bit;
			} 
		}
		++sprite;
	}
	displayDraw = true;
}
