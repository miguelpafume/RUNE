cmake_minimum_required(VERSION 3.26.0)
project(rune LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(snes_core STATIC
  src/Cartridge.cpp
  src/Bus.cpp
  src/CPU.cpp
  src/PPU.cpp
  src/APU.cpp
  src/Snes.cpp
)

target_include_directories(snes_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

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