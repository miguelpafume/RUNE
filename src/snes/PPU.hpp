#pragma once

#include "Constants.hpp"
#include "rune/Types.hpp"

namespace SNES {

using RUNE::U32;
using RUNE::U64;

class PPU {
public:
	// the Bus calls this for every master cycle it charges
	void advance(U32 cycles);

	U32 hClock() const { return m_hClock; }
	U32 scanline() const { return m_scanline; }
	U64 frames() const { return m_frames; }

	bool inVBlank() const { return m_scanline >= VBLANK_START_LINE; }

private:
	U32 m_hClock {}; // master cycles since the line started
	U32 m_scanline {};
	U64 m_frames {};
};

} // namespace SNES
