#include "DMA.hpp"

#include "Bus.hpp"

namespace SNES {

// Last digit of each channel register, $43xN
constexpr U8 DMAP = 0x0;
constexpr U8 BBAD = 0x1;
constexpr U8 A1TL = 0x2;
constexpr U8 A1TH = 0x3;
constexpr U8 A1B = 0x4;
constexpr U8 DASL = 0x5;
constexpr U8 DASH = 0x6;
constexpr U8 DASB = 0x7;
constexpr U8 A2AL = 0x8;
constexpr U8 A2AH = 0x9;
constexpr U8 NTRL = 0xA;
constexpr U8 UNUSED = 0xB;
constexpr U8 UNUSED_MIRROR = 0xF;

constexpr U8 DMAP_MODE_MASK = 0x07;
constexpr U8 DMAP_FIXED = 0x08;
constexpr U8 DMAP_DECREMENT = 0x10;
constexpr U8 DMAP_B_TO_A = 0x80;

constexpr U32 CYCLES_ALIGN = 8; // the DMA only starts on a multiple of 8
constexpr U32 CYCLES_START = 8; // once per transfer
constexpr U32 CYCLES_CHANNEL = 8;
constexpr U32 CYCLES_BYTE = 8;

constexpr U32 B_BUS_BASE = 0x2100;
constexpr U32 WMDATA = 0x2180;

// What to add to the B address on each byte, repeating every 4 bytes
constexpr std::array<std::array<U8, 4>, 8> MODE_OFFSETS {{
    {0, 0, 0, 0}, // mode 0: one register
    {0, 1, 0, 1}, // mode 1: two registers, the VRAM pair
    {0, 0, 0, 0}, // mode 2: one register, written twice
    {0, 0, 1, 1}, // mode 3: two registers, two bytes each
    {0, 1, 2, 3}, // mode 4: four registers
    {0, 1, 0, 1}, // mode 5: same as 1
    {0, 0, 0, 0}, // mode 6: same as 2
    {0, 0, 1, 1}, // mode 7: same as 3
}};

// The A side can't reach the registers: $21xx, $4000-$421F and $4300-$437F
static bool validOnBusA(U32 address) {
	return (address & 0x40FF00) != 0x2100 && (address & 0x40FE00) != 0x4000 && (address & 0x40FFE0) != 0x4200 &&
	       (address & 0x40FF80) != 0x4300;
}

static bool isWram(U32 address) { return (address & 0xFE0000) == 0x7E0000 || (address & 0x40E000) == 0x000000; }

U8 DMA::readRegister(U16 address, U8 openBus) const {
	const Channel& channel = m_channels[(address >> 4) & 0x07];

	switch (address & 0x0F) {
	case DMAP :
		return channel.control;
	case BBAD :
		return channel.bAddress;
	case A1TL :
		return static_cast<U8>(channel.aAddress);
	case A1TH :
		return static_cast<U8>(channel.aAddress >> 8);
	case A1B :
		return channel.aBank;
	case DASL :
		return static_cast<U8>(channel.size);
	case DASH :
		return static_cast<U8>(channel.size >> 8);
	case DASB :
		return channel.indirectBank;
	case A2AL :
		return static_cast<U8>(channel.tableAddress);
	case A2AH :
		return static_cast<U8>(channel.tableAddress >> 8);
	case NTRL :
		return channel.lineCounter;
	case UNUSED :
	case UNUSED_MIRROR :
		return channel.unused;
	default :
		return openBus;
	}
}

void DMA::writeRegister(U16 address, U8 value) {
	Channel& channel = m_channels[(address >> 4) & 0x07];

	switch (address & 0x0F) {
	case DMAP :
		channel.control = value;
		break;
	case BBAD :
		channel.bAddress = value;
		break;
	case A1TL :
		channel.aAddress = (channel.aAddress & 0xFF00) | value;
		break;
	case A1TH :
		channel.aAddress = static_cast<U16>((channel.aAddress & 0x00FF) | (value << 8));
		break;
	case A1B :
		channel.aBank = value;
		break;
	case DASL :
		channel.size = (channel.size & 0xFF00) | value;
		break;
	case DASH :
		channel.size = static_cast<U16>((channel.size & 0x00FF) | (value << 8));
		break;
	case DASB :
		channel.indirectBank = value;
		break;
	case A2AL :
		channel.tableAddress = (channel.tableAddress & 0xFF00) | value;
		break;
	case A2AH :
		channel.tableAddress = static_cast<U16>((channel.tableAddress & 0x00FF) | (value << 8));
		break;
	case NTRL :
		channel.lineCounter = value;
		break;
	case UNUSED :
	case UNUSED_MIRROR :
		channel.unused = value;
		break;
	default :
		break;
	}
}

void DMA::run(U8 channels, U32 cpuCycle) {
	m_elapsed = 0;

	wait(CYCLES_ALIGN - static_cast<U32>(m_bus.masterCycles() % CYCLES_ALIGN));
	wait(CYCLES_START);

	for (U32 number = 0; number < m_channels.size(); number++) {
		if (channels & (1u << number)) {
			runChannel(m_channels[number]);
		}
	}

	// The CPU only comes back at the start of one of its cycles
	wait(cpuCycle - m_elapsed % cpuCycle);
}

void DMA::wait(U32 cycles) {
	m_bus.advanceClock(cycles);
	m_elapsed += cycles;
}

void DMA::runChannel(Channel& channel) {
	const auto& offsets = MODE_OFFSETS[channel.control & DMAP_MODE_MASK];

	wait(CYCLES_CHANNEL);

	U32 index = 0;

	do {
		copyByte(channel, static_cast<U8>(channel.bAddress + offsets[index % offsets.size()]));
		index++;

		if (!(channel.control & DMAP_FIXED)) {
			if (channel.control & DMAP_DECREMENT) {
				channel.aAddress--;
			} else {
				channel.aAddress++;
			}
		}

		channel.size--;
	} while (channel.size != 0);
}

void DMA::copyByte(const Channel& channel, U8 bAddress) {
	const U32 addressA = U32 {channel.aBank} << 16 | channel.aAddress;
	const U32 addressB = B_BUS_BASE | bAddress;

	// WRAM can't copy to itself through its own port
	const bool validB = addressB != WMDATA || !isWram(addressA);
	const bool validA = validOnBusA(addressA);

	wait(CYCLES_BYTE);

	if (channel.control & DMAP_B_TO_A) {
		m_bus.dmaWrite(addressA, m_bus.dmaRead(addressB, validB), validA);
	} else {
		m_bus.dmaWrite(addressB, m_bus.dmaRead(addressA, validA), validB);
	}
}

} // namespace SNES
