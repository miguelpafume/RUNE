#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "snes/Bus.hpp"
#include "snes/CPU.hpp"

#include "harness/CaseReader.hpp"

namespace SNES::HARNESS {

struct FieldMismatch {
	std::string field {};
	std::string expected {};
	std::string actual {};
};

struct MemoryMismatch {
	U32 address {};
	U8  expected {};
	U8  actual {};
};

struct AccessMismatch {
	std::size_t index {};
	std::string expected {};
	std::string actual {};
};

struct CaseResult {
	std::string name {};

	std::vector<FieldMismatch>  registerMismatches {};
	std::vector<MemoryMismatch> memoryMismatches {};
	std::vector<AccessMismatch> accessMismatches {};

	bool passed() const { return registerMismatches.empty() && memoryMismatches.empty() && accessMismatches.empty(); }
};

CaseResult compareCase(const TestCase& testCase, const CPU::Registers& actualRegisters, const Bus& bus);

} // namespace SNES::HARNESS