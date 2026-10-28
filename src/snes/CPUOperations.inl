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

// Logic
// The result goes to A and sets N and Z

// ORA, bitwise or
template <bool W8>
void CPU::opORA(Word<W8> value) {
	const Word<W8> result = narrow<W8>(m_registers.A) | value;

	assign<W8>(m_registers.A, result);
	setNZ<W8>(result);
}

// AND, bitwise and
template <bool W8>
void CPU::opAND(Word<W8> value) {
	const Word<W8> result = narrow<W8>(m_registers.A) & value;

	assign<W8>(m_registers.A, result);
	setNZ<W8>(result);
}

// EOR, exclusive or
template <bool W8>
void CPU::opEOR(Word<W8> value) {
	const Word<W8> result = narrow<W8>(m_registers.A) ^ value;

	assign<W8>(m_registers.A, result);
	setNZ<W8>(result);
}

// BIT : Bit test
// A doesn't change, Z comes from A AND operand

// N and V are copied from the two top bits of the operand
template <bool W8>
void CPU::opBIT(Word<W8> value) {
	setFlag(FLAG_Z, (narrow<W8>(m_registers.A) & value) == 0);
	setFlag(FLAG_V, value & (signBit<W8>() >> 1));
	setFlag(FLAG_N, value & signBit<W8>());
}

// BIT #, there's no memory operand for N and V to describe, so only Z changes
template <bool W8>
void CPU::opBITImmediate(Word<W8> value) {
	setFlag(FLAG_Z, (narrow<W8>(m_registers.A) & value) == 0);
}

// CP : Compare
// A subtraction that only keeps the flags, C set means no borrow (register >= operand)

// S32 so the difference can go below zero
template <bool W8>
void CPU::compare(Word<W8> left, Word<W8> right) {
	const S32 result = static_cast<S32>(left) - static_cast<S32>(right);

	setFlag(FLAG_C, result >= 0);
	setNZ<W8>(static_cast<Word<W8>>(result));
}

// CMP, A with the M width
template <bool W8>
void CPU::opCMP(Word<W8> value) {
	compare<W8>(narrow<W8>(m_registers.A), value);
}

// CPX and CPY, with the X width
template <bool W8>
void CPU::opCPX(Word<W8> value) {
	compare<W8>(narrow<W8>(m_registers.X), value);
}

template <bool W8>
void CPU::opCPY(Word<W8> value) {
	compare<W8>(narrow<W8>(m_registers.Y), value);
}

// ADC and SBC : Add and subtract with carry

// Shared by ADC and SBC, SBC sends the operand already complemented
// In decimal mode each digit (4 bits) is summed and corrected one at a time
template <bool W8>
void CPU::addWithCarry(Word<W8> operand, bool subtract) {
	constexpr int BITS = W8 ? 8 : 16;
	constexpr int TOP_DIGIT = BITS - 4;
	constexpr S32 LIMIT = 1 << BITS;

	const bool decimal = flag(FLAG_D);
	const S32  left = narrow<W8>(m_registers.A);
	const S32  right = operand;

	// Brings a digit back into 0-9, +6 when it passed 9, -6 when it borrowed
	const auto correct = [subtract](S32 sum, int shift) {
		if (!subtract && sum >= (0x0A << shift)) {
			return sum + (0x06 << shift);
		}
		if (subtract && sum < (0x10 << shift)) {
			return sum - (0x06 << shift);
		}
		return sum;
	};

	S32 result {};

	if (!decimal) {
		result = left + right + flag(FLAG_C);
	} else {
		bool carry = flag(FLAG_C);

		// result & below puts back the lower digits that were already corrected
		for (int shift = 0; shift <= TOP_DIGIT; shift += 4) {
			const S32 digit = 0x0F << shift;
			const S32 below = (1 << shift) - 1;

			result = (left & digit) + (right & digit) + (static_cast<S32>(carry) << shift) + (result & below);

			// The top digit is corrected outside the loop, after V
			if (shift < TOP_DIGIT) {
				result = correct(result, shift);
				carry = result >= (0x10 << shift);
			}
		}
	}

	// Overflow: both inputs had the same sign and the result has the other one
	setFlag(FLAG_V, ~(left ^ right) & (left ^ result) & signBit<W8>());

	// The 65816 takes V before this correction and C after it
	if (decimal) {
		result = correct(result, TOP_DIGIT);
	}

	setFlag(FLAG_C, result >= LIMIT);

	const Word<W8> stored = static_cast<Word<W8>>(result);
	assign<W8>(m_registers.A, stored);
	setNZ<W8>(stored);
}

template <bool W8>
void CPU::opADC(Word<W8> value) {
	addWithCarry<W8>(value, false);
}

// A - v - borrow is the same as A + ~v + C, the cast keeps ~ in the width
template <bool W8>
void CPU::opSBC(Word<W8> value) {
	addWithCarry<W8>(static_cast<Word<W8>>(~value), true);
}

// INC and DEC : Increment and decrement

// INC, only N and Z change, C stays even when it wraps to zero
template <bool W8>
CPU::Word<W8> CPU::opINC(Word<W8> value) {
	const Word<W8> result = static_cast<Word<W8>>(value + 1);

	setNZ<W8>(result);

	return result;
}

// DEC
template <bool W8>
CPU::Word<W8> CPU::opDEC(Word<W8> value) {
	const Word<W8> result = static_cast<Word<W8>>(value - 1);

	setNZ<W8>(result);

	return result;
}

// Shifts and rotates, the bit that falls off goes to C

// ASL, shift left, bit 0 gets 0
template <bool W8>
CPU::Word<W8> CPU::opASL(Word<W8> value) {
	setFlag(FLAG_C, value & signBit<W8>());

	const Word<W8> result = static_cast<Word<W8>>(value << 1);

	setNZ<W8>(result);

	return result;
}

// LSR, shift right, the top bit gets 0 so N always ends clear
template <bool W8>
CPU::Word<W8> CPU::opLSR(Word<W8> value) {
	setFlag(FLAG_C, value & 0x01);

	const Word<W8> result = static_cast<Word<W8>>(value >> 1);

	setNZ<W8>(result);

	return result;
}

// ROL, rotate left, the old C goes into bit 0, carryIn is saved before C gets the new value
template <bool W8>
CPU::Word<W8> CPU::opROL(Word<W8> value) {
	const bool     carryIn = flag(FLAG_C);
	const Word<W8> result = static_cast<Word<W8>>(value << 1 | carryIn);

	setFlag(FLAG_C, value & signBit<W8>());
	setNZ<W8>(result);

	return result;
}

// ROR, rotate right, the old C goes into the top bit
template <bool W8>
CPU::Word<W8> CPU::opROR(Word<W8> value) {
	const bool     carryIn = flag(FLAG_C);
	const Word<W8> result = static_cast<Word<W8>>(value >> 1 | (carryIn ? signBit<W8>() : 0));

	setFlag(FLAG_C, value & 0x01);
	setNZ<W8>(result);

	return result;
}

// TSB and TRB : Test and set or reset bits

// TSB, sets in memory the bits that are set in A
template <bool W8>
CPU::Word<W8> CPU::opTSB(Word<W8> value) {
	const Word<W8> mask = narrow<W8>(m_registers.A);

	setFlag(FLAG_Z, (value & mask) == 0);

	return static_cast<Word<W8>>(value | mask);
}

// TRB, clears in memory the bits that are set in A
template <bool W8>
CPU::Word<W8> CPU::opTRB(Word<W8> value) {
	const Word<W8> mask = narrow<W8>(m_registers.A);

	setFlag(FLAG_Z, (value & mask) == 0);

	return static_cast<Word<W8>>(value & ~mask);
}

} // namespace SNES
