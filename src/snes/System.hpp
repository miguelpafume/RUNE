#pragma once

#include "APU.hpp"
#include "Bus.hpp"
#include "CPU.hpp"
#include "Cartridge.hpp"
#include "Constants.hpp"
#include "DMA.hpp"
#include "Gamepad.hpp"
#include "PPU.hpp"
#include "rune/Types.hpp"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace SNES {

using RUNE::U16;
using RUNE::U32;
using RUNE::U8;

class System {
public:
	explicit System();

	void loadRom(const std::string& romPath);
	void runFrame();

	void step() { m_cpu.step(); }

	// Buttons held on a controller port, with the bits of $4218-$4219
	void setButtons(U32 port, U16 buttons) { m_gamepads[port].setButtons(buttons); }

	std::string traceLine() const;

	const U8* framebuffer() const { return m_ppu.framebuffer().data(); }

	const PPU& ppu() const { return m_ppu; }

private:
	Bus m_bus {Bus::Mode::System};
	CPU m_cpu {m_bus};
	APU m_apu {};
	PPU m_ppu {};
	DMA m_dma {m_bus};

	std::array<Gamepad, 2> m_gamepads {};

	std::optional<Cartridge> m_cartridge {};

	void printCartridge(const std::string& romPath) const;
};

} // namespace SNES