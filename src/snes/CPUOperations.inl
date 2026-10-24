#include "CPU.hpp"

namespace SNES {

// Operations don't touch the bus, the pattern handles that

// LD : Load
// Copies the operand into the register and sets N and Z

// LDA, with M = 1 only the low byte changes and B is kept
template <bool W8>
void CPU::opLDA(Word<W8> value) {
	assign<W8>(m_registers.A, value);
	setNZ<W8>(value);
}

// LDX
template <bool W8>
void CPU::opLDX(Word<W8> value) {
	assign<W8>(m_registers.X, value);
	setNZ<W8>(value);
}

// LDY
template <bool W8>
void CPU::opLDY(Word<W8> value) {
	assign<W8>(m_registers.Y, value);
	setNZ<W8>(value);
}

// ST : Store
// Returns the value to write, flags don't change

// STA, A cut to the M width
template <bool W8>
CPU::Word<W8> CPU::opSTA() {
	return narrow<W8>(m_registers.A);
}

// STX, X cut to the X width
template <bool W8>
CPU::Word<W8> CPU::opSTX() {
	return narrow<W8>(m_registers.X);
}

// STY, Y cut to the X width
template <bool W8>
CPU::Word<W8> CPU::opSTY() {
	return narrow<W8>(m_registers.Y);
}

// STZ, writes zero with the M width
template <bool W8>
CPU::Word<W8> CPU::opSTZ() {
	return 0;
}

} // namespace SNES
