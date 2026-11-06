#include "Gamepad.hpp"

namespace SNES {

constexpr U32 BIT_COUNT = 16;

// The cross can't press both sides at once, so a keyboard holding both reads as neither
static U16 withoutOpposites(U16 buttons) {
	if ((buttons & Gamepad::BUTTON_UP) && (buttons & Gamepad::BUTTON_DOWN)) {
		buttons &= static_cast<U16>(~(Gamepad::BUTTON_UP | Gamepad::BUTTON_DOWN));
	}

	if ((buttons & Gamepad::BUTTON_LEFT) && (buttons & Gamepad::BUTTON_RIGHT)) {
		buttons &= static_cast<U16>(~(Gamepad::BUTTON_LEFT | Gamepad::BUTTON_RIGHT));
	}

	return buttons;
}

// The buttons are copied when the latch goes low, and every change starts the reading over
void Gamepad::latch(bool high) {
	if (m_latched == high) {
		return;
	}

	m_latched = high;
	m_counter = 0;

	if (!high) {
		m_shift = withoutOpposites(m_buttons);
	}
}

U8 Gamepad::data() {
	// After the 16 bits the line stays high
	if (m_counter >= BIT_COUNT) {
		return 1;
	}

	// While latched the register keeps reloading, so it only shows B
	if (m_latched) {
		return (m_buttons & BUTTON_B) ? 1 : 0;
	}

	return static_cast<U8>((m_shift >> (BIT_COUNT - 1 - m_counter++)) & 1);
}

} // namespace SNES
