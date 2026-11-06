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

	void setHdmaEnable(U8 channels) { m_hdmaEnable = channels; }

	// Called by the Bus at the start of each frame and at each H-Blank
	void hdmaReset();
	bool hdmaEnabled() const { return m_hdmaEnable != 0; }
	bool hdmaActive() const;

private:
	// Registers $43x0-$43xB
	struct Channel {
		U8  control = 0xFF;
		U8  bAddress = 0xFF;
		U16 aAddress = 0xFFFF;
		U8  aBank = 0xFF;
		U16 size = 0xFFFF; // 0 means 64 KiB. In indirect HDMA, the address of the data
		U8  indirectBank = 0xFF;
		U16 tableAddress = 0xFFFF; // where HDMA is in the table
		U8  lineCounter = 0xFF;
		U8  unused = 0xFF;

		bool hdmaCompleted {}; // the table reached its 0 entry
		bool hdmaDoTransfer {};
	};

	Bus& m_bus;

	std::array<Channel, 8> m_channels {};

	U8 m_dmaEnable {};  // channels still copying in this DMA
	U8 m_hdmaEnable {}; // $420C

	U32 m_elapsed {}; // time this DMA took, without the DRAM refresh

	void wait(U32 cycles);
	void startTransfer();
	void returnToCpu(U32 cpuCycle);
	void runChannel(U32 number);
	void copyByte(const Channel& channel, U32 addressA, U8 registerB);

	void serviceHdma();
	void hdmaSetup();
	void hdmaRun();
	void hdmaReload(U32 number);
	bool hdmaActive(U32 number) const;
	bool hdmaFinished(U32 number) const;
	U8   readTable(U32 address);
};

} // namespace SNES
