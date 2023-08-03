#include "chip8.h"

bool isRunning = true;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;

void loop()
{
	//dodac timing i program counter range checking
	//Fetch, decode and execute instructions from memory
	Chip8::decodeAndExecute(Chip8::instructionFetch());
	//event check 
	SDL_Event event;
	while(SDL_PollEvent(&event)!=0) {
		switch (event.type) {
		//press escape to quit
		case SDL_KEYDOWN:
			if(event.key.keysym.sym==SDLK_ESCAPE)
				isRunning = false;
			break;
		case SDL_QUIT:
			isRunning = false;
			break;
		}
	}
	//wait 12 miliseconds
	SDL_Delay(12);
}

int main(int argc, char* argv[])
{
	if(argc != 2) {
		std::cout << "Invalid number of arguments\n";
		return 1;

	}
	//initialize
	Chip8::init();

	//initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "Error: Couldnt initialize SDL" << '\n';
	}

	//create window, renderer and texture holding pixel data
	window = SDL_CreateWindow("",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,window_width,window_height,0);
	renderer = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
	texture = SDL_CreateTexture(renderer,SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING,64,32);

	//load program into memory
	Chip8::loadFile(argv[1]); 

	//execution loop
	while (isRunning) {
		loop();
		if (displayDraw) {
			Display::draw(renderer,texture);
			displayDraw= false;
		}
	}

	//release resources and quit
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

