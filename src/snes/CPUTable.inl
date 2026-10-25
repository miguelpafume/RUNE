#include "CPU.hpp"

namespace SNES {

// Family 1 opcodes are a pattern (bus cycles) + an operation (the math)
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

	// Family 2, transfers

	t[0xAA] = &CPU::transfer<X8, Register::A, Register::X>;    // TAX
	t[0xA8] = &CPU::transfer<X8, Register::A, Register::Y>;    // TAY
	t[0x9B] = &CPU::transfer<X8, Register::X, Register::Y>;    // TXY
	t[0xBB] = &CPU::transfer<X8, Register::Y, Register::X>;    // TYX
	t[0xBA] = &CPU::transfer<X8, Register::S, Register::X>;    // TSX
	t[0x8A] = &CPU::transfer<M8, Register::X, Register::A>;    // TXA
	t[0x98] = &CPU::transfer<M8, Register::Y, Register::A>;    // TYA
	t[0x3B] = &CPU::transfer<false, Register::S, Register::A>; // TSC
	t[0x5B] = &CPU::transfer<false, Register::A, Register::D>; // TCD
	t[0x7B] = &CPU::transfer<false, Register::D, Register::A>; // TDC
	t[0x1B] = &CPU::transferToS<Register::A>;                  // TCS
	t[0x9A] = &CPU::transferToS<Register::X>;                  // TXS
	t[0xEB] = &CPU::instructionXBA;                            // XBA

	// Family 3, flags and mode

	t[0x18] = &CPU::changeFlag<FLAG_C, false>; // CLC
	t[0x38] = &CPU::changeFlag<FLAG_C, true>;  // SEC
	t[0x58] = &CPU::changeFlag<FLAG_I, false>; // CLI
	t[0x78] = &CPU::changeFlag<FLAG_I, true>;  // SEI
	t[0xD8] = &CPU::changeFlag<FLAG_D, false>; // CLD
	t[0xF8] = &CPU::changeFlag<FLAG_D, true>;  // SED
	t[0xB8] = &CPU::changeFlag<FLAG_V, false>; // CLV
	t[0xC2] = &CPU::instructionREP;            // REP
	t[0xE2] = &CPU::instructionSEP;            // SEP
	t[0xFB] = &CPU::instructionXCE;            // XCE

	// Family 4, pushes

	t[0x48] = &CPU::pushRegister<M8, Register::A>; // PHA
	t[0xDA] = &CPU::pushRegister<X8, Register::X>; // PHX
	t[0x5A] = &CPU::pushRegister<X8, Register::Y>; // PHY
	t[0x08] = &CPU::instructionPHP;                // PHP
	t[0x8B] = &CPU::instructionPHB;                // PHB
	t[0x4B] = &CPU::instructionPHK;                // PHK
	t[0x0B] = &CPU::instructionPHD;                // PHD
	t[0xF4] = &CPU::instructionPEA;                // PEA
	t[0xD4] = &CPU::instructionPEI;                // PEI
	t[0x62] = &CPU::instructionPER;                // PER

	// Family 4, pulls

	t[0x68] = &CPU::pullRegister<M8, Register::A>; // PLA
	t[0xFA] = &CPU::pullRegister<X8, Register::X>; // PLX
	t[0x7A] = &CPU::pullRegister<X8, Register::Y>; // PLY
	t[0x28] = &CPU::instructionPLP;                // PLP
	t[0x2B] = &CPU::instructionPLD;                // PLD
	t[0xAB] = &CPU::instructionPLB;                // PLB

	// If any opcode is left empty this throws, and a throw in constinit doesn't compile
	for (const Handler handler : t) {
		if (handler == nullptr) {
			throw "EVERY OPCODE NEEDS A HANDLER";
		}
	}

	return t;
}

} // namespace SNES
