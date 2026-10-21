# Engine fuzzing — the loaders' analysis

The engine's scene loaders are its widest trust boundary (a scene, a store, a resource index, a dropped file). Each
loader is split into an ANALYSIS — bytes to plain data, no engine resource, no service, no device — and a
materialization that creates the resources (owner decision, 2026-10-18). The analysis is what these targets fuzz,
with every validation it carries.

| Target | Analysis | Since |
|---|---|---|
| `fuzz_wad` | `Scenes/Loaders/WADParser.cpp`, `WAD::parse()` | 2026-10-18 |
| `fuzz_fbx` | `Scenes/Loaders/FBXParser.cpp`, `FBX::analyse()` + `FBX::analyseClipsOnly()` (ufbx linked from the external dependencies) | 2026-10-18 |
| `fuzz_usd` | `Scenes/Loaders/USDParser.cpp` + `USDZArchive.cpp`: `USD::composeStage()`, `USD::analyse()`, `USD::forEachMesh()` (lightusd linked from the external dependencies, UBSan's `vptr` check off: lightusd has no RTTI) | 2026-10-18 |
| `fuzz_gltf` | `Scenes/Loaders/GLTFParser.cpp`: `GLTF::parse()` + `GLTF::analyse()` (fastgltf, simdjson, meshoptimizer and Draco linked from the external dependencies) | 2026-10-18 |
| `fuzz_ktx2` | `Graphics/KTX2Decoder.cpp`: `KTX2Decoder::readKeyValue()` over a container in memory (the bake's fingerprint and alpha coverage, graphics § 40; libktx and zstd linked from the external dependencies) | 2026-10-22 |

The four scene loaders are covered (WAD, FBX, USD, glTF): the former engine item `engine-fuzz-harness-for-loaders`
is done.

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

FBX: the test files of the ufbx repository (`data/*.fbx` under 64 KB: 567 files, binary and ASCII, skins, animations,
materials) make a good local corpus. A Mixamo export (the Paladin) is not ours to publish either: local seeds only.

USD: the test files of the tinyusdz repository (USDA, USDC, USD and USDZ under 64 KB: 1 166 files). The harness is
hermetic: a USDZ resolves against its own table, a plain layer's resolver refuses every asset.

glTF: the self-contained models of projet-alpha's `glTF-Sample-Assets` submodule (`.glb` and `glTF-Embedded` under
256 KB), plus the small Draco, meshopt and quantized variants with their `.bin` folded into `data:` uris (a seed with
an external file reads nothing: every uri is confined to an empty directory). 90 seeds.

KTX2: the first 4 KiB of baked containers (the header and the key/value block, before the levels), with and without
`Emeraude.Bake` / `Emeraude.AlphaCoverage`. 13 seeds.

## Seeing inside a third-party library

`LIB_<library>=/path/lib<library>.a` links that archive instead of the release one — an instrumented build of the
library (clang, `-fsanitize=address,undefined,fuzzer-no-link`, `-g`), so the fuzzer gets coverage feedback and ASan
inside it. lightusd, for example: a copy of the submodule with `patches/tinyusdz.patch` applied, configured with the
options of `libraries/tinyusdz.yaml` plus those flags (and `-fno-sanitize=vptr`), target `lightusd_static`. Measured
2026-10-18: `fuzz_usd` reaches 4 382 edges on the release library, 93 342 on the instrumented one.

## Run

```bash
src/Fuzzing/build/fuzz_wad src/Fuzzing/fuzz_wad.corpus -fork=16 -max_total_time=600 -max_len=200000 \
	-rss_limit_mb=2048 -timeout=10 -close_fd_mask=3 -artifact_prefix=src/Fuzzing/build/
```

`-close_fd_mask=3` silences the Tracer (every refusal is traced). `fuzz_fbx` runs the same way with `-max_len=131072`.

## Campaigns

- 2026-10-18, `fuzz_wad`, first campaign (10 min, 16 forks, 3 seeds from doom.wad E1M1 / E1M2): 679 k executions,
  2 OOM + 1 timeout, ONE cause — `TEXTURE1` widths and heights used as declared (32 768 x 32 768 in a 108 KB file: 4 GB
  of indexes, then 16 GB of RGBA; 21 248 x 12 887 took 17 s to compose). Fixed (owner decision): 4096 per side and 64 M
  composed pixels per map, a refused texture skipped like a missing one; the three reproducers run in 37 ms, 75 MB.
- 2026-10-18, `fuzz_wad`, second campaign (10 min, 16 forks): 690 k executions, 0 finding.
- 2026-10-18, `fuzz_fbx`, first campaign (10 min, 16 forks, 567 ufbx test files): 39.7 M executions, 332 OOM
  reproducers, ONE cause in ufbx — a binary array allocated at its declared size (50 KB asking for 14 GB). Fixed (owner
  decision): each ufbx allocator capped at max(256 MiB, 16 x the file size).
- 2026-10-18, `fuzz_fbx`, second campaign: 49.2 M executions, 12 OOM, ONE cause in the analysis — the 30 Hz animation
  sampling sized on the declared stack duration (up to 10^12 samples; a non-finite duration was undefined behaviour).
  Fixed (owner decision): a stack beyond 4 M keyframes or of a non-finite range is skipped. The 344 reproducers run at
  180 MB peak.
- 2026-10-18, `fuzz_fbx`, third campaign: 51.1 M executions, 0 finding.
- 2026-10-18, `fuzz_usd`, first campaign (10 min, 16 forks, 1 166 tinyusdz test files, release lightusd): 6.7 M
  executions, 6 OOM, NONE in the engine — lightusd's `StreamReader::read()` took a short read for a success (a 1 KB
  USDC asked for 4.8 GB), an internal reference to an ancestor expanded exponentially (4 KB of USDA → 16.7 GB), a
  compressed array decompressed at its declared size (15 KB → 400 MB), and two that do not reproduce alone (memory
  accumulated in a fork job). Fixed (owner decision): `patches/tinyusdz.patch` in ext-deps-generator, and the engine
  budgets a root layer at max(256 MiB, 16 x the file).
- 2026-10-18, `fuzz_usd`, second campaign, lightusd instrumented (patched): 3.7 M executions, 93 342 edges, 0 finding.
- 2026-10-18, `fuzz_usd`, third campaign, the same: 2.5 M executions, 95 085 edges, 0 finding.
- 2026-10-18, `fuzz_gltf`, first campaign (90 seeds): stopped on 13 crashes, two causes — an index past its
  primitive's vertices reaching emeraude-base's tangent computation (out-of-bounds reads and writes), and fastgltf
  decoding a `data:` uri whose base64 length is not a multiple of 4 (a heap overflow). Fixed (owner decisions): the
  analysis checks every index against its own primitive, base's seven normal / tangent computations refuse an
  inconsistent shape, and `patches/fastgltf.patch` (ext-deps-generator) refuses such a uri.
- 2026-10-18, `fuzz_gltf`, second campaign (`-ignore_crashes`): 6.0 M executions, 86 crashes, two more causes —
  fastgltf's `GenerateMeshIndices` running during the parse, before `validate()` (an unchecked accessor index, an
  unbounded allocation; 50), and the Draco extension's bufferView read without a bounds check (26; the 10 others did
  not reproduce alone). Fixed (owner decision): the option is dropped, the analysis draws a non-indexed primitive three
  by three; the Draco view is bounds-checked. The 99 reproducers run clean.
- 2026-10-18, `fuzz_gltf`, third campaign: 4.6 M executions, 0 finding. ⚠️ A first run of it is void: `libEmeraude.so`
  was rebuilt while it ran and every later fork job died at start-up (631 "crashes" without an artifact) — never
  rebuild the engine library a running fuzzer links.
