#pragma once

#include "CPU.hpp"

#include "CPUMemory.inl"

namespace SNES {

// Each pattern does the bus cycles of one addressing mode, Op does the rest

// Read -> Op consumes the operand

// Immediate, the operand comes right after the opcode (1 or 2 bytes)
template <bool W8, auto Op>
void CPU::immediateRead() {
	Word<W8> value = fetch();

	if constexpr (!W8) {
		value |= static_cast<U16>(fetch() << 8);
	}

	(this->*Op)(value);
}

// Direct page

// dp: D + offset
template <bool W8, auto Op>
void CPU::directRead() {
	const U8 offset = fetch();
	idleDirect();

	(this->*Op)(readAt<W8>(atDirect(offset)));
}

// dp,X and dp,Y always take one extra cycle to add the index
template <bool W8, CPU::Index I, auto Op>
void CPU::directIndexedRead() {
	const U8 offset = fetch();
	idleDirect();
	idle(lastFetchAddress());

	(this->*Op)(readAt<W8>(atDirect(offset + index<I>())));
}

// Absolute

// abs: DB:address
template <bool W8, auto Op>
void CPU::absoluteRead() {
	const U16 base = fetchWord();

	(this->*Op)(readAt<W8>(atBank(base)));
}

// abs,X and abs,Y only take the extra cycle with 16 bit index or when crossing a page
template <bool W8, bool X8, CPU::Index I, auto Op>
void CPU::absoluteIndexedRead() {
	const U16 base = fetchWord();
	const U16 offset = index<I>();

	if (!X8 || crossesPage(base, offset))
		idle(bankAddress(uncorrectedAddress(base, offset)));

	(this->*Op)(readAt<W8>(atBank(base + offset)));
}

// long: the instruction has the full 24 bit address
template <bool W8, auto Op>
void CPU::absoluteLongRead() {
	const U32 base = fetchLong();

	(this->*Op)(readAt<W8>(atLong(base)));
}

// long,X: no extra cycle here
template <bool W8, auto Op>
void CPU::absoluteLongIndexedRead() {
	const U32 address = fetchLong();

	(this->*Op)(readAt<W8>(atLong(address + m_registers.X)));
}

// Indirect, the pointer is read from direct page and is always 16 or 24 bits (M and X don't matter)

// (dp): 16 bit pointer, the value is in the data bank
template <bool W8, auto Op>
void CPU::directIndirectRead() {
	const U8 offset = fetch();
	idleDirect();

	const U16 pointer = readAt<false>(atDirect(offset));

	(this->*Op)(readAt<W8>(atBank(pointer)));
}

// (dp,X): X gets added before reading the pointer, costs one cycle
template <bool W8, auto Op>
void CPU::directIndexedIndirectRead() {
	const U8 offset = fetch();
	idleDirect();
	idle(lastFetchAddress());

	const U16 pointer = readAt<false>(atDirectIndexedIndirect(offset + m_registers.X));

	(this->*Op)(readAt<W8>(atBank(pointer)));
}

// (dp),Y: Y gets added after reading the pointer, same extra cycle rule as abs,Y
template <bool W8, bool X8, auto Op>
void CPU::directIndirectIndexedRead() {
	const U8 offset = fetch();
	idleDirect();

	const U16 pointer = readAt<false>(atDirect(offset));

	if (!X8 || crossesPage(pointer, m_registers.Y))
		idle(bankAddress(uncorrectedAddress(pointer, m_registers.Y)));

	(this->*Op)(readAt<W8>(atBank(pointer + m_registers.Y)));
}

// [dp]: 24 bit pointer, no page wrap even in emulation
template <bool W8, auto Op>
void CPU::directIndirectLongRead() {
	const U8 offset = fetch();
	idleDirect();

	const U32 pointer = readLongPointer(atDirectNative(offset));

	(this->*Op)(readAt<W8>(atLong(pointer)));
}

// [dp],Y: same as [dp] plus Y, no extra cycle
template <bool W8, auto Op>
void CPU::directIndirectLongIndexedRead() {
	const U8 offset = fetch();
	idleDirect();

	const U32 pointer = readLongPointer(atDirectNative(offset));

	(this->*Op)(readAt<W8>(atLong(pointer + m_registers.Y)));
}

// Stack relative

// sr,S: S + offset, one cycle to do the sum
template <bool W8, auto Op>
void CPU::stackRelativeRead() {
	const U8 offset = fetch();
	idle(lastFetchAddress());

	(this->*Op)(readAt<W8>(atStack(offset)));
}

// (sr,S),Y: pointer is on the stack, then Y is added
// the second idle always happens, on the address of the pointer's last byte
template <bool W8, auto Op>
void CPU::stackRelativeIndirectIndexedRead() {
	const U8 offset = fetch();
	idle(lastFetchAddress());

	const U16 pointer = readAt<false>(atStack(offset));
	idle(stackAddress(offset + 1));

	(this->*Op)(readAt<W8>(atBank(pointer + m_registers.Y)));
}

// Write -> Op gives the value to write
// Same cycles as the reads, but indexed writes always take the extra cycle

// Direct page

// dp
template <bool W8, auto Op>
void CPU::directWrite() {
	const U8 offset = fetch();
	idleDirect();

	writeAt<W8>(atDirect(offset), (this->*Op)());
}

// dp,X and dp,Y
template <bool W8, CPU::Index I, auto Op>
void CPU::directIndexedWrite() {
	const U8 offset = fetch();
	idleDirect();
	idle(lastFetchAddress());

	writeAt<W8>(atDirect(offset + index<I>()), (this->*Op)());
}

// Absolute

// abs
template <bool W8, auto Op>
void CPU::absoluteWrite() {
	const U16 base = fetchWord();

	writeAt<W8>(atBank(base), (this->*Op)());
}

// abs,X and abs,Y: extra cycle always
template <bool W8, CPU::Index I, auto Op>
void CPU::absoluteIndexedWrite() {
	const U16 base = fetchWord();
	const U16 offset = index<I>();

	idle(bankAddress(uncorrectedAddress(base, offset)));

	writeAt<W8>(atBank(base + offset), (this->*Op)());
}

// long
template <bool W8, auto Op>
void CPU::absoluteLongWrite() {
	const U32 address = fetchLong();

	writeAt<W8>(atLong(address), (this->*Op)());
}

// long,X
template <bool W8, auto Op>
void CPU::absoluteLongIndexedWrite() {
	const U32 address = fetchLong();

	writeAt<W8>(atLong(address + m_registers.X), (this->*Op)());
}

// Indirect

// (dp)
template <bool W8, auto Op>
void CPU::directIndirectWrite() {
	const U8 offset = fetch();
	idleDirect();

	const U16 pointer = readAt<false>(atDirect(offset));

	writeAt<W8>(atBank(pointer), (this->*Op)());
}

// (dp,X)
template <bool W8, auto Op>
void CPU::directIndexedIndirectWrite() {
	const U8 offset = fetch();
	idleDirect();
	idle(lastFetchAddress());

	const U16 pointer = readAt<false>(atDirectIndexedIndirect(offset + m_registers.X));

	writeAt<W8>(atBank(pointer), (this->*Op)());
}

// (dp),Y: extra cycle always
template <bool W8, auto Op>
void CPU::directIndirectIndexedWrite() {
	const U8 offset = fetch();
	idleDirect();

	const U16 pointer = readAt<false>(atDirect(offset));
	idle(bankAddress(uncorrectedAddress(pointer, m_registers.Y)));

	writeAt<W8>(atBank(pointer + m_registers.Y), (this->*Op)());
}

// [dp]
template <bool W8, auto Op>
void CPU::directIndirectLongWrite() {
	const U8 offset = fetch();
	idleDirect();

	const U32 pointer = readLongPointer(atDirectNative(offset));

	writeAt<W8>(atLong(pointer), (this->*Op)());
}

// [dp],Y
template <bool W8, auto Op>
void CPU::directIndirectLongIndexedWrite() {
	const U8 offset = fetch();
	idleDirect();

	const U32 pointer = readLongPointer(atDirectNative(offset));

	writeAt<W8>(atLong(pointer + m_registers.Y), (this->*Op)());
}

// Stack relative

// sr,S
template <bool W8, auto Op>
void CPU::stackRelativeWrite() {
	const U8 offset = fetch();
	idle(lastFetchAddress());

	writeAt<W8>(atStack(offset), (this->*Op)());
}

// (sr,S),Y
template <bool W8, auto Op>
void CPU::stackRelativeIndirectIndexedWrite() {
	const U8 offset = fetch();
	idle(lastFetchAddress());

	const U16 pointer = readAt<false>(atStack(offset));
	idle(stackAddress(offset + 1));

	writeAt<W8>(atBank(pointer + m_registers.Y), (this->*Op)());
}

} // namespace SNES
