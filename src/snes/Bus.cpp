#include "Bus.hpp"
#include "APU.hpp"
#include "Cartridge.hpp"
#include "Constants.hpp"
#include "DMA.hpp"
#include "Gamepad.hpp"
#include "PPU.hpp"

#include <cstddef>
#include <limits>

namespace SNES {

// The 65816 uses 24 address lines, so anything above bank $FF wraps
using RUNE::ADDRESS_MASK;

constexpr std::size_t TEST_MEMORY_SIZE = std::size_t {ADDRESS_MASK} + 1; // 16 MB, flat

constexpr std::size_t EXPECTED_DIRTY_PER_CASE = 16;
constexpr std::size_t EXPECTED_CYCLES_PER_CASE = 16;

constexpr U32 BLOCK_OFFSET_MASK = Bus::BLOCK_SIZE - 1;

constexpr U32 WRAM_START = 0x7E0000;       // Banks $7E-$7F
constexpr U32 WRAM_MIRROR_SIZE = 0x2000;   // the first 8 KiB also show up at $0000-$1FFF
constexpr U32 WRAM_ADDRESS_MASK = 0x1FFFF; // the port address has 17 bits

// Real hardware starts WRAM with garbage. bsnes and MesenCE fill it with random bytes,
// but then two runs of the same ROM could differ, so we use snes9x's fixed $55
constexpr U8 WRAM_POWER_ON_VALUE = 0x55;

// External registers, all at $21xx
constexpr U16 PPU_FIRST = 0x2100;
constexpr U16 PPU_LAST = 0x213F;
constexpr U16 APU_FIRST = 0x2140; // four ports, mirrored up to $217F
constexpr U16 APU_LAST = 0x217F;
constexpr U16 APU_PORT_MASK = 0x0003;

constexpr U16 WMDATA = 0x2180;
constexpr U16 WMADDL = 0x2181;
constexpr U16 WMADDM = 0x2182;
constexpr U16 WMADDH = 0x2183;

// CPU registers
constexpr U16 JOYSER0 = 0x4016; // serial joypad ports, read one bit at a time
constexpr U16 JOYSER1 = 0x4017;
constexpr U16 INTERNAL_FIRST = 0x4200; // NMI/IRQ, multiply and divide, auto joypad
constexpr U16 INTERNAL_LAST = 0x421F;
constexpr U16 DMA_FIRST = 0x4300; // 8 channels x 16 bytes
constexpr U16 DMA_LAST = 0x437F;
constexpr U16 NMITIMEN = 0x4200;
constexpr U16 WRMPYA = 0x4202;
constexpr U16 WRMPYB = 0x4203;
constexpr U16 WRDIVL = 0x4204;
constexpr U16 WRDIVH = 0x4205;
constexpr U16 WRDIVB = 0x4206;
constexpr U16 HTIMEL = 0x4207;
constexpr U16 HTIMEH = 0x4208;
constexpr U16 VTIMEL = 0x4209;
constexpr U16 VTIMEH = 0x420A;
constexpr U16 MDMAEN = 0x420B;
constexpr U16 HDMAEN = 0x420C;
constexpr U16 MEMSEL = 0x420D;
constexpr U16 RDNMI = 0x4210;
constexpr U16 TIMEUP = 0x4211;
constexpr U16 HVBJOY = 0x4212;
constexpr U16 RDDIVL = 0x4214;
constexpr U16 RDMPYL = 0x4216;
constexpr U16 RDMPYH = 0x4217;
constexpr U16 JOY_FIRST = 0x4218; // 4 controllers x 2 bytes
constexpr U16 JOY_LAST = 0x421F;

constexpr U8 NMITIMEN_NMI = 0x80;
constexpr U8 NMITIMEN_V_IRQ = 0x20;
constexpr U8 NMITIMEN_H_IRQ = 0x10;
constexpr U8 NMITIMEN_AUTO_JOYPAD = 0x01;

constexpr U8 MEMSEL_FASTROM = 0x01;

constexpr U8 RDNMI_FLAG = 0x80;
constexpr U8 RDNMI_OPEN_BUS_MASK = 0x70;
constexpr U8 CPU_VERSION = 0x02; // the value bsnes and MesenCE report

constexpr U8 TIMEUP_FLAG = 0x80;
constexpr U8 TIMEUP_OPEN_BUS_MASK = 0x7F;

constexpr U8 HVBJOY_VBLANK = 0x80;
constexpr U8 HVBJOY_HBLANK = 0x40;
constexpr U8 HVBJOY_JOYPAD_BUSY = 0x01;
constexpr U8 HVBJOY_OPEN_BUS_MASK = 0x3E;

constexpr U8 JOYSER0_LATCH = 0x01;
constexpr U8 JOYSER0_OPEN_BUS_MASK = 0xFC;
constexpr U8 JOYSER1_OPEN_BUS_MASK = 0xE0;
constexpr U8 JOYSER1_GROUNDED = 0x1C; // these pins are tied to ground and read as 1

// H-Blank starts at the end of one line and lasts into the next
constexpr U32 HBLANK_START = 1096;
constexpr U32 HBLANK_END = 2;

// The auto joypad read starts on an edge between H = 130 and H = 384 of the first V-Blank line
constexpr U32 JOYPAD_START_FIRST = 130;
constexpr U32 JOYPAD_START_LAST = 384;

constexpr U32 CYCLES_INTERNAL = CYCLES_FAST; // cycles that don't touch memory take 6 too

// HDMA starts the frame early in line 0, then copies late in each visible line, in H-Blank
constexpr U32 HDMA_SETUP_POSITION = 12;
constexpr U32 HDMA_POSITION = 1104;
constexpr U32 DMA_CLOCK_CYCLES = 8;
constexpr U32 NO_HDMA = U32_MAX;

// The CPU stops for this once per line, while the WRAM refreshes
constexpr U32 DRAM_REFRESH_POSITION = 538;
constexpr U32 DRAM_REFRESH_CYCLES = 40;
constexpr U32 DRAM_REFRESH_ALU_STEPS = 5; // the multiplier and divider keep going during it

// Where the IRQ fires in the line, like bsnes. The counters look 10 cycles back
constexpr U32 V_IRQ_POSITION = 10;
constexpr U32 H_IRQ_POSITION = 14; // plus 4 cycles per dot of HTIME
constexpr U32 NO_IRQ = U32_MAX;
constexpr U32 IRQ_BLOCKED_POSITION = 6; // in line 0, both counters still read 0 here
constexpr U8  HV_TIME_HIGH_MASK = 0x01; // HTIME and VTIME have 9 bits

constexpr U32 MULTIPLY_STEPS = 8;
constexpr U32 DIVIDE_STEPS = 16;

Bus::Bus(Mode mode) : m_mode {mode} {
	if (m_mode == Mode::Test) {
		m_testMemory.assign(TEST_MEMORY_SIZE, 0);
		m_dirty.reserve(EXPECTED_DIRTY_PER_CASE);
		m_accessLog.reserve(EXPECTED_CYCLES_PER_CASE);
		return;
	}

	m_wram.assign(WRAM_SIZE, WRAM_POWER_ON_VALUE);
	mapConsole();
}

// System mode only, all the time in the console passes through here
void Bus::advanceClock(U32 cycles) {
	const U32  hClockBefore = m_ppu->hClock();
	const bool vBlankBefore = m_ppu->inVBlank();

	// Stops on every 128-cycle edge on the way, where the auto joypad read takes a step
	while (m_masterCycles + cycles >= m_nextJoypadEdge) {
		const U32 step = static_cast<U32>(m_nextJoypadEdge - m_masterCycles);

		m_masterCycles += step;
		m_ppu->advance(step);
		cycles -= step;
		m_nextJoypadEdge += JOYPAD_STEP_CYCLES;

		joypadEdge();
	}

	m_masterCycles += cycles;
	m_ppu->advance(cycles);

	// Goes up when V-Blank starts and down when it ends
	if (m_ppu->inVBlank() != vBlankBefore) {
		m_nmiFlag = m_ppu->inVBlank();

		if (m_nmiFlag && m_nmiEnable) {
			m_nmiPending = true;
		}
	}

	const U32 hClock = m_ppu->hClock();

	// H went back, so a new line started
	if (hClock < hClockBefore) {
		startHdmaLine(hClock);
		m_irqPosition = irqPosition();
	}

	if (hClock >= m_hdmaPosition)
		hdmaEdge();

	if (hClock >= m_irqPosition) {
		m_irqLine = true;
		m_irqPosition = NO_IRQ;
	}

	// Accesses cost 6, 8 or 12, so H can jump over the exact position
	if (hClockBefore < DRAM_REFRESH_POSITION && hClock >= DRAM_REFRESH_POSITION) {
		advanceClock(DRAM_REFRESH_CYCLES);

		for (U32 step = 0; step < DRAM_REFRESH_ALU_STEPS; step++) {
			aluEdge();
		}
	}
}

// One step of the auto joypad read, the same state machine as bsnes
void Bus::joypadEdge() {
	const U32  hClock = m_ppu->hClock();
	const bool startEdge = m_masterCycles % (2 * JOYPAD_STEP_CYCLES) == 0; // only every other edge can start it

	if (m_ppu->scanline() == VBLANK_START_LINE && startEdge && hClock >= JOYPAD_START_FIRST && hClock <= JOYPAD_START_LAST) {
		m_joypadStep = 0;
	} else if (m_joypadStep >= JOYPAD_STEPS) {
		return;
	} else {
		m_joypadStep++;
	}

	// Step 0 latches the controllers and clears the registers, step 1 lets the latch go
	if (m_joypadStep == 0) {
		m_autoJoypadLatch = m_autoJoypad;
		latchPorts();

		if (m_autoJoypad) {
			m_joy = {};
		}
	}

	if (m_joypadStep == 1) {
		m_autoJoypadLatch = false;
		latchPorts();
	}

	if (m_joypadStep != 1 && !m_autoJoypad) {
		m_joypadStep = JOYPAD_STEPS;
		return;
	}

	// One bit per pair of steps: read on the even step, shifted in on the odd one
	if (m_joypadStep >= 2) {
		for (std::size_t port = 0; port < m_ports.size(); port++) {
			if ((m_joypadStep & 1) == 0) {
				m_joypadBits[port] = m_ports[port]->data();
			} else {
				m_joy[port] = static_cast<U16>(m_joy[port] << 1 | (m_joypadBits[port] & 0x01));
			}
		}
	}
}

void Bus::latchPorts() {
	const bool high = m_cpuLatch || m_autoJoypadLatch;

	for (Gamepad* gamepad : m_ports) {
		gamepad->latch(high);
	}
}

// Taking it also clears it, so each V-Blank fires only one NMI
// false means the IRQ, the only other one interruptPolled() can see
bool Bus::takeNmi() {
	if (!m_nmiPolled) {
		return false;
	}

	m_nmiPolled = false;
	m_nmiPending = false;

	return true;
}

// H where the IRQ fires in the current line, NO_IRQ if it doesn't
U32 Bus::irqPosition() const {
	const U32 line = m_ppu->scanline();

	if (!m_hIrq) {
		return m_vIrq && line == m_vTime ? V_IRQ_POSITION : NO_IRQ;
	}

	// HTIME 340 and above is past the last dot
	if (m_hTime >= DOTS_PER_SCANLINE - 1) {
		return NO_IRQ;
	}

	U32 position = H_IRQ_POSITION + m_hTime * CYCLES_PER_DOT;
	U32 vLine = line;

	// HTIME 338 and 339 fire early in the next line, still matched against the line before
	if (position >= CYCLES_PER_SCANLINE) {
		position -= CYCLES_PER_SCANLINE;
		vLine = (line == 0 ? SCANLINES_PER_FRAME : line) - 1;

		if (line == 0 && position == IRQ_BLOCKED_POSITION) {
			return NO_IRQ;
		}
	}

	if (m_vIrq && vLine != m_vTime) {
		return NO_IRQ;
	}

	return position;
}

// V-IRQ alone holds from its position to the end of the line
bool Bus::vIrqHolds() const { return m_vIrq && !m_hIrq && m_ppu->scanline() == m_vTime && m_ppu->hClock() >= V_IRQ_POSITION; }

// After a write to $4200 or $4207-$420A, in the middle of the line
void Bus::rescheduleIrq(bool heldBefore) {
	m_irqPosition = irqPosition();

	if (m_irqPosition > m_ppu->hClock()) {
		return;
	}

	m_irqPosition = NO_IRQ;

	// The H position already went by, but V-IRQ alone fires when it starts to hold
	if (!heldBefore && vIrqHolds()) {
		m_irqLine = true;
	}
}

// bsnes's long multiply and divide, one bit per CPU cycle
// A write while one is running is ignored
void Bus::startMultiply(U8 value) {
	m_rdmpy = 0;

	if (m_aluSteps != 0) {
		return;
	}

	// RDDIV holds both factors and shifts right, so it ends with WRMPYB
	m_rddiv = static_cast<U16>(value << 8 | m_wrmpya);
	m_aluShift = value;
	m_aluSteps = MULTIPLY_STEPS;
	m_dividing = false;
}

void Bus::startDivide(U8 value) {
	m_rdmpy = m_wrdiva;

	if (m_aluSteps != 0) {
		return;
	}

	// Dividing by 0 gives $FFFF and leaves the dividend as the remainder
	m_aluShift = U32 {value} << 16;
	m_aluSteps = DIVIDE_STEPS;
	m_dividing = true;
}

// Multiply: for each 1 bit of the first factor, adds the second one shifted to that bit
// Divide: wherever the shifted divisor fits, takes it out and that quotient bit becomes 1
void Bus::aluStep() {
	m_aluSteps--;

	if (!m_dividing) {
		if (m_rddiv & 1) {
			m_rdmpy = static_cast<U16>(m_rdmpy + m_aluShift);
		}

		m_rddiv >>= 1;
		m_aluShift <<= 1;
		return;
	}

	m_rddiv = static_cast<U16>(m_rddiv << 1);
	m_aluShift >>= 1;

	if (m_rdmpy >= m_aluShift) {
		m_rdmpy = static_cast<U16>(m_rdmpy - m_aluShift);
		m_rddiv |= 1;
	}
}

void Bus::startHdmaLine(U32 hClock) {
	const U32 line = m_ppu->scanline();

	if (line == 0) {
		// The setup moves with the 8-cycle DMA clock, as it was when the line started
		const U64 lineStart = m_masterCycles - hClock;

		m_hdmaPosition = HDMA_SETUP_POSITION + static_cast<U32>(lineStart % DMA_CLOCK_CYCLES);
		m_hdmaSetupDone = false;
	} else if (line < VBLANK_START_LINE) {
		m_hdmaPosition = HDMA_POSITION;
	} else {
		m_hdmaPosition = NO_HDMA;
	}
}

// H reached the setup on line 0, or the H-Blank of a visible line
void Bus::hdmaEdge() {
	if (!m_hdmaSetupDone) {
		m_hdmaSetupDone = true;
		m_hdmaPosition = HDMA_POSITION;
		m_dma->hdmaReset();

		if (m_dma->hdmaEnabled()) {
			m_hdmaRequest = HdmaRequest::Setup;
		}

		return;
	}

	m_hdmaPosition = NO_HDMA;

	if (m_dma->hdmaActive()) {
		m_hdmaRequest = HdmaRequest::Transfer;
	}
}

// Taking it also clears it, like the NMI
Bus::HdmaRequest Bus::takeHdma() {
	const HdmaRequest request = m_hdmaRequest;

	m_hdmaRequest = HdmaRequest::None;

	return request;
}

// One CPU cycle. A pending DMA or HDMA goes first, then the interrupts are sampled for the CPU
void Bus::chargeAccess(U32 cycles) {
	if (m_dmaPending != 0 || m_hdmaRequest != HdmaRequest::None) {
		const U8 channels = m_dmaPending;

		m_dmaPending = 0;
		m_dma->run(channels, cycles);

		// Right after a DMA the CPU misses one look
		m_nmiPolled = false;
		m_irqPolled = false;
	} else {
		m_nmiPolled = m_nmiPending;
		m_irqPolled = m_irqLine && !m_irqDisable;
	}

	advanceClock(cycles);
}

// runs before the cycle does anything, so a write past the limit never reaches memory
void Bus::spendCycle() {
	if (m_accessLog.size() >= m_cycleBudget)
		throw CycleBudgetExhausted {};
}

U8 Bus::readCycle(U32 address, CycleKind kind, bool memoryLock) {
	const U32 target = address & ADDRESS_MASK;

	if (m_mode == Mode::Test) {
		spendCycle();

		const U8 value = m_testMemory[target];

		m_accessLog.push_back({target, value, kind, false, m_flagE, m_flagM, m_flagX, memoryLock});

		return value;
	}

	// bsnes and MesenCE charge part of this after the access. We charge it all first,
	// the exact moment doesn't matter because the PPU renders whole lines
	chargeAccess(accessCycles(target));

	const U8 value = readSystem(target);

	// The multiplier moves after the read, so the result read is the one from before
	aluEdge();

	// $4xxx registers are inside the CPU, reading them doesn't reach the data bus
	if (m_blockType[target >> BLOCK_SHIFT] != BlockType::CpuRegisters) {
		m_openBus = value;
	}

	return value;
}

// Master cycles of one access, they depend only on the address and on FastROM
U32 Bus::accessCycles(U32 address) const {
	// Banks $40-$7F and $C0-$FF, and $8000-$FFFF of any bank. Only banks $80-$FF can be fast
	if (address & 0x408000) {
		return (address & 0x800000) && m_fastRom ? CYCLES_FAST : CYCLES_SLOW;
	}

	const U16 offset = address & 0xFFFF;

	if (offset < 0x2000 || offset >= 0x6000) {
		return CYCLES_SLOW;
	}

	if (offset >= 0x4000 && offset < 0x4200) {
		return CYCLES_XSLOW;
	}

	return CYCLES_FAST;
}

// Loads one entry of the case's "initial.ram"
void Bus::poke(U32 address, U8 value) {
	const U32 target = address & ADDRESS_MASK;

	m_testMemory[target] = value;
	m_dirty.push_back(target);
}

// Test mode reads the flat memory for the comparator
// System mode skips the registers, reading some of them changes their state
U8 Bus::peek(U32 address) const {
	const U32 target = address & ADDRESS_MASK;

	if (m_mode == Mode::Test) {
		return m_testMemory[target];
	}

	const U32 block = target >> BLOCK_SHIFT;

	if (const U8* data = m_readMap[block]) {
		return data[target & BLOCK_OFFSET_MASK];
	}

	if (m_blockType[block] == BlockType::Sram) {
		return m_cartridge->readSram(target);
	}

	return m_openBus;
}

void Bus::beginCase() {
	for (const U32 address : m_dirty) {
		m_testMemory[address] = 0;
	}

	m_dirty.clear();
	m_accessLog.clear();
	m_cycleBudget = std::numeric_limits<std::size_t>::max();
}

void Bus::writeData(U32 address, U8 value, bool memoryLock) {
	const U32 target = address & ADDRESS_MASK;

	if (m_mode == Mode::Test) {
		spendCycle();

		m_testMemory[target] = value;

		m_dirty.push_back(target);
		m_accessLog.push_back({target, value, CycleKind::DataAccess, true, m_flagE, m_flagM, m_flagX, memoryLock});

		return;
	}

	chargeAccess(accessCycles(target));
	aluEdge();

	m_openBus = value;
	writeSystem(target, value);
}

void Bus::internalCycle(U32 address, bool memoryLock) {
	if (m_mode == Mode::Test) {
		spendCycle();

		const U32 target = address & ADDRESS_MASK;

		m_accessLog.push_back({target, std::nullopt, CycleKind::Internal, false, m_flagE, m_flagM, m_flagX, memoryLock});
		return;
	}

	chargeAccess(CYCLES_INTERNAL);
	aluEdge();
}

void Bus::invalidWrite(U32 address, U8 value, bool memoryLock) {
	if (m_mode == Mode::Test) {
		spendCycle();

		const U32 target = address & ADDRESS_MASK;

		m_accessLog.push_back({target, value, CycleKind::Internal, true, m_flagE, m_flagM, m_flagX, memoryLock});
		return;
	}

	chargeAccess(CYCLES_INTERNAL);
	aluEdge();
}

void Bus::haltedCycle() {
	if (m_mode == Mode::Test) {
		spendCycle();

		m_accessLog.push_back({0, std::nullopt, CycleKind::Internal});
		return;
	}

	// Only WAI and STP get here
	m_irqWake = m_irqLine;

	chargeAccess(CYCLES_INTERNAL);
	aluEdge();
}

void Bus::setFlags(bool emulation, bool flagM, bool flagX) {
	m_flagE = emulation;
	m_flagM = flagM;
	m_flagX = flagX;
}

// An invalid access still takes its time, but it reads $00 and writes nothing
U8 Bus::dmaRead(U32 address, bool valid) {
	m_openBus = valid ? readSystem(address) : U8 {0x00};

	return m_openBus;
}

void Bus::dmaWrite(U32 address, U8 value, bool valid) {
	if (valid)
		writeSystem(address, value);
}

U8 Bus::readSystem(U32 address) {
	const U32 block = address >> BLOCK_SHIFT;

	if (const U8* data = m_readMap[block]) {
		return data[address & BLOCK_OFFSET_MASK];
	}

	switch (m_blockType[block]) {
	case BlockType::ExternalRegisters :
		return readExternalRegister(address);

	case BlockType::CpuRegisters :
		return readCpuRegister(address);

	case BlockType::Sram :
		return m_cartridge->readSram(address);

	case BlockType::OpenBus :
		break;
	}

	return m_openBus;
}

void Bus::writeSystem(U32 address, U8 value) {
	const U32 block = address >> BLOCK_SHIFT;

	if (U8* data = m_writeMap[block]) {
		data[address & BLOCK_OFFSET_MASK] = value;
		return;
	}

	switch (m_blockType[block]) {
	case BlockType::ExternalRegisters :
		writeExternalRegister(address, value);
		break;

	case BlockType::CpuRegisters :
		writeCpuRegister(address, value);
		break;

	case BlockType::Sram :
		m_cartridge->writeSram(address, value);
		break;

	case BlockType::OpenBus :
		break;
	}
}

void Bus::mapBlock(U32 address, const U8* read, U8* write, BlockType type) {
	const U32 block = (address & ADDRESS_MASK) >> BLOCK_SHIFT;

	m_readMap[block] = read;
	m_writeMap[block] = write;
	m_blockType[block] = type;
}

void Bus::mapRange(U8 bankFirst, U8 bankLast, U16 addressFirst, U16 addressLast, BlockType type) {
	for (U32 bank = bankFirst; bank <= bankLast; bank++) {
		for (U32 address = addressFirst; address <= addressLast; address += BLOCK_SIZE) {
			mapBlock(bank << 16 | address, nullptr, nullptr, type);
		}
	}
}

// WRAM and registers, the same for every cartridge
// The cartridge never maps these addresses, so it doesn't matter who maps first
void Bus::mapConsole() {
	for (U32 offset = 0; offset < WRAM_SIZE; offset += BLOCK_SIZE) {
		mapBlock(WRAM_START + offset, &m_wram[offset], &m_wram[offset], BlockType::OpenBus);
	}

	mapSystemArea(0x00, 0x3F);
	mapSystemArea(0x80, 0xBF);
}

// $0000-$5FFF of banks $00-$3F and $80-$BF: WRAM mirror and both register areas
void Bus::mapSystemArea(U8 bankFirst, U8 bankLast) {
	for (U32 bank = bankFirst; bank <= bankLast; bank++) {
		for (U32 offset = 0; offset < WRAM_MIRROR_SIZE; offset += BLOCK_SIZE) {
			mapBlock(bank << 16 | offset, &m_wram[offset], &m_wram[offset], BlockType::OpenBus);
		}
	}

	mapRange(bankFirst, bankLast, 0x2000, 0x3FFF, BlockType::ExternalRegisters);
	mapRange(bankFirst, bankLast, 0x4000, 0x5FFF, BlockType::CpuRegisters);
}

// Chips outside the CPU only see the low byte of the address, so they answer at $21xx
// The rest of $2000-$3FFF reaches nothing and reads as open bus
U8 Bus::readExternalRegister(U32 address) {
	const U16 offset = address & 0xFFFF;

	if (offset >= PPU_FIRST && offset <= PPU_LAST) {
		return m_ppu->readRegister(offset, m_openBus);
	}

	if (offset >= APU_FIRST && offset <= APU_LAST) {
		return m_apu->readPort(static_cast<U8>(offset & APU_PORT_MASK));
	}

	// $2181-$2183 can only be written
	if (offset == WMDATA) {
		const U8 value = m_wram[m_wramAddress];
		m_wramAddress = (m_wramAddress + 1) & WRAM_ADDRESS_MASK;

		return value;
	}

	return m_openBus;
}

void Bus::writeExternalRegister(U32 address, U8 value) {
	const U16 offset = address & 0xFFFF;

	if (offset >= PPU_FIRST && offset <= PPU_LAST) {
		m_ppu->writeRegister(offset, value);
		return;
	}

	if (offset >= APU_FIRST && offset <= APU_LAST) {
		m_apu->writePort(static_cast<U8>(offset & APU_PORT_MASK), value);
		return;
	}

	switch (offset) {
	case WMDATA :
		m_wram[m_wramAddress] = value;
		m_wramAddress = (m_wramAddress + 1) & WRAM_ADDRESS_MASK;
		break;

	case WMADDL :
		m_wramAddress = (m_wramAddress & 0x1FF00) | value;
		break;

	case WMADDM :
		m_wramAddress = (m_wramAddress & 0x100FF) | U32 {value} << 8;
		break;

	case WMADDH : // only bit 0 is used, it picks bank $7E or $7F
		m_wramAddress = (m_wramAddress & 0x0FFFF) | U32 {value & 1u} << 16;
		break;

	default :
		break;
	}
}

U8 Bus::readCpuRegister(U32 address) {
	const U16 offset = address & 0xFFFF;

	if (offset == JOYSER0) {
		return static_cast<U8>((m_openBus & JOYSER0_OPEN_BUS_MASK) | m_ports[0]->data());
	}

	if (offset == JOYSER1) {
		return static_cast<U8>((m_openBus & JOYSER1_OPEN_BUS_MASK) | JOYSER1_GROUNDED | m_ports[1]->data());
	}

	if (offset >= INTERNAL_FIRST && offset <= INTERNAL_LAST) {
		if (offset == RDNMI) {
			const U8 value = static_cast<U8>((m_nmiFlag ? RDNMI_FLAG : 0) | (m_openBus & RDNMI_OPEN_BUS_MASK) | CPU_VERSION);

			// Reading clears the flag
			m_nmiFlag = false;

			return value;
		}

		if (offset == HVBJOY) {
			const U32 hClock = m_ppu->hClock();
			U8        value = m_openBus & HVBJOY_OPEN_BUS_MASK;

			if (m_ppu->inVBlank()) {
				value |= HVBJOY_VBLANK;
			}

			if (hClock <= HBLANK_END || hClock >= HBLANK_START) {
				value |= HVBJOY_HBLANK;
			}

			if (m_autoJoypad && m_joypadStep < JOYPAD_STEPS) {
				value |= HVBJOY_JOYPAD_BUSY;
			}

			return value;
		}

		if (offset >= JOY_FIRST && offset <= JOY_LAST) {
			const U32 index = offset - JOY_FIRST;

			if (index >= 2 * m_joy.size()) {
				return 0;
			}

			const U16 joy = m_joy[index / 2];

			return static_cast<U8>((index & 1) ? joy >> 8 : joy);
		}

		// Reading clears it, so the IRQ handler acknowledges it here
		if (offset == TIMEUP) {
			const U8 value = static_cast<U8>((m_irqLine ? TIMEUP_FLAG : 0) | (m_openBus & TIMEUP_OPEN_BUS_MASK));

			m_irqLine = false;

			return value;
		}

		if (offset >= RDDIVL && offset <= RDMPYH) {
			const U16 result = offset < RDMPYL ? m_rddiv : m_rdmpy;

			return static_cast<U8>((offset & 1) ? result >> 8 : result);
		}

		// TODO: $4213 programmable I/O port
		return m_openBus;
	}

	if (offset >= DMA_FIRST && offset <= DMA_LAST) {
		return m_dma->readRegister(offset, m_openBus);
	}

	return m_openBus;
}

void Bus::writeCpuRegister(U32 address, U8 value) {
	const U16 offset = address & 0xFFFF;

	// Writes to $4017 do nothing, $4016 bit 0 already reaches both ports
	if (offset == JOYSER0) {
		m_cpuLatch = (value & JOYSER0_LATCH) != 0;
		latchPorts();
		return;
	}

	if (offset >= INTERNAL_FIRST && offset <= INTERNAL_LAST) {
		switch (offset) {
		case NMITIMEN : {
			const bool enable = (value & NMITIMEN_NMI) != 0;
			const bool heldBefore = vIrqHolds();

			// Turning it on in the middle of V-Blank fires right away, if $4210 wasn't read yet
			if (enable && !m_nmiEnable && m_nmiFlag) {
				m_nmiPending = true;
			}

			m_nmiEnable = enable;
			m_autoJoypad = (value & NMITIMEN_AUTO_JOYPAD) != 0;

			// On step 0 the latch follows the new value. Turned off after step 1, the reading stops
			if (m_joypadStep == 0) {
				m_autoJoypadLatch = m_autoJoypad;
				latchPorts();
			} else if (!m_autoJoypad && m_joypadStep >= 2) {
				m_joypadStep = JOYPAD_STEPS;
			}

			m_hIrq = (value & NMITIMEN_H_IRQ) != 0;
			m_vIrq = (value & NMITIMEN_V_IRQ) != 0;

			// Turning both off also drops an IRQ that was waiting
			if (!m_hIrq && !m_vIrq) {
				m_irqLine = false;
			}

			rescheduleIrq(heldBefore);
			break;
		}

		case WRMPYA :
			m_wrmpya = value;
			break;

		case WRMPYB :
			startMultiply(value);
			break;

		case WRDIVL :
			m_wrdiva = static_cast<U16>((m_wrdiva & 0xFF00) | value);
			break;

		case WRDIVH :
			m_wrdiva = static_cast<U16>((m_wrdiva & 0x00FF) | value << 8);
			break;

		case WRDIVB :
			startDivide(value);
			break;

		case HTIMEL :
		case HTIMEH :
		case VTIMEL :
		case VTIMEH : {
			const bool heldBefore = vIrqHolds();
			U16&       time = offset <= HTIMEH ? m_hTime : m_vTime;

			if (offset == HTIMEL || offset == VTIMEL) {
				time = static_cast<U16>((time & 0x100) | value);
			} else {
				time = static_cast<U16>((time & 0x0FF) | (value & HV_TIME_HIGH_MASK) << 8);
			}

			rescheduleIrq(heldBefore);
			break;
		}

		case MDMAEN :
			m_dmaPending = value;
			break;

		case HDMAEN :
			m_dma->setHdmaEnable(value);
			break;

		case MEMSEL : // ROM in banks $80-$FF runs at 6 cycles when set
			m_fastRom = (value & MEMSEL_FASTROM) != 0;
			break;

		default : // TODO: $4201 programmable I/O port
			break;
		}

		return;
	}

	if (offset >= DMA_FIRST && offset <= DMA_LAST) {
		m_dma->writeRegister(offset, value);
		return;
	}
}

} // namespace SNES
