#pragma once

#include <cstdint>
#include <limits>

namespace RUNE {

using U8 = std::uint8_t;
using U16 = std::uint16_t;
using U32 = std::uint32_t;
using U64 = std::uint64_t;

using S8 = std::int8_t;
using S16 = std::int16_t;
using S32 = std::int32_t;

inline constexpr U32 U32_MAX = std::numeric_limits<U32>::max();

inline constexpr U32 KIB = 1024;

} // namespace RUNE
