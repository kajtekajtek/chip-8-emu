#include "chip8.h"

std::string Options::filename;

bool Options::debug = false;

void Options::parse(int argc, char* argv[])
{
	if (argc < 2 || 3 < argc) 
		throw std::runtime_error("Invalid number of arguments\n");

	//first argument passed is always a file to open
	filename = argv[1];

	std::vector<std::string> args(argv + 2, argv + argc);

	for (const auto& arg : args) {
		if (arg == "-d" || arg == "--debug") {
			if (Options::debug) 
				throw std::runtime_error("Can't use debug option twice\n");
			Options::debug = true;
			continue;
		} else {
			throw std::runtime_error("Unknown argument: " + arg + '\n');
		}
	}
}
