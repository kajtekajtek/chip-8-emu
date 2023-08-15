#include "chip8.h"

/* boolean flag indicating whether display should be redrawn before
		 executing next instruction			*/
bool Display::redraw = true;

// pixel buffer
unsigned char Display::buffer[2048];

// window resolution variables set to default
int Display::width = 1280;
int Display::height = 640;

// update the display texture
void Display::update(SDL_Texture* texture)
{
	uint32_t pixels[2048];

	for (int i = 0; i < 2048; i++) 
		pixels[i] = Display::buffer[i] ? 0xFFFFFFFF : 0x00000000;

	SDL_UpdateTexture(texture,NULL,pixels,64 * sizeof(uint32_t));
}

// redraw the display texture to the display
void Display::draw(SDL_Renderer* renderer, SDL_Texture* texture)
{
	// all rendering operations will be performed on buffer texture
	SDL_SetRenderTarget(renderer,texture);
	Display::update(texture);
	SDL_SetRenderTarget(renderer,NULL);

	// copy the texture to the window
	SDL_RenderCopy(renderer,texture,NULL,NULL);

	// update the window with the latest rendering operations
	SDL_RenderPresent(renderer);
}

// DXYN - display sprite
void Display::drawSprite(const unsigned short& instr)
{
	// location stored in registers specified by X,Y
	unsigned char X = registers[SECOND_NIBBLE(instr)] % 64;
	unsigned char Y = registers[THIRD_NIBBLE(instr)] % 32;

	// sprite starts at memory address stored in I register
	unsigned char* sprite = &storage[I];

	// set flag register to 0
	registers[0xF] = 0;

	// for every row of the sprite
	for (int y_offset = 0; y_offset < FOURTH_NIBBLE(instr); ++y_offset) {
		// for every bit of the row
		for (int x_offset = 0; x_offset < 8; ++x_offset) {
			/* range checking bit and checking whether sprite
			 	is being drawn on the same row */
			if (PIXEL_INDEX(X, x_offset, Y, y_offset) < 2048 &&
			((X+x_offset) < (64+64*Y+y_offset))) {
				/* variable current_bit - pixel of the
					sprite currently being drawn 	*/
				bool current_bit = (*sprite >> 7-x_offset) & 0b00000001;

				/*  checking if pixel collision occurs
				       	   if so, set VF to 1 		*/
				if (Display::buffer[PIXEL_INDEX(X, x_offset, Y, y_offset)]
				&& current_bit) {
					registers[0xF] = 1;
				}

				// XOR bit with currently drawn pixel
				Display::buffer[PIXEL_INDEX(X, x_offset, Y, y_offset)]
					^= current_bit;
			} 
		}
		++sprite;
	}
}
