#pragma once

#include "rune/Types.hpp"

namespace SNES {

using RUNE::U16;
using RUNE::U32;
using RUNE::U8;

// The standard controller, a 16-bit shift register that the console reads one bit at a time
class Gamepad {
public:
	// Same bits as $4218-$4219, B is the first one out. The 4 lowest are always 0
	static constexpr U16 BUTTON_R = 0x0010;
	static constexpr U16 BUTTON_L = 0x0020;
	static constexpr U16 BUTTON_X = 0x0040;
	static constexpr U16 BUTTON_A = 0x0080;
	static constexpr U16 BUTTON_Y = 0x4000;
	static constexpr U16 BUTTON_B = 0x8000;
	static constexpr U16 BUTTON_RIGHT = 0x0100;
	static constexpr U16 BUTTON_LEFT = 0x0200;
	static constexpr U16 BUTTON_DOWN = 0x0400;
	static constexpr U16 BUTTON_UP = 0x0800;
	static constexpr U16 BUTTON_START = 0x1000;
	static constexpr U16 BUTTON_SELECT = 0x2000;

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
