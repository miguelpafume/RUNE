#include "snes/System.hpp"

#include <cstddef>
#include <exception>
#include <iostream>
#include <string>

// Prints a tilemap as text, for ROMs whose tile numbers are ASCII codes
// mapAddress is the VRAM word address in hex
int main(int argc, char** argv) {
	if (argc != 4) {
		std::cerr << "usage: snes_bgtext rom.sfc frames mapAddress\n";
		return 1;
	}

	constexpr std::size_t MAP_COLUMNS = 32;
	constexpr std::size_t VISIBLE_ROWS = 28;
	constexpr unsigned    TILE_MASK = 0x3FF;

	try {
		SNES::System system {};
		system.loadRom(argv[1]);

		const std::size_t frames = std::stoull(argv[2]);
		const std::size_t map = std::stoull(argv[3], nullptr, 16);

		for (std::size_t i = 0; i < frames; i++) {
			system.runFrame();
		}

		const auto& vram = system.ppu().vram();

		for (std::size_t row = 0; row < VISIBLE_ROWS; row++) {
			std::string line {};

			for (std::size_t column = 0; column < MAP_COLUMNS; column++) {
				const unsigned tile = vram[map + row * MAP_COLUMNS + column] & TILE_MASK;

				line += tile >= 0x20 && tile < 0x7F ? static_cast<char>(tile) : '.';
			}

			std::cout << line << '\n';
		}
	} catch (const std::exception& e) {
		std::cerr << "ERROR: " << e.what() << '\n';
		return 1;
	}

	return 0;
}
