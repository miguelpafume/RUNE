#include "CPU.hpp"

namespace SNES {

CPU::CPU(Bus& bus) : m_bus {bus} {}

U32 CPU::programAddress() const { return (static_cast<U32>(m_registers.PB) << 16 | m_registers.PC); }

void CPU::step() {
	constexpr U8 FLAG_M = 0x20;
	constexpr U8 FLAG_X = 0x10;

	const bool flagM = m_registers.E || (m_registers.P & FLAG_M);
	const bool flagX = m_registers.E || (m_registers.P & FLAG_X);

	m_bus.setFlags(m_registers.E, flagM, flagX);

	const U8 opcode = m_bus.fetchOpcode(programAddress());
	m_registers.PC++;

	(void)opcode;
}

} // namespace SNES