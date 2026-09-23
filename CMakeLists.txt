cmake_minimum_required(VERSION 3.26.0)
project(rune LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# The bench links snes_core alone, so it builds and runs with no display and no
# Vulkan SDK. Turn this off to build the headless bench by itself.
option(SNES_BUILD_FRONTEND "Build the windowed front-end (requires Vulkan)" ON)

add_library(snes_core STATIC
  src/snes/Cartridge.cpp
  src/snes/Bus.cpp
  src/snes/CPU.cpp
  src/snes/PPU.cpp
  src/snes/APU.cpp
  src/snes/System.cpp
)

target_include_directories(snes_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

include(FetchContent)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(json)

# --- TEST SECTION START ---
enable_testing()

add_executable(snes_tests
  tests/harness/main.cpp
  tests/harness/CaseReader.cpp
)

target_link_libraries(snes_tests PRIVATE snes_core nlohmann_json::nlohmann_json)

target_include_directories(snes_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)

target_compile_definitions(snes_tests PRIVATE
  SNES_TESTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/data"
)

add_test(NAME processor_tests COMMAND snes_tests)
# --- TEST SECTION END ---

if (SNES_BUILD_FRONTEND)
  FetchContent_Declare(MAGE
    GIT_REPOSITORY https://github.com/miguelpafume/MAGE-Engine.git
    GIT_TAG v0.1.2
  )

  FetchContent_MakeAvailable(MAGE)

  add_executable(snes
    src/main.cpp
    src/snes/SnesApp.cpp
  )

  target_link_libraries(snes PRIVATE snes_core MAGE::mage)

  target_compile_definitions(snes PRIVATE SNES_ROM_DIR="${CMAKE_CURRENT_SOURCE_DIR}/roms")
endif()
