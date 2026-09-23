# RUNE

An emulator for a 16-bit console, written from scratch in C++20. Undergraduate
final-year project (TCC): see [`_TCC/proposta.md`](_TCC/proposta.md) for the
research problem, scope delimitation, methodology and schedule.

**Audio is out of scope.** The APU is stubbed at the port protocol only, so that
games get past their boot handshake; no samples are ever produced.

## Architecture

```
  Cartridge
  Bus
  CPU
  PPU
  APU
  Snes
```

## Build

Requires CMake >= 3.26 and a C++20 compiler. The windowed front-end also needs
the Vulkan SDK. The test bench does not.

```sh
cmake -B build -G "MinGW Makefiles"
cmake --build build
./build/snes path/to/rom.sfc
```

## Tests

The core is a static library with no Vulkan or GLFW dependency, so the bench
runs headless, with no display. `SNES_BUILD_FRONTEND=OFF` leaves out the
front-end, and with it the engine download and the SDK requirement:

```sh
cmake -B build-tests -G "MinGW Makefiles" -DSNES_BUILD_FRONTEND=OFF
cmake --build build-tests --target snes_tests
ctest --test-dir build-tests --output-on-failure
```

The same target builds from a full configure, when the SDK is at hand:

```sh
cmake --build build --target snes_tests
ctest --test-dir build --output-on-failure
```

`--output-on-failure` matters: without it `ctest` reports only `Failed`, and the
per-opcode score the bench prints is what the run is for.

### Test data

The bench runs the SingleStepTests 65816 suite, which is not distributed with
this repository. Clone it into `tests/data`:

```sh
git clone https://github.com/SingleStepTests/65816.git tests/data/65816
```

That is 512 files, one per opcode per processor mode, 10,000 cases each, and
about 2.7 GB on disk. `tests/data/` is out of version control. The provenance of
every test instrument used in this project is catalogued in
[`_TCC/fontes-de-teste.md`](_TCC/fontes-de-teste.md).

## Controls

| Key | Button | Key | Button |
|-----|--------|-----|--------|
| Arrows | D-pad | <kbd>Q</kbd> / <kbd>W</kbd> | L / R |
| <kbd>Z</kbd> / <kbd>X</kbd> | B / A | <kbd>Enter</kbd> | Start |
| <kbd>A</kbd> / <kbd>S</kbd> | Y / X | <kbd>RShift</kbd> | Select |

## ROMs

No commercial ROM is distributed with this repository. `roms/` holds test ROMs
and freely licensed homebrew only.