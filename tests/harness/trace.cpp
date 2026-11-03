#include "snes/System.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <string>

// Prints the CPU state before each instruction
int main(int argc, char** argv) {
	if (argc != 3) {
		std::cerr << "usage: snes_trace rom.sfc instructions\n";
		return 1;
	}

	try {
		SNES::System system {};
		system.loadRom(argv[1]);

		const std::size_t instructions = std::stoull(argv[2]);

		for (std::size_t i = 0; i < instructions; i++) {
			std::cout << system.traceLine() << '\n';
			system.step();
		}
	} catch (const std::exception& e) {
		std::cerr << "ERROR: " << e.what() << '\n';
		return 1;
	}

	return 0;
}
