#include "CPU.hpp"

namespace SNES {

CPU::CPU(Bus& bus) : m_bus {bus} {}

U32 CPU::programAddress() const { return (static_cast<U32>(m_registers.PB) << 16 | m_registers.PC); }

void CPU::step() {
	const U8 opcode = m_bus.fetchOpcode(programAddress());
	m_registers.PC++;

	(void)opcode;
}

} // namespace SNES