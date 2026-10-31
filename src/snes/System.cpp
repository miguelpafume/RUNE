#include "System.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace SNES {

System::System() {}

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

void System::runFrame() {}

} // namespace SNES
