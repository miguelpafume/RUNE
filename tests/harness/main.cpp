#include "CaseReader.hpp"
#include "Comparator.hpp"

#include "snes/Bus.hpp"
#include "snes/CPU.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using namespace SNES;
using namespace SNES::HARNESS;

struct ModeScore {
	std::size_t total {};
	std::size_t passed {};
	std::size_t registerFailures {};
	std::size_t memoryFailures {};
	std::size_t accessFailures {};
};

struct OpcodeSummary {
	ModeScore emulation {};
	ModeScore native {};
};

struct FileKey {
	std::string opcode {}; // "3d"
	char        mode {};   // 'e' or 'n'
};

std::string toLower(const std::string& text) {
	std::string out = text;
	std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
	return out;
}

std::string toHex(U32 value) {
	std::ostringstream out {};
	out << std::hex << std::uppercase << value;
	return out.str();
}

// "3d.e.json" -> opcode "3d", mode 'e'
std::optional<FileKey> parseFileName(const std::string& fileName) {
	const size_t dot = fileName.find('.');

	if (dot == std::string::npos || dot + 1 >= fileName.size()) {
		return std::nullopt;
	}

	FileKey key {};
	key.opcode = fileName.substr(0, dot);
	key.mode = fileName[dot + 1];

	return key;
}

bool matchesFilter(const std::string& fileName, const FileKey& key, const std::string& filter) {
	if (filter.empty())
		return true;

	if (filter.find('.') != std::string::npos)
		return toLower(fileName) == toLower(filter);

	return toLower(key.opcode) == toLower(filter);
}

ModeScore& scoreFor(OpcodeSummary& summary, char mode) { return mode == 'e' ? summary.emulation : summary.native; }

CaseResult runCase(Bus& bus, CPU& cpu, const TestCase& testCase) {
	bus.beginCase();

	for (const auto& cell : testCase.initial.ram) {
		bus.poke(cell.address, cell.value);
	}

	cpu.setRegisters(testCase.initial.registers);
	cpu.step();

	return compareCase(testCase, cpu.getRegisters(), bus);
}

void printDiagnostic(const CaseResult& result) {
	std::cout << "FAIL " << result.name << "\n\n";

	std::cout << "Registers:\n";
	for (const auto& field : result.registerMismatches) {
		std::cout << "  " << field.field << ": expected=" << field.expected << " actual=" << field.actual << "\n";
	}

	std::cout << "Memory:\n";
	for (const auto& cell : result.memoryMismatches) {
		std::cout << "  $" << toHex(cell.address) << ": expected=" << toHex(cell.expected) << " actual=" << toHex(cell.actual) << "\n";
	}

	std::cout << "Accesses:\n";
	for (const auto& access : result.accessMismatches) {
		std::cout << "  [" << access.index << "] expected=" << access.expected << " actual=" << access.actual << "\n";
	}
}

void printSummary(const std::map<std::string, OpcodeSummary>& summaries) {
	ModeScore totalE {};
	ModeScore totalN {};

	// clang-format off
	std::cout << std::left
        << std::setw(8) << "opcode"
        << std::setw(12) << "emulation"
        << std::setw(12) << "native"
        << std::setw(6) << "reg"
        << std::setw(6) << "mem"
        << std::setw(6) << "acc" << "\n";
	// clang-format on

	for (const auto& [opcode, summary] : summaries) {
		const auto& e = summary.emulation;
		const auto& n = summary.native;

		// clang-format off
        std::cout << std::left
            << std::setw(8) << opcode
            << std::setw(12) << (std::to_string(e.passed) + "/" + std::to_string(e.total))
            << std::setw(12) << (std::to_string(n.passed) + "/" + std::to_string(n.total))
            << std::setw(6) << (e.registerFailures + n.registerFailures)
            << std::setw(6) << (e.memoryFailures + n.memoryFailures)
            << std::setw(6) << (e.accessFailures + n.accessFailures) << "\n";
		// clang-format on

		totalE.total += e.total;
		totalE.passed += e.passed;
		totalN.total += n.total;
		totalN.passed += n.passed;
	}

	const std::size_t totalCases = totalE.total + totalN.total;
	const std::size_t totalPassed = totalE.passed + totalN.passed;

	std::cout << "\nTotal: " << totalPassed << "/" << totalCases << "\n";
}

int main(int argc, char** argv) {
	std::string filter {};
	bool        diagnose = false;

	for (int i = 1; i < argc; i++) {
		const std::string arg = argv[i];

		if (arg == "--diagnose" || arg == "-d") {
			diagnose = true;
		} else {
			filter = arg;
		}
	}

	const fs::path dataDir = fs::path {SNES_TESTS_DIR} / "65816" / "v1";

	if (!fs::exists(dataDir)) {
		std::cerr << "ERROR: test data not found at " << dataDir << ". Clone tests/data first.\n";
		return 1;
	}

	std::vector<fs::path> files {};

	for (const auto& entry : fs::directory_iterator {dataDir}) {
		if (entry.path().extension() != ".json")
			continue;

		const auto key = parseFileName(entry.path().stem().string());

		if (key && matchesFilter(entry.path().stem().string(), *key, filter))
			files.push_back(entry.path());
	}

	std::sort(files.begin(), files.end());

	Bus bus {Bus::Mode::Test};
	CPU cpu {bus};

	std::map<std::string, OpcodeSummary> summaries {};

	for (const auto& file : files) {
		const auto key = parseFileName(file.stem().string());

		if (!key)
			continue;
		try {
			const std::vector<TestCase> cases = loadCases(file);

			ModeScore& score = scoreFor(summaries[key->opcode], key->mode);
			score.total = cases.size();

			for (const auto& testCase : cases) {
				const CaseResult result = runCase(bus, cpu, testCase);

				if (result.passed()) {
					score.passed++;
					continue;
				}

				if (!result.registerMismatches.empty())
					score.registerFailures++;
				if (!result.memoryMismatches.empty())
					score.memoryFailures++;
				if (!result.accessMismatches.empty())
					score.accessFailures++;

				if (diagnose) {
					printDiagnostic(result);
					return 1;
				}
			}
		} catch (const std::exception& e) {
			std::cerr << "ERROR: " << file.filename().string() << ": " << e.what() << "\n";
		}
	}

	printSummary(summaries);

	const bool allPassed = std::all_of(summaries.begin(), summaries.end(), [](const auto& pair) {
		const OpcodeSummary& s = pair.second;
		return s.emulation.passed == s.emulation.total && s.native.passed == s.native.total;
	});

	return allPassed ? 0 : 1;
}
