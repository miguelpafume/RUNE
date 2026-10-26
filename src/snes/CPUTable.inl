#include "CPU.hpp"

namespace SNES {

// Families 1, 5 and 6 are a pattern (bus cycles) + an operation (the math)
// Instructions on A use the M width, the ones on X and Y use the X width
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

	// Family 5, logic

	// ORA
	t[0x01] = &CPU::directIndexedIndirectRead<M8, &CPU::opORA<M8>>;         // ORA (dp,X)
	t[0x03] = &CPU::stackRelativeRead<M8, &CPU::opORA<M8>>;                 // ORA sr,S
	t[0x05] = &CPU::directRead<M8, &CPU::opORA<M8>>;                        // ORA dp
	t[0x07] = &CPU::directIndirectLongRead<M8, &CPU::opORA<M8>>;            // ORA [dp]
	t[0x09] = &CPU::immediateRead<M8, &CPU::opORA<M8>>;                     // ORA #
	t[0x0D] = &CPU::absoluteRead<M8, &CPU::opORA<M8>>;                      // ORA abs
	t[0x0F] = &CPU::absoluteLongRead<M8, &CPU::opORA<M8>>;                  // ORA long
	t[0x11] = &CPU::directIndirectIndexedRead<M8, X8, &CPU::opORA<M8>>;     // ORA (dp),Y
	t[0x12] = &CPU::directIndirectRead<M8, &CPU::opORA<M8>>;                // ORA (dp)
	t[0x13] = &CPU::stackRelativeIndirectIndexedRead<M8, &CPU::opORA<M8>>;  // ORA (sr,S),Y
	t[0x15] = &CPU::directIndexedRead<M8, Index::X, &CPU::opORA<M8>>;       // ORA dp,X
	t[0x17] = &CPU::directIndirectLongIndexedRead<M8, &CPU::opORA<M8>>;     // ORA [dp],Y
	t[0x19] = &CPU::absoluteIndexedRead<M8, X8, Index::Y, &CPU::opORA<M8>>; // ORA abs,Y
	t[0x1D] = &CPU::absoluteIndexedRead<M8, X8, Index::X, &CPU::opORA<M8>>; // ORA abs,X
	t[0x1F] = &CPU::absoluteLongIndexedRead<M8, &CPU::opORA<M8>>;           // ORA long,X

	// AND
	t[0x21] = &CPU::directIndexedIndirectRead<M8, &CPU::opAND<M8>>;         // AND (dp,X)
	t[0x23] = &CPU::stackRelativeRead<M8, &CPU::opAND<M8>>;                 // AND sr,S
	t[0x25] = &CPU::directRead<M8, &CPU::opAND<M8>>;                        // AND dp
	t[0x27] = &CPU::directIndirectLongRead<M8, &CPU::opAND<M8>>;            // AND [dp]
	t[0x29] = &CPU::immediateRead<M8, &CPU::opAND<M8>>;                     // AND #
	t[0x2D] = &CPU::absoluteRead<M8, &CPU::opAND<M8>>;                      // AND abs
	t[0x2F] = &CPU::absoluteLongRead<M8, &CPU::opAND<M8>>;                  // AND long
	t[0x31] = &CPU::directIndirectIndexedRead<M8, X8, &CPU::opAND<M8>>;     // AND (dp),Y
	t[0x32] = &CPU::directIndirectRead<M8, &CPU::opAND<M8>>;                // AND (dp)
	t[0x33] = &CPU::stackRelativeIndirectIndexedRead<M8, &CPU::opAND<M8>>;  // AND (sr,S),Y
	t[0x35] = &CPU::directIndexedRead<M8, Index::X, &CPU::opAND<M8>>;       // AND dp,X
	t[0x37] = &CPU::directIndirectLongIndexedRead<M8, &CPU::opAND<M8>>;     // AND [dp],Y
	t[0x39] = &CPU::absoluteIndexedRead<M8, X8, Index::Y, &CPU::opAND<M8>>; // AND abs,Y
	t[0x3D] = &CPU::absoluteIndexedRead<M8, X8, Index::X, &CPU::opAND<M8>>; // AND abs,X
	t[0x3F] = &CPU::absoluteLongIndexedRead<M8, &CPU::opAND<M8>>;           // AND long,X

	// EOR
	t[0x41] = &CPU::directIndexedIndirectRead<M8, &CPU::opEOR<M8>>;         // EOR (dp,X)
	t[0x43] = &CPU::stackRelativeRead<M8, &CPU::opEOR<M8>>;                 // EOR sr,S
	t[0x45] = &CPU::directRead<M8, &CPU::opEOR<M8>>;                        // EOR dp
	t[0x47] = &CPU::directIndirectLongRead<M8, &CPU::opEOR<M8>>;            // EOR [dp]
	t[0x49] = &CPU::immediateRead<M8, &CPU::opEOR<M8>>;                     // EOR #
	t[0x4D] = &CPU::absoluteRead<M8, &CPU::opEOR<M8>>;                      // EOR abs
	t[0x4F] = &CPU::absoluteLongRead<M8, &CPU::opEOR<M8>>;                  // EOR long
	t[0x51] = &CPU::directIndirectIndexedRead<M8, X8, &CPU::opEOR<M8>>;     // EOR (dp),Y
	t[0x52] = &CPU::directIndirectRead<M8, &CPU::opEOR<M8>>;                // EOR (dp)
	t[0x53] = &CPU::stackRelativeIndirectIndexedRead<M8, &CPU::opEOR<M8>>;  // EOR (sr,S),Y
	t[0x55] = &CPU::directIndexedRead<M8, Index::X, &CPU::opEOR<M8>>;       // EOR dp,X
	t[0x57] = &CPU::directIndirectLongIndexedRead<M8, &CPU::opEOR<M8>>;     // EOR [dp],Y
	t[0x59] = &CPU::absoluteIndexedRead<M8, X8, Index::Y, &CPU::opEOR<M8>>; // EOR abs,Y
	t[0x5D] = &CPU::absoluteIndexedRead<M8, X8, Index::X, &CPU::opEOR<M8>>; // EOR abs,X
	t[0x5F] = &CPU::absoluteLongIndexedRead<M8, &CPU::opEOR<M8>>;           // EOR long,X

	// BIT
	t[0x24] = &CPU::directRead<M8, &CPU::opBIT<M8>>;                        // BIT dp
	t[0x2C] = &CPU::absoluteRead<M8, &CPU::opBIT<M8>>;                      // BIT abs
	t[0x34] = &CPU::directIndexedRead<M8, Index::X, &CPU::opBIT<M8>>;       // BIT dp,X
	t[0x3C] = &CPU::absoluteIndexedRead<M8, X8, Index::X, &CPU::opBIT<M8>>; // BIT abs,X
	t[0x89] = &CPU::immediateRead<M8, &CPU::opBITImmediate<M8>>;            // BIT #

	// Family 5, compare

	// CMP
	t[0xC1] = &CPU::directIndexedIndirectRead<M8, &CPU::opCMP<M8>>;         // CMP (dp,X)
	t[0xC3] = &CPU::stackRelativeRead<M8, &CPU::opCMP<M8>>;                 // CMP sr,S
	t[0xC5] = &CPU::directRead<M8, &CPU::opCMP<M8>>;                        // CMP dp
	t[0xC7] = &CPU::directIndirectLongRead<M8, &CPU::opCMP<M8>>;            // CMP [dp]
	t[0xC9] = &CPU::immediateRead<M8, &CPU::opCMP<M8>>;                     // CMP #
	t[0xCD] = &CPU::absoluteRead<M8, &CPU::opCMP<M8>>;                      // CMP abs
	t[0xCF] = &CPU::absoluteLongRead<M8, &CPU::opCMP<M8>>;                  // CMP long
	t[0xD1] = &CPU::directIndirectIndexedRead<M8, X8, &CPU::opCMP<M8>>;     // CMP (dp),Y
	t[0xD2] = &CPU::directIndirectRead<M8, &CPU::opCMP<M8>>;                // CMP (dp)
	t[0xD3] = &CPU::stackRelativeIndirectIndexedRead<M8, &CPU::opCMP<M8>>;  // CMP (sr,S),Y
	t[0xD5] = &CPU::directIndexedRead<M8, Index::X, &CPU::opCMP<M8>>;       // CMP dp,X
	t[0xD7] = &CPU::directIndirectLongIndexedRead<M8, &CPU::opCMP<M8>>;     // CMP [dp],Y
	t[0xD9] = &CPU::absoluteIndexedRead<M8, X8, Index::Y, &CPU::opCMP<M8>>; // CMP abs,Y
	t[0xDD] = &CPU::absoluteIndexedRead<M8, X8, Index::X, &CPU::opCMP<M8>>; // CMP abs,X
	t[0xDF] = &CPU::absoluteLongIndexedRead<M8, &CPU::opCMP<M8>>;           // CMP long,X

	// CPX
	t[0xE0] = &CPU::immediateRead<X8, &CPU::opCPX<X8>>; // CPX #
	t[0xE4] = &CPU::directRead<X8, &CPU::opCPX<X8>>;    // CPX dp
	t[0xEC] = &CPU::absoluteRead<X8, &CPU::opCPX<X8>>;  // CPX abs

	// CPY
	t[0xC0] = &CPU::immediateRead<X8, &CPU::opCPY<X8>>; // CPY #
	t[0xC4] = &CPU::directRead<X8, &CPU::opCPY<X8>>;    // CPY dp
	t[0xCC] = &CPU::absoluteRead<X8, &CPU::opCPY<X8>>;  // CPY abs

	// Family 5, arithmetic

	// ADC
	t[0x61] = &CPU::directIndexedIndirectRead<M8, &CPU::opADC<M8>>;         // ADC (dp,X)
	t[0x63] = &CPU::stackRelativeRead<M8, &CPU::opADC<M8>>;                 // ADC sr,S
	t[0x65] = &CPU::directRead<M8, &CPU::opADC<M8>>;                        // ADC dp
	t[0x67] = &CPU::directIndirectLongRead<M8, &CPU::opADC<M8>>;            // ADC [dp]
	t[0x69] = &CPU::immediateRead<M8, &CPU::opADC<M8>>;                     // ADC #
	t[0x6D] = &CPU::absoluteRead<M8, &CPU::opADC<M8>>;                      // ADC abs
	t[0x6F] = &CPU::absoluteLongRead<M8, &CPU::opADC<M8>>;                  // ADC long
	t[0x71] = &CPU::directIndirectIndexedRead<M8, X8, &CPU::opADC<M8>>;     // ADC (dp),Y
	t[0x72] = &CPU::directIndirectRead<M8, &CPU::opADC<M8>>;                // ADC (dp)
	t[0x73] = &CPU::stackRelativeIndirectIndexedRead<M8, &CPU::opADC<M8>>;  // ADC (sr,S),Y
	t[0x75] = &CPU::directIndexedRead<M8, Index::X, &CPU::opADC<M8>>;       // ADC dp,X
	t[0x77] = &CPU::directIndirectLongIndexedRead<M8, &CPU::opADC<M8>>;     // ADC [dp],Y
	t[0x79] = &CPU::absoluteIndexedRead<M8, X8, Index::Y, &CPU::opADC<M8>>; // ADC abs,Y
	t[0x7D] = &CPU::absoluteIndexedRead<M8, X8, Index::X, &CPU::opADC<M8>>; // ADC abs,X
	t[0x7F] = &CPU::absoluteLongIndexedRead<M8, &CPU::opADC<M8>>;           // ADC long,X

	// SBC
	t[0xE1] = &CPU::directIndexedIndirectRead<M8, &CPU::opSBC<M8>>;         // SBC (dp,X)
	t[0xE3] = &CPU::stackRelativeRead<M8, &CPU::opSBC<M8>>;                 // SBC sr,S
	t[0xE5] = &CPU::directRead<M8, &CPU::opSBC<M8>>;                        // SBC dp
	t[0xE7] = &CPU::directIndirectLongRead<M8, &CPU::opSBC<M8>>;            // SBC [dp]
	t[0xE9] = &CPU::immediateRead<M8, &CPU::opSBC<M8>>;                     // SBC #
	t[0xED] = &CPU::absoluteRead<M8, &CPU::opSBC<M8>>;                      // SBC abs
	t[0xEF] = &CPU::absoluteLongRead<M8, &CPU::opSBC<M8>>;                  // SBC long
	t[0xF1] = &CPU::directIndirectIndexedRead<M8, X8, &CPU::opSBC<M8>>;     // SBC (dp),Y
	t[0xF2] = &CPU::directIndirectRead<M8, &CPU::opSBC<M8>>;                // SBC (dp)
	t[0xF3] = &CPU::stackRelativeIndirectIndexedRead<M8, &CPU::opSBC<M8>>;  // SBC (sr,S),Y
	t[0xF5] = &CPU::directIndexedRead<M8, Index::X, &CPU::opSBC<M8>>;       // SBC dp,X
	t[0xF7] = &CPU::directIndirectLongIndexedRead<M8, &CPU::opSBC<M8>>;     // SBC [dp],Y
	t[0xF9] = &CPU::absoluteIndexedRead<M8, X8, Index::Y, &CPU::opSBC<M8>>; // SBC abs,Y
	t[0xFD] = &CPU::absoluteIndexedRead<M8, X8, Index::X, &CPU::opSBC<M8>>; // SBC abs,X
	t[0xFF] = &CPU::absoluteLongIndexedRead<M8, &CPU::opSBC<M8>>;           // SBC long,X

	// Family 6, increments and decrements

	// INC
	t[0x1A] = &CPU::impliedModify<M8, Register::A, &CPU::opINC<M8>>; // INC A
	t[0xE6] = &CPU::directModify<M8, &CPU::opINC<M8>>;               // INC dp
	t[0xEE] = &CPU::absoluteModify<M8, &CPU::opINC<M8>>;             // INC abs
	t[0xF6] = &CPU::directIndexedModify<M8, &CPU::opINC<M8>>;        // INC dp,X
	t[0xFE] = &CPU::absoluteIndexedModify<M8, &CPU::opINC<M8>>;      // INC abs,X

	// DEC
	t[0x3A] = &CPU::impliedModify<M8, Register::A, &CPU::opDEC<M8>>; // DEC A
	t[0xC6] = &CPU::directModify<M8, &CPU::opDEC<M8>>;               // DEC dp
	t[0xCE] = &CPU::absoluteModify<M8, &CPU::opDEC<M8>>;             // DEC abs
	t[0xD6] = &CPU::directIndexedModify<M8, &CPU::opDEC<M8>>;        // DEC dp,X
	t[0xDE] = &CPU::absoluteIndexedModify<M8, &CPU::opDEC<M8>>;      // DEC abs,X

	// INX, INY, DEX and DEY, with the X width
	t[0xE8] = &CPU::impliedModify<X8, Register::X, &CPU::opINC<X8>>; // INX
	t[0xC8] = &CPU::impliedModify<X8, Register::Y, &CPU::opINC<X8>>; // INY
	t[0xCA] = &CPU::impliedModify<X8, Register::X, &CPU::opDEC<X8>>; // DEX
	t[0x88] = &CPU::impliedModify<X8, Register::Y, &CPU::opDEC<X8>>; // DEY

	// Family 6, shifts and rotates

	// ASL
	t[0x06] = &CPU::directModify<M8, &CPU::opASL<M8>>;               // ASL dp
	t[0x0A] = &CPU::impliedModify<M8, Register::A, &CPU::opASL<M8>>; // ASL A
	t[0x0E] = &CPU::absoluteModify<M8, &CPU::opASL<M8>>;             // ASL abs
	t[0x16] = &CPU::directIndexedModify<M8, &CPU::opASL<M8>>;        // ASL dp,X
	t[0x1E] = &CPU::absoluteIndexedModify<M8, &CPU::opASL<M8>>;      // ASL abs,X

	// ROL
	t[0x26] = &CPU::directModify<M8, &CPU::opROL<M8>>;               // ROL dp
	t[0x2A] = &CPU::impliedModify<M8, Register::A, &CPU::opROL<M8>>; // ROL A
	t[0x2E] = &CPU::absoluteModify<M8, &CPU::opROL<M8>>;             // ROL abs
	t[0x36] = &CPU::directIndexedModify<M8, &CPU::opROL<M8>>;        // ROL dp,X
	t[0x3E] = &CPU::absoluteIndexedModify<M8, &CPU::opROL<M8>>;      // ROL abs,X

	// LSR
	t[0x46] = &CPU::directModify<M8, &CPU::opLSR<M8>>;               // LSR dp
	t[0x4A] = &CPU::impliedModify<M8, Register::A, &CPU::opLSR<M8>>; // LSR A
	t[0x4E] = &CPU::absoluteModify<M8, &CPU::opLSR<M8>>;             // LSR abs
	t[0x56] = &CPU::directIndexedModify<M8, &CPU::opLSR<M8>>;        // LSR dp,X
	t[0x5E] = &CPU::absoluteIndexedModify<M8, &CPU::opLSR<M8>>;      // LSR abs,X

	// ROR
	t[0x66] = &CPU::directModify<M8, &CPU::opROR<M8>>;               // ROR dp
	t[0x6A] = &CPU::impliedModify<M8, Register::A, &CPU::opROR<M8>>; // ROR A
	t[0x6E] = &CPU::absoluteModify<M8, &CPU::opROR<M8>>;             // ROR abs
	t[0x76] = &CPU::directIndexedModify<M8, &CPU::opROR<M8>>;        // ROR dp,X
	t[0x7E] = &CPU::absoluteIndexedModify<M8, &CPU::opROR<M8>>;      // ROR abs,X

	// Family 6, TSB and TRB

	// TSB
	t[0x04] = &CPU::directModify<M8, &CPU::opTSB<M8>>;   // TSB dp
	t[0x0C] = &CPU::absoluteModify<M8, &CPU::opTSB<M8>>; // TSB abs

	// TRB
	t[0x14] = &CPU::directModify<M8, &CPU::opTRB<M8>>;   // TRB dp
	t[0x1C] = &CPU::absoluteModify<M8, &CPU::opTRB<M8>>; // TRB abs

	// If any opcode is left empty this throws, and a throw in constinit doesn't compile
	for (const Handler handler : t) {
		if (handler == nullptr) {
			throw "EVERY OPCODE NEEDS A HANDLER";
		}
	}

	return t;
}

} // namespace SNES
