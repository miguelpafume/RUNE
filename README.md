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

Requires CMake >= 3.26, a C++20 compiler, and the Vulkan SDK.

```sh
cmake -B build
cmake --build build
./build/snes path/to/rom.sfc
```

## Controls

| Key | Button | Key | Button |
|-----|--------|-----|--------|
| Arrows | D-pad | <kbd>Q</kbd> / <kbd>W</kbd> | L / R |
| <kbd>Z</kbd> / <kbd>X</kbd> | B / A | <kbd>Enter</kbd> | Start |
| <kbd>A</kbd> / <kbd>S</kbd> | Y / X | <kbd>RShift</kbd> | Select |

## ROMs

No commercial ROM is distributed with this repository. `roms/` holds test ROMs
and freely licensed homebrew only.