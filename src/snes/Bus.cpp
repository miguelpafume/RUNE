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

Bus::Bus(Mode mode) : m_mode {mode} {
	if (m_mode == Mode::Test) {
		m_testMemory.assign(TEST_MEMORY_SIZE, 0);
		m_dirty.reserve(EXPECTED_DIRTY_PER_CASE);
		m_accessLog.reserve(EXPECTED_CYCLES_PER_CASE);
	}
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
	case BlockType::Sram :
		return m_cartridge->readSram(address);

	case BlockType::Io : // TODO: decode $2100-$21FF and $4000-$43FF
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
	case BlockType::Sram :
		m_cartridge->writeSram(address, value);
		break;

	case BlockType::Io : // TODO: decode $2100-$21FF and $4000-$43FF
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

} // namespace SNES