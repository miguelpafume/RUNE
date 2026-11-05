#include "PPU.hpp"

#include <array>

namespace SNES {

constexpr U16 INIDISP = 0x2100;

constexpr U16 OBSEL = 0x2101;
constexpr U16 OAMADDL = 0x2102;
constexpr U16 OAMADDH = 0x2103;
constexpr U16 OAMDATA = 0x2104;

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

constexpr U16 RDOAM = 0x2138;
constexpr U16 RDVRAML = 0x2139;
constexpr U16 RDVRAMH = 0x213A;
constexpr U16 RDCGRAM = 0x213B;

constexpr U8 INIDISP_FORCED_BLANK = 0x80;
constexpr U8 INIDISP_BRIGHTNESS_MASK = 0x0F;

constexpr U8 OAMADDH_PRIORITY_ROTATION = 0x80;

constexpr U8 BGMODE_BG3_ON_TOP = 0x08;

constexpr U8 TM_SPRITES = 0x10;

constexpr U8 VMAIN_INCREMENT_ON_HIGH = 0x80;

constexpr U16 VRAM_ADDRESS_MASK = 0x7FFF;
constexpr U16 SCROLL_MASK = 0x3FF;

// Bits of one tilemap entry
constexpr U16 MAP_FLIP_Y = 0x8000;
constexpr U16 MAP_FLIP_X = 0x4000;
constexpr U16 MAP_PRIORITY = 0x2000;
constexpr U16 MAP_TILE_MASK = 0x03FF;

constexpr U32 MAX_BRIGHTNESS = 15;

constexpr U16 OAM_ADDRESS_MASK = 0x3FF;
constexpr U16 OAM_HIGH_TABLE = 0x200;

constexpr U32 SPRITE_COUNT = 128;
constexpr U32 MAX_SPRITES_PER_LINE = 32;
constexpr U32 MAX_SPRITE_TILES_PER_LINE = 34;
constexpr U32 SPRITE_PALETTE_BASE = 128;

// Small and large sprite sizes for each value of $2101 bits 5-7
constexpr std::array<U32, 8> SMALL_WIDTHS {8, 8, 8, 16, 16, 32, 16, 16};
constexpr std::array<U32, 8> SMALL_HEIGHTS {8, 8, 8, 16, 16, 32, 32, 32};
constexpr std::array<U32, 8> LARGE_WIDTHS {16, 32, 64, 32, 64, 64, 32, 32};
constexpr std::array<U32, 8> LARGE_HEIGHTS {16, 32, 64, 32, 64, 64, 64, 32};

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

	// Every V-Blank starts the OAM address over, unless the screen is off
	if (m_scanline == VBLANK_START_LINE && !m_forcedBlank) {
		m_oamAddress = m_oamBaseAddress;
	}

	if (m_scanline == SCANLINES_PER_FRAME) {
		m_scanline = 0;
		m_frames++;
	}
}

// The high table is only 32 bytes, repeated over the whole upper half of the address
static U16 oamIndex(U16 address) { return (address & OAM_HIGH_TABLE) ? (OAM_HIGH_TABLE | (address & 0x1F)) : address; }

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

	case RDOAM : {
		const U8 value = m_oam[oamIndex(m_oamAddress)];

		m_oamAddress = (m_oamAddress + 1) & OAM_ADDRESS_MASK;

		return value;
	}

	default : // TODO: the other read registers, $2134-$2137 and $213C-$213F
		return openBus;
	}
}

void PPU::writeRegister(U16 address, U8 value) {
	switch (address) {
	case INIDISP :
		m_forcedBlank = (value & INIDISP_FORCED_BLANK) != 0;
		m_brightness = value & INIDISP_BRIGHTNESS_MASK;
		break;

	case OBSEL :
		m_spriteTileAddress = (value << 13) & 0x6000;
		m_spriteNameGap = (value >> 3) & 0x03;
		m_spriteSize = (value >> 5) & 0x07;
		break;

	// The register counts in words, the address inside counts in bytes
	case OAMADDL :
		m_oamBaseAddress = static_cast<U16>((m_oamBaseAddress & 0x0200) | (value << 1));
		m_oamAddress = m_oamBaseAddress;
		break;

	case OAMADDH :
		m_oamBaseAddress = static_cast<U16>(((value & 0x01) << 9) | (m_oamBaseAddress & 0x01FE));
		m_oamPriorityRotation = (value & OAMADDH_PRIORITY_ROTATION) != 0;
		m_oamAddress = m_oamBaseAddress;
		break;

	// The low table only takes a whole word, so the even byte waits for the odd one
	case OAMDATA : {
		const U16 oamAddress = m_oamAddress;

		if (!(oamAddress & 0x01)) {
			m_oamLatch = value;
		}

		if (oamAddress & OAM_HIGH_TABLE) {
			m_oam[oamIndex(oamAddress)] = value;
		} else if (oamAddress & 0x01) {
			m_oam[oamAddress - 1] = m_oamLatch;
			m_oam[oamAddress] = value;
		}

		m_oamAddress = (oamAddress + 1) & OAM_ADDRESS_MASK;
		break;
	}

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

	case TM :
		for (U32 number = 0; number < m_backgrounds.size(); number++) {
			m_backgrounds[number].onMainScreen = (value & (1u << number)) != 0;
		}

		m_spritesOnMainScreen = (value & TM_SPRITES) != 0;
		break;

	default : // TODO: the other write registers, $2106 and $211A-$2133
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

// Where each of the 4 sprite priorities falls among the backgrounds
U8 PPU::spritePriority(U32 priority) const {
	static constexpr std::array<U8, 4> MODE_0 {3, 6, 9, 12};
	static constexpr std::array<U8, 4> MODE_1 {2, 4, 7, 10};
	static constexpr std::array<U8, 4> MODE_1_BG3_ON_TOP {2, 3, 6, 9};

	switch (m_bgMode) {
	case 0 :
		return MODE_0[priority];

	case 1 :
		return m_bg3OnTop ? MODE_1_BG3_ON_TOP[priority] : MODE_1[priority];

	default : // TODO: modes 2-7
		return 0;
	}
}

void PPU::renderLine() {
	m_line.fill({});

	for (U32 number = 0; number < m_backgrounds.size(); number++) {
		renderBackground(number);
	}

	renderSprites();

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

	const U32 color = tileColor(rowAddress, format.bitsPerPixel, bit);

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

// Each sprite takes 4 bytes in the low table and 2 bits in the high one
PPU::Sprite PPU::readSprite(U32 index) const {
	const U8*  entry = &m_oam[index * 4];
	const U32  high = m_oam[OAM_HIGH_TABLE + index / 4] >> ((index & 0x03) * 2);
	const bool large = (high & 0x02) != 0;

	Sprite sprite;

	sprite.x = entry[0] | (high & 0x01) << 8;
	sprite.y = entry[1];
	sprite.tile = entry[2];
	sprite.secondTable = (entry[3] & 0x01) != 0;
	sprite.palette = (entry[3] >> 1) & 0x07;
	sprite.priority = (entry[3] >> 4) & 0x03;
	sprite.flipX = (entry[3] & 0x40) != 0;
	sprite.flipY = (entry[3] & 0x80) != 0;
	sprite.width = large ? LARGE_WIDTHS[m_spriteSize] : SMALL_WIDTHS[m_spriteSize];
	sprite.height = large ? LARGE_HEIGHTS[m_spriteSize] : SMALL_HEIGHTS[m_spriteSize];

	return sprite;
}

void PPU::renderSprites() {
	if (!m_spritesOnMainScreen) {
		return;
	}

	// Sprite Y counts from the first line on screen
	const U32 line = m_scanline - 1;

	// $2103 bit 7 makes the search start at the sprite the OAM address points to
	const U32 first = m_oamPriorityRotation ? (m_oamAddress >> 2) & 0x7F : 0;

	std::array<Sprite, MAX_SPRITES_PER_LINE> onLine {};

	U32 count = 0;

	for (U32 n = 0; n < SPRITE_COUNT && count < MAX_SPRITES_PER_LINE; n++) {
		const Sprite sprite = readSprite((first + n) & 0x7F);

		if (sprite.x > 256 && sprite.x + sprite.width - 1 < 512) {
			continue;
		}

		if (((line - sprite.y) & 0xFF) < sprite.height) {
			onLine[count++] = sprite;
		}
	}

	// Tiles are fetched from the last sprite back to the first, and only 34 fit in a line
	// So the first sprites are the ones that lose tiles, and they are drawn last, on top of the others
	std::array<LinePixel, VIDEO_WIDTH> pixels {};

	U32 tiles = 0;

	for (U32 n = count; n-- > 0 && tiles < MAX_SPRITE_TILES_PER_LINE;) {
		const Sprite& sprite = onLine[n];
		const U32     columns = sprite.width / 8;
		U32           row = (line - sprite.y) & 0xFF;

		// In the tall sizes (16x32 and 32x64) each square half flips in place, so the top half stays on top
		if (sprite.flipY) {
			if (sprite.width == sprite.height) {
				row = sprite.height - 1 - row;
			} else if (row < sprite.width) {
				row = sprite.width - 1 - row;
			} else {
				row = sprite.width * 3 - 1 - row;
			}
		}

		U32 base = m_spriteTileAddress;

		if (sprite.secondTable) {
			base += (1u + m_spriteNameGap) << 12;
		}

		// The tiles of a sprite sit in a grid 16 tiles wide, and both numbers wrap inside it
		const U32 tileRow = ((sprite.tile >> 4) + (row >> 3)) & 0x0F;

		for (U32 column = 0; column < columns && tiles < MAX_SPRITE_TILES_PER_LINE; column++) {
			const U32 x = (sprite.x + column * 8) & 0x1FF;

			// A tile off the screen takes no time, except in a sprite at X = 256
			if (sprite.x != 256 && x >= 256 && x + 7 < 512) {
				continue;
			}

			tiles++;

			const U32 tileColumn = (sprite.tile + (sprite.flipX ? columns - 1 - column : column)) & 0x0F;
			const U32 rowAddress = ((base + ((tileRow << 4 | tileColumn) << 4)) & 0x7FF0) + (row & 0x07);

			for (U32 pixel = 0; pixel < 8; pixel++) {
				const U32 screenX = (x + pixel) & 0x1FF;
				const U32 color = tileColor(rowAddress, 4, sprite.flipX ? pixel : 7 - pixel);

				if (screenX < VIDEO_WIDTH && color != 0) {
					pixels[screenX] = {
					    spritePriority(sprite.priority),
					    static_cast<U8>(SPRITE_PALETTE_BASE + (sprite.palette << 4) + color),
					};
				}
			}
		}
	}

	// Only now the sprites meet the backgrounds, each pixel with the priority of the sprite that won it
	for (U32 x = 0; x < VIDEO_WIDTH; x++) {
		if (pixels[x].priority > m_line[x].priority) {
			m_line[x] = pixels[x];
		}
	}
}

// The color of one pixel in a row of a tile
// Each word holds 2 bits of each pixel of the row. With 4 bits, the other two come 8 words later
U32 PPU::tileColor(U32 rowAddress, U32 bitsPerPixel, U32 bit) const {
	U32 color = 0;

	for (U32 pair = 0; pair < bitsPerPixel / 2; pair++) {
		const U16 planes = m_vram[(rowAddress + pair * 8) & VRAM_ADDRESS_MASK];

		color |= (((planes >> bit) & 0x01) | (((planes >> (bit + 8)) & 0x01) << 1)) << (pair * 2);
	}

	return color;
}

} // namespace SNES
