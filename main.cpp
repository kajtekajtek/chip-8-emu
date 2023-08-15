#include "chip8.h"

bool isRunning = true;

SDL_Window* Display::window = nullptr;
SDL_Renderer* Display::renderer = nullptr;
SDL_Texture* Display::texture = nullptr;

int main(int argc, char* argv[])
try {
	// parse console arguments
	Options::parse(argc, argv);
	
	// initialize
	Chip8::init();

	// initialize global clock (60Hz)
	SDL_TimerID timerID = SDL_AddTimer(17,Chip8::timerCallback,nullptr);

	// load program into memory
	Chip8::loadFile(Options::filename); 

	// standard execution loop
	if (!Options::debug) {
		while (isRunning) {
			Chip8::loop();
		}
	/* debug mode - advance through program step by step, log
	 * every executed instruction and current registers' states	*/
	} else {
		/* variable debug_flags is used to indicate user's actions
		 * such as advancing to next instruction 
		 * and printing registers' values 			*/
		uint8_t debug_flags = 0;
		while (isRunning) {
			Chip8::loopDebug(debug_flags);
		}
	}

	// release resources and quit
	SDL_DestroyTexture(Display::texture);
	SDL_DestroyRenderer(Display::renderer);
	SDL_DestroyWindow(Display::window);
	SDL_RemoveTimer(timerID);

	SDL_Quit();

} catch(std::runtime_error& e) {
	std::cerr << e.what();

	// release resources and quit
	SDL_DestroyTexture(Display::texture);
	SDL_DestroyRenderer(Display::renderer);
	SDL_DestroyWindow(Display::window);

	SDL_Quit();
}
