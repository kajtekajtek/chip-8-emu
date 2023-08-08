#include "chip8.h"

bool isRunning = true;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;

int main(int argc, char* argv[])
try {
	//parse console arguments
	Options::parse(argc, argv);
	
	//initialize
	Chip8::init();

	//initialize global timer
	SDL_TimerID timerID = SDL_AddTimer(17,Chip8::timerCallback,nullptr);

	//load program into memory
	Chip8::loadFile(Options::filename); 

	//execution loop
	if (!Options::debug) {
		while (isRunning) {
			Chip8::loop();
		}
	} else {
		std::cout << 1; 
	}

	//release resources and quit
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_RemoveTimer(timerID);
	SDL_Quit();
} catch(std::runtime_error& e) {
	std::cerr << e.what();
}
