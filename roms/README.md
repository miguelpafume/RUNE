# Commercial ROMs are never committed. Test ROMs and homebrew only.

The `.sfc` files stay out of git, so download them into this folder (no subfolders, the
`.gitignore` pattern only covers this level).

| File | Used for | Source | Commit | SHA-256 |
|---|---|---|---|---|
| `CPUADC.sfc` | RESET up to the wait loop | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `CPUTest/CPU/ADC/` | `350b394` | `d6c4f5a284ce7842b8a3944b2bc51c8cde2f5932305f9b561334f4b7f49b5b4c` |
| `PlayNoise.sfc` | APU boot handshake | [PeterLemon/SNES](https://github.com/PeterLemon/SNES), `SPC700/PlayNoise/` | `350b394` | `ae0841adaf3785a54c74852e9e0d8b7ac0bdfef5049f79e305dbeb137a8ff347` |

The PeterLemon/SNES repository declares no license, so these files are used locally and not
redistributed here.

```sh
S=350b394e86ec5d62f600b5cbf64cdce3721bb6ef
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/CPUTest/CPU/ADC/CPUADC.sfc
curl -fLO https://raw.githubusercontent.com/PeterLemon/SNES/$S/SPC700/PlayNoise/PlayNoise.sfc
```
