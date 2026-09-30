---
id: triad-engine-pass
title: The triad (Ave Robustus, Allocatus Reduxus, Ave Performus) applied to the engine, section by section
status: in-progress
priority: unranked
scope: whole engine (src/)
opened: 2026-09-30
tags: [robustness, memory, performance, clang-tidy, owner-request, long-process]
---

# The triad applied to the engine, section by section

> **Resume with "continuer la triade"**: read this file, take the FIRST section whose status is not ✅, and continue
> at its first unchecked step. Update this file at the end of every step (it is the only memory of the process).
> The plans: projet-alpha `docs/plans/` (README + the three plans). The rules: projet-alpha
> `.claude/rules/ave-robustus.md`, `.claude/rules/optimization-plans.md`.

## Why

Owner request (2026-09-30): apply the three code-quality plans iteratively, domain by domain, section by section,
to the whole engine. Rank order: Ave Robustus > Allocatus Reduxus > Ave Performus.

## Owner decisions (2026-09-30)

- **Order**: trust boundaries first — Console/MCP → Resources → Scenes/Loaders → Net (with the 2026-08-27 network
  audit, ~60 verified defects) → Input; then the hot core — Scenes, Graphics, Saphir, Vulkan; then the rest — Audio,
  Physics, Animations, Overlay, PlatformSpecific, Tool, Help, the root files.
- **Cycle of one section**: (1) clang-tidy baseline → `docs/clang-tidy-ledger.md`; (2) a report of findings ranked
  by plan (file:line, mechanism, fix, proof); (3) MECHANICAL behaviour-neutral fixes applied directly ([[nodiscard]],
  noexcept, const, move, reserve, clang-tidy false-positive rewrites); every item touching an INTENT (reject / clamp /
  default, API, contract) or an architecture goes to the owner as a question; (4) the cascade compiles + clangcheck +
  runtime verification (bench, MCP conformance, 0 VUID); (5) a report; commit + push on the owner's order, one commit
  per section (split by plan when it helps the review).
- **Ave Performus in the engine**: CANDIDATES only (with the A/B to run), nothing adopted without a measurement; pure
  logic that can move into emeraude-base gets its Google Benchmark there; an engine runtime A/B harness is a separate
  item, built when a candidate deserves it.

## Tooling

- clang-tidy: `clang-tidy-19` with the engine `.clang-tidy`, on a compile database cleaned of GCC-only flags (a
  scratch script `tidy.py <prefix> <out.json> <files…>`: rebuild it from this description if the scratchpad is gone
  — copy `.claude-build-release/compile_commands.json` entries, driver `clang++-19`, drop `-flto*`,
  `-fno-fat-lto-objects`, `-fuse-linker*`; run each TU `--quiet`; keep the findings whose file is under the section).
- ✅ **clang-tidy 21.1.6** is the triad's version (owner, 2026-09-30). ⚠️ clang-tidy 19 CRASHES (segfault) in `modernize-use-designated-initializers` on `src/CoreTypes.hpp`
  `EngineContext` (a reference member brace-initialised with a forward-declared type: `graphicsRenderer{renderer}`).
  8 of the 12 Console TUs crash → rerun those without that one check and note it. Owner decision pending (below).
- clangcheck (macOS-like clang warnings) and GCC `-Wfloat-conversion` checks: the scratch `clangcheck.py`,
  `checkfiles.py` (same recipes as the pre-push checks).

## Sections

| # | Section | Lines | Status |
|---|---|---|---|
| 1 | `src/Console` (+ `MCP/`) | 8 327 | ✅ pushed 2026-09-30 (engine `35f868fc`, base `995159e`, alpha `93eab54a`); VALIDATED macOS M2 (R2 proven: 1.0e999 / 1.0e-60 refused) + Windows NVIDIA (MSVC clean, console conformance 4439/0 — its command set) |
| 2 | `src/Resources` | 6 307 | 🟠 step 5 — verified, awaiting the owner's commit order |
| 3 | `src/Scenes/Loaders` | — | ⬜ |
| 4 | `src/Net` (+ the 2026-08-27 audit) | 9 429 | ⬜ |
| 5 | `src/Input` | 5 357 | ⬜ |
| 6 | `src/Scenes` (the rest, by sub-group) | 70 278 | ⬜ |
| 7 | `src/Graphics` (by sub-group) | 137 872 | ⬜ |
| 8 | `src/Saphir` | 31 645 | ⬜ |
| 9 | `src/Vulkan` | 32 843 | ⬜ |
| 10 | `src/Audio` | 18 919 | ⬜ |
| 11 | `src/Physics` | 9 197 | ⬜ |
| 12 | `src/Animations` | 3 488 | ⬜ |
| 13 | `src/Overlay` | 7 303 | ⬜ |
| 14 | `src/PlatformSpecific` | 8 996 | ⬜ |
| 15 | `src/Tool`, `src/Help`, root files | 1 292 + 40 files | ⬜ |

## Section 1 — `src/Console` (started 2026-09-30)

- [x] (1) clang-tidy baseline: **64 findings** in 12 TUs (8 rerun without designated-initializers, tool crash):
  22 bugprone-unused-return-value, 21 misc-no-recursion, 6 performance-inefficient-string-concatenation,
  4 bugprone-unchecked-optional-access, 3 readability-qualified-auto, 2 cppcoreguidelines-pro-type-reinterpret-cast,
  1 each: performance-unnecessary-value-param, avoid-c-arrays, special-member-functions, modernize-loop-convert,
  misc-const-correctness, modernize-pass-by-value. (To copy into the ledger at the end of the section.)
- [x] (2) Review. The trust boundary is sound: MCP headers ≤ 16 KiB, body ≤ 1 MiB, Content-Length digits-only and
  bounded, `Transfer-Encoding` refused, JSON depth 64, connections / pending requests capped, timeouts; TCP console
  lines ≤ 8 KiB, clients and queues capped. Findings:
  - **Ave Robustus — real defects (INTENT → owner):**
    - R1 `MCP/Protocol.cpp` `scalarArgument()`: a FINITE JSON double beyond the float range (1e300) →
      `static_cast<float>` is UB (inf in practice). Proposed: refuse "outside the float range", like the 32-bit
      integer refusal just above.
    - R2 `Expression.cpp` (macOS branch, `strtof`): an overflow ("1.0e999") parses as ±inf, underflow as 0 (ERANGE
      ignored) — Linux (`from_chars`) refuses it. Proposed: check ERANGE / finiteness there, and refuse non-finite in
      `convertArgument(float)` (the single choke point of every typed float).
    - R3 `Argument::asInteger()`: `static_cast<int32_t>(std::round(float))` is UB out of range / NaN. Proposed: the
      existing fallback (warning + 0).
  - **Ave Robustus — mechanical (applied at step 3):** R4 `Controller` rule of five (copy / move deleted
    explicitly); R10 the 4 unchecked-optional false positives (bind `defaultValue()` once); qualified-auto,
    loop-convert, const-correctness.
  - **Ave Robustus — policy (owner):** R5 the 22 asio `ec` overloads whose returned `error_code` duplicates the out
    parameter (setup: the out param is checked — but `Server.cpp` set_option's code is overwritten by `bind`;
    teardown: best-effort close); R6 the 21 no-recursion (tree walks of the controllable hierarchy, depth built by
    the engine; asio async re-arm chains, no stack recursion) → on-purpose in the ledger?; R7 the C-array reference
    parameter of `bindCommand` (deduces N from a brace list: on purpose?); R8 two reinterpret_cast (setsockopt's
    `const char *` on Windows: on purpose; PNG bytes → `std::string`: `reducedPNG()` could write a string directly);
    R9 the clang-tidy crash (parentheses in `CoreTypes.hpp`, keep rerunning without the check, or a newer clang-tidy).
  - **Allocatus Reduxus (mechanical, step 3):** A1 the 6 `path + "." + name` concatenations of the recursive walks →
    one reserved string; A2 `bindTypedCommand` signature by value → `const &`, `RemoteListener` ctor address → by
    value + move; A3 = R8's PNG copy. The Expression parser allocates per token, but it is a cold path (a few
    commands per second): no change proposed.
  - **Ave Performus**: nothing worth an A/B (cold, I/O-bound).
- [x] (3) Mechanical fixes applied 2026-09-30: R4 (`Controller` copy / move deleted — which let clang PROVE two dead
  fields, `m_directInputWasEnabled` / `m_pointerWasLocked`, declared in 0.7.53 and never used: removed, else
  AppleClang `-Wunused-private-field` fails), R10, A2 (`bindTypedCommand` takes `binding` and `signature` by
  `const &`: each alias copies them anyway; `RemoteListener` address by value + move), qualified-auto ×3,
  loop-convert, const-correctness. clang-tidy 64 → 53. Build clean, clangcheck 0, `-Wfloat-conversion` 0, MCP
  conformance 1707/0, console conformance 4448/0, 0 VUID. A1 HELD: waits for the owner's ruling on a base helper
  (A4: `String::concat` itself is naive — `std::string{a} + std::string{b}`).
- [x] (3b) Owner rulings (2026-09-30), APPLIED:
  - R1-R3 **REFUSE** — done (MCP "outside the 32-bit floating point range", macOS strtof ERANGE, non-finite refused in
    `convertArgument(float)`, `asInteger()` warning + 0). Console doc 11 § critical points.
  - R5 — first ruled "use the returned error_code", then REVISED by the owner once told that form is DEPRECATED in
    asio: **`ASIO_NO_DEPRECATED`** cascade-wide (emeraude-base `cmake/SetupASIO.cmake`; sync ops return void). The
    whole cascade built with it at once (no other deprecated asio use). The `ec` out-parameter read after each
    step: `Server.cpp` `reuse_address` no longer overwritten by `bind`; `SO_SNDTIMEO` / `TCP_NODELAY` failures now
    warned (were silently dropped). Base `docs/error-handling.md` § asio.
  - R6, R7, setsockopt cast, `NotificationCode` enum — ON PURPOSE (ledger, with reasons). The enum one was not in
    the owner's questions: it is the engine-wide Observer convention (24 unscoped `NotificationCode` enums).
  - R8 — the PNG cast AND two full image copies removed, by a different means than the ruling said (same intent):
    base `String::encodeBase64` now reads bytes IN PLACE (`std::span< const std::byte >`, + `string_view` and
    `span< const uint8_t >` overloads); MCP `imageContent()` and `RemoteProtocol` pass the bytes directly.
  - R9 — **clang-tidy 21.1.6** (PyPI, scratch venv): no crash. Recipe in every ledger.
  - A4 — base `String::concatenate(parts...)` (one reserved allocation), `String::concat` overloads rewired onto it;
    the 7 Console concatenation sites use it. Base tests added: concatenate, base64 RFC 4648 vectors + overloads on
    binary bytes (base had NO base64 test).
  - clang-tidy 21 extras, mechanical: `std::scoped_lock` ×12, one precedence parenthesis.
- [x] (4) Verified 2026-09-30: cascade builds (0 warning), clangcheck 0, `-Wfloat-conversion` 0, clang-tidy 21 on
  Console = 24, all on purpose (from 64 / 66); emeraude-base 2161/2161 Release AND ASan/UBSan (3 live skipped); MCP
  conformance 1707/0, console conformance 4448/0, 0 VUID; runtime R1: 1e300 and 3.5e38 refused, 20000 accepted;
  `1.0e999` refused on the TCP console. R2 is macOS-only: peer check at validation.
- [x] (5) Pushed 2026-09-30: engine `35f868fc`, base `995159e`, alpha `93eab54a`; peers asked (macOS: R2)
  - macOS peer 2026-09-30: PASS (build 0 warning with ASIO_NO_DEPRECATED on every PlatformSpecific / Net TU, base
    2158 + 3 skipped, MCP 1707/0, console conformance 4427/0 — 21 fewer than Linux because the audio recorder /
    external-input services are off on that Mac, so their commands are absent; 0 VUID).

## Section 2 — `src/Resources` (started 2026-09-30)

- [x] (1) clang-tidy 21.1.6 baseline: **20 findings**, no crash: 6 modernize-use-scoped-lock, 3
  performance-inefficient-string-concatenation, 3 misc-no-recursion, 2 cppcoreguidelines-use-enum-class
  (`NotificationCode`: on purpose, the convention), 1 each: modernize-use-ranges, performance-unnecessary-value-param,
  cppcoreguidelines-avoid-do-while, misc-const-correctness, performance-enum-size, readability-use-anyofallof.
  `Container.hpp` (1 871 lines of templates) is only analysed where instantiated: reviewed by hand.
- [x] (2) Review. The JSON boundary itself is sound (every key type-checked, errors logged, a bad index skipped).
  Findings:
  - **Ave Robustus — real defects (INTENT / architecture → owner):**
    - S1 **path traversal**: `BaseInformation::parseData()` (LocalData) → `FileSystem::getFilepathFromDataDirectories()`
      appends the JSON `data` to each data directory with `std::filesystem::path::append`: an ABSOLUTE `data`
      REPLACES the whole path (`"/etc/passwd"`), and `"../../x"` leaves the data store. A resource definition can
      point anywhere on disk.
    - S2 **`ResourceTrait::wouldCreateCycle()`**: (a) reads each sub-dependency's `m_dependenciesToWaitFor` WITHOUT
      that resource's lock (only `this` and the direct dependency are locked by `addDependency()`): a data race with
      a loading thread's `dependencyLoaded()` erasing from it; (b) no visited set: exponential on shared (diamond)
      dependencies; (c) recursive. `dependencyLoaded()` → `checkDependencies()` → parents' `dependencyLoaded()`
      recursion: depth = the dependency chain, bounded by the resource types (scene → mesh → material → texture →
      image).
    - S3 **throwing `std::filesystem` calls, CASCADE-WIDE**: `directory_iterator` / `recursive_directory_iterator`
      range-for (the constructor without `error_code` AND `operator++` throw — even the two sites passing an
      `error_code` to the constructor), and 38 calls without `error_code` (`exists`, `relative`, `canonical`,
      `create_directories`, `permissions`, `current_path`…): engine 17 + 4 iterations, base 14 + 2, projet-alpha 7 +
      3. Under `-fno-exceptions` any filesystem error (permission, a directory removed, a broken link) = `std::terminate`.
      Base already has non-throwing `IO::` wrappers for part of it.
  - **Mechanical (step 3):** scoped_lock ×6 + const, `Action` enum → `uint8_t`, `std::ranges::sort`,
    `Container::loadingTask()` request by `const &`, the 3 console concatenations → `String::concatenate`.
  - **Policy (owner):** the `Manager::unloadUnusedResources()` do-while (a "repeat until a pass frees nothing" loop).
  - **Ave Performus**: nothing to A/B here (loading is I/O bound; the pool is the base ThreadPool already).
- [x] (3) Mechanical fixes applied 2026-09-30 (scoped_lock ×6 + const, `Action : uint8_t`, `std::ranges::sort`,
  `loadingTask(const LoadingRequest &)` — the task lambdas already own their copy, the 6 console messages through
  `String::concatenate`); builds clean.
- [x] (3b) Owner rulings (2026-09-30):
  - S1 **LEXICAL CONFINEMENT** in `FileSystem::getFilepathFromDataDirectories()`: an absolute `data`, a root name, or
    a `lexically_normal()` form starting with `..` is refused (error, resource Failed). Symlinks inside a store are
    accepted (whoever places them controls the disk).
  - S2 **GLOBAL GRAPH LOCK**: a static dependency-graph mutex taken FIRST by `addDependency()`; iterative search
    (explicit stack + visited set, O(V+E)) copying each node's list under that node's lock alone; then the pair lock
    for the insertion. Lock order graph → node.
  - S3 **ALL AT ONCE, cascade-wide**: base `IO::forEachDirectoryEntry(path, recursive, callback) → bool` (+ tests),
    the 9 iterations onto it, the 38 calls onto `error_code` overloads / `IO::` wrappers, and an Ave Robustus rule
    line "std::filesystem only through its error_code overloads".
  - do-while **ON PURPOSE** (ledger).
  APPLIED 2026-09-30:
  - S1: `FileSystem::getFilepathFromDataDirectories()` confines the filename under `<data dir>/<path>` through the new
    base `IO::confinedPath()` (10 019 real index entries checked: all inside their store). The SAME traversal found
    in base `ZipReader::extract()` ("Zip Slip": entry names from the archive) — the ruling applied there too.
  - S2: graph lock (function-local static mutex: `avoid-non-const-global-variables`, and no static-init order) +
    iterative walk with visited set. Deadlock audit: no code holds a node lock while calling `addDependency()`.
  - S3: base `IO::forEachDirectoryEntry()` (template, no std::function) + `IO::confinedPath()` + tests; migrated:
    base (FileTimestamps, ZipReader, ZipWriter, `IO::directoryEntries()` itself threw on `++`), engine (Resources
    Manager scans — `lexically_relative()` instead of `relative()`: the SAME 14 396 resources as the 39 earlier runs,
    without two canonicalisations per file —, FileSystem, Core wipe scan + RushMaker script permissions, Renderer
    pipeline cache ×5, USDLoader ×3, Scenes console, Desktop Commands ×4 ADL, SystemInfo.linux `canonical`),
    projet-alpha (Paladin, AnimationDebug, AssetLoader, DoomLoader, GeometryGenerator, NormalMapDebug, the three
    `main` `current_path`). The census missed ADL calls at first (`is_directory(p)` unqualified): 9 more found.
  - Also found and fixed: `ZipWriter` path-kind checks used `&&` for "or" (a directory accepted as a file and the
    reverse, then a throwing walk); Core's wipe scan added `file_size()`'s error value (uintmax −1) to its total.
  - Hygiene: 40 source files had no final newline (36 base — library code included —, 4 engine): fixed.
- [x] (4) Verified 2026-09-30: cascade builds (0 warning), clangcheck 0, `-Wfloat-conversion` 0, clang-tidy 21 on
  Resources 20 → 5 (all on purpose, ledger); base 2165/2165 Release AND ASan/UBSan (new: forEachDirectoryEntry,
  confinedPath, Zip Slip, Zip path kinds); runtime `citadel`: 37 containers, 1 073 resources loaded, dynamic scan
  14 396 (identical), 0 VUID, no confinement / walk error.
- [ ] (5) Ledger (done), report, commit + push on the owner's order, then peers.

### Leads noted for later sections (seen while passing)

- Graphics: `CubemapResource.cpp` `CubemapFaceNames.at(faceIndex)` — a throwing `.at()`.
- PlatformSpecific: `SystemInfo.linux.cpp` `line.at(position)` (memory parsing) — a throwing `.at()`.
- A cascade-wide census of the other throwing std calls (`.at()`, `std::stoi`, `optional::value()` unchecked,
  `std::thread` ctor) would follow the filesystem one.
