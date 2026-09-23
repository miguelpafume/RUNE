#include "CaseReader.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace SNES::HARNESS {

namespace nlm = nlohmann;

constexpr U8 PIN_COUNT = 8;
constexpr U8 CELL_ADDRESS = 0;
constexpr U8 CELL_VALUE = 1;

enum PinIndex : U8 {
	PIN_VDA = 0,
	PIN_VPA = 1,
	PIN_VPB = 2,
	PIN_RW = 3,
	PIN_E = 4,
	PIN_M = 5,
	PIN_X = 6,
	PIN_MLB = 7
};

CPU::Registers readRegisters(const nlm::json& state) {
	CPU::Registers registers {};

	registers.PC = state.at("pc").get<U16>();
	registers.S = state.at("s").get<U16>();
	registers.P = state.at("p").get<U8>();
	registers.A = state.at("a").get<U16>();
	registers.X = state.at("x").get<U16>();
	registers.Y = state.at("y").get<U16>();
	registers.DB = state.at("dbr").get<U8>();
	registers.D = state.at("d").get<U16>();
	registers.PB = state.at("pbr").get<U8>();
	registers.E = state.at("e").get<int>() != 0;

	return registers;
}

std::vector<MemoryCell> readMemory(const nlm::json& state) {
	const auto& ram = state.at("ram");

	std::vector<MemoryCell> cells {};
	cells.reserve(ram.size());

	for (const auto& cell : ram) {
		const U32 address = cell.at(CELL_ADDRESS).get<U32>();
		const U8  value = cell.at(CELL_VALUE).get<U8>();

		cells.push_back({address, value});
	}

	return cells;
}

CycleEntry readCycleEntry(const nlm::json& cycle) {
	const;
	const;
}

std::vector<TestCase> loadCases(const std::filesystem::path& file) {
	std::ifstream stream {file};

	if (!stream) {
		throw std::runtime_error {"Cannot open " + file.string()};
	}
}

} // namespace SNES::HARNESS