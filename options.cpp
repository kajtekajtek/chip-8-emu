#include "chip8.h"

// path to the file to load
std::string Options::filename;

// debug mode flag
bool Options::debug = false;

// parse and decode command line arguments
void Options::parse(int argc, char* argv[])
{
	// range checking the number of arguments
	if (argc < 2 || 3 < argc) 
		throw std::runtime_error("Invalid number of arguments\n");

	// first argument passed is always a path to the file to load
	filename = argv[1];

	// vector of the rest of arguments 
	std::vector<std::string> args(argv + 2, argv + argc);

	for (const auto& arg : args) {
		/* run the program in debug mode (execution flow
		 * controlled by the user & useful information) */
		if (arg == "-d" || arg == "--debug") {
			if (Options::debug) 
				throw std::runtime_error("Can't use debug option twice\n");
			Options::debug = true;
			continue;
		
		/* argument -r passed with one of corresponding options 
		 *           change default window size		*/
		} else if (arg.substr(0,2) == "-r" && arg.size() > 2) {
			switch (std::stoi(arg.substr(2))) {
			case 640:
				Display::width = 640;
				Display::height = 320;
				break;
			case 1240:
				Display::width = 1280;
				Display::height = 640;
				break;
			case 1920:
				Display::width = 1920;
				Display::height = 960;
				break;
			case 2560:
				Display::width = 2560;
				Display::height = 1280;
				break;
			default:
				throw std::runtime_error
				("Invalid resolution argument\n");
			}
		} else {
			throw std::runtime_error("Unknown argument: " + arg + '\n');
		}
	}
}
