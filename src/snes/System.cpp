#include "System.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace SNES {

System::System() {
	m_bus.attachApu(m_apu);
	m_bus.attachPpu(m_ppu);
}

const U8* System::framebuffer() { return m_framebuffer.data(); }

void System::loadRom(const std::string& romPath) {
	// file_size already throws a useful error if the file doesn't exist
	std::vector<U8> image(std::filesystem::file_size(romPath));

	std::ifstream file {romPath, std::ios::binary};
	file.read(reinterpret_cast<char*>(image.data()), static_cast<std::streamsize>(image.size()));

	if (!file) {
		throw std::runtime_error("cannot read ROM file: " + romPath);
	}

	m_cartridge.emplace(std::move(image));

	m_bus.attachCartridge(*m_cartridge);
	m_cartridge->map(m_bus);

	printCartridge(romPath);

	m_cpu.reset();
}

void System::printCartridge(const std::string& romPath) const {
	const Cartridge::Header& header = m_cartridge->header();

	const bool isHiRom = header.mapMode == Cartridge::MapMode::HiRom;
	const bool isPal = header.region == Cartridge::Region::Pal;

	std::cerr << std::format("ROM:      {}\n", romPath) << std::format("Title:    {}\n", header.title)
	          << std::format("Map:      {}, {}\n", isHiRom ? "HiROM" : "LoROM", header.fastRom ? "FastROM" : "SlowROM")
	          << std::format("ROM size: {} KiB (header declares {} KiB)\n", m_cartridge->romSize() / KIB, header.declaredRomSize / KIB)
	          << std::format("SRAM:     {} KiB\n", header.sramSize / KIB)
	          << std::format("Region:   {} (destination ${:02X})\n", isPal ? "PAL" : "NTSC", header.destination)
	          << std::format("Version:  1.{}\n", header.version)
	          << std::format("Checksum: ${:04X} computed, ${:04X} declared, complement ${:04X}\n", m_cartridge->computedChecksum(),
	                         header.checksum, header.complement)
	          << std::format("RESET:    $00:{:04X}\n", header.resetVector);

	if (isPal) {
		std::cerr << "WARNING: PAL cartridge, running with NTSC timing\n";
	}

	if (m_cartridge->computedChecksum() != header.checksum) {
		std::cerr << "WARNING: checksum does not match the header\n";
	}
}

// A frame ends when V goes back to 0
void System::runFrame() {
	const U64 frame = m_ppu.frames();

	while (m_ppu.frames() == frame) {
		m_cpu.step();
	}
}

std::string System::traceLine() const {
	const CPU::Registers& registers = m_cpu.getRegisters();

	return std::format("{:02X}{:04X} A:{:04X} X:{:04X} Y:{:04X} S:{:04X} D:{:04X} DB:{:02X} P:{:02X} V:{} H:{} MC:{}", registers.PB,
	                   registers.PC, registers.A, registers.X, registers.Y, registers.S, registers.D, registers.DB, registers.P,
	                   m_ppu.scanline(), m_ppu.hClock(), m_bus.masterCycles());
}

} // namespace SNES
