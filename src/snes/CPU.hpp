#pragma once

#include "rune/Types.hpp"

#include "snes/Bus.hpp"

#include <array>
#include <cstddef>
#include <type_traits>

namespace SNES {

using RUNE::U16;
using RUNE::U32;
using RUNE::U8;

// Ricoh 5A22
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

	void step();

private:
	void opcodeMissing(); // REMOVE WHEN ALL OPCODES ARE WRITTEN

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
	// Emulation mode doesn't have BRK vector on its own, it shared the IRQ vector
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

	static constexpr bool LOCK = true;

	using Handler = void (CPU::*)();
	using Table = std::array<Handler, 256>; // 256 opcodes

	template <bool W8>
	using UInt = std::conditional_t<W8, U8, U16>;

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

	Bus&      m_bus;
	Registers m_registers {};
	bool      m_waiting {}; // WAI: halted until an interrupt
	bool      m_stopped {}; // STP: halted until reset

	static const std::array<Table, 4> TABLES;

	template <bool M8, bool X8>
	static constexpr Table buildTable();

	bool flag(U8 mask) const { return m_registers.P & mask; }
	void setFlag(U8 mask, bool set);
	void setP(U8 value);
	void setE(bool emulation);

	template <bool W8>
	void setNZ(UInt<W8> value);

	template <bool W8>
	static UInt<W8> narrow(U16 value);

	template <bool W8>
	static void assign(U16& target, UInt<W8> value);

	U16& reg(Register id);

	template <Index I>
	U16 index() const;

	U32 programAddress() const;
};

} // namespace SNES