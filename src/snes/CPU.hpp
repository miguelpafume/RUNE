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

	// don't use it for M and X, those go through setP
	void setFlag(U8 mask, bool set);

	// forces M and X in emulation, and X = 1 drops the high bytes of X and Y
	void setP(U8 value);

	// entering emulation also forces 8 bit and moves S to page $01
	void setE(bool emulation);

	template <bool W8>
	void setNZ(Word<W8> value);

	// only the low byte when W8
	template <bool W8>
	static Word<W8> narrow(U16 value);

	// when W8 the high byte stays, so B survives an 8 bit LDA
	template <bool W8>
	static void assign(U16& target, Word<W8> value);

	U16& reg(Register id);

	template <Index I>
	U16 index() const;

	// CPUTable.inl

	// One table for each M/X combination
	static const std::array<Table, 4> TABLES;

	template <bool M8, bool X8>
	static constexpr Table buildTable();

	// CPUMemory.inl: bus helpers

	// Program fetch

	// PB:PC
	U32 programAddress() const;

	// PB:PC - 1, most internal cycles put this on the bus
	U32 lastFetchAddress() const;

	U8  fetch();
	U16 fetchWord();
	U32 fetchLong();

	// Single cycles

	U8   read(U32 address, bool lock = false);
	void write(U32 address, U8 value, bool lock = false);

	// no access, but the address still goes on the bus
	void idle(U32 address, bool lock = false);

	// on the next opcode address
	void idleImplied();

	// only when DL isn't zero
	void idleDirect();

	// Effective addresses

	// bank $00, wraps inside the page in emulation when DL = 0
	U32 directAddress(U32 offset) const;

	// DB:address, can carry into the next bank
	U32 bankAddress(U32 address) const;

	// S + offset, bank $00
	U32 stackAddress(U32 offset) const;

	// what the bus shows before the index carry gets fixed
	static U32 uncorrectedAddress(U32 base, U16 offset);

	static bool crossesPage(U32 base, U16 offset);

	// Stack, the native ones skip the page $01 wrap until endNativeStack

	void push(U8 value);
	U8   pull();
	void pushNative(U8 value);
	U8   pullNative();

	// back to page $01 if in emulation
	void endNativeStack();

	// Operand locators, they return a lambda that gives the address of byte k

	// D + offset, with the emulation page wrap
	auto atDirect(U32 offset) const;

	// no emulation page wrap, for [dp] and PEI
	auto atDirectNative(U32 offset) const;

	// the wrap is only on offset + X, the pointer bytes go past it
	auto atDirectIndexedIndirect(U32 offset) const;

	// DB:address
	auto atBank(U32 address) const;

	// full 24 bit address, DB isn't used
	auto atLong(U32 address) const;

	// S + offset
	auto atStack(U32 offset) const;

	// pointers of JMP (abs) and JML [abs]
	auto atBankZero(U32 address) const;

	// pointers of JMP (abs,X) and JSR (abs,X)
	auto atProgramBank(U32 address) const;

	// Multi-byte accesses (one bus cycle per byte)

	template <bool W8, typename Locator>
	Word<W8> readAt(Locator at, bool lock = false);

	// low, high, bank
	template <typename Locator>
	U32 readLongPointer(Locator at);

	template <bool W8, typename Locator>
	void writeAt(Locator at, Word<W8> value);

	// read, idle and write back, all with MLB low (family 6)
	template <bool W8, auto Op, typename Locator>
	void modifyAt(Locator at);

	// CPUPatterns.inl: addressing patterns
	// each pattern does the bus cycles of one addressing mode

	// Read -> Op consumes the operand

	// #
	template <bool W8, auto Op>
	void immediateRead();

	// dp
	template <bool W8, auto Op>
	void directRead();

	// dp,X / dp,Y
	template <bool W8, Index I, auto Op>
	void directIndexedRead();

	// abs
	template <bool W8, auto Op>
	void absoluteRead();

	// abs,X / abs,Y, extra cycle with 16 bit index or a page cross
	template <bool W8, bool X8, Index I, auto Op>
	void absoluteIndexedRead();

	// long
	template <bool W8, auto Op>
	void absoluteLongRead();

	// long,X
	template <bool W8, auto Op>
	void absoluteLongIndexedRead();

	// (dp)
	template <bool W8, auto Op>
	void directIndirectRead();

	// (dp,X)
	template <bool W8, auto Op>
	void directIndexedIndirectRead();

	// (dp),Y, same extra cycle rule as abs,Y
	template <bool W8, bool X8, auto Op>
	void directIndirectIndexedRead();

	// [dp]
	template <bool W8, auto Op>
	void directIndirectLongRead();

	// [dp],Y
	template <bool W8, auto Op>
	void directIndirectLongIndexedRead();

	// sr,S
	template <bool W8, auto Op>
	void stackRelativeRead();

	// (sr,S),Y
	template <bool W8, auto Op>
	void stackRelativeIndirectIndexedRead();

	// Write -> Op gives the value
	// same addresses as the reads, but the indexed ones always take the extra cycle

	// dp
	template <bool W8, auto Op>
	void directWrite();

	// dp,X / dp,Y
	template <bool W8, Index I, auto Op>
	void directIndexedWrite();

	// abs
	template <bool W8, auto Op>
	void absoluteWrite();

	// abs,X / abs,Y
	template <bool W8, Index I, auto Op>
	void absoluteIndexedWrite();

	// long
	template <bool W8, auto Op>
	void absoluteLongWrite();

	// long,X
	template <bool W8, auto Op>
	void absoluteLongIndexedWrite();

	// (dp)
	template <bool W8, auto Op>
	void directIndirectWrite();

	// (dp,X)
	template <bool W8, auto Op>
	void directIndexedIndirectWrite();

	// (dp),Y
	template <bool W8, auto Op>
	void directIndirectIndexedWrite();

	// [dp]
	template <bool W8, auto Op>
	void directIndirectLongWrite();

	// [dp],Y
	template <bool W8, auto Op>
	void directIndirectLongIndexedWrite();

	// sr,S
	template <bool W8, auto Op>
	void stackRelativeWrite();

	// (sr,S),Y
	template <bool W8, auto Op>
	void stackRelativeIndirectIndexedWrite();

	// Modify -> Op changes the operand (family 6)

	// A, X or Y (ASL A, INC A, INX, DEY...)
	template <bool W8, Register R, auto Op>
	void impliedModify();

	// dp
	template <bool W8, auto Op>
	void directModify();

	// dp,X
	template <bool W8, auto Op>
	void directIndexedModify();

	// abs
	template <bool W8, auto Op>
	void absoluteModify();

	// abs,X, the index cycle always happens
	template <bool W8, auto Op>
	void absoluteIndexedModify();

	// CPUOperations.inl: opcodes

	// LD: Load

	template <bool W8>
	void opLDA(Word<W8> value);

	template <bool W8>
	void opLDX(Word<W8> value);

	template <bool W8>
	void opLDY(Word<W8> value);

	// ST: Store

	template <bool W8>
	Word<W8> opSTA();

	template <bool W8>
	Word<W8> opSTX();

	template <bool W8>
	Word<W8> opSTY();

	template <bool W8>
	Word<W8> opSTZ();

	// Logic

	template <bool W8>
	void opORA(Word<W8> value);

	template <bool W8>
	void opAND(Word<W8> value);

	template <bool W8>
	void opEOR(Word<W8> value);

	// BIT: Bit test

	// N and V come from the operand, only Z uses the AND
	template <bool W8>
	void opBIT(Word<W8> value);

	// BIT #, only Z changes
	template <bool W8>
	void opBITImmediate(Word<W8> value);

	// CP: Compare

	// C set means no borrow (left >= right)
	template <bool W8>
	void compare(Word<W8> left, Word<W8> right);

	template <bool W8>
	void opCMP(Word<W8> value);

	template <bool W8>
	void opCPX(Word<W8> value);

	template <bool W8>
	void opCPY(Word<W8> value);

	// ADC and SBC: Add and subtract with carry

	// shared by ADC and SBC, decimal mode included
	template <bool W8>
	void addWithCarry(Word<W8> operand, bool subtract);

	template <bool W8>
	void opADC(Word<W8> value);

	// ADC with the operand complemented
	template <bool W8>
	void opSBC(Word<W8> value);

	// INC and DEC: Increment and decrement

	// also INX and INY, C doesn't change
	template <bool W8>
	Word<W8> opINC(Word<W8> value);

	// also DEX and DEY
	template <bool W8>
	Word<W8> opDEC(Word<W8> value);

	// Shifts and rotates

	template <bool W8>
	Word<W8> opASL(Word<W8> value);

	template <bool W8>
	Word<W8> opLSR(Word<W8> value);

	template <bool W8>
	Word<W8> opROL(Word<W8> value);

	template <bool W8>
	Word<W8> opROR(Word<W8> value);

	// TSB and TRB: Test and set or reset bits

	// Z from A AND the old value, N and V don't change
	template <bool W8>
	Word<W8> opTSB(Word<W8> value);

	// same Z as TSB
	template <bool W8>
	Word<W8> opTRB(Word<W8> value);

	// CPUInstructions.inl: whole instructions, no pattern + operation

	// Family 2: transfers

	// TAX, TAY, TXA, TYA, TXY, TYX, TSX, TSC, TCD, TDC, with the width of TO
	template <bool W8, Register FROM, Register TO>
	void transfer();

	// TCS and TXS, no flags
	template <Register FROM>
	void transferToS();

	void instructionXBA();

	// Family 3: flags and mode

	// CLC, SEC, CLI, SEI, CLD, SED, CLV
	template <U8 MASK, bool SET>
	void changeFlag();

	void instructionREP();
	void instructionSEP();
	void instructionXCE();

	// Family 4: stack, 6502 instructions (push/pull)

	// PHA, PHX, PHY
	template <bool W8, Register R>
	void pushRegister();

	void instructionPHP();
	void instructionPHB();
	void instructionPHK();

	// PLA, PLX, PLY
	template <bool W8, Register R>
	void pullRegister();

	void instructionPLP();

	// Family 4: stack, 65816 instructions (pushNative/pullNative)

	void instructionPHD();
	void instructionPEA();
	void instructionPEI();
	void instructionPER();
	void instructionPLD();
	void instructionPLB();

	// Family 7: branches

	// BPL, BMI, BVC, BVS, BCC, BCS, BNE, BEQ
	template <U8 MASK, bool SET>
	void branchIf();

	void instructionBRA();
	void instructionBRL();
	void takeBranch(S8 displacement);

	// Family 7: jumps

	// JMP abs
	void instructionJMPAbsolute();

	// JML long
	void instructionJMPLong();

	// JMP (abs)
	void instructionJMPIndirect();

	// JMP (abs,X)
	void instructionJMPIndexedIndirect();

	// JML [abs]
	void instructionJMPIndirectLong();

	// Family 7: subroutines and returns

	// JSR abs
	void instructionJSRAbsolute();

	// JSR (abs,X)
	void instructionJSRIndexedIndirect();

	void instructionRTS();
	void instructionJSL();
	void instructionRTL();
	void instructionRTI();

	// Family 8: interrupts and the rest

	// BRK and COP
	template <U16 NATIVE_VECTOR, U16 EMULATION_VECTOR>
	void softwareInterrupt();

	// MVN (STEP +1) and MVP (STEP -1)
	template <bool X8, int STEP>
	void blockMove();

	void instructionWAI();
	void instructionSTP();
	void instructionNOP();
	void instructionWDM();
};

} // namespace SNES
