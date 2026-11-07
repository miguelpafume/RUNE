#pragma once

#include "rune/Bits.hpp"
#include "rune/Types.hpp"

namespace SNES {

using RUNE::U16;
using RUNE::U32;

// Bit helpers, every SNES file reads register fields with them
using RUNE::bit;
using RUNE::bits;
using RUNE::mask;

// NTSC master clock
constexpr U32 MASTER_CLOCK_HZ = 21477272;

constexpr U32 CYCLES_PER_DOT = 4;
constexpr U32 DOTS_PER_SCANLINE = 341;
constexpr U32 CYCLES_PER_SCANLINE = DOTS_PER_SCANLINE * CYCLES_PER_DOT; // 1364
constexpr U32 SCANLINES_PER_FRAME = 262;
constexpr U32 CYCLES_PER_FRAME = CYCLES_PER_SCANLINE * SCANLINES_PER_FRAME;

constexpr U32 VIDEO_WIDTH = 256;
constexpr U32 VIDEO_HEIGHT = 224;

// First scanline of vertical blanking in 224-line mode.
constexpr U32 VBLANK_START_LINE = 225;

// 65816 address space

// page of an address (256 bytes each)
constexpr U16 PAGE_MASK = mask<8, 15>;

// address inside the bank
constexpr U16 OFFSET_MASK = mask<0, 15>;

// bank and full 24 bit address
constexpr U32 BANK_MASK = mask<16, 23>;
constexpr U32 ADDRESS_MASK = mask<0, 23>;

// WRAM data port, the Bus serves it and the DMA can't copy WRAM through it
constexpr U16 WMDATA = 0x2180;

} // namespace SNES