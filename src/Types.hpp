#pragma once

#include <cstdint>

namespace RUNE {

using U8 = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;

using S8 = std::int8_t;
using S16 = std::int16_t;
using S32 = std::int32_t;

} // namespace RUNE

namespace SNES {

// NTSC master clock. Every other timing constant derives from this one
constexpr RUNE::U32 MASTER_CLOCK_HZ = 21477272;

constexpr RUNE::U32 CYCLES_PER_DOT      = 4;
constexpr RUNE::U32 DOTS_PER_SCANLINE   = 341;
constexpr RUNE::U32 CYCLES_PER_SCANLINE = DOTS_PER_SCANLINE * CYCLES_PER_DOT; // 1364
constexpr RUNE::U32 SCANLINES_PER_FRAME = 262;
constexpr RUNE::U32 CYCLES_PER_FRAME    = CYCLES_PER_SCANLINE * SCANLINES_PER_FRAME;

// Memory access costs, in master cycles. The 65C816 has no uniform cycle time,
// the bus holds it for a different number of master cycles depending on which region is being addressed
constexpr RUNE::U32 CYCLES_FAST  = 6;   // internal registers, FastROM
constexpr RUNE::U32 CYCLES_SLOW  = 8;   // WRAM, SlowROM, cartridge
constexpr RUNE::U32 CYCLES_XSLOW = 12;  // $4000-$41FF (old-style joypad registers)

constexpr RUNE::U32 VIDEO_WIDTH  = 256;
constexpr RUNE::U32 VIDEO_HEIGHT = 224;

// First scanline of vertical blanking in 224-line mode.
constexpr RUNE::U32 VBLANK_START_LINE = 225;

// TODO: make this a user setting instead of a compile-time constant
constexpr float DISPLAY_ASPECT = static_cast<float>(VIDEO_WIDTH) / static_cast<float>(VIDEO_HEIGHT);

} // namespace SNES