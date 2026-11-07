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
using RUNE::U32_MAX;
using RUNE::U64;
using RUNE::U8;

class APU;
class Cartridge;
class DMA;
class Gamepad;
class PPU;

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

	// What the DMA controller has to do for HDMA at the next CPU cycle
	enum class HdmaRequest : U8 {
		None,
		Setup,   // start of the frame
		Transfer // H-Blank of a visible line
	};

	enum class BlockType : U8 {
		OpenBus,
		ExternalRegisters, // $2000-$3FFF: PPU, APU and WRAM port, outside the CPU
		CpuRegisters,      // $4000-$5FFF: joypad, interrupts and DMA, inside the CPU
		Sram
	};

	static constexpr U32 BLOCK_SHIFT = 12;
	static constexpr U32 BLOCK_SIZE = 1u << BLOCK_SHIFT; // 4 KiB
	static constexpr U32 BLOCK_COUNT = 0x1000;           // 4096 x 4 KiB = 16 MiB

	static constexpr U32 WRAM_SIZE = 0x20000; // 128 KiB

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
	void attachApu(APU& apu) { m_apu = &apu; }
	void attachPpu(PPU& ppu) { m_ppu = &ppu; }
	void attachDma(DMA& dma) { m_dma = &dma; }
	void attachGamepads(Gamepad& port1, Gamepad& port2) { m_ports = {&port1, &port2}; }

	// Read that doesn't change anything, used by the comparator and the viewers
	U8 peek(U32 address) const;

	// System mode only
	U64  masterCycles() const { return m_masterCycles; }
	void advanceClock(U32 cycles);
	bool takeNmi();

	// What the CPU saw at the start of the last cycle of the instruction
	// The IRQ stays up until $4211 is read, so taking it doesn't clear it
	bool interruptPolled() const { return m_nmiPolled || m_irqPolled; }
	bool irqWakes() const { return m_irqWake; }

	// Copy of the CPU's I flag, the Bus samples it with the IRQ line
	void setIrqDisable(bool disable) { m_irqDisable = disable; }

	HdmaRequest takeHdma();

	U8   dmaRead(U32 address, bool valid);
	void dmaWrite(U32 address, U8 value, bool valid);

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

	std::vector<U8> m_wram {};

	// Address of the WRAM port, moves forward on every $2180 access
	U32 m_wramAddress {};

	// Last value seen on the data bus, unmapped reads return it
	U8 m_openBus {};

	// The system's only clock, it moves when the bus is used
	U64  m_masterCycles {};
	bool m_autoJoypad {}; // $4200 bit 0
	bool m_nmiEnable {};  // $4200 bit 7
	bool m_nmiFlag {};    // $4210 bit 7
	bool m_nmiPending {}; // the CPU takes it before the next instruction
	bool m_nmiPolled {};  // m_nmiPending at the start of the last CPU cycle
	bool m_fastRom {};    // $420D bit 0

	// The console reads the controllers on its own, in 33 steps of 128 cycles
	// Past the last step the read is idle
	static constexpr U32 JOYPAD_STEPS = 33;
	static constexpr U32 JOYPAD_STEP_CYCLES = 128;

	U32                m_joypadStep = JOYPAD_STEPS;
	U64                m_nextJoypadEdge = JOYPAD_STEP_CYCLES;
	bool               m_cpuLatch {};        // $4016 bit 0
	bool               m_autoJoypadLatch {}; // the auto read holds the same line
	std::array<U8, 2>  m_joypadBits {};      // read on one step, shifted in on the next
	std::array<U16, 2> m_joy {};             // $4218-$421B

	bool m_flagE {};
	bool m_flagM {};
	bool m_flagX {};

	Cartridge* m_cartridge {};
	APU*       m_apu {};
	PPU*       m_ppu {};
	DMA*       m_dma {};

	std::array<Gamepad*, 2> m_ports {};

	U8 m_dmaPending {}; // channels written to $420B

	bool m_hIrq {};       // $4200 bit 4
	bool m_vIrq {};       // $4200 bit 5
	U16  m_hTime = 0x1FF; // $4207-$4208, in dots
	U16  m_vTime = 0x1FF; // $4209-$420A
	bool m_irqLine {};    // $4211 bit 7

	U32 m_irqPosition = U32_MAX; // H of the IRQ in this line

	bool m_irqDisable = true; // the CPU's I flag
	bool m_irqPolled {};      // m_irqLine and I clear, at the start of the last CPU cycle
	bool m_irqWake {};        // m_irqLine at the start of the last halted cycle, I doesn't matter

	// Multiplier and divider, one bit per CPU cycle
	U8   m_wrmpya = 0xFF;   // $4202
	U16  m_wrdiva = 0xFFFF; // $4204-$4205
	U16  m_rddiv {};        // $4214-$4215
	U16  m_rdmpy {};        // $4216-$4217
	U32  m_aluShift {};
	U32  m_aluSteps {}; // left in the one running
	bool m_dividing {};

	HdmaRequest m_hdmaRequest {};
	U32         m_hdmaPosition = U32_MAX; // H of the next HDMA step in this line
	bool        m_hdmaSetupDone = true;

	std::size_t m_cycleBudget = std::numeric_limits<std::size_t>::max();

	void joypadEdge();
	void latchPorts();

	void startHdmaLine(U32 hClock);
	void hdmaEdge();

	U32  irqPosition() const;
	bool vIrqHolds() const;
	void rescheduleIrq(bool heldBefore);

	void startMultiply(U8 value);
	void startDivide(U8 value);
	void aluStep();

	void aluEdge() {
		if (m_aluSteps != 0)
			aluStep();
	}

	void spendCycle();
	void chargeAccess(U32 cycles);
	U32  accessCycles(U32 address) const;
	U8   readCycle(U32 address, CycleKind kind, bool memoryLock = false);

	U8   readSystem(U32 address);
	void writeSystem(U32 address, U8 value);

	void mapConsole();
	void mapSystemArea(U8 bankFirst, U8 bankLast);

	U8   readExternalRegister(U32 address);
	void writeExternalRegister(U32 address, U8 value);
	U8   readCpuRegister(U32 address);
	void writeCpuRegister(U32 address, U8 value);
};

} // namespace SNES
