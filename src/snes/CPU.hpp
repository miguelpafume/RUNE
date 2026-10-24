#pragma once

#include "rune/Types.hpp"

#include "snes/Bus.hpp"

#include <array>
#include <type_traits>

using namespace RUNE;

namespace SNES {

// Ricoh 5A22 (65C816 core), all bus cycles go through Bus which moves the clock
class CPU {
public:
	explicit CPU(Bus& bus);

	struct Registers {
		U16 A {};  // Accumulator (low byte A / high byte B)
		U16 X {};  // Index
		U16 Y {};  // Index
		U16 PC {}; // Program counter
		U16 S {};  // Stack pointer
		U16 D {};  // Zero-page offset (direct page)
		U8  P {};  // Processor status
		U8  DB {}; // Data bank
		U8  PB {}; // Program counter bank

		bool E {}; // Emulation mode

		bool operator==(const Registers&) const = default;
	};

	const Registers& getRegisters() const { return m_registers; }
	void             setRegisters(const Registers& registers);

	// Runs a full instruction
	void step();

private:
	void opcodeMissing(); // REMOVE WHEN ALL OPCODES ARE WRITTEN

	// Constants

	// Status registers (P) bits
	static constexpr U8 FLAG_C = 0x01; // Carry
	static constexpr U8 FLAG_Z = 0x02; // Zero
	static constexpr U8 FLAG_I = 0x04; // IRQ disable
	static constexpr U8 FLAG_D = 0x08; // Decimal
	static constexpr U8 FLAG_X = 0x10; // Index width 8 bits (Break in the pushed copy, emulation mode)
	static constexpr U8 FLAG_M = 0x20; // Accumulator width 8 bits
	static constexpr U8 FLAG_V = 0x40; // Overflow
	static constexpr U8 FLAG_N = 0x80; // Negative

	// Interrupt vectors, all in bank $00.
	// Emulation mode doesn't have a BRK vector of its own, it shares the IRQ vector
	static constexpr U16 VECTOR_NATIVE_COP = 0xFFE4;
	static constexpr U16 VECTOR_NATIVE_BRK = 0xFFE6;
	static constexpr U16 VECTOR_NATIVE_ABORT = 0xFFE8;
	static constexpr U16 VECTOR_NATIVE_NMI = 0xFFEA;
	static constexpr U16 VECTOR_NATIVE_IRQ = 0xFFEE;
	static constexpr U16 VECTOR_EMULATION_COP = 0xFFF4;
	static constexpr U16 VECTOR_EMULATION_ABORT = 0xFFF8;
	static constexpr U16 VECTOR_EMULATION_NMI = 0xFFFA;
	static constexpr U16 VECTOR_EMULATION_RESET = 0xFFFC;
	static constexpr U16 VECTOR_EMULATION_IRQ_BRK = 0xFFFE;

	// In emulation, the stack is in page $01
	static constexpr U16 EMULATION_STACK_PAGE = 0x0100;

	// Locks the bus (MLB low) during read-modify-write
	static constexpr bool LOCK = true;

	// Types

	using Handler = void (CPU::*)();
	using Table = std::array<Handler, 256>; // 256 opcodes

	// U8 when the width is 8 bit (M or X set), U16 otherwise
	template <bool W8>
	using Word = std::conditional_t<W8, U8, U16>;

	// Top bit for the width, used for the N flag
	template <bool W8>
	static constexpr U16 signBit() {
		return W8 ? 0x0080 : 0x8000;
	}

	enum class Index {
		X,
		Y
	};

	enum class Register {
		A,
		X,
		Y,
		S,
		D
	};

	// State

	Bus&      m_bus;
	Registers m_registers {};
	bool      m_waiting {}; // WAI: halted until an interrupt
	bool      m_stopped {}; // STP: halted until reset

	// CPU.cpp: status and registers

	bool flag(U8 mask) const { return m_registers.P & mask; }
	void setFlag(U8 mask, bool set);
	void setP(U8 value);
	void setE(bool emulation);

	template <bool W8>
	void setNZ(Word<W8> value);

	template <bool W8>
	static Word<W8> narrow(U16 value);

	template <bool W8>
	static void assign(U16& target, Word<W8> value);

	U16& reg(Register id);

	template <Index I>
	U16 index() const;

	// CPUTable.inl

	// One table for each M/X combination, index is (M << 1) | X
	static const std::array<Table, 4> TABLES;

	template <bool M8, bool X8>
	static constexpr Table buildTable();

	// CPUMemory.inl: bus helpers

	// Program fetch
	U32 programAddress() const;
	U32 lastFetchAddress() const;
	U8  fetch();
	U16 fetchWord();
	U32 fetchLong();

	// Single cycles
	U8   read(U32 address, bool lock = false);
	void write(U32 address, U8 value, bool lock = false);
	void idle(U32 address, bool lock = false);
	void idleImplied();
	void idleDirect();

	// Effective addresses
	U32         directAddress(U32 offset) const;
	U32         bankAddress(U32 address) const;
	U32         stackAddress(U32 offset) const;
	static U32  uncorrectedAddress(U32 base, U16 offset);
	static bool crossesPage(U32 base, U16 offset);

	// Stack
	void push(U8 value);
	U8   pull();
	void pushNative(U8 value);
	U8   pullNative();
	void endNativeStack();

	// Operand locators, they return a lambda that gives the address of byte k
	auto atDirect(U32 offset) const;
	auto atDirectNative(U32 offset) const;
	auto atDirectIndexedIndirect(U32 offset) const;
	auto atBank(U32 address) const;
	auto atLong(U32 address) const;
	auto atStack(U32 offset) const;
	auto atBankZero(U32 address) const;    // JMP (a), JML [a]
	auto atProgramBank(U32 address) const; // JMP (a,X), JSR (a,X)

	// Multi-byte accesses (one bus cycle per byte)
	template <bool W8, typename Locator>
	Word<W8> readAt(Locator at, bool lock = false);

	template <typename Locator>
	U32 readLongPointer(Locator at);

	template <bool W8, typename Locator>
	void writeAt(Locator at, Word<W8> value);

	template <bool W8, auto Op, typename Locator>
	void modifyAt(Locator at); // family 6

	// CPUPatterns.inl: addressing patterns
	// each pattern does the bus cycles of one addressing mode

	// Read -> Op consumes the operand

	template <bool W8, auto Op>
	void immediateRead();

	template <bool W8, auto Op>
	void directRead();

	template <bool W8, Index I, auto Op>
	void directIndexedRead();

	template <bool W8, auto Op>
	void absoluteRead();

	template <bool W8, bool X8, Index I, auto Op>
	void absoluteIndexedRead();

	template <bool W8, auto Op>
	void absoluteLongRead();

	template <bool W8, auto Op>
	void absoluteLongIndexedRead();

	template <bool W8, auto Op>
	void directIndirectRead();

	template <bool W8, auto Op>
	void directIndexedIndirectRead();

	template <bool W8, bool X8, auto Op>
	void directIndirectIndexedRead();

	template <bool W8, auto Op>
	void directIndirectLongRead();

	template <bool W8, auto Op>
	void directIndirectLongIndexedRead();

	template <bool W8, auto Op>
	void stackRelativeRead();

	template <bool W8, auto Op>
	void stackRelativeIndirectIndexedRead();

	// Write -> Op gives the value

	template <bool W8, auto Op>
	void directWrite();

	template <bool W8, Index I, auto Op>
	void directIndexedWrite();

	template <bool W8, auto Op>
	void absoluteWrite();

	template <bool W8, Index I, auto Op>
	void absoluteIndexedWrite();

	template <bool W8, auto Op>
	void absoluteLongWrite();

	template <bool W8, auto Op>
	void absoluteLongIndexedWrite();

	template <bool W8, auto Op>
	void directIndirectWrite();

	template <bool W8, auto Op>
	void directIndexedIndirectWrite();

	template <bool W8, auto Op>
	void directIndirectIndexedWrite();

	template <bool W8, auto Op>
	void directIndirectLongWrite();

	template <bool W8, auto Op>
	void directIndirectLongIndexedWrite();

	template <bool W8, auto Op>
	void stackRelativeWrite();

	template <bool W8, auto Op>
	void stackRelativeIndirectIndexedWrite();

	// Modify -> Op changes the operand (family 6)

	template <bool W8, Register R, auto Op>
	void impliedModify();

	template <bool W8, auto Op>
	void directModify();

	template <bool W8, auto Op>
	void directIndexedModify();

	template <bool W8, auto Op>
	void absoluteModify();

	template <bool W8, auto Op>
	void absoluteIndexedModify();

	// CPUOperations.inl: opcodes

	// LD : Load

	template <bool W8>
	void opLDA(Word<W8> value);

	template <bool W8>
	void opLDX(Word<W8> value);

	template <bool W8>
	void opLDY(Word<W8> value);

	// ST : Store

	template <bool W8>
	Word<W8> opSTA();

	template <bool W8>
	Word<W8> opSTX();

	template <bool W8>
	Word<W8> opSTY();

	template <bool W8>
	Word<W8> opSTZ();
};

} // namespace SNES
