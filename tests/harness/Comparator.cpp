#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>

#include "harness/Comparator.hpp"

namespace SNES::HARNESS {

constexpr std::array<char, 8> FLAG_NAMES {'N', 'V', 'M', 'X', 'D', 'I', 'Z', 'C'};

std::string toHex(U32 value) {
	std::ostringstream out {};
	out << std::hex << std::uppercase << value;
	return out.str();
}

std::string formatFlags(U8 p) {
	std::string text(FLAG_NAMES.size(), '-');

	for (std::size_t bit = 0; bit < FLAG_NAMES.size(); bit++) {
		const U8 mask = static_cast<U8>(1u << (7 - bit));

		if (p & mask) {
			text[bit] = FLAG_NAMES[bit];
		}
	}

	return text;
}

void pinsFromCycleKind(Bus::CycleKind kind, bool& vda, bool& vpa, bool& vpb) {
	vda = false;
	vpa = false;
	vpb = false;

	switch (kind) {
	case Bus::CycleKind::OpcodeFetch :
		vda = true;
		vpa = true;
		break;
	case Bus::CycleKind::OperandFetch :
		vpa = true;
		break;
	case Bus::CycleKind::DataAccess :
		vda = true;
		break;
	case Bus::CycleKind::VectorFetch :
		vda = true;
		vpb = true;
		break;
	case Bus::CycleKind::Internal :
		break;
	}
}

std::string formatPins(bool vda, bool vpa, bool vpb, bool isWrite, bool emulation, bool flagM, bool flagX, bool memoryLock) {
	std::string text {};

	text += vda ? 'd' : '-';
	text += vpa ? 'p' : '-';
	text += vpb ? 'v' : '-';
	text += isWrite ? 'w' : 'r';
	text += emulation ? 'e' : '-';
	text += flagM ? 'm' : '-';
	text += flagX ? 'x' : '-';
	text += memoryLock ? 'l' : '-';

	return text;
}

std::string formatValue(const std::optional<U8>& value) { return value ? toHex(*value) : std::string {"--"}; }

std::string formatCycleEntry(const CycleEntry& cycle) {
	return "$" + toHex(cycle.address) + " " + formatValue(cycle.value) + " " +
	       formatPins(cycle.vda, cycle.vpa, cycle.vpb, cycle.isWrite, cycle.emulation, cycle.flagM, cycle.flagX, cycle.memoryLock);
}

std::string formatAccess(const Bus::Access& access) {
	bool vda {};
	bool vpa {};
	bool vpb {};

	pinsFromCycleKind(access.cycleKind, vda, vpa, vpb);

	return "$" + toHex(access.address) + " " + formatValue(access.value) + " " +
	       formatPins(vda, vpa, vpb, access.isWrite, access.emulation, access.flagM, access.flagX, access.memoryLock);
}

std::vector<FieldMismatch> compareRegisters(const CPU::Registers& expected, const CPU::Registers& actual) {
	std::vector<FieldMismatch> mismatches {};

	const auto check = [&](const char* field, auto expectedValue, auto actualValue) {
		if (expectedValue != actualValue) {
			mismatches.push_back({field, std::to_string(expectedValue), std::to_string(actualValue)});
		}
	};

	check("pc", expected.PC, actual.PC);
	check("s", expected.S, actual.S);
	check("a", expected.A, actual.A);
	check("x", expected.X, actual.X);
	check("y", expected.Y, actual.Y);
	check("dbr", expected.DB, actual.DB);
	check("d", expected.D, actual.D);
	check("pbr", expected.PB, actual.PB);
	check("e", expected.E, actual.E);

	if (expected.P != actual.P) {
		mismatches.push_back({"p", formatFlags(expected.P), formatFlags(actual.P)});
	}

	return mismatches;
}

std::vector<MemoryMismatch> compareMemory(const std::vector<MemoryCell>& expected, const Bus& bus) {
	std::vector<MemoryMismatch> mismatches {};

	for (const auto& cell : expected) {
		const U8 actual = bus.peek(cell.address);

		if (actual != cell.value) {
			mismatches.push_back({cell.address, cell.value, actual});
		}
	}

	return mismatches;
}

std::vector<AccessMismatch> compareAccesses(const std::vector<CycleEntry>& expected, const std::vector<Bus::Access>& actual) {
	std::vector<AccessMismatch> mismatches {};

	const std::size_t count = std::max(expected.size(), actual.size());

	for (std::size_t i = 0; i < count; ++i) {
		if (i >= actual.size()) {
			mismatches.push_back({i, formatCycleEntry(expected[i]), "(missing)"});
			continue;
		}

		if (i >= expected.size()) {
			mismatches.push_back({i, "(none)", formatAccess(actual[i])});
			continue;
		}

		const auto& e = expected[i];
		const auto& a = actual[i];

		bool vda {};
		bool vpa {};
		bool vpb {};

		pinsFromCycleKind(a.cycleKind, vda, vpa, vpb);

		const bool matches = e.address == a.address && e.value == a.value && e.vda == vda && e.vpa == vpa && e.vpb == vpb &&
		                     e.isWrite == a.isWrite && e.emulation == a.emulation && e.flagM == a.flagM && e.flagX == a.flagX &&
		                     e.memoryLock == a.memoryLock;

		if (!matches) {
			mismatches.push_back({i, formatCycleEntry(e), formatAccess(a)});
		}
	}

	return mismatches;
}

CaseResult compareCase(const TestCase& testCase, const CPU::Registers& actualRegisters, const Bus& bus) {
	CaseResult result {};

	result.name = testCase.name;
	result.registerMismatches = compareRegisters(testCase.expected.registers, actualRegisters);
	result.memoryMismatches = compareMemory(testCase.expected.ram, bus);
	result.accessMismatches = compareAccesses(testCase.cycles, bus.accessLog());

	return result;
}

} // namespace SNES::HARNESS