#include "DMA.hpp"

#include "Bus.hpp"

namespace SNES {

using HdmaRequest = Bus::HdmaRequest;

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
constexpr U8 DMAP_INDIRECT = 0x40; // HDMA only
constexpr U8 DMAP_B_TO_A = 0x80;

constexpr U32 CYCLES_ALIGN = 8; // the DMA only starts on a multiple of 8
constexpr U32 CYCLES_START = 8; // once per transfer
constexpr U32 CYCLES_CHANNEL = 8;
constexpr U32 CYCLES_BYTE = 8;
constexpr U32 CYCLES_HDMA_START = 8; // once per frame setup and once per line

// The first byte of each table entry: how many lines, and whether to copy on all of them
constexpr U8 LINE_COUNT_MASK = 0x7F;
constexpr U8 LINE_REPEAT = 0x80;

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

// Bytes HDMA copies in each line, for each mode
constexpr std::array<U32, 8> HDMA_LENGTHS {1, 2, 2, 4, 4, 4, 2, 4};

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

// When both are waiting, HDMA goes first, then the DMA
void DMA::run(U8 channels, U32 cpuCycle) {
	const HdmaRequest request = m_bus.takeHdma();

	if (request != HdmaRequest::None && m_hdmaEnable != 0) {
		startTransfer();
		request == HdmaRequest::Setup ? hdmaSetup() : hdmaRun();
		returnToCpu(cpuCycle);
	}

	if (channels == 0) {
		return;
	}

	m_dmaEnable = channels;

	startTransfer();
	wait(CYCLES_START);
	serviceHdma();

	for (U32 number = 0; number < m_channels.size(); number++) {
		if (m_dmaEnable & (1u << number)) {
			runChannel(number);
		}
	}

	m_dmaEnable = 0;

	returnToCpu(cpuCycle);
}

void DMA::startTransfer() {
	m_elapsed = 0;
	wait(CYCLES_ALIGN - static_cast<U32>(m_bus.masterCycles() % CYCLES_ALIGN));
}

// The CPU only comes back at the start of one of its cycles
void DMA::returnToCpu(U32 cpuCycle) { wait(cpuCycle - m_elapsed % cpuCycle); }

void DMA::wait(U32 cycles) {
	m_bus.advanceClock(cycles);
	m_elapsed += cycles;
}

void DMA::runChannel(U32 number) {
	Channel&    channel = m_channels[number];
	const auto& offsets = MODE_OFFSETS[channel.control & DMAP_MODE_MASK];
	const U8    bit = static_cast<U8>(1u << number);

	wait(CYCLES_CHANNEL);
	serviceHdma();

	U32 index = 0;

	// HDMA on this same channel stops the copy, and then the size is left as it was
	do {
		const U32 addressA = U32 {channel.aBank} << 16 | channel.aAddress;

		copyByte(channel, addressA, static_cast<U8>(channel.bAddress + offsets[index % offsets.size()]));
		index++;

		if (!(channel.control & DMAP_FIXED)) {
			if (channel.control & DMAP_DECREMENT) {
				channel.aAddress--;
			} else {
				channel.aAddress++;
			}
		}

		serviceHdma();
	} while ((m_dmaEnable & bit) && --channel.size != 0);
}

void DMA::copyByte(const Channel& channel, U32 addressA, U8 registerB) {
	const U32 addressB = B_BUS_BASE | registerB;

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

// HDMA can cut into a DMA between two bytes
void DMA::serviceHdma() {
	const HdmaRequest request = m_bus.takeHdma();

	if (request == HdmaRequest::None || m_hdmaEnable == 0) {
		return;
	}

	request == HdmaRequest::Setup ? hdmaSetup() : hdmaRun();
}

void DMA::hdmaReset() {
	for (Channel& channel : m_channels) {
		channel.hdmaCompleted = false;
		channel.hdmaDoTransfer = false;
	}
}

bool DMA::hdmaActive(U32 number) const { return (m_hdmaEnable & (1u << number)) && !m_channels[number].hdmaCompleted; }

bool DMA::hdmaActive() const {
	for (U32 number = 0; number < m_channels.size(); number++) {
		if (hdmaActive(number)) {
			return true;
		}
	}

	return false;
}

// Start of the frame: each enabled channel goes back to the top of its table
void DMA::hdmaSetup() {
	wait(CYCLES_HDMA_START);

	for (U32 number = 0; number < m_channels.size(); number++) {
		Channel& channel = m_channels[number];

		channel.hdmaDoTransfer = true;

		if (!(m_hdmaEnable & (1u << number))) {
			continue;
		}

		// HDMA stops a DMA running on the same channel
		m_dmaEnable &= static_cast<U8>(~(1u << number));

		channel.tableAddress = channel.aAddress;
		channel.lineCounter = 0;
		hdmaReload(number);
	}
}

// Each H-Blank: every channel copies its bytes for the next line, then they all move down their tables
void DMA::hdmaRun() {
	wait(CYCLES_HDMA_START);

	for (U32 number = 0; number < m_channels.size(); number++) {
		Channel& channel = m_channels[number];

		if (!hdmaActive(number))
			continue;

		m_dmaEnable &= static_cast<U8>(~(1u << number));

		if (!channel.hdmaDoTransfer)
			continue;

		const U32   mode = channel.control & DMAP_MODE_MASK;
		const auto& offsets = MODE_OFFSETS[mode];

		for (U32 index = 0; index < HDMA_LENGTHS[mode]; index++) {
			const U32 address = (channel.control & DMAP_INDIRECT) ? U32 {channel.indirectBank} << 16 | channel.size++
			                                                      : U32 {channel.aBank} << 16 | channel.tableAddress++;

			copyByte(channel, address, static_cast<U8>(channel.bAddress + offsets[index]));
		}
	}

	for (U32 number = 0; number < m_channels.size(); number++) {
		Channel& channel = m_channels[number];

		if (!hdmaActive(number)) {
			continue;
		}

		channel.lineCounter--;
		channel.hdmaDoTransfer = (channel.lineCounter & LINE_REPEAT) != 0;
		hdmaReload(number);
	}
}

// The table byte is read on every line, but only used when the entry runs out of lines
void DMA::hdmaReload(U32 number) {
	Channel& channel = m_channels[number];
	U8       data = readTable(U32 {channel.aBank} << 16 | channel.tableAddress);

	if ((channel.lineCounter & LINE_COUNT_MASK) != 0) {
		return;
	}

	channel.lineCounter = data;
	channel.tableAddress++;

	// An entry of 0 ends the table for this frame
	channel.hdmaCompleted = channel.lineCounter == 0;
	channel.hdmaDoTransfer = !channel.hdmaCompleted;

	if (channel.control & DMAP_INDIRECT) {
		data = readTable(U32 {channel.aBank} << 16 | channel.tableAddress++);
		channel.size = static_cast<U16>(data << 8);

		// The last channel to end its table skips the high byte, and the low one stays in the high half
		if (channel.hdmaCompleted && hdmaFinished(number)) {
			return;
		}

		data = readTable(U32 {channel.aBank} << 16 | channel.tableAddress++);
		channel.size = static_cast<U16>(data << 8 | channel.size >> 8);
	}
}

// No channel after this one still has a table running
bool DMA::hdmaFinished(U32 number) const {
	for (U32 next = number + 1; next < m_channels.size(); next++) {
		if (hdmaActive(next)) {
			return false;
		}
	}

	return true;
}

U8 DMA::readTable(U32 address) {
	wait(CYCLES_BYTE);

	return m_bus.dmaRead(address, validOnBusA(address));
}

} // namespace SNES
