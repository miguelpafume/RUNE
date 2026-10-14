#include "CPU.hpp"

#include "CPUInstructions.inl"
#include "CPUTable.inl"

namespace SNES {

// clang-format off
constinit const std::array<CPU::Table, 4> CPU::TABLES = {
	buildTable<false, false>(),
	buildTable<false, true>(),
	buildTable<true, false>(),
	buildTable<true, true>()
};
// clang-format on

CPU::CPU(Bus& bus) : m_bus {bus} {}

U32 CPU::programAddress() const { return (static_cast<U32>(m_registers.PB) << 16 | m_registers.PC); }

void CPU::step() {
	if (m_stopped || m_waiting) {
		m_bus.haltedCycle();
		return;
	}

	const bool flagM = flag(FLAG_M);
	const bool flagX = flag(FLAG_X);

	m_bus.setFlags(m_registers.E, flagM, flagX);

	const U8 opcode = m_bus.fetchOpcode(programAddress());
	m_registers.PC++;

	const std::size_t table = (flagM ? 2 : 0) | (flagX ? 1 : 0);
	(this->*TABLES[table][opcode])();
}

void CPU::setRegisters(const Registers& registers) {
	m_registers = registers;
	m_waiting = false;
	m_stopped = false;

	setE(m_registers.E);
	setP(m_registers.P);
}

void CPU::setFlag(U8 mask, bool set) {
	if (set) {
		m_registers.P |= mask;
	} else {
		m_registers.P &= ~mask;
	}
}

void CPU::setP(U8 value) {
	if (m_registers.E)
		value |= FLAG_M | FLAG_X; // set M and X to 8 bit

	m_registers.P = value;

	// with 8bit index the high bytes for X and Y are lost
	if (value & FLAG_X) {
		m_registers.X &= 0x00FF;
		m_registers.Y &= 0x00FF;
	}
}

void CPU::setE(bool emulation) {
	m_registers.E = emulation;

	if (emulation) {
		setP(m_registers.P);
		m_registers.S = EMULATION_STACK_PAGE | (m_registers.S & 0x00FF);
	}
}

template <bool W8>
void CPU::setNZ(UInt<W8> value) {
	setFlag(FLAG_N, value & signBit<W8>());
	setFlag(FLAG_Z, value == 0);
}

template <bool W8>
CPU::UInt<W8> CPU::narrow(U16 value) {
	return static_cast<UInt<W8>>(value);
}

// in 8bit only the low byte is replaced
template <bool W8>
void CPU::assign(U16& target, UInt<W8> value) {
	if constexpr (W8) {
		target = (target & 0xFF00) | value;
	} else {
		target = value;
	}
}

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

template <CPU::Index I>
U16 CPU::index() const {
	return I == Index::X ? m_registers.X : m_registers.Y;
}

void CPU::opcodeMissing() {}

} // namespace SNES