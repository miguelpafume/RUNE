#include "Bus.hpp"

#include "Cartridge.hpp"

#include <cstddef>
#include <limits>

namespace SNES {

// The 65816 uses 24 address lines, so anything above bank $FF wraps
constexpr U32         ADDRESS_MASK = 0x00FFFFFF;
constexpr std::size_t TEST_MEMORY_SIZE = 0x00FFFFFF + 1; // 16 MB, flat

constexpr std::size_t EXPECTED_DIRTY_PER_CASE = 16;
constexpr std::size_t EXPECTED_CYCLES_PER_CASE = 16;

constexpr U32 BLOCK_OFFSET_MASK = Bus::BLOCK_SIZE - 1;

constexpr U32 WRAM_START = 0x7E0000;       // banks $7E-$7F hold all of it
constexpr U32 WRAM_MIRROR_SIZE = 0x2000;   // the first 8 KiB also show up at $0000-$1FFF
constexpr U32 WRAM_ADDRESS_MASK = 0x1FFFF; // the port address has 17 bits

// Real hardware starts WRAM with garbage. bsnes and MesenCE fill it with random bytes,
// but then two runs of the same ROM could differ, so we use snes9x's fixed $55
constexpr U8 WRAM_POWER_ON_VALUE = 0x55;

// External registers, all at $21xx
constexpr U16 PPU_FIRST = 0x2100;
constexpr U16 PPU_LAST = 0x213F;
constexpr U16 APU_FIRST = 0x2140; // four ports, mirrored up to $217F
constexpr U16 APU_LAST = 0x217F;
constexpr U16 WMDATA = 0x2180;
constexpr U16 WMADDL = 0x2181;
constexpr U16 WMADDM = 0x2182;
constexpr U16 WMADDH = 0x2183;

// CPU registers
constexpr U16 JOYPAD_FIRST = 0x4016; // old serial joypad ports
constexpr U16 JOYPAD_LAST = 0x4017;
constexpr U16 INTERNAL_FIRST = 0x4200; // NMI/IRQ, multiply and divide, auto joypad
constexpr U16 INTERNAL_LAST = 0x421F;
constexpr U16 DMA_FIRST = 0x4300; // 8 channels x 16 bytes
constexpr U16 DMA_LAST = 0x437F;

Bus::Bus(Mode mode) : m_mode {mode} {
	if (m_mode == Mode::Test) {
		m_testMemory.assign(TEST_MEMORY_SIZE, 0);
		m_dirty.reserve(EXPECTED_DIRTY_PER_CASE);
		m_accessLog.reserve(EXPECTED_CYCLES_PER_CASE);
		return;
	}

	m_wram.assign(WRAM_SIZE, WRAM_POWER_ON_VALUE);
	mapConsole();
}

// runs before the cycle does anything, so a write past the limit never reaches memory
void Bus::spendCycle() {
	if (m_accessLog.size() >= m_cycleBudget)
		throw CycleBudgetExhausted {};
}

U8 Bus::readCycle(U32 address, CycleKind kind, bool memoryLock) {
	if (m_mode == Mode::Test) {
		spendCycle();

		const U32 target = address & ADDRESS_MASK;
		const U8  value = m_testMemory[target];

		m_accessLog.push_back({target, value, kind, false, m_flagE, m_flagM, m_flagX, memoryLock});

		return value;
	}

	// TODO: charge the region's cost in master cycles
	m_openBus = readSystem(address & ADDRESS_MASK);

	return m_openBus;
}

// Loads one entry of the case's "initial.ram"
void Bus::poke(U32 address, U8 value) {
	const U32 target = address & ADDRESS_MASK;

	m_testMemory[target] = value;
	m_dirty.push_back(target);
}

// Test mode reads the flat memory for the comparator
// System mode skips the registers, reading some of them changes their state
U8 Bus::peek(U32 address) const {
	const U32 target = address & ADDRESS_MASK;

	if (m_mode == Mode::Test) {
		return m_testMemory[target];
	}

	const U32 block = target >> BLOCK_SHIFT;

	if (const U8* data = m_readMap[block]) {
		return data[target & BLOCK_OFFSET_MASK];
	}

	if (m_blockType[block] == BlockType::Sram) {
		return m_cartridge->readSram(target);
	}

	return m_openBus;
}

void Bus::beginCase() {
	for (const U32 address : m_dirty) {
		m_testMemory[address] = 0;
	}

	m_dirty.clear();
	m_accessLog.clear();
	m_cycleBudget = std::numeric_limits<std::size_t>::max();
}

void Bus::writeData(U32 address, U8 value, bool memoryLock) {
	if (m_mode == Mode::Test) {
		spendCycle();

		const U32 target = address & ADDRESS_MASK;

		m_testMemory[target] = value;

		m_dirty.push_back(target);
		m_accessLog.push_back({target, value, CycleKind::DataAccess, true, m_flagE, m_flagM, m_flagX, memoryLock});

		return;
	}

	// TODO: charge the region's cost in master cycles
	m_openBus = value;
	writeSystem(address & ADDRESS_MASK, value);
}

void Bus::internalCycle(U32 address, bool memoryLock) {
	if (m_mode == Mode::Test) {
		spendCycle();

		const U32 target = address & ADDRESS_MASK;

		m_accessLog.push_back({target, std::nullopt, CycleKind::Internal, false, m_flagE, m_flagM, m_flagX, memoryLock});
	}
}

void Bus::invalidWrite(U32 address, U8 value, bool memoryLock) {
	if (m_mode == Mode::Test) {
		spendCycle();

		const U32 target = address & ADDRESS_MASK;

		m_accessLog.push_back({target, value, CycleKind::Internal, true, m_flagE, m_flagM, m_flagX, memoryLock});
	}
}

void Bus::haltedCycle() {
	if (m_mode == Mode::Test) {
		spendCycle();

		m_accessLog.push_back({0, std::nullopt, CycleKind::Internal});
	}
}

void Bus::setFlags(bool emulation, bool flagM, bool flagX) {
	m_flagE = emulation;
	m_flagM = flagM;
	m_flagX = flagX;
}

U8 Bus::readSystem(U32 address) {
	const U32 block = address >> BLOCK_SHIFT;

	if (const U8* data = m_readMap[block]) {
		return data[address & BLOCK_OFFSET_MASK];
	}

	switch (m_blockType[block]) {
	case BlockType::ExternalRegisters :
		return readExternalRegister(address);

	case BlockType::CpuRegisters :
		return readCpuRegister(address);

	case BlockType::Sram :
		return m_cartridge->readSram(address);

	case BlockType::OpenBus :
		break;
	}

	return m_openBus;
}

void Bus::writeSystem(U32 address, U8 value) {
	const U32 block = address >> BLOCK_SHIFT;

	if (U8* data = m_writeMap[block]) {
		data[address & BLOCK_OFFSET_MASK] = value;
		return;
	}

	switch (m_blockType[block]) {
	case BlockType::ExternalRegisters :
		writeExternalRegister(address, value);
		break;

	case BlockType::CpuRegisters :
		writeCpuRegister(address, value);
		break;

	case BlockType::Sram :
		m_cartridge->writeSram(address, value);
		break;

	case BlockType::OpenBus :
		break;
	}
}

void Bus::mapBlock(U32 address, const U8* read, U8* write, BlockType type) {
	const U32 block = (address & ADDRESS_MASK) >> BLOCK_SHIFT;

	m_readMap[block] = read;
	m_writeMap[block] = write;
	m_blockType[block] = type;
}

void Bus::mapRange(U8 bankFirst, U8 bankLast, U16 addressFirst, U16 addressLast, BlockType type) {
	for (U32 bank = bankFirst; bank <= bankLast; bank++) {
		for (U32 address = addressFirst; address <= addressLast; address += BLOCK_SIZE) {
			mapBlock(bank << 16 | address, nullptr, nullptr, type);
		}
	}
}

// WRAM and registers, the same for every cartridge
// The cartridge never maps these addresses, so it doesn't matter who maps first
void Bus::mapConsole() {
	for (U32 offset = 0; offset < WRAM_SIZE; offset += BLOCK_SIZE) {
		mapBlock(WRAM_START + offset, &m_wram[offset], &m_wram[offset], BlockType::OpenBus);
	}

	mapSystemArea(0x00, 0x3F);
	mapSystemArea(0x80, 0xBF);
}

// $0000-$5FFF of banks $00-$3F and $80-$BF: WRAM mirror and both register areas
void Bus::mapSystemArea(U8 bankFirst, U8 bankLast) {
	for (U32 bank = bankFirst; bank <= bankLast; bank++) {
		for (U32 offset = 0; offset < WRAM_MIRROR_SIZE; offset += BLOCK_SIZE) {
			mapBlock(bank << 16 | offset, &m_wram[offset], &m_wram[offset], BlockType::OpenBus);
		}
	}

	mapRange(bankFirst, bankLast, 0x2000, 0x3FFF, BlockType::ExternalRegisters);
	mapRange(bankFirst, bankLast, 0x4000, 0x5FFF, BlockType::CpuRegisters);
}

// Chips outside the CPU only see the low byte of the address, so they answer at $21xx
// The rest of $2000-$3FFF reaches nothing and reads as open bus
U8 Bus::readExternalRegister(U32 address) {
	const U16 offset = address & 0xFFFF;

	if (offset >= PPU_FIRST && offset <= PPU_LAST) {
		// TODO: PPU registers
		return m_openBus;
	}

	if (offset >= APU_FIRST && offset <= APU_LAST) {
		// TODO: APU stub, port offset & 3
		return m_openBus;
	}

	// $2181-$2183 can only be written
	if (offset == WMDATA) {
		const U8 value = m_wram[m_wramAddress];
		m_wramAddress = (m_wramAddress + 1) & WRAM_ADDRESS_MASK;

		return value;
	}

	return m_openBus;
}

void Bus::writeExternalRegister(U32 address, U8 value) {
	const U16 offset = address & 0xFFFF;

	if (offset >= PPU_FIRST && offset <= PPU_LAST) {
		// TODO: PPU registers
		return;
	}

	if (offset >= APU_FIRST && offset <= APU_LAST) {
		// TODO: APU stub, port offset & 3
		return;
	}

	switch (offset) {
	case WMDATA :
		m_wram[m_wramAddress] = value;
		m_wramAddress = (m_wramAddress + 1) & WRAM_ADDRESS_MASK;
		break;

	case WMADDL :
		m_wramAddress = (m_wramAddress & 0x1FF00) | value;
		break;

	case WMADDM :
		m_wramAddress = (m_wramAddress & 0x100FF) | U32 {value} << 8;
		break;

	case WMADDH : // only bit 0 is used, it picks bank $7E or $7F
		m_wramAddress = (m_wramAddress & 0x0FFFF) | U32 {value & 1u} << 16;
		break;

	default :
		break;
	}
}

U8 Bus::readCpuRegister(U32 address) {
	const U16 offset = address & 0xFFFF;

	if (offset >= JOYPAD_FIRST && offset <= JOYPAD_LAST) {
		// TODO: serial joypad, some bits come from open bus
		return m_openBus;
	}

	if (offset >= INTERNAL_FIRST && offset <= INTERNAL_LAST) {
		// TODO: $4210-$421F status, results and auto joypad
		return m_openBus;
	}

	if (offset >= DMA_FIRST && offset <= DMA_LAST) {
		// TODO: DMA channel registers
		return m_openBus;
	}

	return m_openBus;
}

// value stays unused until a CPU register is implemented
void Bus::writeCpuRegister(U32 address, [[maybe_unused]] U8 value) {
	const U16 offset = address & 0xFFFF;

	if (offset >= JOYPAD_FIRST && offset <= JOYPAD_LAST) {
		// TODO: joypad latch
		return;
	}

	if (offset >= INTERNAL_FIRST && offset <= INTERNAL_LAST) {
		// TODO: $4200-$420D NMI/IRQ enable, multiply and divide, DMA start, FastROM
		return;
	}

	if (offset >= DMA_FIRST && offset <= DMA_LAST) {
		// TODO: DMA channel registers
		return;
	}
}

} // namespace SNES