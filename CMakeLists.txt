cmake_minimum_required(VERSION 3.26.0)
project(snes_emulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ---------------------------------------------------------------------------
# Emulator core
#
# Deliberately free of any dependency on the renderer or the windowing system.
# This is what lets the same code be driven by the graphical front end and by
# the headless validation harness (see _TCC/proposta.md, section 3.2).
# ---------------------------------------------------------------------------
add_library(snes_core STATIC
  src/Cartridge.cpp
  src/Bus.cpp
  src/CPU.cpp
  src/PPU.cpp
  src/APU.cpp
  src/Snes.cpp
)

target_include_directories(snes_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---------------------------------------------------------------------------
# Graphical front end
# ---------------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(MAGE
  GIT_REPOSITORY https://github.com/miguelpafume/MAGE-Engine.git
  GIT_TAG v0.1.2
)

FetchContent_MakeAvailable(MAGE)

add_executable(snes
  src/main.cpp
  src/SnesApp.cpp
)

target_link_libraries(snes PRIVATE snes_core MAGE::mage)

target_compile_definitions(snes PRIVATE
  SNES_ROM_DIR="${CMAKE_CURRENT_SOURCE_DIR}/roms")

# ---------------------------------------------------------------------------
# Validation harness
#
# Links only against the core, so it builds and runs with no Vulkan SDK and no
# display - which is what lets it run in CI and on a headless machine.
# ---------------------------------------------------------------------------
enable_testing()

add_executable(snes_smoke tests/smoke.cpp)
target_link_libraries(snes_smoke PRIVATE snes_core)

add_test(NAME smoke COMMAND snes_smoke)
