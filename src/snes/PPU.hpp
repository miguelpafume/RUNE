#pragma once

#include "Constants.hpp"
#include "rune/Types.hpp"

#include <array>
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
	const std::vector<U8>&  oam() const { return m_oam; }
	const std::vector<U8>&  framebuffer() const { return m_framebuffer; }

private:
	// $2105, $2107-$2114 and $212C
	struct Background {
		U16  mapAddress {};  // in words
		U8   mapSize {};     // bit 0 doubles the width, bit 1 the height
		U16  tileAddress {}; // in words
		U16  hScroll {};
		U16  vScroll {};
		bool bigTiles {}; // 16x16
		bool onMainScreen {};
	};

	// Background in a video mode
	struct LayerFormat {
		U8 bitsPerPixel {}; // 0 means the mode doesn't have this layer
		U8 lowPriority {};
		U8 highPriority {}; // tiles with bit 13 of the map entry on
	};

	// One pixel of the line being drawn. Priority 0 and color 0 are the backdrop
	struct LinePixel {
		U8 priority {};
		U8 color {}; // CGRAM index
	};

	// One sprite, put back together from the two OAM tables
	struct Sprite {
		U32  x {}; // 9 bits, 256-511 is left of the screen
		U32  y {};
		U32  tile {};
		bool secondTable {};
		U32  palette {};
		U32  priority {};
		bool flipX {};
		bool flipY {};
		U32  width {};
		U32  height {};
	};

	U32 m_hClock {}; // master cycles since the line started
	U32 m_scanline {};
	U64 m_frames {};

	static constexpr std::size_t VRAM_WORDS = 0x8000; // 64 KiB
	static constexpr std::size_t CGRAM_COLORS = 256;
	static constexpr std::size_t OAM_BYTES = 544; // 512 in the low table, 32 in the high one

	std::vector<U16> m_vram = std::vector<U16>(VRAM_WORDS);
	std::vector<U16> m_cgram = std::vector<U16>(CGRAM_COLORS);
	std::vector<U8>  m_oam = std::vector<U8>(OAM_BYTES);

	U8   m_brightness {};      // $2100 bits 0-3
	bool m_forcedBlank = true; // $2100 bit 7
	U8   m_bgMode {};          // $2105 bits 0-2
	bool m_bg3OnTop {};        // $2105 bit 3

	std::array<Background, 4> m_backgrounds {};

	// Every scroll register shares these two
	U8 m_scrollLatch {};
	U8 m_hScrollLatch {};

	U16  m_vramAddress {};
	U16  m_vramIncrement = 1;
	U8   m_vramRemap {};
	bool m_vramIncrementOnHigh {};
	U16  m_vramReadBuffer {};

	U8   m_cgramAddress {};
	bool m_cgramHighByte {}; // $2122 and $213B share it, each access flips it
	U8   m_cgramLatch {};
	U8   m_ppu2LastRead {};

	U16  m_oamBaseAddress {}; // $2102-$2103, in bytes
	U16  m_oamAddress {};
	U8   m_oamLatch {};
	bool m_oamPriorityRotation {}; // $2103 bit 7

	U16  m_spriteTileAddress {}; // $2101, in words
	U8   m_spriteNameGap {};
	U8   m_spriteSize {};
	bool m_spritesOnMainScreen {}; // $212C bit 4

	std::array<LinePixel, VIDEO_WIDTH> m_line {};

	std::vector<U8> m_framebuffer = std::vector<U8>(std::size_t {VIDEO_WIDTH} * VIDEO_HEIGHT * 4); // RGBA

	// While the PPU draws, the VRAM is all its own
	bool canAccessVram() const { return m_forcedBlank || inVBlank(); }
	U16  vramIndex() const;
	U16  readVram() const;
	void stepVramAddress();

	const LayerFormat& layerFormat(U32 number) const;
	U8                 spritePriority(U32 priority) const;
	Sprite             readSprite(U32 index) const;

	void      renderLine();
	void      renderBackground(U32 number);
	LinePixel backgroundPixel(U32 number, const LayerFormat& format, U32 hPosition, U32 vPosition) const;
	void      renderSprites();
	U32       tileColor(U32 rowAddress, U32 bitsPerPixel, U32 bit) const;
};

} // namespace SNES
