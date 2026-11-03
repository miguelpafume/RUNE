#include "PPU.hpp"

namespace SNES {

void PPU::advance(U32 cycles) {
	const bool isShort = m_scanline == SHORT_SCANLINE && (m_frames & 1);
	const U32  length = isShort ? CYCLES_PER_SCANLINE - CYCLES_PER_DOT : CYCLES_PER_SCANLINE;

	m_hClock += cycles;

	if (m_hClock < length) {
		return;
	}

	m_hClock -= length;
	m_scanline++;

	if (m_scanline == SCANLINES_PER_FRAME) {
		m_scanline = 0;
		m_frames++;
	}
}

} // namespace SNES
