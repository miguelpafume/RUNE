#pragma once

#include "rune/Types.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace SNES {

using RUNE::U16;
using RUNE::U32;
using RUNE::U8;

class Bus;

class Cartridge {
public:
	enum class MapMode {
		LoRom,
		HiRom
	};

	enum class Region {
		Ntsc,
		Pal
	};

	struct Header {
		std::string title {};
		MapMode     mapMode {};
		Region      region {};
		U32         declaredRomSize {};
		U32         sramSize {};
		U16         complement {};
		U16         checksum {};
		U16         resetVector {};
		U8          version {};
		U8          type {};
		U8          destination {};
		bool        fastRom {};
	};

	explicit Cartridge(std::vector<U8> image);

	Cartridge(const Cartridge&) = delete;
	Cartridge& operator=(const Cartridge&) = delete;

	const Header& header() const { return m_header; }
	std::span<U8> sram() { return m_sram; }

	void map(Bus& bus) const;

	U8   readSram(U32 address) const;
	void writeSram(U32 address, U8 value);

	std::size_t romSize() const { return m_rom.size(); }
	U16         computedChecksum() const { return m_computedChecksum; }

private:
	std::vector<U8> m_rom {};
	std::vector<U8> m_sram {};
	Header          m_header {};
	U16             m_computedChecksum {};

	std::size_t m_headerBase {};

	void stripCopierHeader();
	void checkSize() const;
	void readHeader();
	void checkCoprocessor() const;
	int  scoreHeader(std::size_t base) const;
	U16  checksum() const;

	U32 sramOffset(U32 address) const;

	void mapLoRom(Bus& bus) const;
	void mapHiRom(Bus& bus) const;
	void mapRom(Bus& bus, U8 bankFirst, U8 bankLast, U16 addressFirst, U16 addressLast) const;

	static U32 mirror(U32 offset, U32 size);
};

} // namespace SNES