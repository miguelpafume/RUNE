#pragma once

#include "CPU.hpp"
#include "rune/Types.hpp"

namespace SNES {

// All bus cycles of the CPU go through these

// Program counter

U32 CPU::programAddress() const { return static_cast<U32>(m_registers.PB) << 16 | m_registers.PC; }

// Most internal cycles leave the last program fetch on the bus
// If m_registers.PC is 0x0000 it wraps to 0xFFFF because of the cast
U32 CPU::lastFetchAddress() const { return static_cast<U32>(m_registers.PB) << 16 | static_cast<U16>(m_registers.PC - 1); }

// PC wraps inside its bank, it never goes to the next one
U8 CPU::fetch() {
	const U8 value = m_bus.fetchOperand(programAddress());
	m_registers.PC++;
	return value;
}

// Little endian, low byte first
U16 CPU::fetchWord() {
	const U8 low = fetch();
	const U8 high = fetch();
	return static_cast<U16>(high << 8 | low);
}

// 24 bit operand, word and then bank
U32 CPU::fetchLong() {
	const U16 word = fetchWord();
	const U8  bank = fetch();
	return static_cast<U32>(bank) << 16 | word;
}

// Single Cycles

U8 CPU::read(U32 address, bool lock) { return m_bus.readData(address, lock); }

void CPU::write(U32 address, U8 value, bool lock) { m_bus.writeData(address, value, lock); }

// Internal cycle, no access but the bus still has an address and the clock runs
void CPU::idle(U32 address, bool lock) { m_bus.internalCycle(address, lock); }

// Single byte instructions put the next opcode address on the bus during their internal cycle
void CPU::idleImplied() { idle(programAddress()); }

// Direct page costs one extra cycle when DL is not zero
void CPU::idleDirect() {
	if (m_registers.D & LOW_BYTE_MASK)
		idle(lastFetchAddress());
}

// Effective addresses

// Direct page is always in bank $00
// in emulation with DL = 0 it wraps inside the page like the 6502
U32 CPU::directAddress(U32 offset) const {
	if (m_registers.E && (m_registers.D & LOW_BYTE_MASK) == 0)
		return (m_registers.D & HIGH_BYTE_MASK) | (offset & LOW_BYTE_MASK);

	return (m_registers.D + offset) & OFFSET_MASK;
}

// Data bank addressing carries across banks
U32 CPU::bankAddress(U32 address) const { return ((static_cast<U32>(m_registers.DB) << 16) + address) & ADDRESS_MASK; }

// Stack relative is new in the 65816 so there's no page $01 wrap here
U32 CPU::stackAddress(U32 offset) const { return (m_registers.S + offset) & OFFSET_MASK; }

// While the index is being added, the bus already shows base high byte with the low byte of the sum
U32 CPU::uncorrectedAddress(U32 base, U16 offset) { return (base & HIGH_BYTE_MASK) | ((base + offset) & LOW_BYTE_MASK); }

// Adding the index changed the high byte (costs an extra cycle)
bool CPU::crossesPage(U32 base, U16 offset) { return ((base + offset) & PAGE_MASK) != (base & PAGE_MASK); }

// Stack
// Push writes and then moves S down, pull moves S up and then reads

// 6502 instructions keep S inside page $01 in emulation, even between two bytes
void CPU::push(U8 value) {
	write(m_registers.S, value);

	if (m_registers.E) {
		m_registers.S = EMULATION_STACK_PAGE | ((m_registers.S - 1) & LOW_BYTE_MASK);
	} else {
		m_registers.S--;
	}
}

U8 CPU::pull() {
	if (m_registers.E) {
		m_registers.S = EMULATION_STACK_PAGE | ((m_registers.S + 1) & LOW_BYTE_MASK);
	} else {
		m_registers.S++;
	}

	return read(m_registers.S);
}

// The 65816 instructions move S in 16 bits so in emulation they can go out to page $00 or $02 while running
void CPU::pushNative(U8 value) {
	write(m_registers.S, value);
	m_registers.S--;
}

U8 CPU::pullNative() {
	m_registers.S++;
	return read(m_registers.S);
}

// And when they finish, S goes back to page $01
void CPU::endNativeStack() {
	if (m_registers.E) {
		m_registers.S = EMULATION_STACK_PAGE | (m_registers.S & LOW_BYTE_MASK);
	}
}

// Operand locators
// Each mode has its own rule for where the next operand byte is

// Uses directAddress, so the emulation page wrap is included
auto CPU::atDirect(U32 offset) const {
	return [this, offset](U32 k) { return directAddress(offset + k); };
}

// [dp] pointers and PEI ignore the emulation page wrap
auto CPU::atDirectNative(U32 offset) const {
	return [this, offset](U32 k) { return (m_registers.D + offset + k) & OFFSET_MASK; };
}

// (dp,X): the emulation page wrap applies to offset + X only
// The pointer's second byte is the next address, even across the page
auto CPU::atDirectIndexedIndirect(U32 offset) const {
	return [this, offset](U32 k) { return (directAddress(offset) + k) & OFFSET_MASK; };
}

// The next byte can end up in the next bank
auto CPU::atBank(U32 address) const {
	return [this, address](U32 k) { return bankAddress(address + k); };
}

// The bank comes from the instruction, DB isn't used
auto CPU::atLong(U32 address) const {
	return [address](U32 k) { return (address + k) & ADDRESS_MASK; };
}

// Bank $00, wraps at $FFFF
auto CPU::atStack(U32 offset) const {
	return [this, offset](U32 k) { return stackAddress(offset + k); };
}

// Multi-byte accesses

// Low byte first, with 16 bit it also reads at(1)
template <bool W8, typename Locator>
CPU::Word<W8> CPU::readAt(Locator at, bool lock) {
	Word<W8> value = read(at(0), lock);

	if constexpr (!W8)
		value |= static_cast<U16>(read(at(1), lock) << 8);

	return value;
}

// 24 bit pointer for the [dp] modes (low, high, bank)
template <typename Locator>
U32 CPU::readLongPointer(Locator at) {
	const U32 low = read(at(0));
	const U32 high = read(at(1));
	const U32 bank = read(at(2));

	return bank << 16 | high << 8 | low;
}

// Same order as readAt
template <bool W8, typename Locator>
void CPU::writeAt(Locator at, Word<W8> value) {
	write(at(0), static_cast<U8>(value));

	if constexpr (!W8) {
		write(at(1), static_cast<U8>(value >> 8));
	}
}

// Read-modify-write, every cycle here keeps MLB low
// In emulation the internal cycle writes the old value back, like the 6502
template <bool W8, auto Op, typename Locator>
void CPU::modifyAt(Locator at) {
	Word<W8> value = readAt<W8>(at, LOCK);

	if (m_registers.E) {
		m_bus.invalidWrite(at(0), static_cast<U8>(value), LOCK);
	} else {
		idle(at(W8 ? 0 : 1), LOCK);
	}

	value = (this->*Op)(value);

	// High byte first, the opposite of writeAt
	if constexpr (!W8)
		write(at(1), static_cast<U8>(value >> 8), LOCK);

	write(at(0), static_cast<U8>(value), LOCK);
}

} // namespace SNES
