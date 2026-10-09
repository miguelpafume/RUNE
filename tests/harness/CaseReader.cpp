#include "CaseReader.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <stdexcept>

namespace SNES::HARNESS {

namespace nlm = nlohmann;

constexpr U8 PIN_COUNT = 8;
constexpr U8 CELL_ADDRESS = 0;
constexpr U8 CELL_VALUE = 1;
constexpr U8 CELL_PINS = 2;

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
	const auto& pins = cycle.at(CELL_PINS).get_ref<const std::string&>();

	if (pins.size() != PIN_COUNT) {
		throw std::runtime_error {"ERROR: Cycle reports " + std::to_string(pins.size()) + " pins, expected 8."};
	}

	CycleEntry entry {};

	if (!cycle.at(CELL_ADDRESS).is_null())
		entry.address = cycle.at(CELL_ADDRESS).get<U32>();

	if (!cycle.at(CELL_VALUE).is_null()) {
		entry.value = cycle.at(CELL_VALUE).get<U8>();
	}

	entry.vda = pins[PIN_VDA] == 'd';
	entry.vpa = pins[PIN_VPA] == 'p';
	entry.vpb = pins[PIN_VPB] == 'v';
	entry.isWrite = pins[PIN_RW] == 'w';
	entry.emulation = pins[PIN_E] == 'e';
	entry.flagM = pins[PIN_M] == 'm';
	entry.flagX = pins[PIN_X] == 'x';
	entry.memoryLock = pins[PIN_MLB] == 'l';

	return entry;
}

std::vector<TestCase> loadCases(const std::filesystem::path& file) {
	std::ifstream stream {file};

	if (!stream) {
		throw std::runtime_error {"ERROR: Cannot open " + file.string()};
	}

	const nlm::json root = nlm::json::parse(stream);

	if (!root.is_array()) {
		throw std::runtime_error {"ERROR: " + file.string() + " is not an array of cases."};
	}

	std::vector<TestCase> cases {};
	cases.reserve(root.size());

	for (const auto& node : root) {
		const auto& initialState = node.at("initial");
		const auto& expectedState = node.at("final");
		const auto& cycles = node.at("cycles");

		TestCase testCase {};

		testCase.name = node.at("name").get<std::string>();

		testCase.initial.registers = readRegisters(initialState);
		testCase.initial.ram = readMemory(initialState);

		testCase.expected.registers = readRegisters(expectedState);
		testCase.expected.ram = readMemory(expectedState);

		testCase.cycles.reserve(cycles.size());

		for (const auto& cycle : cycles) {
			testCase.cycles.push_back(readCycleEntry(cycle));
		}

		cases.push_back(std::move(testCase));
	}

	return cases;
}

} // namespace SNES::HARNESS