#include "Bus.hpp"

#include <cstddef>
#include <limits>

namespace SNES {

// The 65816 uses 24 address lines, so anything above bank $FF wraps
constexpr U32         ADDRESS_MASK = 0x00FFFFFF;
constexpr std::size_t TEST_MEMORY_SIZE = 0x00FFFFFF + 1; // 16 MB, flat

constexpr std::size_t EXPECTED_DIRTY_PER_CASE = 16;
constexpr std::size_t EXPECTED_CYCLES_PER_CASE = 16;

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

	return 0;
}

// Loads one entry of the case's "initial.ram"
void Bus::poke(U32 address, U8 value) {
	const U32 target = address & ADDRESS_MASK;

	m_testMemory[target] = value;
	m_dirty.push_back(target);
}

// Reads one entry of the case's "final.ram" for the comparator
U8 Bus::peek(U32 address) const { return m_testMemory[address & ADDRESS_MASK]; }

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
	}
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

} // namespace SNES