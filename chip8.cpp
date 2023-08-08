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
unsigned char* stack[16];
//stack counter
unsigned char sc = 0;

//display variables
bool displayDraw = true;

//pixel buffer
unsigned char display_buffer[2048];

//font
unsigned char font[90] ={ 0xF0, 0x90, 0x90, 0x90, 0xF0,   // 0
		          0x20, 0x60, 0x20, 0x20, 0x70,   // 1
		          0xF0, 0x10, 0xF0, 0x80, 0xF0,   // 2
			  0xF0, 0x10, 0xF0, 0x10, 0xF0,   // 3
			  0x90, 0x90, 0xF0, 0x10, 0x10,   // 4
			  0xF0, 0x80, 0xF0, 0x10, 0xF0,   // 5
			  0xF0, 0x80, 0xF0, 0x90, 0xF0,   // 6
			  0xF0, 0x10, 0x20, 0x40, 0x40,   // 7
			  0xF0, 0x90, 0xF0, 0x90, 0xF0,   // 8
			  0xF0, 0x90, 0xF0, 0x10, 0xF0,   // 9
			  0xF0, 0x90, 0xF0, 0x90, 0x90,   // A
			  0xE0, 0x90, 0xE0, 0x90, 0xE0,   // B
			  0xF0, 0x80, 0x80, 0x80, 0xF0,   // C
			  0xE0, 0x90, 0x90, 0x90, 0xE0,   // D
			  0xF0, 0x80, 0xF0, 0x80, 0xF0,   // E
			  0xF0, 0x80, 0xF0, 0x80, 0x80 }; // F

//---Chip8-----------------------------------------------------------------

void Chip8::init()
{
	//initialize registers
	for(int i=0; i < 16; ++i) registers[i] = 0;

	//initialize stack
	for (int i=0; i < 16; ++i) stack[i] = nullptr;

	//initialize display buffer
	for (int i = 0; i < 2048; ++i) display_buffer[i] = 0;

	//initialize key states
	for (int i = 0; i < 322; ++i) Keyboard::state[i] = 0;

	//load font into memory
	for (int i = 0; i < 90; ++i) storage[i] = font[i];

	//initialize SDL with its modules
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		std::cout << "Error: Couldnt initialize SDL" << '\n';
	}

	//create window, renderer and texture (display handling)
	window = SDL_CreateWindow("CHIP-8",SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		window_width,window_height,0);
	renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,64,32);
}

void Chip8::loadFile(const std::string& f)
{
	std::ifstream ifs {f,std::ios_base::binary};

	if(!ifs) { 
		throw std::runtime_error("Error: can't open file " + f + '\n');
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
			registers[0xF] = 0;
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] | registers[THIRD_NIBBLE(instr)];
			break;
		//8XY2 - VX is set to VX AND VY
		case 0x2:
			registers[0xF] = 0;
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] & registers[THIRD_NIBBLE(instr)];
			break;
		//8XY3 - VX is set to VX XOR VY
		case 0x3:
			registers[0xF] = 0;
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] ^ registers[THIRD_NIBBLE(instr)];
			break;
		//8XY4 - VX is set to VX + VY (VF = 1 if overflow)
		case 0x4: {
			bool overflow = false;
			if (registers[SECOND_NIBBLE(instr)] + registers[THIRD_NIBBLE(instr)] > 0xFF)
				overflow = true;
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] + registers[THIRD_NIBBLE(instr)];
			if (overflow) registers[0xF] = 1;
			else registers[0xF] = 0;
			break;
			 }
		//8XY5 - VX is set to VX - VY
		case 0x5: {
			bool underflow = false;
			if (registers[SECOND_NIBBLE(instr)] < registers[THIRD_NIBBLE(instr)])
				underflow = true;
			registers[SECOND_NIBBLE(instr)] -= registers[THIRD_NIBBLE(instr)];
			if (underflow) registers[0xF] = 0;
			else registers[0xF] = 1;
			break;
		}
		//8XY6 - load VY in to VX and shift 1 to the right (VF = shifted out bit)
		case 0x6: {
			bool shifted_out = registers[THIRD_NIBBLE(instr)] & 0b00000001;
			registers[SECOND_NIBBLE(instr)] = registers[THIRD_NIBBLE(instr)] >> 1;
			registers[0xF] = shifted_out;
			break;
		}
		//8XY7 - VX is set to VY - VX
		case 0x7:
			registers[SECOND_NIBBLE(instr)] = 
				registers[THIRD_NIBBLE(instr)] - registers[SECOND_NIBBLE(instr)];
			if (registers[THIRD_NIBBLE(instr)] >= registers[SECOND_NIBBLE(instr)])
				registers[0xF] = 1;
			else if (registers[THIRD_NIBBLE(instr)] < registers[SECOND_NIBBLE(instr)])
				registers[0xF] = 0;
			break;
		//8XYE - load VY in to VX and shift 1 to the left (VF = shifted out bit)
		case 0xE:
			bool shifted_out = registers[THIRD_NIBBLE(instr)] & 0b10000000;
			registers[SECOND_NIBBLE(instr)] = registers[THIRD_NIBBLE(instr)] << 1;
			registers[0xF] = shifted_out;
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
	//BNNN - Jump with offset
	case 0xB:
		pc = &storage[NNN(instr) + registers[0x0]];
		break;
	//CXNN - generate random number, AND it with NN and put the result in VX
	case 0xC:
		std::srand(time(NULL));
		registers[SECOND_NIBBLE(instr)] = (std::rand() % 0xFF) & NN(instr,0);
		break;
	//DXYN - Display sprite
	case 0xD:
		Display::drawSprite(instr);
		break;
	case 0xE:
		switch (NN(instr,0)) {
		//EX9E - skip next instruction if the key corresponding to value in VX is pressed
		case 0x9E:
			if (Keyboard::state[Keyboard::scancodes[registers[SECOND_NIBBLE(instr)]]])
				pc += 2;
			break;
		//EXA1 - skip next instruction if the key corresponding to value in VX is not pressed
		case 0xA1:
			if (!Keyboard::state[Keyboard::scancodes[registers[SECOND_NIBBLE(instr)]]])
				pc += 2;
			break;
		}
		break;
	case 0xF:
		switch (NN(instr,0)) {
		//FX07 - set VX to the current value of the delay timer
		case 0x07:
			registers[SECOND_NIBBLE(instr)] = delay_timer;
			break;
		//FX15 - set the delay timer to the value in VX
		case 0x15:
			delay_timer = registers[SECOND_NIBBLE(instr)];
			break;
		//FX18 - set the sound timer to the value in VX
		case 0x18:
			sound_timer = registers[SECOND_NIBBLE(instr)];
			break;
		//FX1E - add to index
		case 0x1E:
			//set VF to 1 if I overflows
			if (I + registers[SECOND_NIBBLE(instr)] > 0xFFF) registers[0xF] = 1;
			I += registers[SECOND_NIBBLE(instr)];;
			break;
		//FX0A - get key (wait for input;store hexadecimal value of pressed key in VX)
		case 0x0A: {
			SDL_Event event;
			bool key_released = false;
			while (!key_released) {
				while(SDL_PollEvent(&event)!=0) {
					if (event.type == SDL_KEYUP) {
						for (int i = 0; i < 16; ++i) {
						if(event.key.keysym.scancode
							== Keyboard::scancodes[i])
							registers[SECOND_NIBBLE(instr)] = i;
						}
						key_released = true;
					}
				}
			}	
			break;
			}
		//FX29 - font character (I is set to the addres of the hex character in VX)
		case 0x29:
			I = SECOND_NIBBLE(instr) * 5;
			break;
		//FX33 - binary-coded decimal conversion (convert the number
		//in VX to three decimal digits and store them at the I address)
		case 0x33:
			storage[I] = registers[SECOND_NIBBLE(instr)] / 100;
			storage[I+1] = (registers[SECOND_NIBBLE(instr)] % 100) / 10;
			storage[I+2] = registers[SECOND_NIBBLE(instr)] % 10;
			break;
		//ZDEBUGOWAC F6 I F5
		//FX55 - store register values in memory
		case 0x55:
			for (int i = 0; i <= SECOND_NIBBLE(instr); ++i)
				storage[I + i] = registers[i];
			//save and load opcodes increment index register as
			//original COSMAC VIP interpreter
			I += SECOND_NIBBLE(instr) + 1;
			break;
		//FX65 - load values from memory to registers
		case 0x65:
			for (int i = 0; i <= SECOND_NIBBLE(instr); ++i) 
				registers[i] = storage[I + i];
			//save and load opcodes increment index register as
			//original COSMAC VIP interpreter
			I += SECOND_NIBBLE(instr) + 1;
			break;
		}
		break;
	default:
		std::cout << "Error: couldn't decode instruction" << '\n';
		break;
	}
}

//main program loop
void Chip8::loop()
{
	//Fetch, decode and execute instructions
	Chip8::decodeAndExecute(Chip8::instructionFetch());
	//event check
	SDL_Event main_event;
	while(SDL_PollEvent(&main_event)!=0) {
		switch(main_event.type) {
		case SDL_KEYDOWN:
			if(main_event.key.keysym.scancode==ESCAPE)
				isRunning = false;
			Keyboard::state[main_event.key.keysym.scancode] = true;
			break;
		case SDL_KEYUP:
			Keyboard::state[main_event.key.keysym.scancode] = false;
			break;
		case SDL_QUIT:
			isRunning = false;
		}
	}

	//update display
	if(displayDraw) {
		Display::draw(renderer,texture);
		displayDraw = false;
	}

	SDL_Delay(1);
}

//callback function for SDL_AddTimer()(decrement timer registers by 1 every 17ms(60Hz))
uint32_t Chip8::timerCallback(uint32_t interval, void* param)
{
	//decrement timer registers
	if (delay_timer > 0) delay_timer -= 1;	
	if (sound_timer > 0) sound_timer -= 1;	
	//return next 17ms interval to timer
	return 17;
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
