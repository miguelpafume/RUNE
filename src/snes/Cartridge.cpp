#include "Cartridge.hpp"
#include "Bus.hpp"

#include <bit>
#include <stdexcept>
#include <string>
#include <utility>

namespace SNES {

constexpr std::size_t COPIER_HEADER_SIZE = 512;
constexpr std::size_t LOROM_BANK_SIZE = 0x8000;

constexpr std::size_t MIN_ROM_SIZE = LOROM_BANK_SIZE;
constexpr std::size_t MAX_ROM_SIZE = 0x400000;

constexpr std::size_t LOROM_HEADER = 0x7FC0;
constexpr std::size_t HIROM_HEADER = 0xFFC0;
constexpr std::size_t HEADER_SIZE = 0x40;

constexpr U8 SRAM_INITIAL_VALUE = 0xFF;

constexpr std::size_t TITLE = 0x00;
constexpr std::size_t TITLE_LENGTH = 21;
constexpr std::size_t MAP_MODE = 0x15;
constexpr std::size_t CART_TYPE = 0x16;
constexpr std::size_t ROM_SIZE = 0x17;
constexpr std::size_t SRAM_SIZE = 0x18;
constexpr std::size_t DESTINATION = 0x19;
constexpr std::size_t VERSION = 0x1B;
constexpr std::size_t COMPLEMENT = 0x1C;
constexpr std::size_t CHECKSUM = 0x1E;
constexpr std::size_t RESET_VECTOR = 0x3C;

constexpr U16 RESET_VECTOR_MIN = 0x8000;

constexpr int SCORE_LIKELY = 8;
constexpr int SCORE_PLAUSIBLE = 4;
constexpr int SCORE_IMPLAUSIBLE = -4;
constexpr int SCORE_UNLIKELY = -8;
constexpr int SCORE_CHECKSUM = 4;
constexpr int SCORE_MAP_MODE = 2;

constexpr U8 MAP_MODE_FASTROM = 0x10;
constexpr U8 MAP_MODE_LOROM_VALUE = 0x20;
constexpr U8 MAP_MODE_HIROM_VALUE = 0x21;

constexpr U32 SIZE_UNIT = 1024;
constexpr U8  SIZE_FIELD_MASK = 0x0F;
constexpr U8  MAX_SRAM_SIZE_FIELD = 8;

constexpr U8 CART_TYPE_COPROCESSOR = 0x03;

constexpr std::size_t LOROM_RAM_A_MAX_ROM = 0x200000;

U16 readWord(const std::vector<U8>& data, std::size_t offset) { return static_cast<U16>(data[offset] | data[offset + 1] << 8); }

int opcodeScore(U8 opcode) {
	switch (opcode) {
	case 0x78 : // SEI
	case 0x18 : // CLC (CLC; XCE)
	case 0x38 : // SEC (SEC; XCE)
	case 0x9C : // STZ abs (STZ $4200)
	case 0x4C : // JMP abs
	case 0x5C : // JML long
		return SCORE_LIKELY;

	case 0xC2 : // REP
	case 0xE2 : // SEP
	case 0xAD : // LDA abs
	case 0xAE : // LDX abs
	case 0xAC : // LDY abs
	case 0xAF : // LDA long
	case 0xA9 : // LDA #
	case 0xA2 : // LDX #
	case 0xA0 : // LDY #
	case 0x20 : // JSR abs
	case 0x22 : // JSL long
		return SCORE_PLAUSIBLE;

	case 0x40 : // RTI
	case 0x60 : // RTS
	case 0x6B : // RTL
	case 0xCD : // CMP abs
	case 0xEC : // CPX abs
	case 0xCC : // CPY abs
		return SCORE_IMPLAUSIBLE;

	case 0x00 : // BRK
	case 0x02 : // COP
	case 0xDB : // STP
	case 0x42 : // WDM
	case 0xFF : // SBC long,X
		return SCORE_UNLIKELY;

	default :
		return 0;
	}
}

const char* coprocessorName(U8 type) {
	switch (type >> 4) {
	case 0x0 :
		return "DSP-n";
	case 0x1 :
		return "SuperFX";
	case 0x2 :
		return "OBC1";
	case 0x3 :
		return "SA-1";
	case 0x4 :
		return "S-DD1";
	case 0x5 :
		return "S-RTC";
	case 0xE :
		return "Super Game Boy or Satellaview";
	case 0xF :
		return "SPC7110, ST01x, ST018 or CX4";
	default :
		return "unknown";
	}
}

Cartridge::Cartridge(std::vector<U8> image) : m_rom {std::move(image)} {
	stripCopierHeader();
	checkSize();

	// Pad with zeros so the last 4 KiB block isn't cut short
	if (m_rom.size() % Bus::BLOCK_SIZE != 0) {
		m_rom.resize((m_rom.size() / Bus::BLOCK_SIZE + 1) * Bus::BLOCK_SIZE, 0);
	}

	m_headerBase = scoreHeader(HIROM_HEADER) > scoreHeader(LOROM_HEADER) ? HIROM_HEADER : LOROM_HEADER;

	readHeader();
	checkCoprocessor();

	m_sram.assign(m_header.sramSize, SRAM_INITIAL_VALUE);
	m_computedChecksum = checksum();
}

void Cartridge::stripCopierHeader() {
	if (m_rom.size() % LOROM_BANK_SIZE == COPIER_HEADER_SIZE) {
		m_rom.erase(m_rom.begin(), m_rom.begin() + COPIER_HEADER_SIZE);
	}
}

void Cartridge::checkSize() const {
	if (m_rom.size() < MIN_ROM_SIZE) {
		throw std::runtime_error("ROM image of " + std::to_string(m_rom.size()) + " bytes is smaller than one 32 KiB LoROM bank");
	}

	if (m_rom.size() > MAX_ROM_SIZE) {
		throw std::runtime_error("ROM image larger than 4 MiB: ExLoROM and ExHiROM are not supported");
	}
}

int Cartridge::scoreHeader(std::size_t base) const {
	// A 32 KiB ROM is too small to have a header at $FFC0
	if (m_rom.size() < base + HEADER_SIZE) {
		return 0;
	}

	const U16 resetVector = readWord(m_rom, base + RESET_VECTOR);

	if (resetVector < RESET_VECTOR_MIN) {
		return 0;
	}

	// The reset vector is a CPU address, this finds where it lands in the file
	const std::size_t firstInstruction = (base & ~(LOROM_BANK_SIZE - 1)) | (resetVector & (LOROM_BANK_SIZE - 1));

	int score = opcodeScore(m_rom[firstInstruction]);

	if (static_cast<U16>(readWord(m_rom, base + CHECKSUM) + readWord(m_rom, base + COMPLEMENT)) == 0xFFFF) {
		score += SCORE_CHECKSUM;
	}

	const U8 mapMode = m_rom[base + MAP_MODE] & ~MAP_MODE_FASTROM;

	if ((base == LOROM_HEADER && mapMode == MAP_MODE_LOROM_VALUE) || (base == HIROM_HEADER && mapMode == MAP_MODE_HIROM_VALUE)) {
		score += SCORE_MAP_MODE;
	}

	return score < 0 ? 0 : score;
}

bool isPalDestination(U8 destination) {
	return (destination >= 0x02 && destination <= 0x0C) || destination == 0x11 || destination == 0x12;
}

void Cartridge::readHeader() {
	const std::size_t base = m_headerBase;

	// Japanese titles can have katakana, anything that isn't ASCII is shown as '?'
	for (std::size_t i = 0; i < TITLE_LENGTH; i++) {
		const U8 c = m_rom[base + TITLE + i];

		m_header.title += (c >= 0x20 && c <= 0x7E) ? static_cast<char>(c) : '?';
	}

	m_header.title.erase(m_header.title.find_last_not_of(' ') + 1);

	// Trust the score over the map mode byte, some titles spill into it
	m_header.mapMode = base == HIROM_HEADER ? MapMode::HiRom : MapMode::LoRom;
	m_header.fastRom = m_rom[base + MAP_MODE] & MAP_MODE_FASTROM;

	m_header.type = m_rom[base + CART_TYPE];
	m_header.declaredRomSize = SIZE_UNIT << (m_rom[base + ROM_SIZE] & SIZE_FIELD_MASK);

	U8 sramSizeField = m_rom[base + SRAM_SIZE] & SIZE_FIELD_MASK;

	if (sramSizeField > MAX_SRAM_SIZE_FIELD) {
		sramSizeField = MAX_SRAM_SIZE_FIELD;
	}

	m_header.sramSize = sramSizeField ? SIZE_UNIT << sramSizeField : 0;

	m_header.destination = m_rom[base + DESTINATION];
	m_header.region = isPalDestination(m_header.destination) ? Region::Pal : Region::Ntsc;
	m_header.version = m_rom[base + VERSION];
	m_header.complement = readWord(m_rom, base + COMPLEMENT);
	m_header.checksum = readWord(m_rom, base + CHECKSUM);
	m_header.resetVector = readWord(m_rom, base + RESET_VECTOR);
}

void Cartridge::checkCoprocessor() const {
	if ((m_header.type & SIZE_FIELD_MASK) >= CART_TYPE_COPROCESSOR) {
		throw std::runtime_error(std::string("unsupported cartridge: ") + coprocessorName(m_header.type) + " coprocessor");
	}
}

U16 Cartridge::checksum() const {
	const U32 size = static_cast<U32>(m_rom.size());
	const U32 decoded = std::bit_ceil(size);

	U16 sum = 0;

	for (U32 offset = 0; offset < decoded; offset++) {
		sum = static_cast<U16>(sum + m_rom[mirror(offset, size)]);
	}

	return sum;
}

// Same as bsnes Bus::mirror. A 1.5 or 3 MiB ROM is two chips, and the smaller one
// shows up more than once in the address space
U32 Cartridge::mirror(U32 offset, U32 size) {
	if (size == 0) {
		return 0;
	}

	U32 base = 0;
	U32 mask = 1u << 23;

	while (offset >= size) {
		while (!(offset & mask)) {
			mask >>= 1;
		}

		offset -= mask;

		if (size > mask) {
			size -= mask;
			base += mask;
		}

		mask >>= 1;
	}

	return base + offset;
}

U32 Cartridge::sramOffset(U32 address) const {
	const U32 bank = (address & RUNE::BANK_MASK) >> 16;
	U32       offset {};

	if (m_header.mapMode == MapMode::LoRom) {
		offset = (bank & 0x0F) << 15 | (address & 0x7FFF);
	} else {
		offset = (bank & 0x1F) << 13 | (address & 0x1FFF);
	}

	return offset & (static_cast<U32>(m_sram.size()) - 1);
}

U8 Cartridge::readSram(U32 address) const { return m_sram[sramOffset(address)]; }

void Cartridge::writeSram(U32 address, U8 value) { m_sram[sramOffset(address)] = value; }

void Cartridge::map(Bus& bus) const {
	if (m_header.mapMode == MapMode::LoRom) {
		mapLoRom(bus);
	} else {
		mapHiRom(bus);
	}
}

void Cartridge::mapLoRom(Bus& bus) const {
	if (m_sram.empty()) {
		mapRom(bus, 0x00, 0x7D, 0x8000, 0xFFFF);
		mapRom(bus, 0x80, 0xFF, 0x8000, 0xFFFF);

		return;
	}

	if (m_rom.size() <= LOROM_RAM_A_MAX_ROM) {
		mapRom(bus, 0x00, 0x6F, 0x8000, 0xFFFF);
		mapRom(bus, 0x80, 0xEF, 0x8000, 0xFFFF);

		bus.mapRange(0x70, 0x7D, 0x0000, 0xFFFF, Bus::BlockType::Sram);
		bus.mapRange(0xF0, 0xFF, 0x0000, 0xFFFF, Bus::BlockType::Sram);

		return;
	}

	mapRom(bus, 0x00, 0x7D, 0x8000, 0xFFFF);
	mapRom(bus, 0x80, 0xFF, 0x8000, 0xFFFF);

	bus.mapRange(0x70, 0x7D, 0x0000, 0x7FFF, Bus::BlockType::Sram);
	bus.mapRange(0xF0, 0xFF, 0x0000, 0x7FFF, Bus::BlockType::Sram);
}

void Cartridge::mapHiRom(Bus& bus) const {
	mapRom(bus, 0x00, 0x3F, 0x8000, 0xFFFF);
	mapRom(bus, 0x80, 0xBF, 0x8000, 0xFFFF);
	mapRom(bus, 0x40, 0x7D, 0x0000, 0xFFFF);
	mapRom(bus, 0xC0, 0xFF, 0x0000, 0xFFFF);

	if (!m_sram.empty()) {
		bus.mapRange(0x20, 0x3F, 0x6000, 0x7FFF, Bus::BlockType::Sram);
		bus.mapRange(0xA0, 0xBF, 0x6000, 0x7FFF, Bus::BlockType::Sram);
	}
}

// ROM is read only, so there's no write pointer and writes are ignored
void Cartridge::mapRom(Bus& bus, U8 bankFirst, U8 bankLast, U16 addressFirst, U16 addressLast) const {
	const U32 size = static_cast<U32>(m_rom.size());

	for (U32 bank = bankFirst; bank <= bankLast; bank++) {
		for (U32 address = addressFirst; address <= addressLast; address += Bus::BLOCK_SIZE) {
			U32 offset {};

			if (m_header.mapMode == MapMode::LoRom) {
				offset = (bank & 0x7F) << 15 | (address & 0x7FFF);
			} else {
				offset = (bank & 0x3F) << 16 | address;
			}

			// Each block is a whole 4 KiB of the ROM, so mirror runs once per block
			bus.mapBlock(bank << 16 | address, &m_rom[mirror(offset, size)], nullptr, Bus::BlockType::OpenBus);
		}
	}
}

} // namespace SNES
