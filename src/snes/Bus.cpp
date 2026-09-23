#include "Bus.hpp"

#include <cstddef>

namespace SNES {

// The 65816 uses 24 address lines, so anything above bank $FF wraps
constexpr U32 ADDRESS_MASK = 0x00FFFFFF;
constexpr std::size_t TEST_MEMORY_SIZE = 0x00FFFFFF + 1; // 16 MB, flat

// A suite case seeds at most seven addresses and no opcode runs for more than
// a dozen cycles. Reserving once keeps reallocation out of the case loop.
constexpr std::size_t EXPECTED_DIRTY_PER_CASE  = 16;
constexpr std::size_t EXPECTED_CYCLES_PER_CASE = 16;

Bus::Bus(Mode mode) : m_mode {mode} {
	// An empty vector costs no allocation, so the test buffer only exists where it is used
	if (m_mode == Mode::Test) {
		m_testMemory.assign(TEST_MEMORY_SIZE, 0);
		m_dirty.reserve(EXPECTED_DIRTY_PER_CASE);
		m_accessLog.reserve(EXPECTED_CYCLES_PER_CASE);
	}
}

U8 Bus::fetchOpcode(U32 address) {
	return readCycle(address, CycleKind::OpcodeFetch);
}

U8 Bus::fetchOperand(U32 address) {
	return readCycle(address, CycleKind::OperandFetch);
}

U8 Bus::fetchVector(U32 address) {
	return readCycle(address, CycleKind::VectorFetch);
}

U8 Bus::readData(U32 address) {
	return readCycle(address, CycleKind::DataAccess);
}

U8 Bus::readCycle(U32 address, CycleKind kind) {
	if (m_mode == Mode::Test) {
		const U32 target = address & ADDRESS_MASK;
		const U8  value  = m_testMemory[target];
	
		m_accessLog.push_back({target, value, kind, false});
	
		return value;
	}

	// TODO(S5): route through the memory map and charge the region's master cycles.

	return 0;
}

// Loads one entry of the case's "initial.ram"
void Bus::poke(U32 address, U8 value) {
	const U32 target = address & ADDRESS_MASK;

	m_testMemory[target] = value;
	m_dirty.push_back(target);
}

// Reads one entry of the case's "final.ram" for the comparator
U8 Bus::peek(U32 address) const
{
	return m_testMemory[address & ADDRESS_MASK];
}

void Bus::beginCase() {
	// Restoring only the touched addresses keeps the cost proportional to the case
	for (const U32 address : m_dirty) {
		m_testMemory[address] = 0;
	}

	m_dirty.clear();
	m_accessLog.clear();
}

void Bus::writeData(U32 address, U8 value) {
	if (m_mode == Mode::Test) {
		const U32 target = address & ADDRESS_MASK;

		m_testMemory[target] = value;

		m_dirty.push_back(target);
		m_accessLog.push_back({target, value, CycleKind::DataAccess, true});
	}
}

void Bus::internalCycle(U32 address) {
	if (m_mode == Mode::Test) {
		const U32 target = address & ADDRESS_MASK;

		m_accessLog.push_back({target, std::nullopt, CycleKind::Internal, false});
	}
}

} // namespace SNES