#include "snes/System.hpp"

#include <cstddef>
#include <exception>
#include <fstream>
#include <iostream>
#include <string>

// Saves the picture after some frames as a PPM, so it runs without a window
int main(int argc, char** argv) {
	if (argc != 4) {
		std::cerr << "usage: snes_frame rom.sfc frames output.ppm\n";
		return 1;
	}

	try {
		SNES::System system {};
		system.loadRom(argv[1]);

		const std::size_t frames = std::stoull(argv[2]);

		for (std::size_t i = 0; i < frames; i++) {
			system.runFrame();
		}

		std::ofstream file {argv[3], std::ios::binary};
		file << "P6\n" << SNES::VIDEO_WIDTH << ' ' << SNES::VIDEO_HEIGHT << "\n255\n";

		const auto* pixels = reinterpret_cast<const char*>(system.framebuffer());

		// PPM has no alpha
		for (std::size_t i = 0; i < std::size_t {SNES::VIDEO_WIDTH} * SNES::VIDEO_HEIGHT; i++) {
			file.write(pixels + i * 4, 3);
		}
	} catch (const std::exception& e) {
		std::cerr << "ERROR: " << e.what() << '\n';
		return 1;
	}

	return 0;
}
