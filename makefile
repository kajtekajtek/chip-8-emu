chip8 : chip8.cpp chip8.h display.cpp options.cpp main.cpp
	g++ -o chip8 main.cpp chip8.h chip8.cpp display.cpp options.cpp \
		`sdl2-config --cflags --libs`
clean : 
	rm chip8 
