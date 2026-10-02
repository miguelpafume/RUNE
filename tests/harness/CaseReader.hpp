#pragma once

#include "rune/Types.hpp"
#include "snes/CPU.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace SNES::HARNESS {

struct MemoryCell {
	U32 address {};
	U8  value {};
};

struct CycleEntry {
	U32 address {};

	std::optional<U8> value {};

	bool vda {};        // Valid Data Address
	bool vpa {};        // Valid Program Address
	bool vpb {};        // Vector Pull
	bool isWrite {};    // Write, otherwise Read
	bool emulation {};  // Emulation mode
	bool flagM {};      // Accumulator Width
	bool flagX {};      // Index Width
	bool memoryLock {}; // low through a read-modify-write
};

struct CaseState {
	CPU::Registers          registers {};
	std::vector<MemoryCell> ram {};
};

struct TestCase {
	std::string name {};
	CaseState   initial {};
	CaseState   expected {};

	std::vector<CycleEntry> cycles {};
};

std::vector<TestCase> loadCases(const std::filesystem::path& file);

} // namespace SNES::HARNESS