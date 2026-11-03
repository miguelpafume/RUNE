#include "APU.hpp"

namespace SNES {

// IPL ignores port 0 until it sees $CC
void APU::writePort(U8 port, U8 value) {
	if (port != 0) {
		return;
	}

	if (value == IPL_KICK) {
		m_kicked = true;
	}

	if (m_kicked) {
		m_toCpu[0] = value;
	}
}

} // namespace SNES
