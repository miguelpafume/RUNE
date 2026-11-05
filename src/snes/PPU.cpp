#include "PPU.hpp"

#include <array>

namespace SNES {

constexpr U16 INIDISP = 0x2100;

constexpr U16 BGMODE = 0x2105;
constexpr U16 BG1SC = 0x2107;
constexpr U16 BG2SC = 0x2108;
constexpr U16 BG3SC = 0x2109;
constexpr U16 BG4SC = 0x210A;
constexpr U16 BG12NBA = 0x210B;
constexpr U16 BG34NBA = 0x210C;
constexpr U16 BG1HOFS = 0x210D;
constexpr U16 BG1VOFS = 0x210E;
constexpr U16 BG2HOFS = 0x210F;
constexpr U16 BG2VOFS = 0x2110;
constexpr U16 BG3HOFS = 0x2111;
constexpr U16 BG3VOFS = 0x2112;
constexpr U16 BG4HOFS = 0x2113;
constexpr U16 BG4VOFS = 0x2114;

constexpr U16 VMAIN = 0x2115;
constexpr U16 VMADDL = 0x2116;
constexpr U16 VMADDH = 0x2117;
constexpr U16 VMDATAL = 0x2118;
constexpr U16 VMDATAH = 0x2119;

constexpr U16 CGADD = 0x2121;
constexpr U16 CGDATA = 0x2122;

constexpr U16 TM = 0x212C;

constexpr U16 RDVRAML = 0x2139;
constexpr U16 RDVRAMH = 0x213A;
constexpr U16 RDCGRAM = 0x213B;

constexpr U8 INIDISP_FORCED_BLANK = 0x80;
constexpr U8 INIDISP_BRIGHTNESS_MASK = 0x0F;

constexpr U8 BGMODE_BG3_ON_TOP = 0x08;

constexpr U8 VMAIN_INCREMENT_ON_HIGH = 0x80;

constexpr U16 VRAM_ADDRESS_MASK = 0x7FFF;
constexpr U16 SCROLL_MASK = 0x3FF;

// Bits of one tilemap entry
constexpr U16 MAP_FLIP_Y = 0x8000;
constexpr U16 MAP_FLIP_X = 0x4000;
constexpr U16 MAP_PRIORITY = 0x2000;
constexpr U16 MAP_TILE_MASK = 0x03FF;

constexpr U32 MAX_BRIGHTNESS = 15;

// Same moment as the fast PPU of bsnes. A write after this only shows on the next line
constexpr U32 RENDER_POSITION = 512;

// How many words the address moves per access, $2115 bits 0-1
constexpr std::array<U16, 4> VRAM_INCREMENTS {1, 32, 128, 128};

void PPU::advance(U32 cycles) {
	const bool isShort = m_scanline == SHORT_SCANLINE && (m_frames & 1);
	const U32  length = isShort ? CYCLES_PER_SCANLINE - CYCLES_PER_DOT : CYCLES_PER_SCANLINE;
	const U32  hClockBefore = m_hClock;

	m_hClock += cycles;

	// Line 0 never shows up
	if (hClockBefore < RENDER_POSITION && m_hClock >= RENDER_POSITION && m_scanline >= 1 && m_scanline < VBLANK_START_LINE) {
		renderLine();
	}

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
		m_brightness = value & INIDISP_BRIGHTNESS_MASK;
		break;

	case BGMODE :
		m_bgMode = value & 0x07;
		m_bg3OnTop = (value & BGMODE_BG3_ON_TOP) != 0;

		for (U32 number = 0; number < m_backgrounds.size(); number++) {
			m_backgrounds[number].bigTiles = (value & (0x10u << number)) != 0;
		}
		break;

	case BG1SC :
	case BG2SC :
	case BG3SC :
	case BG4SC : {
		Background& background = m_backgrounds[address - BG1SC];

		background.mapSize = value & 0x03;
		background.mapAddress = (value << 8) & 0x7C00;
		break;
	}

	case BG12NBA :
		m_backgrounds[0].tileAddress = (value << 12) & 0x7000;
		m_backgrounds[1].tileAddress = (value << 8) & 0x7000;
		break;

	case BG34NBA :
		m_backgrounds[2].tileAddress = (value << 12) & 0x7000;
		m_backgrounds[3].tileAddress = (value << 8) & 0x7000;
		break;

	// Each one takes two writes. The new byte is the high part, the low part comes from the last write to any of them
	case BG1HOFS :
	case BG2HOFS :
	case BG3HOFS :
	case BG4HOFS :
		m_backgrounds[(address - BG1HOFS) >> 1].hScroll =
		    (value << 8 | (m_scrollLatch & ~0x07) | (m_hScrollLatch & 0x07)) & SCROLL_MASK;
		m_scrollLatch = value;
		m_hScrollLatch = value;
		break;

	case BG1VOFS :
	case BG2VOFS :
	case BG3VOFS :
	case BG4VOFS :
		m_backgrounds[(address - BG1VOFS) >> 1].vScroll = (value << 8 | m_scrollLatch) & SCROLL_MASK;
		m_scrollLatch = value;
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

	case TM : // TODO: bit 4, sprites
		for (U32 number = 0; number < m_backgrounds.size(); number++) {
			m_backgrounds[number].onMainScreen = (value & (1u << number)) != 0;
		}
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

// Turns one color channel from CGRAM into one framebuffer byte
// Brightness comes first: 15 keeps the channel as it is, 0 makes it black
// Then 5 bits become 8. Shifting alone would turn 31 into 248,
// so the top 3 bits also fill the empty low bits, and 31 becomes 255
static U8 outputChannel(U32 channel, U32 brightness) {
	const U32 level = channel * brightness / MAX_BRIGHTNESS;

	return static_cast<U8>(level << 3 | level >> 2);
}

// The layers of each video mode
// A higher priority covers a lower one, the numbers in between are for the sprites
const PPU::LayerFormat& PPU::layerFormat(U32 number) const {
	static constexpr std::array<LayerFormat, 4> MODE_0 {{
	    {2, 8, 11},
	    {2, 7, 10},
	    {2, 2, 5},
	    {2, 1, 4},
	}};
	static constexpr std::array<LayerFormat, 4> MODE_1 {{
	    {4, 6, 9},
	    {4, 5, 8},
	    {2, 1, 3},
	    {},
	}};
	static constexpr std::array<LayerFormat, 4> MODE_1_BG3_ON_TOP {{
	    {4, 5, 8},
	    {4, 4, 7},
	    {2, 1, 10},
	    {},
	}};
	static constexpr std::array<LayerFormat, 4> NO_LAYERS {};

	switch (m_bgMode) {
	case 0 :
		return MODE_0[number];

	case 1 :
		return m_bg3OnTop ? MODE_1_BG3_ON_TOP[number] : MODE_1[number];

	default : // TODO: modes 2-7
		return NO_LAYERS[number];
	}
}

void PPU::renderLine() {
	m_line.fill({});

	for (U32 number = 0; number < m_backgrounds.size(); number++) {
		renderBackground(number);
	}

	const U32 brightness = m_forcedBlank ? 0 : m_brightness;

	U8* pixel = &m_framebuffer[std::size_t {m_scanline - 1} * VIDEO_WIDTH * 4];

	for (const LinePixel& linePixel : m_line) {
		const U16 color = m_cgram[linePixel.color];

		pixel[0] = outputChannel(color & 0x1F, brightness);
		pixel[1] = outputChannel((color >> 5) & 0x1F, brightness);
		pixel[2] = outputChannel((color >> 10) & 0x1F, brightness);
		pixel[3] = 0xFF;
		pixel += 4;
	}
}

void PPU::renderBackground(U32 number) {
	const Background&  background = m_backgrounds[number];
	const LayerFormat& format = layerFormat(number);

	if (!background.onMainScreen || format.bitsPerPixel == 0) {
		return;
	}

	for (U32 x = 0; x < VIDEO_WIDTH; x++) {
		const LinePixel pixel = backgroundPixel(number, format, x + background.hScroll, m_scanline + background.vScroll);

		// A transparent pixel has priority 0, so it never wins
		if (pixel.priority > m_line[x].priority) {
			m_line[x] = pixel;
		}
	}
}

// The priority and the CGRAM color of one pixel of a background
PPU::LinePixel PPU::backgroundPixel(U32 number, const LayerFormat& format, U32 hPosition, U32 vPosition) const {
	const Background& background = m_backgrounds[number];

	const U32 tileShift = background.bigTiles ? 4 : 3;
	const U32 mapX = hPosition >> tileShift;
	const U32 mapY = vPosition >> tileShift;

	// In memory, a bigger map is just more 32x32 screens, one after the other
	U32 entryOffset = (mapY & 0x1F) << 5 | (mapX & 0x1F);

	if ((mapX & 0x20) && (background.mapSize & 0x01)) {
		entryOffset += 0x400;
	}

	if ((mapY & 0x20) && (background.mapSize & 0x02)) {
		entryOffset += (background.mapSize & 0x01) ? 0x800 : 0x400;
	}

	const U16  entry = m_vram[(background.mapAddress + entryOffset) & VRAM_ADDRESS_MASK];
	const bool flipX = (entry & MAP_FLIP_X) != 0;
	const bool flipY = (entry & MAP_FLIP_Y) != 0;

	U32 tile = entry & MAP_TILE_MASK;

	// A 16x16 tile is made of 4 tiles of 8x8: this one, the next one, and the two 16 numbers later
	if (background.bigTiles) {
		if (((hPosition & 0x08) != 0) != flipX) {
			tile += 1;
		}

		if (((vPosition & 0x08) != 0) != flipY) {
			tile += 16;
		}
	}

	const U32 row = flipY ? 7 - (vPosition & 0x07) : vPosition & 0x07;
	const U32 bit = flipX ? hPosition & 0x07 : 7 - (hPosition & 0x07);

	// 2 bits take 8 words per tile, 4 bits take 16
	const U32 rowAddress = background.tileAddress + (tile & MAP_TILE_MASK) * format.bitsPerPixel * 4 + row;

	U32 color = 0;

	// Each word holds 2 bits of each pixel of the row. With 4 bits, the other two come 8 words later
	for (U32 pair = 0; pair < format.bitsPerPixel / 2u; pair++) {
		const U16 planes = m_vram[(rowAddress + pair * 8) & VRAM_ADDRESS_MASK];

		color |= (((planes >> bit) & 0x01) | (((planes >> (bit + 8)) & 0x01) << 1)) << (pair * 2);
	}

	if (color == 0) {
		return {};
	}

	// In mode 0 each background has its own 32 colors
	const U32 base = m_bgMode == 0 ? number << 5 : 0;
	const U32 palette = (entry >> 10) & 0x07;

	return {
	    (entry & MAP_PRIORITY) ? format.highPriority : format.lowPriority,
	    static_cast<U8>(base + (palette << format.bitsPerPixel) + color)
	};
}

} // namespace SNES
