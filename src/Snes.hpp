#pragma once

#include "Types.hpp"

#include <string>
#include <vector>
#include <array>

namespace RUNE {

class Snes {
public:
    explicit Snes();

    void loadRom(const std::string& romPath);
    void runFrame();

    const U8* framebuffer();

private:
    //TEMP
    std::vector<U8> m_framebuffer = std::vector<U8>(SNES::VIDEO_HEIGHT * SNES::VIDEO_WIDTH * 4, 0xFF);

};

} // namespace RUNE