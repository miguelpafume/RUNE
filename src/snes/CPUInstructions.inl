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

// Family 7: branches
// PC wraps inside the bank, PB doesn't change

// not taken costs only the operand fetch
template <U8 MASK, bool SET>
void CPU::branchIf() {
	const S8 displacement = static_cast<S8>(fetch());

	if (flag(MASK) == SET) {
		takeBranch(displacement);
	}
}

// emulation keeps the 6502 penalty for landing on another page
void CPU::takeBranch(S8 displacement) {
	const U16 target = static_cast<U16>(m_registers.PC + displacement);

	if (m_registers.E && (target & PAGE_MASK) != (m_registers.PC & PAGE_MASK)) {
		idle(lastFetchAddress());
	}

	idle(lastFetchAddress());

	m_registers.PC = target;
}

void CPU::instructionBRA() { takeBranch(static_cast<S8>(fetch())); }

// no page penalty here
void CPU::instructionBRL() {
	const U16 displacement = fetchWord();

	idle(lastFetchAddress());

	m_registers.PC = static_cast<U16>(m_registers.PC + displacement);
}

// Family 7: jumps
// JMP stays in the bank, JML also changes PB

void CPU::instructionJMPAbsolute() { m_registers.PC = fetchWord(); }

void CPU::instructionJMPLong() {
	const U32 target = fetchLong();

	m_registers.PC = static_cast<U16>(target);
	m_registers.PB = static_cast<U8>(target >> 16);
}

// the pointer is always read from bank $00
void CPU::instructionJMPIndirect() {
	const U16 pointer = fetchWord();

	m_registers.PC = readAt<false>(atBankZero(pointer));
}

// the pointer is read from the program bank
void CPU::instructionJMPIndexedIndirect() {
	const U16 pointer = fetchWord();

	idle(lastFetchAddress());

	m_registers.PC = readAt<false>(atProgramBank(pointer + m_registers.X));
}

// 3 byte pointer, also in bank $00
void CPU::instructionJMPIndirectLong() {
	const U16 pointer = fetchWord();
	const U32 target = readLongPointer(atBankZero(pointer));

	m_registers.PC = static_cast<U16>(target);
	m_registers.PB = static_cast<U8>(target >> 16);
}

// Family 7: subroutines and returns
// The return address pushed is the last byte of the call, RTS and RTL add 1

void CPU::instructionJSRAbsolute() {
	const U16 target = fetchWord();

	idle(lastFetchAddress());

	const U16 returnAddress = static_cast<U16>(m_registers.PC - 1);

	push(static_cast<U8>(returnAddress >> 8));
	push(static_cast<U8>(returnAddress));

	m_registers.PC = target;
}

// the pushes happen between the two pointer fetches, PC is already on the last byte
// it's a 65816 instruction but it uses push, S stays in page $01
void CPU::instructionJSRIndexedIndirect() {
	const U8 low = fetch();

	push(static_cast<U8>(m_registers.PC >> 8));
	push(static_cast<U8>(m_registers.PC));

	const U8 high = fetch();

	idle(lastFetchAddress());

	const U16 pointer = static_cast<U16>(high << 8 | low);

	m_registers.PC = readAt<false>(atProgramBank(pointer + m_registers.X));
}

// the last idle is on S
void CPU::instructionRTS() {
	idleImplied();
	idleImplied();

	const U8 low = pull();
	const U8 high = pull();

	idle(m_registers.S);

	m_registers.PC = static_cast<U16>((high << 8 | low) + 1);
}

// PB goes on the stack before the bank byte is fetched
void CPU::instructionJSL() {
	const U16 target = fetchWord();

	pushNative(m_registers.PB);
	idle(stackAddress(1));

	const U8  bank = fetch();
	const U16 returnAddress = static_cast<U16>(m_registers.PC - 1);

	pushNative(static_cast<U8>(returnAddress >> 8));
	pushNative(static_cast<U8>(returnAddress));

	m_registers.PC = target;
	m_registers.PB = bank;

	endNativeStack();
}

// + 1 only on PC, it doesn't carry into PB
void CPU::instructionRTL() {
	idleImplied();
	idleImplied();

	const U8 low = pullNative();
	const U8 high = pullNative();

	m_registers.PB = pullNative();
	m_registers.PC = static_cast<U16>((high << 8 | low) + 1);

	endNativeStack();
}

// no + 1 here, and no PB in emulation
void CPU::instructionRTI() {
	idleImplied();
	idleImplied();

	setP(pull());

	const U8 low = pull();
	const U8 high = pull();

	m_registers.PC = static_cast<U16>(high << 8 | low);

	if (!m_registers.E) {
		m_registers.PB = pull();
	}
}

// Family 8: interrupts and the rest

// the byte after the opcode is a signature, fetching it makes the return address skip it
// TODO: IRQ and NMI reuse this without the signature fetch, and with bit 4 of P clear
template <U16 NATIVE_VECTOR, U16 EMULATION_VECTOR>
void CPU::softwareInterrupt() {
	fetch();

	if (!m_registers.E) {
		push(m_registers.PB);
	}

	push(static_cast<U8>(m_registers.PC >> 8));
	push(static_cast<U8>(m_registers.PC));

	// in emulation X is always 1, so bit 4 goes out set and the handler can tell BRK from IRQ
	push(m_registers.P);

	setFlag(FLAG_I, true);
	setFlag(FLAG_D, false);

	const U16 vector = m_registers.E ? EMULATION_VECTOR : NATIVE_VECTOR;
	const U8  low = m_bus.fetchVector(vector);
	const U8  high = m_bus.fetchVector(vector + 1);

	m_registers.PC = static_cast<U16>(high << 8 | low);
	m_registers.PB = 0;
}

// one byte per run, PC goes back to the opcode until A wraps to $FFFF
template <bool X8, int STEP>
void CPU::blockMove() {
	// the destination bank comes first in the code, the assembler writes it last
	const U8 destinationBank = fetch();
	const U8 sourceBank = fetch();
	m_registers.DB = destinationBank;

	const U32 source = static_cast<U32>(sourceBank) << 16 | m_registers.X;
	const U32 destination = static_cast<U32>(destinationBank) << 16 | m_registers.Y;

	write(destination, read(source));
	idle(destination);
	idle(destination);

	assign<X8>(m_registers.X, static_cast<Word<X8>>(narrow<X8>(m_registers.X) + STEP));
	assign<X8>(m_registers.Y, static_cast<Word<X8>>(narrow<X8>(m_registers.Y) + STEP));

	if (m_registers.A-- != 0) {
		m_registers.PC -= 3;
	}
}

// the suite ends the case on the first halted cycle, step() does the ones after it
// TODO: any interrupt ends WAI, even with I set
void CPU::instructionWAI() {
	idleImplied();
	idleImplied();

	m_waiting = true;
	m_bus.haltedCycle();
}

// TODO: only a reset ends STP
void CPU::instructionSTP() {
	idleImplied();
	idleImplied();

	m_stopped = true;
	m_bus.haltedCycle();
}

void CPU::instructionNOP() { idleImplied(); }

// reserved, the second byte is skipped with an internal cycle instead of a fetch
void CPU::instructionWDM() {
	idleImplied();

	m_registers.PC++;
}

} // namespace SNES
