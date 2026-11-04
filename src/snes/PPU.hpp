#pragma once

#include "Constants.hpp"
#include "rune/Types.hpp"

#include <cstddef>
#include <vector>

namespace SNES {

using RUNE::U16;
using RUNE::U32;
using RUNE::U64;
using RUNE::U8;

class PPU {
public:
	// the Bus calls this for every master cycle it charges
	void advance(U32 cycles);

	U32 hClock() const { return m_hClock; }
	U32 scanline() const { return m_scanline; }
	U64 frames() const { return m_frames; }

	bool inVBlank() const { return m_scanline >= VBLANK_START_LINE; }

	U8   readRegister(U16 address, U8 openBus);
	void writeRegister(U16 address, U8 value);

	const std::vector<U16>& vram() const { return m_vram; }
	const std::vector<U16>& cgram() const { return m_cgram; }

private:
	U32 m_hClock {}; // master cycles since the line started
	U32 m_scanline {};
	U64 m_frames {};

	static constexpr std::size_t VRAM_WORDS = 0x8000; // 64 KiB
	static constexpr std::size_t CGRAM_COLORS = 256;

	std::vector<U16> m_vram = std::vector<U16>(VRAM_WORDS);
	std::vector<U16> m_cgram = std::vector<U16>(CGRAM_COLORS);

	bool m_forcedBlank = true; // $2100 bit 7

	U16  m_vramAddress {};
	U16  m_vramIncrement = 1;
	U8   m_vramRemap {};
	bool m_vramIncrementOnHigh {};
	U16  m_vramReadBuffer {};

	U8   m_cgramAddress {};
	bool m_cgramHighByte {}; // $2122 and $213B share it, each access flips it
	U8   m_cgramLatch {};
	U8   m_ppu2LastRead {};

	// While the PPU draws, the VRAM is all its own
	bool canAccessVram() const { return m_forcedBlank || inVBlank(); }
	U16  vramIndex() const;
	U16  readVram() const;
	void stepVramAddress();
};

} // namespace SNES
