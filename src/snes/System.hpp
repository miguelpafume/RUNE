#pragma once

#include "APU.hpp"
#include "Bus.hpp"
#include "CPU.hpp"
#include "Cartridge.hpp"
#include "Constants.hpp"
#include "rune/Types.hpp"

#include <optional>
#include <string>
#include <vector>

namespace SNES {

using RUNE::U8;

class System {
public:
	explicit System();

	void loadRom(const std::string& romPath);
	void runFrame();

	void step() { m_cpu.step(); }

	std::string traceLine() const;

	const U8* framebuffer();

private:
	Bus m_bus {Bus::Mode::System};
	CPU m_cpu {m_bus};
	APU m_apu {};

	std::optional<Cartridge> m_cartridge {};

	void printCartridge(const std::string& romPath) const;

	// TEMP
	const std::vector<U8> m_framebuffer = std::vector<U8>(VIDEO_HEIGHT * VIDEO_WIDTH * 4, 0xFF);
};

} // namespace SNES