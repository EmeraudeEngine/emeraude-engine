# Engine fuzzing — the loaders' analysis

The engine's scene loaders are its widest trust boundary (a scene, a store, a resource index, a dropped file). Each
loader is split into an ANALYSIS — bytes to plain data, no engine resource, no service, no device — and a
materialization that creates the resources (owner decision, 2026-10-18). The analysis is what these targets fuzz,
with every validation it carries.

| Target | Analysis | Since |
|---|---|---|
| `fuzz_wad` | `Scenes/Loaders/WADParser.cpp`, `WAD::parse()` | 2026-10-18 |

FBX, USD and glTF follow, one per lot (engine item `engine-fuzz-harness-for-loaders`).

## Build

```bash
BUILD_DIR=/path/to/projet-alpha/.claude-build-release CXX=clang++-19 src/Fuzzing/build-fuzzers.sh
```

The analysis TU of each target is compiled with clang, libFuzzer, ASan and UBSan, with the include paths and
definitions of the regular build (its `compile_commands.json`); the rest links from that build's `libEmeraude.so` and
`libEmeraudeBase.a`. The targets land in `src/Fuzzing/build/` (git-ignored).

## Seeds — derived locally, never committed

⚠️ A seed or a reproducer derived from a commercial IWAD (`doom.wad`, `doom2.wad`) is id Software's data: it must
NEVER enter this public repository (`src/Fuzzing/*.corpus/` is git-ignored). Derive compact PWADs from a local IWAD
(or use Freedoom's): one map's 11 lumps with `PLAYPAL`, `PNAMES`, `TEXTURE1` and a few flats between `F_START` /
`F_END` — about 100 KB, fast per execution (a whole 12 MB IWAD is not).

## Run

```bash
src/Fuzzing/build/fuzz_wad src/Fuzzing/fuzz_wad.corpus -fork=16 -max_total_time=600 -max_len=200000 \
	-rss_limit_mb=2048 -timeout=10 -close_fd_mask=3 -artifact_prefix=src/Fuzzing/build/
```

`-close_fd_mask=3` silences the Tracer (every refusal is traced).

## Campaigns

- 2026-10-18, `fuzz_wad`, first campaign (10 min, 16 forks, 3 seeds from doom.wad E1M1 / E1M2): 679 k executions,
  2 OOM + 1 timeout, ONE cause — `TEXTURE1` widths and heights used as declared (32 768 x 32 768 in a 108 KB file: 4 GB
  of indexes, then 16 GB of RGBA; 21 248 x 12 887 took 17 s to compose). Fixed (owner decision): 4096 per side and 64 M
  composed pixels per map, a refused texture skipped like a missing one; the three reproducers run in 37 ms, 75 MB.
