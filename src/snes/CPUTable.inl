#include "CPU.hpp"

namespace SNES {

// Each opcode is a pattern (bus cycles) + an operation (the math)
// LDA, STA and STZ use the M width, LDX, LDY, STX and STY use the X width
template <bool M8, bool X8>
constexpr CPU::Table CPU::buildTable() {
	Table t {};

	t.fill(&CPU::opcodeMissing); // REMOVE WHEN ALL OPCODES ARE WRITTEN

	// Family 1, loads

	// LDA
	t[0xA9] = &CPU::immediateRead<M8, &CPU::opLDA<M8>>;                     // LDA #
	t[0xA5] = &CPU::directRead<M8, &CPU::opLDA<M8>>;                        // LDA dp
	t[0xB5] = &CPU::directIndexedRead<M8, Index::X, &CPU::opLDA<M8>>;       // LDA dp,X
	t[0xAD] = &CPU::absoluteRead<M8, &CPU::opLDA<M8>>;                      // LDA abs
	t[0xBD] = &CPU::absoluteIndexedRead<M8, X8, Index::X, &CPU::opLDA<M8>>; // LDA abs,X
	t[0xB9] = &CPU::absoluteIndexedRead<M8, X8, Index::Y, &CPU::opLDA<M8>>; // LDA abs,Y
	t[0xAF] = &CPU::absoluteLongRead<M8, &CPU::opLDA<M8>>;                  // LDA long
	t[0xBF] = &CPU::absoluteLongIndexedRead<M8, &CPU::opLDA<M8>>;           // LDA long,X
	t[0xB2] = &CPU::directIndirectRead<M8, &CPU::opLDA<M8>>;                // LDA (dp)
	t[0xA1] = &CPU::directIndexedIndirectRead<M8, &CPU::opLDA<M8>>;         // LDA (dp,X)
	t[0xB1] = &CPU::directIndirectIndexedRead<M8, X8, &CPU::opLDA<M8>>;     // LDA (dp),Y
	t[0xA7] = &CPU::directIndirectLongRead<M8, &CPU::opLDA<M8>>;            // LDA [dp]
	t[0xB7] = &CPU::directIndirectLongIndexedRead<M8, &CPU::opLDA<M8>>;     // LDA [dp],Y
	t[0xA3] = &CPU::stackRelativeRead<M8, &CPU::opLDA<M8>>;                 // LDA sr,S
	t[0xB3] = &CPU::stackRelativeIndirectIndexedRead<M8, &CPU::opLDA<M8>>;  // LDA (sr,S),Y

	// LDX
	t[0xA2] = &CPU::immediateRead<X8, &CPU::opLDX<X8>>;                     // LDX #
	t[0xA6] = &CPU::directRead<X8, &CPU::opLDX<X8>>;                        // LDX dp
	t[0xB6] = &CPU::directIndexedRead<X8, Index::Y, &CPU::opLDX<X8>>;       // LDX dp,Y
	t[0xAE] = &CPU::absoluteRead<X8, &CPU::opLDX<X8>>;                      // LDX abs
	t[0xBE] = &CPU::absoluteIndexedRead<X8, X8, Index::Y, &CPU::opLDX<X8>>; // LDX abs,Y

	// LDY
	t[0xA0] = &CPU::immediateRead<X8, &CPU::opLDY<X8>>;                     // LDY #
	t[0xA4] = &CPU::directRead<X8, &CPU::opLDY<X8>>;                        // LDY dp
	t[0xB4] = &CPU::directIndexedRead<X8, Index::X, &CPU::opLDY<X8>>;       // LDY dp,X
	t[0xAC] = &CPU::absoluteRead<X8, &CPU::opLDY<X8>>;                      // LDY abs
	t[0xBC] = &CPU::absoluteIndexedRead<X8, X8, Index::X, &CPU::opLDY<X8>>; // LDY abs,X

	// Family 1, stores

	// STA
	t[0x85] = &CPU::directWrite<M8, &CPU::opSTA<M8>>;                       // STA dp
	t[0x95] = &CPU::directIndexedWrite<M8, Index::X, &CPU::opSTA<M8>>;      // STA dp,X
	t[0x8D] = &CPU::absoluteWrite<M8, &CPU::opSTA<M8>>;                     // STA abs
	t[0x9D] = &CPU::absoluteIndexedWrite<M8, Index::X, &CPU::opSTA<M8>>;    // STA abs,X
	t[0x99] = &CPU::absoluteIndexedWrite<M8, Index::Y, &CPU::opSTA<M8>>;    // STA abs,Y
	t[0x8F] = &CPU::absoluteLongWrite<M8, &CPU::opSTA<M8>>;                 // STA long
	t[0x9F] = &CPU::absoluteLongIndexedWrite<M8, &CPU::opSTA<M8>>;          // STA long,X
	t[0x92] = &CPU::directIndirectWrite<M8, &CPU::opSTA<M8>>;               // STA (dp)
	t[0x81] = &CPU::directIndexedIndirectWrite<M8, &CPU::opSTA<M8>>;        // STA (dp,X)
	t[0x91] = &CPU::directIndirectIndexedWrite<M8, &CPU::opSTA<M8>>;        // STA (dp),Y
	t[0x87] = &CPU::directIndirectLongWrite<M8, &CPU::opSTA<M8>>;           // STA [dp]
	t[0x97] = &CPU::directIndirectLongIndexedWrite<M8, &CPU::opSTA<M8>>;    // STA [dp],Y
	t[0x83] = &CPU::stackRelativeWrite<M8, &CPU::opSTA<M8>>;                // STA sr,S
	t[0x93] = &CPU::stackRelativeIndirectIndexedWrite<M8, &CPU::opSTA<M8>>; // STA (sr,S),Y

	// STX
	t[0x86] = &CPU::directWrite<X8, &CPU::opSTX<X8>>;                  // STX dp
	t[0x96] = &CPU::directIndexedWrite<X8, Index::Y, &CPU::opSTX<X8>>; // STX dp,Y
	t[0x8E] = &CPU::absoluteWrite<X8, &CPU::opSTX<X8>>;                // STX abs

	// STY
	t[0x84] = &CPU::directWrite<X8, &CPU::opSTY<X8>>;                  // STY dp
	t[0x94] = &CPU::directIndexedWrite<X8, Index::X, &CPU::opSTY<X8>>; // STY dp,X
	t[0x8C] = &CPU::absoluteWrite<X8, &CPU::opSTY<X8>>;                // STY abs

	// STZ
	t[0x64] = &CPU::directWrite<M8, &CPU::opSTZ<M8>>;                    // STZ dp
	t[0x74] = &CPU::directIndexedWrite<M8, Index::X, &CPU::opSTZ<M8>>;   // STZ dp,X
	t[0x9C] = &CPU::absoluteWrite<M8, &CPU::opSTZ<M8>>;                  // STZ abs
	t[0x9E] = &CPU::absoluteIndexedWrite<M8, Index::X, &CPU::opSTZ<M8>>; // STZ abs,X

	// If any opcode is left empty this throws, and a throw in constinit doesn't compile
	for (const Handler handler : t) {
		if (handler == nullptr) {
			throw "EVERY OPCODE NEEDS A HANDLER";
		}
	}

	return t;
}

} // namespace SNES
