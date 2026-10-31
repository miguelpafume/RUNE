#pragma once

#include "rune/Types.hpp"

#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

namespace SNES {

using RUNE::U16;
using RUNE::U32;
using RUNE::U8;

class Cartridge;

class Bus {
public:
	enum class Mode {
		System,
		Test
	};

	enum class CycleKind {
		OpcodeFetch,  // dp-
		OperandFetch, // -p-
		DataAccess,   // d--
		VectorFetch,  // d-v
		Internal      // ---, no memory activity
	};

	struct Access {
		U32               address {};
		std::optional<U8> value {};
		CycleKind         cycleKind {};
		bool              isWrite {};
		bool              emulation {};
		bool              flagM {};
		bool              flagX {};
		bool              memoryLock {};
	};

	enum class BlockType : U8 {
		OpenBus,
		Io,
		Sram
	};

	static constexpr U32 BLOCK_SHIFT = 12;
	static constexpr U32 BLOCK_SIZE = 1u << BLOCK_SHIFT; // 4 KiB
	static constexpr U32 BLOCK_COUNT = 0x1000;           // 4096 x 4 KiB = 16 MiB

	explicit Bus(Mode mode);

	U8 fetchOpcode(U32 address) { return readCycle(address, CycleKind::OpcodeFetch); }
	U8 fetchOperand(U32 address) { return readCycle(address, CycleKind::OperandFetch); }
	U8 fetchVector(U32 address) { return readCycle(address, CycleKind::VectorFetch); }
	U8 readData(U32 address, bool memoryLock = false) { return readCycle(address, CycleKind::DataAccess, memoryLock); }

	void writeData(U32 address, U8 value, bool memoryLock = false);
	void internalCycle(U32 address, bool memoryLock = false);

	void setFlags(bool emulation, bool flagM, bool flagX);

	void invalidWrite(U32 address, U8 value, bool memoryLock = false);
	void haltedCycle();

	void attachCartridge(Cartridge& cartridge) { m_cartridge = &cartridge; }

	// Read that doesn't change anything, used by the comparator and the viewers
	U8 peek(U32 address) const;

	// Test mode only
	void                       poke(U32 address, U8 value);
	void                       beginCase();
	const std::vector<Access>& accessLog() const { return m_accessLog; }

	// thrown on a cycle past the budget, so the CPU can stop in the middle of an instruction
	// the suite cuts MVN and MVP at 100 cycles, in the middle of a repetition
	struct CycleBudgetExhausted {};

	void setCycleBudget(std::size_t cycles) { m_cycleBudget = cycles; }

	// With a null pointer the access falls into the switch on the block type
	void mapBlock(U32 address, const U8* read, U8* write, BlockType type);

	// Marks a whole range with a type and no pointers (SRAM, I/O)
	void mapRange(U8 bankFirst, U8 bankLast, U16 addressFirst, U16 addressLast, BlockType type);

private:
	Mode m_mode;

	std::vector<U8>     m_testMemory {};
	std::vector<U32>    m_dirty {};
	std::vector<Access> m_accessLog {};

	std::array<const U8*, BLOCK_COUNT> m_readMap {};
	std::array<U8*, BLOCK_COUNT>       m_writeMap {};
	std::array<BlockType, BLOCK_COUNT> m_blockType {};

	// Last value seen on the data bus, unmapped reads return it
	U8 m_openBus {};

	bool m_flagE {};
	bool m_flagM {};
	bool m_flagX {};

	Cartridge* m_cartridge {};

	std::size_t m_cycleBudget = std::numeric_limits<std::size_t>::max();

	void spendCycle();

	U8 readCycle(U32 address, CycleKind kind, bool memoryLock = false);

	U8   readSystem(U32 address);
	void writeSystem(U32 address, U8 value);
};

} // namespace SNES