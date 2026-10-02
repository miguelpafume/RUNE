#pragma once

#include "rune/Types.hpp"

#include <optional>
#include <vector>

namespace SNES {

using RUNE::U32;
using RUNE::U8;

class Bus {
public:
	enum class Mode {
		System,
		Test
	};

	enum class CycleKind {
		OpcodeFetch,  // dp-
		OperandFetch, // -p-
		DataAccess,   // d--
		VectorFetch,  // d-v
		Internal      // ---, no memory activity
	};

	struct Access {
		U32               address {};
		std::optional<U8> value {};
		CycleKind         cycleKind {};
		bool              isWrite {};
		bool              emulation {};
		bool              flagM {};
		bool              flagX {};
		bool              memoryLock {};
	};

	explicit Bus(Mode mode);

	U8 fetchOpcode(U32 address) { return readCycle(address, CycleKind::OpcodeFetch); }
	U8 fetchOperand(U32 address) { return readCycle(address, CycleKind::OperandFetch); }
	U8 fetchVector(U32 address) { return readCycle(address, CycleKind::VectorFetch); }
	U8 readData(U32 address, U8 value, bool memoryLock = false) { return readCycle(address, CycleKind::DataAccess, memoryLock); }

	void writeData(U32 address, U8 value, bool memoryLock = false);
	void internalCycle(U32 address, bool memoryLock = false);

	void setFlags(bool emulation, bool flagM, bool flagX);

	// Test mode only
	void                       poke(U32 address, U8 value);
	U8                         peek(U32 address) const;
	void                       beginCase();
	const std::vector<Access>& accessLog() const { return m_accessLog; }

private:
	Mode m_mode;

	std::vector<U8>     m_testMemory {};
	std::vector<U32>    m_dirty {};
	std::vector<Access> m_accessLog {};

	bool m_flagE {};
	bool m_flagM {};
	bool m_flagX {};

	U8 readCycle(U32 address, CycleKind kind, bool memoryLock = false);
};

} // namespace SNES