#pragma once

#include "rune/Types.hpp"

#include <array>

namespace SNES {

using RUNE::U8;

// Audio is out of scope, so there is no SPC700 here
class APU {
public:
	U8   readPort(U8 port) const { return m_toCpu[port]; }
	void writePort(U8 port, U8 value);

private:
	static constexpr U8 IPL_READY_0 = 0xAA;
	static constexpr U8 IPL_READY_1 = 0xBB;
	static constexpr U8 IPL_KICK = 0xCC;

	std::array<U8, 4> m_toCpu {IPL_READY_0, IPL_READY_1, 0x00, 0x00};

	bool m_kicked {};
};

} // namespace SNES
