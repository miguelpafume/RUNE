#include "PPU.hpp"

#include <array>

namespace SNES {

constexpr U16 INIDISP = 0x2100;
constexpr U16 VMAIN = 0x2115;
constexpr U16 VMADDL = 0x2116;
constexpr U16 VMADDH = 0x2117;
constexpr U16 VMDATAL = 0x2118;
constexpr U16 VMDATAH = 0x2119;
constexpr U16 CGADD = 0x2121;
constexpr U16 CGDATA = 0x2122;
constexpr U16 RDVRAML = 0x2139;
constexpr U16 RDVRAMH = 0x213A;
constexpr U16 RDCGRAM = 0x213B;

constexpr U8 INIDISP_FORCED_BLANK = 0x80;
constexpr U8 VMAIN_INCREMENT_ON_HIGH = 0x80;

constexpr U16 VRAM_ADDRESS_MASK = 0x7FFF;

// How many words the address moves per access, $2115 bits 0-1
constexpr std::array<U16, 4> VRAM_INCREMENTS {1, 32, 128, 128};

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

U8 PPU::readRegister(U16 address, U8 openBus) {
	switch (address) {
	// The buffer reloads before the address moves, so the first word comes out twice
	case RDVRAML : {
		const U8 value = static_cast<U8>(m_vramReadBuffer);

		if (!m_vramIncrementOnHigh) {
			m_vramReadBuffer = readVram();
			stepVramAddress();
		}

		return value;
	}

	case RDVRAMH : {
		const U8 value = static_cast<U8>(m_vramReadBuffer >> 8);

		if (m_vramIncrementOnHigh) {
			m_vramReadBuffer = readVram();
			stepVramAddress();
		}

		return value;
	}

	case RDCGRAM : {
		const U16 color = m_cgram[m_cgramAddress];

		if (!m_cgramHighByte) {
			m_ppu2LastRead = static_cast<U8>(color);
		} else {
			// Colors have 15 bits, so bit 7 keeps what this chip read last
			m_ppu2LastRead = static_cast<U8>((m_ppu2LastRead & 0x80) | (color >> 8));
			m_cgramAddress++;
		}

		m_cgramHighByte = !m_cgramHighByte;

		return m_ppu2LastRead;
	}

	default : // TODO: the other read registers, $2134-$2138 and $213C-$213F
		return openBus;
	}
}

void PPU::writeRegister(U16 address, U8 value) {
	switch (address) {
	case INIDISP :
		m_forcedBlank = (value & INIDISP_FORCED_BLANK) != 0;
		break;

	case VMAIN :
		m_vramIncrement = VRAM_INCREMENTS[value & 0x03];
		m_vramRemap = (value >> 2) & 0x03;
		m_vramIncrementOnHigh = (value & VMAIN_INCREMENT_ON_HIGH) != 0;
		break;

	case VMADDL :
		m_vramAddress = (m_vramAddress & 0x7F00) | value;
		m_vramReadBuffer = readVram();
		break;

	case VMADDH :
		m_vramAddress = static_cast<U16>((m_vramAddress & 0x00FF) | (value << 8)) & VRAM_ADDRESS_MASK;
		m_vramReadBuffer = readVram();
		break;

	case VMDATAL :
		if (canAccessVram()) {
			U16& word = m_vram[vramIndex()];

			word = (word & 0xFF00) | value;
		}

		if (!m_vramIncrementOnHigh) {
			stepVramAddress();
		}
		break;

	case VMDATAH :
		if (canAccessVram()) {
			U16& word = m_vram[vramIndex()];

			word = static_cast<U16>((word & 0x00FF) | (value << 8));
		}

		if (m_vramIncrementOnHigh) {
			stepVramAddress();
		}
		break;

	case CGADD :
		m_cgramAddress = value;
		m_cgramHighByte = false;
		break;

	case CGDATA :
		// Nothing reaches CGRAM until the second byte arrives
		if (!m_cgramHighByte) {
			m_cgramLatch = value;
		} else {
			m_cgram[m_cgramAddress] = static_cast<U16>((value & 0x7F) << 8 | m_cgramLatch);
			m_cgramAddress++;
		}

		m_cgramHighByte = !m_cgramHighByte;
		break;

	default : // TODO: the other write registers, $2101-$2133
		break;
	}
}

// $2115 bits 2-3 rotate the low bits of the address, so a bitmap can be written line by line
U16 PPU::vramIndex() const {
	const U16 address = m_vramAddress;

	switch (m_vramRemap) {
	case 1 :
		return static_cast<U16>((address & 0x7F00) | ((address << 3) & 0x00F8) | ((address >> 5) & 0x07));

	case 2 :
		return static_cast<U16>((address & 0x7E00) | ((address << 3) & 0x01F8) | ((address >> 6) & 0x07));

	case 3 :
		return static_cast<U16>((address & 0x7C00) | ((address << 3) & 0x03F8) | ((address >> 7) & 0x07));

	default :
		return address;
	}
}

U16 PPU::readVram() const { return canAccessVram() ? m_vram[vramIndex()] : U16 {0}; }

void PPU::stepVramAddress() { m_vramAddress = (m_vramAddress + m_vramIncrement) & VRAM_ADDRESS_MASK; }

} // namespace SNES
