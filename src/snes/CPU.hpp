#pragma once

#include "rune/Types.hpp"

#include "snes/Bus.hpp"

namespace SNES {

using RUNE::U16;
using RUNE::U32;
using RUNE::U8;

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
	void             setRegisters(const Registers& registers) { m_registers = registers; }

	void step();

private:
	Bus&      m_bus;
	Registers m_registers {};

	U32 programAddress() const;
};

} // namespace SNES