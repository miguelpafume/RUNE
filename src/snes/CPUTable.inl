namespace SNES {

template <bool M8, bool X8>
constexpr CPU::Table CPU::buildTable() {
	Table t {};

	t.fill(&CPU::opcodeMissing); // REMOVE WHEN ALL OPCODES ARE WRITTEN

	for (const Handler handler : t) {
		if (handler == nullptr) {
			throw "EVERY OPCODE NEEDS A HANDLER";
		}
	}

	return t;
}

} // namespace SNES