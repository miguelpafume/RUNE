#pragma once

#include "rune/Types.hpp"

#include <concepts>
#include <limits>

namespace RUNE {

// Bit ranges are inclusive and count from bit 0

// A constant with bits LOW to HIGH set
template <unsigned LOW, unsigned HIGH = LOW>
    requires(LOW <= HIGH && HIGH < 32)
inline constexpr U32 mask = (U32_MAX >> (31 - HIGH)) & (U32_MAX << LOW);

// Bits LOW to HIGH of value, moved down to bit 0
template <unsigned LOW, unsigned HIGH = LOW, std::unsigned_integral T>
constexpr T bits(T value) {
	static_assert(LOW <= HIGH && HIGH < std::numeric_limits<T>::digits, "the range has to fit in the value");

	return static_cast<T>((value >> LOW) & mask<0, HIGH - LOW>);
}

// One bit, as true or false
template <unsigned N, std::unsigned_integral T>
constexpr bool bit(T value) {
	return bits<N>(value) != 0;
}

// Low and high byte of a 16 bit value
inline constexpr U16 LOW_BYTE_MASK = mask<0, 7>;
inline constexpr U16 HIGH_BYTE_MASK = mask<8, 15>;

} // namespace RUNE
