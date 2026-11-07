#pragma once

#include "rune/Bits.hpp"
#include "rune/Types.hpp"

namespace SNES {

using RUNE::mask;
using RUNE::U16;
using RUNE::U32;
using RUNE::U8;

// The standard controller, a 16-bit shift register that the console reads one bit at a time
class Gamepad {
public:
	// Same bits as $4218-$4219, B is the first one out. The 4 lowest are always 0
	static constexpr U16 BUTTON_R = mask<4>;
	static constexpr U16 BUTTON_L = mask<5>;
	static constexpr U16 BUTTON_X = mask<6>;
	static constexpr U16 BUTTON_A = mask<7>;
	static constexpr U16 BUTTON_Y = mask<14>;
	static constexpr U16 BUTTON_B = mask<15>;
	static constexpr U16 BUTTON_RIGHT = mask<8>;
	static constexpr U16 BUTTON_LEFT = mask<9>;
	static constexpr U16 BUTTON_DOWN = mask<10>;
	static constexpr U16 BUTTON_UP = mask<11>;
	static constexpr U16 BUTTON_START = mask<12>;
	static constexpr U16 BUTTON_SELECT = mask<13>;

	void setButtons(U16 buttons) { m_buttons = buttons; }

	void latch(bool high);
	U8   data();

private:
	U16  m_buttons {}; // held right now, set by the front-end
	U16  m_shift {};   // copied when the latch goes low, read from bit 15 down
	U32  m_counter {};
	bool m_latched {};
};

} // namespace SNES
