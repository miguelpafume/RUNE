#pragma once

#include "rune/Types.hpp"

namespace SNES {

using namespace RUNE;

// NTSC master clock
constexpr U32 MASTER_CLOCK_HZ = 21477272;

constexpr U32 CYCLES_PER_DOT = 4;
constexpr U32 DOTS_PER_SCANLINE = 341;
constexpr U32 CYCLES_PER_SCANLINE = DOTS_PER_SCANLINE * CYCLES_PER_DOT; // 1364
constexpr U32 SCANLINES_PER_FRAME = 262;
constexpr U32 CYCLES_PER_FRAME = CYCLES_PER_SCANLINE * SCANLINES_PER_FRAME;

// Memory access costs, in master cycles
constexpr U32 CYCLES_FAST = 6;   // internal registers, FastROM
constexpr U32 CYCLES_SLOW = 8;   // WRAM, SlowROM, cartridge
constexpr U32 CYCLES_XSLOW = 12; // $4000-$41FF (old-style joypad registers)

constexpr U32 VIDEO_WIDTH = 256;
constexpr U32 VIDEO_HEIGHT = 224;

// First scanline of vertical blanking in 224-line mode.
constexpr U32 VBLANK_START_LINE = 225;

// TODO: make this a user setting instead of a compile-time constant
constexpr float DISPLAY_ASPECT = static_cast<float>(VIDEO_WIDTH) / static_cast<float>(VIDEO_HEIGHT);

constexpr U32 KIB = 1024;

} // namespace SNES