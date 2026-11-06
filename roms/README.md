# Commercial ROMs are never committed. Test ROMs and homebrew only.

The `.sfc` files stay out of git, so download them into this folder (no subfolders, the
`.gitignore` pattern only covers this level).

| File | Used for | Source | Commit | SHA-256 |
|---|---|---|---|---|
| `CPUADC.sfc` | RESET up to the wait loop | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `CPUTest/CPU/ADC/` | `350b394` | `d6c4f5a284ce7842b8a3944b2bc51c8cde2f5932305f9b561334f4b7f49b5b4c` |
| `PlayNoise.sfc` | APU boot handshake | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `SPC700/PlayNoise/` | `350b394` | `ae0841adaf3785a54c74852e9e0d8b7ac0bdfef5049f79e305dbeb137a8ff347` |
| `CPUBIT.sfc` | Another CPU test screen, BIT and TSB | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `CPUTest/CPU/BIT/` | `350b394` | `fdfc621bcd1f9dfcc0800b331d540af69ca70dd7e075632b144b46d9583fa15e` |
| `BANKWRAM.sfc` | Code running from WRAM banks | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `BANK/WRAM/` | `350b394` | `6ee3c92304298e5b6b083f1309285d5b8a75ad4287220ba16d1eb7df01167b06` |
| `HelloWorld.sfc` | Text on BG1 | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `HelloWorld/` | `350b394` | `63ed338621788ad4ab856bb782e16044193882b7840370b500c8359a41fb87ef` |
| `8x8BG1Map2BPP32x328PAL.sfc` | Picture on BG1, mode 0 with 8 palettes | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `PPU/BGMAP/8x8/2BPP/8x8BG1Map2BPP32x328PAL/` | `350b394` | `66083b119131d4cd7b3c41301750342152d2c3a3cfce103de074a0ae53330146` |
| `cputest-basic.sfc` | Second opinion on the CPU, 1107 tests | [gilyon/snes-tests](https://github.com/gilyon/snes-tests), release `v1.4`, `cputest/` | `5ecdf55` | `d479bde706b9e16d76c0dfe98e2b2d3f5348f9dcb911e6a36b898f4201fc98bb` |
| `SNES_04.sfc` | Mode 1 with three layers and priority bits | [nesdoug/SNES_04](https://github.com/nesdoug/SNES_04) | `9cb5b5b` | `6b2a9db603dd8c9654d82a86317d230ed4cae78f0f6c2387bfaf7e0fbe6b51b7` |
| `SNES_4B.sfc` | The same screen with BG3 on top, `$2105` bit 3 | [nesdoug/SNES_04](https://github.com/nesdoug/SNES_04) | `9cb5b5b` | `621056282383671c9fd02f0a9051a65b79a62309031469f2f851fff8dd18516c` |
| `SNES_05.sfc` | Sprites | [nesdoug/SNES_05](https://github.com/nesdoug/SNES_05) | `69bb3df` | `b5dd5d3a31a180a66ff6e660d61872c35c40f2676da56119cb7c49a6dd29f76a` |
| `SNES_06.sfc` | Controller reads moving a sprite | [nesdoug/SNES_06](https://github.com/nesdoug/SNES_06) | `844ae00` | `a586bbae524513f21285ed74ab8e5585ccea6aed6bee50895969a18c05a08f0e` |
| `SNES_07.sfc` | Pong clone, the first game target | [nesdoug/SNES_07](https://github.com/nesdoug/SNES_07) | `1016a0a` | `749ff9ddb4c57bb4d86b54ee7283e68bb572c2219c6c4e8180984f1ccaf8ab2b` |
| `RedSpaceHDMA.sfc` | HDMA changing the backdrop color on each line | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `PPU/HDMA/RedSpaceHDMA/` | `350b394` | `f59bc2cc89b22cad7308fe65f5d9fee1025311c84a9f809e14ebf96ab9a4c4de` |
| `RedSpaceIndirectHDMA.sfc` | The same gradient through indirect HDMA | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `PPU/HDMA/RedSpaceIndirectHDMA/` | `350b394` | `bfc85c6c05399512e33c4ec5393e90eba6a4465bdcdb6c3f2624d2f7c2cf90ff` |
| `RedSpace9BitHDMA.sfc` | Two HDMA channels, color and brightness on each line | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `PPU/HDMA/RedSpace9BitHDMA/` | `350b394` | `ad06e07d45dc82d744d6d9fbe0a9b7897f3386fb475168c163824ee0997c1e44` |
| `SNES_09.sfc` | A sprite moving over a background map, flipped sideways | [nesdoug/SNES_09](https://github.com/nesdoug/SNES_09) | `50257bd` | `60bfd8bb488a29738b5f64a51210d7159a9d47c2a11bac924c2237d29cf65044` |
| `SNES_11.sfc` | HDMA effects switched with A, B, X or Y: color gradient, wavy scroll | [nesdoug/SNES_11](https://github.com/nesdoug/SNES_11) | `d25ba88` | `a61036d60fc891735c0f052b389c62bb9975891cb381c3a02d03f54df07be483` |
| `ctrltest.sfc` | Controller bits read by hand through `$4016` and `$4017` | [bbbradsmith/SNES_stuff](https://github.com/bbbradsmith/SNES_stuff), `ctrltest/` | `3246b58` | `48432413b80863ed884ba76a2892f6eda2277ea5349e8ef13347f70b9aa98983` |
| `ctrltest_auto.sfc` | The same bits from the automatic read, `$4218`-`$421F` | [bbbradsmith/SNES_stuff](https://github.com/bbbradsmith/SNES_stuff), `ctrltest/` | `3246b58` | `ae660b7c49941901b48e25837793ab946befe00de5d3038d404cfdee77dfc82f` |
| `ctrltest_simple.sfc` | Each button of both controllers, from the automatic read | [bbbradsmith/SNES_stuff](https://github.com/bbbradsmith/SNES_stuff), `ctrltest/` | `3246b58` | `445afc4c735790e272d5312f3520570013c999534740b8dd7c729c2b066d8e7c` |

The PeterLemon/SNES and bbbradsmith/SNES_stuff repositories declare no license, so their files are
used locally and not redistributed here. gilyon/snes-tests and the nesdoug/SNES_* repositories are MIT licensed.

```sh
S=350b394e86ec5d62f600b5cbf64cdce3721bb6ef
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/CPUTest/CPU/ADC/CPUADC.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/SPC700/PlayNoise/PlayNoise.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/CPUTest/CPU/BIT/CPUBIT.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/BANK/WRAM/BANKWRAM.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/HelloWorld/HelloWorld.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/PPU/BGMAP/8x8/2BPP/8x8BG1Map2BPP32x328PAL/8x8BG1Map2BPP32x328PAL.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/PPU/HDMA/RedSpaceHDMA/RedSpaceHDMA.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/PPU/HDMA/RedSpaceIndirectHDMA/RedSpaceIndirectHDMA.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/PPU/HDMA/RedSpace9BitHDMA/RedSpace9BitHDMA.sfc
curl -fLO https://github.com/gilyon/snes-tests/releases/download/v1.4/snes-tests.zip
unzip -j snes-tests.zip cputest/cputest-basic.sfc
rm snes-tests.zip
N=https://raw.githubusercontent.com/nesdoug
curl -fLO $N/SNES_04/9cb5b5b5f7fcec98e272f0a9494f9031a30491c5/SNES_04.sfc
curl -fLO $N/SNES_04/9cb5b5b5f7fcec98e272f0a9494f9031a30491c5/SNES_4B.sfc
curl -fLO $N/SNES_05/69bb3dff2c6549fc484bfb6a6c95d15033a3940a/SNES_05.sfc
curl -fLO $N/SNES_06/844ae002eef857396b43ece3ddee93d22571f27c/SNES_06.sfc
curl -fLO $N/SNES_07/1016a0a3474e783290c5b0828428a0afe4706ce5/SNES_07.sfc
curl -fLO $N/SNES_09/50257bd197ba5b3a3b1b3be6fcc3f741c5a72994/SNES_09.sfc
curl -fLO $N/SNES_11/d25ba887f5f0542da77c6d44e04ac60524d0b959/SNES_11.sfc
B=https://raw.githubusercontent.com/bbbradsmith/SNES_stuff/3246b5854c91a5a2f44d1569ead0aec0e211d930/ctrltest
curl -fLO $B/ctrltest.sfc
curl -fLO $B/ctrltest_auto.sfc
curl -fLO $B/ctrltest_simple.sfc
```
