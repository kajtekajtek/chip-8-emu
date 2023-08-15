#include "chip8.h"

// 4 Kb of memory
unsigned char storage[4096];

// program counter - starts at memory location 0x200
unsigned char* pc = &storage[0x200];

// 16x8bit data registers
unsigned char registers[16];
// 1x16bit addres register (12bit used)
unsigned short I = 0;
// timer registers (decrementing at 60Hz)
unsigned char delay_timer = 0;
unsigned char sound_timer = 0;		//beep if >1

// stack (16x entries)
unsigned char* stack[16];
// stack counter
unsigned char sc = 0;

// font
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

// states of keys
bool Keyboard::state[322];

void Chip8::init()
{
	// initialize registers
	for(int i=0; i < 16; ++i) registers[i] = 0;

	// initialize stack
	for (int i=0; i < 16; ++i) stack[i] = nullptr;

	// initialize display buffer
	for (int i = 0; i < 2048; ++i) Display::buffer[i] = 0;

	// initialize key states
	for (int i = 0; i < 322; ++i) Keyboard::state[i] = 0;

	// load font into memory
	for (int i = 0; i < 90; ++i) storage[i] = font[i];

	// initialize SDL with its modules
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
		std::cout << "Error: Couldnt initialize SDL" << '\n';
	}

	// create window, renderer and texture (display handling)
	Display::window = SDL_CreateWindow("CHIP-8",SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		Display::width,Display::height,0);
	Display::renderer = SDL_CreateRenderer(Display::window,-1,SDL_RENDERER_ACCELERATED);
	Display::texture = SDL_CreateTexture(Display::renderer,SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,64,32);
}

void Chip8::loadFile(const std::string& f)
{
	std::ifstream ifs {f,std::ios_base::binary};

	if(!ifs) { 
		throw std::runtime_error("Error: can't open file " + f + '\n');
	}
	
	// point program counter to memory location 0x200
	pc = &storage[0x200];

	// load bytes from file to memory adress 0x200 to 0xEAO
	while(ifs && pc != &storage[0xEA0]) {
		ifs.read(reinterpret_cast<char*>(pc),1);
		++pc;
	}
	pc = &storage[0x200];
}

unsigned short Chip8::instructionFetch()
{
	// read two bytes from memory
	unsigned char byte_1 = *pc;
	unsigned char byte_2 = pc[1];

	// increment program counter by two
	Chip8::incrementPC(2);

	// combine 2 bytes into one 16 bit instruction
	unsigned short instruction = (byte_1 << 8) | byte_2;	

	return instruction;
}

// instructions decoding and execution
void Chip8::decodeAndExecute(const unsigned short& instr)
{
	switch(FIRST_NIBBLE(instr)) {
	case 0x0:
		switch(FOURTH_NIBBLE(instr)) {
		// 00EE - return from subroutine
		case 0xE: {
			if (!sc) 
				throw std::runtime_error
					("Error: decrementing empty stack\n");
			// set program counter to the last stack element
			pc = stack[sc-1];
			// "pop" last element of the stack
			stack[sc-1] = nullptr;
			// decrement stack counter
			--sc;
			break;	
		}
		// 00E0 - Clear screen
		case 0x0: {
			for (int i = 0; i < 2048; ++i) Display::buffer[i] = 0;
			break;
		}
		}
		break;
	// 1NNN - Jump
	case 0x1:
		pc = &storage[NNN(instr)];
		break;
	// 2NNN - call subroutine
	case 0x2:
		if (sc == 16)
			throw std::runtime_error("Error: stack overflow\n");
		// push current program counter on to the stack 
		stack[sc] = pc;
		//set program counter to NNN
		pc = &storage[NNN(instr)];
		//increment stack counter
		++sc;
		break;
	// 3XNN - skip one instruction if VX is equal to NN
	case 0x3:
		if (registers[SECOND_NIBBLE(instr)] == NN(instr,0))
			Chip8::incrementPC(2);
		break;
	// 4XNN - skip one instruction if VX is not equal to NN
	case 0x4:
		if (registers[SECOND_NIBBLE(instr)] != NN(instr,0)) 
			Chip8::incrementPC(2);
		break;
	// 5XY0 - skip one instruction if VX == VY
	case 0x5:
		if (registers[SECOND_NIBBLE(instr)] == registers[THIRD_NIBBLE(instr)]) 
			Chip8::incrementPC(2);
		break;
	// 6XNN - Set register VX value to NN 
	case 0x6:
		registers[SECOND_NIBBLE(instr)] = NN(instr,0);
		break;
	//7XNN - Add NN to register VX value
	case 0x7:
		registers[SECOND_NIBBLE(instr)] += NN(instr,0);
		break;
	case 0x8:
		switch (FOURTH_NIBBLE(instr)) {
		// 8XY0 - VX is set to the value of VY
		case 0x0:
			registers[SECOND_NIBBLE(instr)] = registers[THIRD_NIBBLE(instr)];
			break;
		// 8XY1 - VX is set to VX OR VY
		case 0x1:
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] | registers[THIRD_NIBBLE(instr)];
			break;
		// 8XY2 - VX is set to VX AND VY
		case 0x2:
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] & registers[THIRD_NIBBLE(instr)];
			break;
		// 8XY3 - VX is set to VX XOR VY
		case 0x3:
			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] ^ registers[THIRD_NIBBLE(instr)];
			break;
		// 8XY4 - VX is set to VX + VY (set VF to 1 if overflow occured) 
		case 0x4: {
			bool overflow = false;

			if (registers[SECOND_NIBBLE(instr)] + registers[THIRD_NIBBLE(instr)] > 0xFF)
				overflow = true;

			registers[SECOND_NIBBLE(instr)] = 
				registers[SECOND_NIBBLE(instr)] + registers[THIRD_NIBBLE(instr)];

			/* VF has to be changed after operation because it 
			 	can be used as an operand	 */
			if (overflow) 
				registers[0xF] = 1;
			else 
				registers[0xF] = 0;
			break;
		}
		// 8XY5 - VX is set to VX - VY (set VF to 0 if underflow occured)
		case 0x5: {
			bool underflow = false;

			if (registers[SECOND_NIBBLE(instr)] < registers[THIRD_NIBBLE(instr)])
				underflow = true;

			registers[SECOND_NIBBLE(instr)] -= registers[THIRD_NIBBLE(instr)];

			/* VF has to be changed after operation because it 
			 	can be used as an operand	 */
			if (underflow)
				registers[0xF] = 0;
			else 
				registers[0xF] = 1;
			break;
		}
		/* 8XY6 - load VY in to VX and shift 1 to the right 
			(shifted out bit is loaded into VF) 	*/
		case 0x6: {
			registers[SECOND_NIBBLE(instr)] = registers[THIRD_NIBBLE(instr)] >> 1;

			registers[0xF] = registers[THIRD_NIBBLE(instr)] & 0b00000001;
			break;
		}
		// 8XY7 - VX is set to VY - VX (set VF to 0 if underflow occured)
		case 0x7: {
			bool underflow = false;
			
			if (registers[THIRD_NIBBLE(instr)] < registers[SECOND_NIBBLE(instr)])
				underflow = true;

			registers[SECOND_NIBBLE(instr)] = 
				registers[THIRD_NIBBLE(instr)] - registers[SECOND_NIBBLE(instr)];

			/* VF has to be changed after operation because it 
			 	can be used as an operand	 */
			if (underflow)
				registers[0xF] = 0;
			else
				registers[0xF] = 1;
			break;
		}
		/* 8XYE - load VY in to VX and shift 1 to the left 
			(shifted out bit is loaded into VF) 	*/
		case 0xE:
			registers[SECOND_NIBBLE(instr)] = registers[THIRD_NIBBLE(instr)] << 1;

			registers[0xF] = (registers[THIRD_NIBBLE(instr)] & 0b10000000) >> 7;
			break;
		}
		break;
	// 9XY0 - skip one instruction if VX != VY
	case 0x9:
		if (registers[SECOND_NIBBLE(instr)] != registers[THIRD_NIBBLE(instr)])
			Chip8::incrementPC(2);
		break;
	// ANNN - Set index(address register) to NNN
	case 0xA:
		I = NNN(instr);
		break;
	/* BNNN - Jump with offset
		(jump to the address NNN + register 0 value) */
	case 0xB:
		pc = &storage[NNN(instr)];
		Chip8::incrementPC(registers[0x0]);
		break;
	// CXNN - generate random number, AND it with NN and put the result in VX
	case 0xC:
		std::srand(time(NULL));
		registers[SECOND_NIBBLE(instr)] = (std::rand() % 0xFF) & NN(instr,0);
		break;
	// DXYN - Display sprite
	case 0xD:
		Display::drawSprite(instr);
		break;
	case 0xE:
		switch (NN(instr,0)) {
		// EX9E - skip one instruction if the key corresponding to value in VX is pressed
		case 0x9E:
			if (Keyboard::state[Keyboard::scancodes[registers[SECOND_NIBBLE(instr)]]])
				Chip8::incrementPC(2);
			break;
		// EXA1 - skip one instruction if the key corresponding to value in VX is not pressed
		case 0xA1:
			if (!Keyboard::state[Keyboard::scancodes[registers[SECOND_NIBBLE(instr)]]])
				Chip8::incrementPC(2);
			break;
		}
		break;
	case 0xF:
		switch (NN(instr,0)) {
		// FX07 - set VX to the current value of the delay timer
		case 0x07:
			registers[SECOND_NIBBLE(instr)] = delay_timer;
			break;
		// FX15 - set the delay timer to the value in VX
		case 0x15:
			delay_timer = registers[SECOND_NIBBLE(instr)];
			break;
		// FX18 - set the sound timer to the value in VX
		case 0x18:
			sound_timer = registers[SECOND_NIBBLE(instr)];
			break;
		// FX1E - add to index(address register)
		case 0x1E:
			// set VF to 1 if I overflows
			if (I + registers[SECOND_NIBBLE(instr)] > 0xFFF)
				registers[0xF] = 1;

			I += registers[SECOND_NIBBLE(instr)];
			break;
		// FX0A - get key (wait for input;store hexadecimal value of pressed key in VX)
		case 0x0A: {
			SDL_Event event;
			bool key_released = false;

			while (!key_released) {
				while(SDL_PollEvent(&event)!=0) {
					for (int i = 0; i < 16; ++i) {
						if(event.key.keysym.scancode == Keyboard::scancodes[i]
						&& event.type == SDL_KEYUP) {
							registers[SECOND_NIBBLE(instr)] = i;
							key_released = true;
						}
					}
				}
			}	
			break;
			}
		/* FX29 - font character 
		(I is set to the addres of the hex digit stored in VX) */
		case 0x29:
			/* font characters are stored in memory starting at
			memory address 0 and every character takes 5 bytes
			of space 					*/
			I = registers[SECOND_NIBBLE(instr)] * 5;
			break;
		/* FX33 - binary-coded decimal conversion (convert the number
		in VX to three decimal digits and store in memory starting
			at memory location pointed to by I)		*/
		case 0x33:
			storage[I] = registers[SECOND_NIBBLE(instr)] / 100;
			storage[I+1] = (registers[SECOND_NIBBLE(instr)] % 100) / 10;
			storage[I+2] = registers[SECOND_NIBBLE(instr)] % 10;
			break;
		// FX55 - store registers V0 - VX values in memory
		case 0x55:
			for (int i = 0; i <= SECOND_NIBBLE(instr); ++i)
				storage[I + i] = registers[i];

			/* FX55 instruction increments index register 
				(COSMAC VIP interpreter way)	*/
			I += SECOND_NIBBLE(instr) + 1;
			break;
		//FX65 - load values from memory to registers
		case 0x65:
			for (int i = 0; i <= SECOND_NIBBLE(instr); ++i) 
				registers[i] = storage[I + i];

			/* FX65 instruction increments index register 
				(COSMAC VIP interpreter way)	*/
			I += SECOND_NIBBLE(instr) + 1;
			break;
		}
		break;
	default:
		throw std::runtime_error("Error - couldn't decode instruction\n");
		break;
	}
}

// main program loop
void Chip8::loop()
{
	// Fetch, decode and execute instructions
	Chip8::decodeAndExecute(Chip8::instructionFetch());

	// event check
	SDL_Event main_event;
	while(SDL_PollEvent(&main_event)!=0) {
		switch(main_event.type) {
		case SDL_KEYDOWN:
			// press escape to quit
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

	// redraw display
	if(Display::redraw) {
		Display::draw(Display::renderer,Display::texture);
		Display::redraw = false;	
	}

	SDL_Delay(1);
}

// debug mode program loop
void Chip8::loopDebug(uint8_t& options)
{
	// event check
	SDL_Event main_event;
	while(SDL_PollEvent(&main_event)!=0) {
		switch(main_event.type) {
		case SDL_KEYDOWN:
			switch (main_event.key.keysym.scancode) {
			case ESCAPE:
				isRunning = false;
				break;
				
			// press right arrow to execute next instruction
			case SDL_SCANCODE_RIGHT:
				options |= ADVANCE;
				break;

			// press right control to show register values
			case SDL_SCANCODE_RCTRL:
				options |= SHOW_REGISTERS;
				break;
			}

			Keyboard::state[main_event.key.keysym.scancode] = true;
			break;

		case SDL_KEYUP:
			Keyboard::state[main_event.key.keysym.scancode] = false;
			break;

		case SDL_QUIT:
			isRunning = false;
		}
	}

	// execution flow controlled by user
	if(options & ADVANCE) {
		// fetch instruction
		unsigned short instr = Chip8::instructionFetch();
		
		// decode and execute instructions
		Chip8::decodeAndExecute(instr);

		//print every executed instruction
		std::cout << "Executed instruction: " 		
			<< std::setfill('0') << std::setw(4) << std::hex
			<< instr << '\n';
	}

	// display current registers values
	if(options & SHOW_REGISTERS) {
		for (int i = 0; i < 16; i+=4) std::cout << i << ": " << int(registers[i])
					<< '\t' << i+1 << ": " << int(registers[i+1])
					<< '\t' << i+2 << ": " << int(registers[i+2])
					<< '\t' << i+3 << ": " << int(registers[i+3])
					<< '\n';
	}

	options = 0;
	
	// update display
	if(Display::redraw) {
		Display::draw(Display::renderer,Display::texture);
		Display::redraw = false;	
	}
}

// increment program counter
void Chip8::incrementPC(const int& n)
{
	 if (pc >= &storage[0] && pc <= &storage[0xFFF - n])
		 pc += n;
}
// callback function for SDL_AddTimer() - ticking at 60Hz
uint32_t Chip8::timerCallback(uint32_t interval, void* param)
{
	// decrement timer registers
	if (delay_timer > 0) delay_timer -= 1;	
	if (sound_timer > 0) sound_timer -= 1;	

	Display::redraw = true;

	// return next 17ms interval to the timer
	return 17;
}
