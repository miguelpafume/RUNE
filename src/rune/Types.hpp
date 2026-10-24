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

// Masks

// low and high byte of a 16 bit value
inline constexpr U16 LOW_BYTE_MASK = 0x00FF;
inline constexpr U16 HIGH_BYTE_MASK = 0xFF00;

// TODO: possibly move those to another file

// page of an address (256 bytes each)
inline constexpr U16 PAGE_MASK = 0xFF00;

// address inside the bank
inline constexpr U16 OFFSET_MASK = 0xFFFF;

// bank and full 24 bit address
inline constexpr U32 BANK_MASK = 0xFF0000;
inline constexpr U32 ADDRESS_MASK = 0xFFFFFF;

} // namespace RUNE
