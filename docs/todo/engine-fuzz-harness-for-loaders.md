---
id: engine-fuzz-harness-for-loaders
title: A libFuzzer harness for the engine's scene loaders (glTF, FBX, USD, WAD)
status: open
priority: medium
scope: Scenes/Loaders, tooling
opened: 2026-09-30
tags: [fuzzing, robustness, loaders, owner-request]
---

# A libFuzzer harness for the engine's scene loaders (glTF, FBX, USD, WAD)

## Why

Owner decision (2026-09-30, triad section 3): the scene loaders are the engine's widest trust boundary (a dropped
file, `Core.openFiles()`), and their hardening was proven by hand-crafted hostile files only — a glTF with an index
out of range, a `children` cycle, a 200 000-node chain; WADs with a lump outside the file, an oversized TEXTURE1
table, a BSP cycle. Fuzzing finds what nobody thinks of: emeraude-base proved it (`src/Fuzzing/`, 11 real crashes on
malformed input during the 2026 Ave robustus plan, then clean multi-million-run campaigns).

## Progress
- 2026-10-18, owner decisions: split each loader into an ANALYSIS (bytes → plain data, no resource) and a
  materialization, and fuzz the analysis. WAD done: `WAD::parse()` (`WADParser.hpp/.cpp`), `src/Fuzzing/fuzz_wad.cpp`,
  `src/Fuzzing/build-fuzzers.sh` (see `docs/subsystems/scenes-loaders/03-implemented-loaders/04-wadloader.md`).
  Next: FBX, USD, glTF (the largest), one per lot, each verified identical on its demos.

## What remains

- A clang libFuzzer target per loader, on the base model (`emeraude-base/src/Fuzzing/build-fuzzers.sh`: a
  standalone clang build, ASan + UBSan, a seed corpus). The loaders need a `Resources::Manager` and a device for the
  resources they create: either a headless / null-device mode, or split each loader's PARSE (bytes → SceneData) from
  its resource creation, and fuzz the parse only.
- Seeds: the glTF-Sample-Assets corpus, the system IWADs, a few FBX / USDZ.
- Campaigns recorded in the loaders' docs; every crash fixed with its reproducer kept as a regression seed.

## References

- emeraude-base `src/Fuzzing/README.md`, `docs/plans/ave-robustus.md` § A.3.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § Section 3.
