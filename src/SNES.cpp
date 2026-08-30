#include "Snes.hpp"

namespace RUNE {
    
Snes::Snes() {
}

const U8* Snes::framebuffer() {

    return m_framebuffer.data();
}

void Snes::loadRom(const std::string &romPath) {
}

void Snes::runFrame() {
}

} //namespace RUNE
