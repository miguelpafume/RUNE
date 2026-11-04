#pragma once

#include "rune/Types.hpp"

#include <array>

namespace SNES {

using RUNE::U16;
using RUNE::U32;
using RUNE::U8;

class Bus;

class DMA {
public:
	explicit DMA(Bus& bus) : m_bus {bus} {}

	U8   readRegister(U16 address, U8 openBus) const;
	void writeRegister(U16 address, U8 value);

	void run(U8 channels, U32 cpuCycle);

private:
	// Registers $43x0-$43xB
	struct Channel {
		U8  control = 0xFF;
		U8  bAddress = 0xFF;
		U16 aAddress = 0xFFFF;
		U8  aBank = 0xFF;
		U16 size = 0xFFFF; // 0 means 64 KiB
		U8  indirectBank = 0xFF;
		U16 tableAddress = 0xFFFF;
		U8  lineCounter = 0xFF;
		U8  unused = 0xFF;
	};

	Bus& m_bus;

	std::array<Channel, 8> m_channels {};

	U32 m_elapsed {}; // time this DMA took, without the DRAM refresh

	void wait(U32 cycles);
	void runChannel(Channel& channel);
	void copyByte(const Channel& channel, U8 bAddress);
};

} // namespace SNES
