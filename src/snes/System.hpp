#pragma once

#include "rune/Types.hpp"
#include "Constants.hpp"

#include <string>
#include <vector>

namespace SNES {

using RUNE::U8;

class System {
public:
    explicit System();

    void loadRom(const std::string& romPath);
    void runFrame();

    const U8* framebuffer();

private:
    //TEMP
    const std::vector<U8> m_framebuffer = std::vector<U8>(VIDEO_HEIGHT * VIDEO_WIDTH * 4, 0xFF);

};

} // namespace SNES