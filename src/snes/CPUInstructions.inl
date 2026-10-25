#pragma once

#include "CPU.hpp"
#include "CPUMemory.inl"
#include "rune/Types.hpp"

namespace SNES {

// Instructions that don't split into pattern + operation, each one is written whole

// Family 2: transfers

// the width is the destination
template <bool W8, CPU::Register FROM, CPU::Register TO>
void CPU::transfer() {
	idleImplied();

	const Word<W8> value = narrow<W8>(reg(FROM));

	assign<W8>(reg(TO), value);
	setNZ<W8>(value);
}

// TCS and TXS don't set flags, and in emulation they only change the low byte of S
template <CPU::Register FROM>
void CPU::transferToS() {
	idleImplied();

	const U16 value = reg(FROM);

	m_registers.S = m_registers.E ? EMULATION_STACK_PAGE | (value & LOW_BYTE_MASK) : value;
}

// Swaps A and B, N and Z come from the new low byte no matter what M is
void CPU::instructionXBA() {
	idleImplied();
	idleImplied();

	m_registers.A = static_cast<U16>(m_registers.A >> 8 | m_registers.A << 8);

	setNZ<true>(static_cast<U8>(m_registers.A));
}

// Family 3: flags and mode

// none of these flags change a width
template <U8 MASK, bool SET>
void CPU::changeFlag() {
	idleImplied();

	setFlag(MASK, SET);
}

// Clears the bits that are set in the operand
// goes through setP, so in emulation M and X stay at 1
void CPU::instructionREP() {
	const U8 mask = fetch();

	idle(lastFetchAddress());

	setP(m_registers.P & ~mask);
}

// Sets the bits of the operand
void CPU::instructionSEP() {
	const U8 mask = fetch();

	idle(lastFetchAddress());

	setP(m_registers.P | mask);
}

// The only way in and out of emulation mode: E and C trade places
void CPU::instructionXCE() {
	idleImplied();

	const bool carry = flag(FLAG_C);

	setFlag(FLAG_C, m_registers.E);
	setE(carry);
}

// Family 4: stack
// Pushes go high byte first, so the value ends up low byte first in memory

// 6502 instructions, with the page $01 wrap (push/pull)

template <bool W8, CPU::Register R>
void CPU::pushRegister() {
	idleImplied();

	const U16 value = reg(R);

	if constexpr (!W8) {
		push(static_cast<U8>(value >> 8));
	}

	push(static_cast<U8>(value));
}

// PHP, PHB and PHK push a single byte
void CPU::instructionPHP() {
	idleImplied();

	push(m_registers.P);
}

void CPU::instructionPHB() {
	idleImplied();

	push(m_registers.DB);
}

void CPU::instructionPHK() {
	idleImplied();

	push(m_registers.PB);
}

// PLA, PLX, PLY, pulls take two internal cycles before reading
template <bool W8, CPU::Register R>
void CPU::pullRegister() {
	idleImplied();
	idleImplied();

	Word<W8> value = pull();

	if constexpr (!W8) {
		value |= static_cast<U16>(pull() << 8);
	}

	assign<W8>(reg(R), value);
	setNZ<W8>(value);
}

// Goes through setP, the pulled P can change M and X
void CPU::instructionPLP() {
	idleImplied();
	idleImplied();

	setP(pull());
}

// 65816 instructions, S moves in 16 bits until the end (pushNative/pullNative)

void CPU::instructionPHD() {
	idleImplied();

	pushNative(static_cast<U8>(m_registers.D >> 8));
	pushNative(static_cast<U8>(m_registers.D));

	endNativeStack();
}

// Pushes the 16 bit operand, there's no internal cycle
void CPU::instructionPEA() {
	const U16 value = fetchWord();

	pushNative(static_cast<U8>(value >> 8));
	pushNative(static_cast<U8>(value));
	endNativeStack();
}

// Pushes a word read from direct page, same locator as [dp] (no emulation page wrap)
void CPU::instructionPEI() {
	const U8 offset = fetch();

	idleDirect();

	const U16 value = readAt<false>(atDirectNative(offset));

	pushNative(static_cast<U8>(value >> 8));
	pushNative(static_cast<U8>(value));
	endNativeStack();
}

// Pushes PC + displacement, PC is already pointing at the next instruction
void CPU::instructionPER() {
	const U16 displacement = fetchWord();

	idle(lastFetchAddress());

	const U16 value = static_cast<U16>(m_registers.PC + displacement);

	pushNative(static_cast<U8>(value >> 8));
	pushNative(static_cast<U8>(value));
	endNativeStack();
}

// D is always 16 bit, so the flags are too
void CPU::instructionPLD() {
	idleImplied();
	idleImplied();

	const U8 low = pullNative();
	const U8 high = pullNative();

	m_registers.D = static_cast<U16>(high << 8 | low);

	setNZ<false>(m_registers.D);
	endNativeStack();
}

// Only one byte, but in emulation with S = $01FF it comes from $0200
void CPU::instructionPLB() {
	idleImplied();
	idleImplied();

	m_registers.DB = pullNative();

	setNZ<true>(m_registers.DB);
	endNativeStack();
}

} // namespace SNES
