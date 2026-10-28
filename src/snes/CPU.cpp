#include "CPU.hpp"

// The templates are in .inl files so only CPU.cpp includes them
// Keep this order, each file uses stuff from the ones above
#include "CPUMemory.inl"

#include "CPUPatterns.inl"

#include "CPUOperations.inl"

#include "CPUInstructions.inl"

#include "CPUTable.inl"

#include <cstddef>

namespace SNES {

// Tables are built at compile time, a missing handler won't compile
// clang-format off
constinit const std::array<CPU::Table, 4> CPU::TABLES = {
	buildTable<false, false>(),
	buildTable<false, true>(),
	buildTable<true, false>(),
	buildTable<true, true>()
};
// clang-format on

CPU::CPU(Bus& bus) : m_bus {bus} {}

// Execution

void CPU::step() {
	// WAI/STP: the clock keeps going but nothing is fetched
	if (m_stopped || m_waiting) {
		m_bus.haltedCycle();
		return;
	}

	const bool flagM = flag(FLAG_M);
	const bool flagX = flag(FLAG_X);

	// E/M/X pins use the state from the start of the instruction
	m_bus.setFlags(m_registers.E, flagM, flagX);

	const U8 opcode = m_bus.fetchOpcode(programAddress());
	m_registers.PC++;

	// M and X choose the table, so each handler already knows its widths when compiled
	const std::size_t table = (flagM ? 2 : 0) | (flagX ? 1 : 0);
	(this->*TABLES[table][opcode])();
}

// Used by the tests to load a state, emulation rules still get applied
void CPU::setRegisters(const Registers& registers) {
	m_registers = registers;
	m_waiting = false;
	m_stopped = false;

	setE(m_registers.E);
	setP(m_registers.P);
}

// Status

void CPU::setFlag(U8 mask, bool set) {
	if (set) {
		m_registers.P |= mask;
	} else {
		m_registers.P &= ~mask;
	}
}

// All writes to P go through here so the width rules are always applied
void CPU::setP(U8 value) {
	if (m_registers.E)
		value |= FLAG_M | FLAG_X; // set M and X to 8 bit

	m_registers.P = value;

	// with 8bit index the high bytes for X and Y are lost
	if (value & FLAG_X) {
		m_registers.X &= LOW_BYTE_MASK;
		m_registers.Y &= LOW_BYTE_MASK;
	}
}

// Emulation forces 8 bit widths and puts the stack in page $01
void CPU::setE(bool emulation) {
	m_registers.E = emulation;

	if (emulation) {
		setP(m_registers.P);
		m_registers.S = EMULATION_STACK_PAGE | (m_registers.S & LOW_BYTE_MASK);
	}
}

// N is the top bit of the value, Z is set when it's zero
template <bool W8>
void CPU::setNZ(Word<W8> value) {
	setFlag(FLAG_N, value & signBit<W8>());
	setFlag(FLAG_Z, value == 0);
}

// Registers

// Cuts the register down to the operand width
template <bool W8>
CPU::Word<W8> CPU::narrow(U16 value) {
	return static_cast<Word<W8>>(value);
}

// In 8bit only the low byte is replaced
template <bool W8>
void CPU::assign(U16& target, Word<W8> value) {
	if constexpr (W8) {
		target = (target & HIGH_BYTE_MASK) | value;
	} else {
		target = value;
	}
}

// So one handler can work with any register (TAX, INX, PHA...) via template
U16& CPU::reg(Register id) {
	switch (id) {
	case Register::A :
		return m_registers.A;
	case Register::X :
		return m_registers.X;
	case Register::Y :
		return m_registers.Y;
	case Register::S :
		return m_registers.S;
	case Register::D :
		return m_registers.D;
	}

	return m_registers.A;
}

// X or Y for the indexed modes, picked at compile time
template <CPU::Index I>
U16 CPU::index() const {
	return I == Index::X ? m_registers.X : m_registers.Y;
}

} // namespace SNES
