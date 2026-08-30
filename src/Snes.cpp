#include "Snes.hpp"

using namespace RUNE;

namespace SNES {
    
System::System() {
}

const U8* System::framebuffer() {

    return m_framebuffer.data();
}

void System::loadRom(const std::string &romPath) {
}

void System::runFrame() {
}

} //namespace SNES
