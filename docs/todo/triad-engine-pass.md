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
| 2 | `src/Resources` | 6 307 | ✅ pushed 2026-09-30 (base `4e029ab` + `fb9339a`, engine `d2207206` + `feac0ef4`, alpha `1a9e9554`); VALIDATED macOS M2 + Windows NVIDIA (tests incl. the Windows confinedPath block; scans unchanged, 9 891 there) |
| 3 | `src/Scenes/Loaders` | 11 292 | ✅ pushed 2026-09-30 (engine `bf5f901c`); VALIDATED Windows NVIDIA + macOS M2 (glTF hostile set + samples + FBX demos, 0 VUID; the WAD step skipped on both: no IWAD — proven on Linux) |
| 4 | `src/Net` (+ the 2026-08-27 audit) | 9 429 | ✅ pushed 2026-09-30 (engine `e40abf15`); VALIDATED macOS M2 + Windows NVIDIA (the `.windows.cpp` / Apple branches compiled clean, cache round-trip, 0 VUID; no serial device on either) |
| 5 | `src/Input` | 5 465 | ✅ pushed 2026-09-30 (engine `fe74dac0`, alpha `2856df1e`); VALIDATED macOS M2 + Windows NVIDIA (conformance unchanged, injection + refusals, 0 VUID; NO gamepad on any machine: the axis fix awaits a physical pad) |
| 6 | `src/Scenes` (the rest, by sub-group: 6a-6e below) | ~59 000 | ✅ 6a-6e pushed and VALIDATED on the three OS (2026-10-01) |
| 7 | `src/Graphics` (by sub-group: 7a-7g below) | 137 872 | ✅ 7a-7g pushed and VALIDATED on the three OS (2026-10-01) |
| 8 | `src/Saphir` (by sub-group: 8a-8c below) | 31 645 | ✅ 8a-8c pushed and VALIDATED on the three OS (2026-10-01) |
| 9 | `src/Vulkan` (by sub-group: 9a-9c below) | 32 843 | ✅ 9a-9c pushed and VALIDATED on the three OS (2026-10-01) |
| 10 | `src/Audio` (by sub-group: 10a-10b below) | 18 919 | ✅ 10a-10b pushed and VALIDATED on the three OS (2026-10-01) |
| 11 | `src/Physics` | 9 197 | ✅ pushed and VALIDATED on the three OS (2026-10-01) |
| 12 | `src/Animations` (+ the glTF skins and extents it led to) | 3 488 | ✅ pushed and VALIDATED on the three OS (2026-10-01) |
| 13 | `src/Overlay` | 7 303 | ✅ pushed and VALIDATED on the three OS (2026-10-01) |
| 14 | `src/PlatformSpecific` (+ the ARC flag of every engine `.mm`) | 8 996 | ✅ pushed (peers pending: Windows and macOS changes uncompiled on Linux) |
| 15 | `src/Tool`, `src/Help`, root files (leads: the `Window.resize` console command has no upper bound and reports the requested size, 13 peers) | 1 292 + 40 files | ⬜ |

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
- [x] (5) Pushed 2026-09-30: base `4e029ab` (newlines) + `fb9339a`, engine `d2207206` (newlines) + `feac0ef4`, alpha `1a9e9554`; peers asked.

### Leads noted for later sections (seen while passing)

- Graphics: `CubemapResource.cpp` `CubemapFaceNames.at(faceIndex)` — a throwing `.at()`.
- PlatformSpecific: `SystemInfo.linux.cpp` `line.at(position)` (memory parsing) — a throwing `.at()`.
- A cascade-wide census of the other throwing std calls (`.at()`, `std::stoi`, `optional::value()` unchecked,
  `std::thread` ctor) would follow the filesystem one.
  - Peers 2026-09-30: macOS PASS (1 076 loaded: +3 audio, timing), Windows PASS (1 072). The Windows peer found an
    UNRELATED engine defect on `citadel`: a mesh-shader cubemap-shadow pipeline with 6 views > the device's
    `maxMeshMultiviewViewCount` (4) → engine item `mesh-shader-multiview-view-count-limit`.

## Section 3 — `src/Scenes/Loaders` (started 2026-09-30)

- [x] (1) clang-tidy 21.1.6 baseline: **221 findings**: 139 cppcoreguidelines-pro-type-union-access (ALL in
  FBXLoader: the ufbx API is unions — `ufbx_vec3::x`…), 22 modernize-use-designated-initializers, 18
  pro-bounds-constant-array-index, 9 math-missing-parentheses, 7 reinterpret-cast, 7 misc-no-recursion, 3 each
  avoid-const-or-ref-data-members / use-internal-linkage / inefficient-string-concatenation / integer-sign-comparison,
  1 each use-after-move, nondeterministic-pointer-iteration-order, Padding, nested-conditional, use-enum-class,
  enum-size, make-member-function-const.
- [x] (2) Review — findings:
  - **Real bug, fixed at once (no intent)**: `GLTFLoader.cpp` a multi-material mesh MOVED `materialList` into the
    MultiLayerMeshResource lambda, then filled its SceneData descriptor from the moved-from list → the DEFAULT
    material (MultiLayerMeshResource::load(sceneData) then built ONE layer instead of N). Proof by code +
    `bugprone-use-after-move`; no shipped asset takes that path (0 single-node multi-material mesh in the stores).
  - G1 glTF indices never validated (fastgltf's parse does not check them; `fastgltf::validate()` is never called):
    27 raw `asset.X[i]` sites.
  - G2 recursive walks over hierarchies FROM THE FILE: glTF nodes (a `children` cycle = infinite recursion, a deep
    chain = stack overflow; fastgltf::validate() does not check cycles), 5 USD prim walks.
  - W1-W6 WADLoader: lump offset / size never checked against the file; `patchNames.reserve(count)` from the file;
    TEXTURE1/2 table and patch list read past their lump; a BSP children cycle = an endless loop piling polygons;
    `sectors[subSectorSectors[i]]` checked on `< 0` only. (Checked sound: blitPatch, linedef / sidedef / seg / sector
    references elsewhere, the directory itself.)
  - FBX array subscripts: bounded (false positives). The 139 union accesses: the ufbx API (on purpose, ledger).
- [x] (3b) Owner rulings (2026-09-30):
  - G1 **`fastgltf::validate()` + refusal**, after checking with the conformance bench that no asset loading today
    is rejected (if some are: the list goes to the owner first).
  - G2 **iterative + visited set**: a node reached twice (cycle / shared child) → the glTF is refused; USD walks
    iterative (a prim tree cannot cycle).
  - W1-W6 **structure refused, references skipped**: a lump outside the file, a table larger than its lump, a BSP
    cycle → "Corrupted WAD"; an out-of-range cross-reference → the element skipped; reserve() bounded by the lump.
  - Fuzzing **engine todo item** (a libFuzzer harness per loader later); now: hand-crafted hostile files through
    the console → a clean refusal, no crash.
  APPLIED 2026-09-30:
  - G1: `fastgltf::validate()` after the parse → refusal. Checked BEFORE with a standalone harness (same fastgltf,
    same options): 314 parseable glTF-Sample-Assets + 11 store assets, 0 rejected.
  - G2: glTF `isStrictNodeForest()` right after validate (at most one parent, every node reachable from a parentless
    one: no cycle) — 349 real assets, 0 rejected; `buildNodeDescriptors()` iterative; the 6 USD walks iterative
    (children pushed through `std::views::reverse`: the former order). ALSO `SceneDataConsumer` (Scenes, the next
    stage of the same hierarchy): the hostile 200 000-node chain OVERFLOWED THE STACK in `processNodeAsStatic()` →
    both consumer walks iterative, + a visited set (every loader feeds it).
  - W1-W6 applied (lump outside the file, TEXTURE1/2 table / entry / patch list, BSP node reached twice → "Corrupted
    WAD"; `reserve()` bounded; the sector upper bound).
  - Found while passing: FBX nodes wired by iterating an `unordered_map` keyed by pointer → a different root / child
    order (and duplicate-name winner) on every run: now the file order.
  - clang-tidy fix-its (designated initializers, parentheses, `std::cmp_*`, `getOptions() const`): the run also
    rewrote 7 emeraude-base headers and `Graphics/SharedUniformBuffer.hpp` (headers included by the TUs) — REVERTED
    (outside the section). ⚠️ Run fix-its with a header filter restricted to the section.
  - Engine items opened: `engine-fuzz-harness-for-loaders` (ruling), `texture-destroyed-while-upload-in-flight` (a
    load refused after its textures were uploaded → VUID destroy-in-use / use-after-destroy; also at shutdown after
    heavy loads), `rt-light-ssbo-unbound-with-disabled-light-set` (doom-loader: binding 3 never written, pre-existing).
- [x] (4) Verified 2026-09-30: cascade builds (0 warning), clangcheck 0, `-Wfloat-conversion` 0; clang-tidy 21 on the
  section 221 → 172 (all on purpose, ledger); runtime: hostile glTF (index out of range, cycle) refused, the 200 000
  chain loads, 20 + 24 real glTF samples load, 3 corrupted WADs refused with the right reason, doom1.wad loads
  (E1M1, 53 textures), a USDA tree (26 prims, depth 3, 4 meshes) and the FBX demos load; VUIDs seen belong to the
  two opened lifetime / binding items only.
- [x] (5) Pushed 2026-09-30: engine `bf5f901c`; peers asked.

## Section 4 — `src/Net` (started 2026-09-30)

- The 2026-08-27 network audit (memory `project_network_audit_2026_08_27`) was ALREADY FIXED the same day, lots 1-6
  (base `8f1eaa3` / `23e31f3`, engine `9ca76041` / `32524962` / `64cb8545` / `ad142ee9`, app_system `c69838e5`). What
  it left is base-side (totalTimeout not a hard budget, URI layer breaking signed URLs, no URI fuzzer, OCSP / CRL,
  TLS floor) and app_system (detached threads) — outside this engine pass.
- [x] (1) clang-tidy 21.1.6 baseline (the 10 Linux TUs; `*.windows.cpp` read by hand): **103 findings**: 30
  use-scoped-lock, 20 reinterpret-cast, 16 use-anonymous-namespace, 7 designated-initializers, 7
  make-member-function-const, 5 concise-preprocessor-directives, 5 pro-type-vararg, 3 const-correctness, 2
  use-enum-class, 1 each array-to-pointer-decay, macro-usage, avoid-c-arrays, use-ranges, interfaces-global-init (a
  false positive: `max_listen_connections` is `SOMAXCONN`, constant), constant-array-index, unnecessary-copy,
  starts-ends-with.
- [x] (2) Review — real defects, all under EARLIER rulings (no new owner question):
  - N1 (S3, "all at once"): four throwing directory walks the first census missed — a range-for over a NAMED
    `directory_iterator` or a braced one: `Net/Manager.cpp` ×2 (`.part` sweep, `clearCache`), `SerialPort.linux.cpp`
    (`/sys/class/tty`), `Graphics/TextureCache.cpp` (`clearCache`).
  - N2 (S1, "paths from data confined"): the download-cache index (`index.json`, on disk) names each file; `m_cacheDirectory
    / filename` with a tampered `"../../x"` would make an EVICTION `remove()` an arbitrary file.
  - N3 (rule "no throwing std call"): `SerialPort.windows.cpp` `std::stoul` on the USB VID / PID (terminate on a
    non-hex value; the Linux one was fixed by lot 4). And the WLAN SSID lengths used unbounded against their 32-byte
    array (`WiFiScanner.windows.cpp`).
- [x] (3) Applied 2026-09-30: N1 (4 walks: `IO::forEachDirectoryEntry()`, SerialPort an explicit `increment()`),
  N2 (cache index names confined), N3 (Windows `from_chars` + bounded `strnlen`, SSID clamp). Mechanical: scoped_lock
  ×30, designated initializers, concise preprocessor, `starts_with`, ranges, 16 helpers into anonymous namespaces.
  REVERTED fix-its: `make-member-function-const` on the I/O methods (logical mutation; and the fix-it changed
  `SerialPort.hpp` / `.linux.cpp` but not `.windows.cpp` → the Windows build would have broken). Fix-its run with
  `--header-filter='.*/src/Net/.*'`: nothing outside the section touched (the section 3 lesson).
- [x] (4) Verified 2026-09-30: cascade builds (0 warning), clangcheck 0, `-Wfloat-conversion` 0; clang-tidy 103 → 39
  (all on purpose, ledger); runtime: a tampered cache index (a relative and an absolute escape, a 1-byte budget
  forcing an eviction) → both dropped, the victim file intact, the `.part` swept, `clearCache()` fine; a real
  download → cached → index reloaded after a restart → cleared; 0 VUID. The owner's cache was restored to its
  original state (empty). UDP / serial / WiFi changes: compile-checked only (no device); `*.windows.cpp` compiled by
  the Windows peer only.
  Found: the CEF helper processes start the download manager on the SAME cache → engine item
  `net-cache-managed-by-cef-helper-processes`.
- [x] (5) Pushed 2026-09-30: engine `e40abf15`; peers asked.
  - macOS peer: PASS. Noticed a LEGACY `downloads_db.json` at the cache ROOT (the pre-2026-08-27 manager's index,
    `"FileDataBase": []`), never read nor cleaned by the current manager — a leftover, harmless; owner to decide
    (a one-time cleanup, or leave it).

## Section 5 — `src/Input` (started 2026-09-30)

- [x] (1) clang-tidy 21.1.6 baseline (6 TUs): **47 findings**: 25 pro-bounds-constant-array-index, 8 use-enum-class, 7
  implicit-bool-conversion, 4 misc-confusable-identifiers (`KeyI` / `Key1`, `KeyO` / `Key0`: GLFW's names), 1 each
  unintended-char-ostream-output, narrowing-conversions, inconsistent-declaration-parameter-name.
- [x] (2) Review. The console / MCP injection validates its input (keys 32-348, buttons 0-7, modifiers 0-63). Real
  defects, all intent-obvious (public engine API = checked always, the two-level ruling):
  - J1 `JoystickController::axeValue()` reads the axis only when the device is NOT usable: a connected joystick always
    answers 0 (joystick movement never worked); with no device, `s_devicesState.at(-1)` throws → std::terminate —
    `Player::getJoystickMoveInput()` calls it every frame when joystick control is enabled.
  - G1 `GamepadController::axeValue()`: the same inversion, and `s_devicesState[-1]` (out of bounds) with no device.
  - G2 `GamepadController::isButtonReleased()` returned `== GLFW_PRESS`, exactly like `isButtonPressed()`.
  - J2 / J3 joystick buttons / hats: bound checked in DEBUG only, with `>` instead of `>=`; nothing in Release.
  - K1 keyboard / pointer state arrays indexed by a key / button checked only against `KeyUnknown` (-1): any other
    out-of-range value from a caller is an out-of-bounds read / write.
  - C1 `isConnected()` (joystick AND gamepad) accepts `m_deviceID == DeviceCount` (16), one past the array.
  - projet-alpha `Player::getJoystickMoveInput()`: `else if ( value > 0.0F )` three times where `< 0.0F` is meant —
    the negative directions (Left, Upward, Forward) were dead branches.
  - Throwing `.at()` (gamepad / joystick / keyboard `getRawState()`, joystick reads): → bounded `operator[]`.
- [x] (3) Applied 2026-09-30: J1, G1, G2, J2 / J3 (checked always, `>=`), K1 (keyboard / pointer bounds on every
  query and on `changeKeyState()`), C1 (`< DeviceCount`), the `.at()` → bounded `[]`, the hat printed as an int, the
  `to_cstring(Key)` parameter name; projet-alpha `Player` negative directions (`< 0.0F`).
- [x] (4) Verified 2026-09-30: cascade builds (0 warning), clangcheck 0, `-Wfloat-conversion` 0; clang-tidy 47 → 40 (all
  on purpose, ledger); console conformance 4448/0, MCP 1707/0, `keyPress(298)` / `mouseClick(0)` injected,
  `keyPress(9999)` / `mouseClick(99)` refused, 0 VUID. The joystick / gamepad fixes: proven by code only (no device on
  Linux, macOS or Windows).
- [x] (5) Pushed 2026-09-30: engine `fe74dac0`, alpha `2856df1e`; peers asked.

## Section 6 — `src/Scenes` (started 2026-09-30), five sub-sections

| Sub | Content | Lines | Status |
|---|---|---|---|
| 6a | Scene graph core: `Node`, `NodeCrawler`, `AbstractEntity` (+ debug), `StaticEntity`, `NodeController`, `OrbitController`, `LocatableInterface`, `OctreeSector` (+ crawler), `Scene.cpp`, `Scene.entities.cpp`, `Scene.hpp` | ~11 000 | ✅ pushed `23f04e76`; VALIDATED macOS M2 + Windows NVIDIA (1 MiB stack) — hostile refused, engine alive, watch + RecursiveSkeletons render, 0 VUID |
| 6b | Scene rendering / lighting / physics: `Scene.rendering/lighting/physics/debug.cpp`, `LightSet`, `SceneInstanceTransforms`, `SceneMetaData`, `RenderBatch`, `InstanceCluster`, `BindlessTextureSet`, `CloudSet`, `ParticipatingMedium`, influence areas, shadow options, ground / sea interfaces | ~9 000 | ✅ pushed base `33dc712`, engine `53116abe`, alpha `e8c7f692` (+ fixes base `f4319cc`, engine `7a3e540c`, alpha `805095b0`); VALIDATED macOS M2 + Windows NVIDIA |
| 6c | `Manager` (+ console), `Toolkit`, `DefinitionResource` (JSON scene definitions: a trust boundary) | ~5 000 | ✅ pushed base `9636ea0`, engine `e8dd6016`, alpha `e5b8ce2b`; VALIDATED macOS M2 + Windows NVIDIA/AMD |
| 6d | `Component/` | 18 617 | ✅ pushed engine `e32b26f2`; VALIDATED macOS M2 + Windows NVIDIA/AMD |
| 6e | `Editor/`, `AVConsole/`, `Viewers/`, `EffectsToolkit/`, `Debug/` | ~8 500 | ✅ pushed engine `c7f7dd15`, alpha `47128cce`; VALIDATED macOS M2 + Windows NVIDIA/AMD |

Lead carried from section 3: `SceneDataConsumer::processNodeAsNode()` can build a DEEP `Node` chain from a file (a
node with a transform is never flattened) — check `Node`'s destructor and every recursive traversal for depth.

### 6a — scene graph core (started 2026-09-30)

- [x] (1) clang-tidy 21.1.6 baseline (8 TUs, findings of the 6a files only): **48**: 13 misc-no-recursion, 7
  math-missing-parentheses, 4 use-enum-class, 4 constant-array-index, 3 qualified-auto, 3 bugprone-use-after-move (a
  callable `std::forward`ed INSIDE a loop: `forEachComponent` ×2, `forEachModifiers`), 3 missing-std-forward, 3
  implicit-bool-conversion, 2 static-cast-downcast, and singles.
- [x] (2) REPRODUCED CRASH (the lead from section 3): a dropped glTF — a 200 000-level chain with a node animation, so
  ModelViewer's NODE mode — overflows the stack in `Scene::onNotification()`: a notification climbs the `Node` tree
  hop by hop (`Node::onUnhandledNotification()` → `notify()` → the parent…), and every graph walk recurses
  (`destroyTree`, `trimTree`, `destroyChildren`, `onLocationDataUpdate`, ModelViewer's `mergeSubtree`). The deepest
  real asset: 30 levels (349 measured; `RecursiveSkeletons`, `Dragon.glb` 22).
- [x] (3b) Owner ruling (2026-09-30): **an ENGINE-WIDE DEPTH CAP** — `Node::MaxDepth = 256`, `Node::depth()`,
  `createChild()` refuses beyond; SceneDataConsumer skips a refused subtree and fails the build (it also dereferenced
  the null node of a DUPLICATE name, in node and flatten modes); ModelViewer drops the load.
- [x] (3) Mechanical: the 3 forwards inside loops → lvalue calls; the render-state READ getters checked in Debug (like
  the writes); `OctreeSector::collapse()` `= nullptr` (the pointee has its own `reset()`); `NodeCrawler` by `const &`;
  fix-its (parentheses, qualified auto, implicit bool, loop convert, redundant member init) with a header filter
  restricted to the 6a files; an unused `using`.
- [x] (4) Verified 2026-09-30: cascade builds (0 warning), clangcheck 0, `-Wfloat-conversion` 0; clang-tidy 48 → 31 (all
  on purpose, ledger); the hostile 200 000-level animated glTF dropped in the model viewer → "would sit 257 levels
  under the root… refused", "build failed", no crash (it overflowed the stack before); `RecursiveSkeletons` (30
  levels) and `ChronographWatch` (node mode) load and render; `animation-debug` (15 entities), `citadel` (750): 0 VUID.
- [x] (5) Pushed 2026-09-30: engine `23f04e76`; peers asked.
  - macOS peer (twice now, citadel and the watch): a `screenshot()` sent while an asset is still uploading answers
    "did not complete in time … is the window rendering?" — a misleading wording (the window renders, the frame waits
    for the upload). A candidate for 6e / the renderer's console: say "the scene is still loading".
  - Both peers asked whether RecursiveSkeletons' "single white column" is right: it is NOT (the Khronos reference shows
    several branching shapes); reproduced on Linux → engine item `gltf-recursive-skeletons-render-as-one-straight-column`
    (not proven pre-existing; the triad's glTF changes do not touch skins).

### 6b — scene rendering / lighting / physics (2026-09-30)

- [x] (1) clang-tidy 21.1.6 baseline (13 TUs, the 6b files only): **98**: 40 use-scoped-lock, 21 constant-array-index,
  7 math-missing-parentheses, 5 use-std-min-max, 4 misc-unused-parameters, 3 designated-initializers, 3
  avoid-const-or-ref, 2 each convert-to-static / special-member-functions / reinterpret / const-cast, singles.
- [x] (2) Review: the 21 subscripts are all bounded by construction (slots, enums, clamped LOD, cloud census, the line
  light's ≤ 9 points); no throwing call in 6b. Found while scanning: **40 value-form `std::any_cast< T >(data)`**
  cascade-wide (engine 28 — Scene.entities ×19, Core ×4, Scene ×2, Scene.rendering ×2, Resources/Container ×1 —,
  projet-alpha 12): a payload of the wrong type = `bad_any_cast` = an abort.
- [x] (3b) Owner ruling (2026-09-30): **a base helper + explicit skip, all at once** — `Base::anyValue< T >(data,
  context)` (`emeraude-base/src/AnyValue.hpp`, pointer form, logs; test in test_ObserverPattern), every site `if (
  const auto * x = anyValue< … >(data, ClassId) ) { … }` (a handler returning bool answers `true`: ignored, still
  listening). APPLIED: 0 value-form left.
- [x] (3) Mechanical (fix-its restricted to the 6b files): scoped_lock ×40, parentheses, `std::min` / `std::max`,
  designated initializers, `globalIndex` / `DebugPath` initialised, the forward inside `forEachDirectionalLight`'s
  loop, `GroundLevelInterface` / `SeaLevelInterface` copy / move deleted (polymorphic interfaces), the unused `scene`
  parameters named in a comment, `applyCollisionResponse` in an anonymous namespace, two helpers made static.
- [x] (4) Verified 2026-09-30: cascade builds (0 warning), clangcheck 0, `-Wfloat-conversion` 0; base 2166/2166 Release
  AND ASan/UBSan; clang-tidy 98 → 32 (all on purpose, ledger); `beams` (camera + line light registered, console
  conformance 4448/0), `citadel` (sun shadows, torches), `lighten-marbles` (physics): 0 VUID, 0 payload-mismatch log.
- [x] (5) Pushed 2026-09-30: base `33dc712`, engine `53116abe`, alpha `e8c7f692`; peers asked.
  - macOS peer: PASS (0 payload errors, scenes lit, the helmet dropped renders). It reported ONE pre-existing error on
    lighten-marbles: "Refusing to link the component 'Light' to entity 'ACTOR_…40' from a component's processLogics()"
    — a marble lost its light. Cause confirmed: `AbstractEntity::linkComponent()` / `removeComponent()` /
    `clearComponents()` / `onContainerMove()` read `m_dispatchingComponentLogics` (a plain bool) BEFORE taking the
    recursive `m_componentsMutex`: a data race, and a FALSE refusal when another thread (the scene timer spawning the
    marble) links while the logic thread iterates that new entity. FIXED (pushed 2026-09-30, with the shutdown fix below):
    the flag is read UNDER the lock — the logic thread re-enters and refuses (the legitimate case), another thread
    waits for the loop to end. Verified: lighten-marbles (129 entities) and animation-debug, 0 refusal, 0 VUID.
  - Windows peer: PASS on everything else, but lighten-marbles CRASHED AT SHUTDOWN 5/5 (heap corruption
    `0xc0000374`). Reproduced on Linux 2/2 under `MALLOC_CHECK_=3 MALLOC_PERTURB_=165`; addr2line: the
    `LightenMarbles` timer lambda. Cause: projet-alpha `Stage` erases the act BEFORE `deleteScene()`, the scene's
    timer (capturing `&act`) keeps firing on the dead act. FIXED in projet-alpha (pushed 2026-09-30): `Act::~Act()` calls
    `m_scene->destroyTimers()` first — 3/3 clean under the same poisoning, 0 VUID. Caution: projet-alpha
    `docs/caution-points.md` § Scene Building; the join-under-lock trap it exposed: base
    `docs/todo/event-trait-join-under-lock.md`.
  - Re-run 2026-09-30 (base `f4319cc`, engine `7a3e540c`, alpha `805095b0`): VALIDATED. Windows (RTX 3060 Laptop,
    MSVC /W4 /WX 0 warning): lighten-marbles 5/5 exit 0 (was 5/5 `0xc0000374`), 0 crash event, 0 VUID, 0 refusal —
    the spawner was live at shutdown (128 nodes / 127 point lights at 25 s). macOS (M2): 3/3 clean, 0 "Refusing to
    link", every marble lit. animation-debug and beams clean on both.

### 6c — scene manager, toolkit, scene definitions (started 2026-09-30)

- [x] (1) clang-tidy 21.1.6 baseline (4 TUs: `Manager.cpp`, `Manager.console.cpp`, `Toolkit.cpp`,
  `DefinitionResource.cpp`; the 6c files only): **16**: 7 use-scoped-lock, 5 constant-array-index
  (`getRenderStatistics`: three `std::array` of the SAME `Geometry::MaxLODLevels` size walked by one index — bounded),
  2 missing-std-forward (`withShared/ExclusiveActiveScene`), 2 raw-string-literal, and singles: use-enum-class
  (`NotificationCode`, the convention), isolate-declaration, misc-no-recursion (`readNodes`: bounded by the parse's
  `stackLimit` 16 and by `Node::MaxDepth`), designated-initializers. The 71 other findings of these TUs are in headers
  of 6a / 6b / 6d / 6e / Loaders (their own passes). ⚠️ 3 `bugprone-use-after-move` in `Scene.hpp` (1248, 1808, 1859:
  a `std::forward` inside a loop) only show when instantiated from these TUs: 6a missed them → fix with 6c.
- [x] (2) Review. REPRODUCED at runtime (`Core.openFiles()` = a dropped file, the documented remote path), each a
  **SIGABRT** — jsoncpp's LIBRARY is built with exceptions: `Json::LogicError` escapes a `noexcept` (the headers'
  `JSON_USE_EXCEPTION 0` only changes inline code; with it the library would `abort()` anyway):
  - D1 a non-object root (`[1, 2]`): `Core::openSceneDefinition()` calls `root.isMember()` on an array ("requires
    objectValue or nullValue"); `DefinitionResource` does the same everywhere on `m_root`.
  - D2 a non-numeric array element (`"Position": ["a", "b", "c"]`): the raw `asFloat()` of `readNodes()` /
    `readStaticEntities()` / the ambient colour ("Value is not convertible to float"). Base `FastJSON::getValue<
    Vector / Matrix / Color >` has the SAME defect (`createFromJsonImpl` never checks an element's type).
  - D3 an out-of-range integer (`"GridDivision": -1`): base `FastJSON::getValue< uint32_t >` checks `isNumeric()` then
    `asUInt()` ("LargestInt out of UInt range"); the same for `asInt()` of `1e20`. **Base-wide**: every integral
    `getValue` of the cascade. Census of the raw conversions: engine 13 integral (+ ~126 float/double, of which the
    unchecked ones abort on a non-number), base 4, projet-alpha 0 integral.
  - D4 non-finite floats are ACCEPTED silently: the parser runs with `allowSpecialFloats = true` (NaN, Infinity,
    and `1e999` → inf). `"Boundary": NaN` built scene `H4` with a NaN boundary, no log (`Scene::rebuildRenderingOctree()`
    tests `<= 0`, which NaN passes). No range checks either: `GridDivision` unbounded (`(n+1)²` vertices), `Scale`
    <= 0 / NaN, ambient colour / illuminance negative.
  - T1 `Toolkit::generateNode()` under `setParentNode()` creates TWO nodes: `parent = m_previousNode->createChild(name)`
    then `parent->createChild(name)` — an empty node, then the real one under it, the cursor offset applied TWICE.
    `collision-debug` `listEntities()`: `HierarchyParent6/HierarchyChildA7` (no component) →
    `HierarchyChildA7/HierarchyChildA7` (the cube), the same for ChildB (also `node-relation`).
  - T2 same function: a `Reusable` / `Parent` policy with a null node (`setParentNode(nullptr)`, a failed build
    passed on) `break`s with `parent` still null → `parent->createChild()` dereferences null; and a refused
    `createChild()` in the Parent case (depth cap, duplicate name) the same.
  - M1 `Manager` emits UNDER `m_sceneListAccess`: `SceneCreated` (newScene, before the emplace), `SceneDestroyed`
    (deleteScene, onTerminate): Core's handler runs `Resources::Manager::unloadUnusedResources()` under the list lock,
    and a handler calling `getScene()` / `getSceneNames()` would deadlock (non-recursive mutex).
  - M2 `hasSceneNamed()` (public: ImageViewer, ModelViewer, the console, projet-alpha `AbstractDemo`) read `m_scenes`
    WITHOUT the lock → fixed in (3).
  - L1 every scene-definition load logs "resource '…' is destroyed while still enqueueing dependencies (Manual mode)":
    `DefinitionResource::load(json)` never completes the resource's load state.
  - Not a defect: the console commands read `m_activeScene` unlocked — they run on the main thread (the only writer).
  - Leads for later sections: `BasicGroundResource.cpp:171` / `TerrainResource` raw `asUInt()` after `isNumeric()`
    (D3 pattern, section 7); `Core.cpp` tools mode `arguments().get(…).value()` (section 15).
- [x] (3) Mechanical (2026-09-30): scoped_lock ×7; `std::forward` of the once-called callable in
  `withShared/ExclusiveActiveScene`; `*rootCheck` instead of the throwing `.value()` ×2; `hasSceneNamed()` takes the list
  lock (and `newScene()`, which holds it, reads `m_scenes.contains()` directly); the ambient declaration isolated;
  `TreeImposter` designated initializer; `bakeTreeImposter()` also refuses a NaN / infinite radius (its existing
  refusal, `<= 0` let NaN through); two raw strings; `Scene.hpp` the 3 flagged forwards inside a loop + a 4th, same
  shape, in `forEachRenderToShadowMap()` → lvalue calls. Builds clean.
- [x] (3b) Owner rulings (2026-09-30), all as recommended:
  - D1-D3 **ALL AT ONCE, cascade-wide**: base FastJSON fixed (an out-of-range integer, a non-numeric element, a
    non-number → `std::nullopt`) + tests; a non-object root refused (`DefinitionResource`, `Core::openSceneDefinition`);
    EVERY raw jsoncpp conversion of the cascade (`asInt/UInt/Int64/UInt64/Float/Double/String/Bool/CString`: ~210
    sites, ~40 files — engine ~190, base ~14, alpha 2) moved onto the checked FastJSON helpers.
  - D4 **getValue REFUSES a non-finite float** (and a double out of float range): `std::nullopt` + debug log = absent,
    the caller's default applies; the parser's `allowSpecialFloats` stays (1e999 → inf regardless of it).
  - Scene-definition values: **warning + the key's default** (as if absent); `GridDivision` capped at 1024 (≈ 1 M
    vertices), beyond → warning + 64.
  - T1 (child directly under the set parent: the demos' children move to their commented offset), T2 (null node →
    the root; a refused `createChild()` → nullptr + error), M1 (emit after the list lock, `SceneCreated` after the
    emplace), L1 (the definition marks its load complete): APPLY.
  APPLIED 2026-09-30:
  - Base `FastJSON::asValue< T >(node)` (the checked counterpart of jsoncpp's `as*()`), the keyed `getValue` on top of
    it, Vector / Matrix / Color elements checked; tests failing first (`LargestInt out of UInt range`, NaN accepted,
    `Value is not convertible to float`) then passing. Rule: base `docs/error-handling.md` § JSON, projet-alpha
    `.claude/rules/ave-robustus.md`, `docs/plans/ave-robustus.md`.
  - EXACT census (a `[[deprecated]]` overlay of `json/value.h`, recipe in base `docs/error-handling.md`; the grep
    counted base `Variant::asFloat()` too): **89 sites / 19 files → 6, all inside `asValue()`**. Migrated:
    DefinitionResource, BasicGroundResource, TerrainResource, CubemapResource, SkyBoxResource, Material Texture +
    Helpers, Settings, MCP Protocol + Server, BaseInformation (+ `dataString()`), Container, LoadingRequest,
    Net::Manager, BodyPhysicalProperties, Playlist, Soundfont, projet-alpha Forest.
  - Non-object roots refused at EVERY root-parse site that expects an object (the ruling extended from the scene path
    to the same defect elsewhere): `ResourceTrait::load(path)` (every resource file), DefinitionResource, Core (store
    index + scene definition), both grounds, `Resources::Manager` indexes, `Net::Manager` cache index,
    `Settings::readFile()` (`getMemberNames()` on an array); a non-object store entry refused by
    `BaseInformation::parse()`.
  - Defects found while migrating, fixed on the lines touched: `getComponentAsValue()` required the `Data` of a
    `Value` component to be an OBJECT and then read it as a float (an abort; the documented form is a number);
    `BasicGroundResource` tested `!isMember && !isString` (`||` meant) and checked the material NAME key's presence
    against the TYPE key; `parseColorComponent()`'s throwing `.at()`.
  - Scene definitions: warning + default (`readNumber()` / `readVector()`), GridDivision cap 1024; the definition
    completes its load state (L1: the "destroyed while still enqueueing" warning is gone).
  - T1 + T2 (Toolkit), M1 (Manager): `SceneCreated` / `SceneDestroyed` after the list lock; ALSO `SceneEnabled` /
    `SceneDisabled` after the exclusive active-scene access (same DEFER rule: a handler calling `hasActiveScene()`
    would deadlock). Residual: the `SceneDisabled` of `deleteScene()` / `onTerminate()` still fires under the list
    lock (they disable from inside it) — documented in `subsystems/source-tree/02-main-components.md`.
  - Item `console-json-non-finite-numbers`: `getNode()` / `getNodePhysics()` done (`writeJSONVector()`: null for a
    non-finite component, 9 digits); the other hand-streamed answers listed in the item (Renderer, APIClient, Window).
- [x] (4) Verified 2026-09-30 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 128 TUs 0;
  `-Wfloat-conversion` + CEF-flags clang on the 23 touched TUs 0; clang-tidy 6c 20 → 7 (on purpose, ledger), 0 new
  finding on the changed lines of every touched TU but one bounded subscript (ledger); base **2170/2170** Release AND
  ASan/UBSan. Runtime (`Core.openFiles()`): the 3 hostile definitions that aborted now load with a warning, NaN
  boundary → warning + 1000, an all-invalid definition → 7 warnings, scene built; `collision-debug`: 21 entities (was
  23), ChildA directly under the parent; `citadel`: MCP conformance 1707/0, console conformance 4466/0, 0 VUID, the
  log's error / warning classes identical to the 6b run but the two the console conformance provokes on purpose.
  The 3 `*** stack smashing detected ***` lines at exit are the pre-existing CEF helper item
  (`cef-memoryinfra-check-sigill`), present before the change.
- [x] (5) Pushed 2026-09-30: base `9636ea0`, engine `e8dd6016`, alpha `e5b8ce2b` (+ the owner's macOS MoltenVK validation record `c0856970`); peers asked 2026-09-30 (macOS-PA, Windows-PA: base tests, the 5 hostile definitions, collision-debug hierarchy, citadel conformance + delete/create scene).
  - macOS peer (M2, AppleClang 0 warning): PASS on steps 1-4 (2170 = 2167 + 3 skipped, the 5 hostile definitions
    alive with the exact warnings, collision-debug 21 entities, MCP 1707/0, console 4445/0 — its command set). Side
    finding, reproduced on Linux, PRE-EXISTING and independent of 6c: after `deleteScene(citadel)` +
    `createScene(Fresh…)`, the act still holds the citadel scene and destroys it at shutdown without a GPU drain
    (Linux 20 VUIDs, macOS 31). Owner: option 2 (GPU-synchronised destruction). ROOT CAUSE (gdb on the validation
    callback): the renderer updates EVERY registered surface geometry every frame, so the inactive citadel's FFT
    ocean was still in flight in Fresh's frames and `OceanSurfaceResource::destroyFromHardware()` freed in place (a
    `waitIdle` in `~Scene` was tried: useless, the next frame resubmits). FIXED 2026-09-30: the ocean AND
    `CDLODTerrainResource` retire their GPU objects through `Renderer::deferredDestructor()`; projet-alpha
    `Stage::deleteActScene()` no longer deletes a scene the console already deleted. Linux: citadel delete/create
    0 VUID 2/2, terrain delete/create 0, no false error. Engine caution-points § "A surface geometry must RETIRE its
    GPU objects". Then FIXED 2026-09-30 (both owner rulings): only the surfaces the previous frame drew are
    updated (an inactive scene's sea / terrain cost nothing: 0 FFT update in 25 s on beams), and imposter resources
    are named per scene with the atlas retired at the bake (the pre-existing 20× `vkEndCommandBuffer-00059` after
    citadel → beams → terrain gone). Linux: citadel → beams → terrain → citadel → delete/create → shutdown, 0 VUID,
    0 error. Engine caution-points § Scene Rendering (both entries).
    Pushed engine `f3c5284f`. macOS M2 VALIDATED 2026-09-30 (AppleClang 0 warning; the full sequence 0 VUID, 0
    UNASSIGNED, 0 [Error], exit 0 — was 31 VUIDs + 2 errors; 20 'Imposter/terrain/…' atlases baked). Windows
    VALIDATED 2026-09-30 (MSVC /W4 /WX 0 warning): NVIDIA RTX 3060 Laptop 0 teardown VUID (was 25; only the known
    12325 mesh-shader one), AMD iGPU 0 VUID 0 error (was 5); terrain pixel-close NVIDIA vs AMD.
  - Windows peer (RTX 3060 Laptop + AMD iGPU, MSVC /W4 /WX 0 warning): PASS on steps 1-4 (2170 = 2167 + 3 skipped,
    the 5 hostile definitions alive with the exact warnings, collision-debug 21 entities — ChildA at the 6 m offset
    from the screenshot geometry —, MCP 1707/0, console 4457/0). The same delete/create teardown defect (fixed above): NVIDIA 25
    VUIDs 2/2, AMD 1/1; controls clean (collision-debug + delete/create: 0). Only known VUID: the mesh-shader
    viewMask 12325.
  - ✅ 6c VALIDATED on Linux, macOS M2 and Windows NVIDIA + AMD.
- Leads noted for later sections: `BasicGroundResource` passes `DefaultGeometryFlags` as the grid's UV MULTIPLIER
  (`VertexGridResource::load(float, uint32_t, float)`) and its `load(path)` / `load(json)` call `setLoadSuccess()`
  without `beginLoading()` (section 7); `SoundfontResource` opens the JSON `file` path unconfined (section 10);
  Core tools mode `arguments().get(…).value()` (section 15).

### 6d — `Component/` (started 2026-09-30)

- [x] (1) clang-tidy 21.1.6 baseline (31 TUs, the `Component/` files only): **75**: 22 constant-array-index, 12
  math-missing-parentheses, 9 use-enum-class, 8 implicit-widening, 5 missing-std-forward, 3 non-private members, 3
  special-member-functions, 2 non-const globals, 2 designated-initializers, 2 inconsistent parameter names, and
  singles: mt-unsafe (`std::lgamma`), scoped-lock, string concatenation, const-ref member, static-cast downcast,
  nested conditional, a dead store.
- [x] (2) Review. The console / MCP boundary is sound: every component adapter validates its numbers (finite, sign,
  range) and `parsePointList()` refuses a non-finite point. Findings:
  - C1 **the component setters (public engine API) accept NaN / infinity**: ~45 with no check at all, others only a
    sign policy (`std::abs`) or a clamp NaN may cross; `SunCourse::setPhase(NaN)` feeds `std::lround()` (undefined).
  - C2 `ParticlesEmitter`: a respawned particle sets `newLocation = true`, then the physics `switch` overwrites the flag
    in every case (clang-analyzer dead store): a respawned particle whose physics reports "no move" is not written.
  - On purpose (ledger): the static name counters are atomics; `std::lgamma` (`Beam::equivalentTubeLuminance()`) has
    one caller, on the logic thread; the light emitters' protected projection state is shared by the derived lights;
    the subscripts are bounded (slots, cascades, the line light's ≤ 9 points).
- [x] (3) Mechanical (2026-09-30): parentheses, `size_t` before the multiplications used as offsets, designated
  initializers, scoped_lock, the nested conditional of `LineLight::updateWorldPoints` as an `if`, `*points` instead of
  `.value()` ×3, `std::forward` of the callables invoked once (4; `forEachSuffix` calls in a loop: kept), the suffix
  built without temporaries, copy / move deleted on CloudVolume, SkyFollowsSun, SoundEmitter. ⚠️ clang-tidy `--fix`
  REFORMATS the lines it touches (a mangled `bindCommand` in PathConsoleAdapter): three files redone by hand. Builds
  clean.
- [x] (3b) Owner rulings (2026-09-30): C1 **refuse a non-finite value, keep the previous one, log an error** (the sign
  / range policies stay; the adapters keep their explicit replies); C2 **fix** (write a particle respawned OR moved).
  APPLIED 2026-09-30: `Component::Abstract::acceptsFinite(setter, values…)` (floats, vectors, colours, spans of points;
  the refusal logged by `traceNonFiniteValue()`) as the first line of 68 setters; `ParticlesEmitter` writes a particle
  respawned OR moved. Contract: `docs/subsystems/scenes/02-scenes-specific-rules.md`.
- [x] (4) Verified 2026-09-30 (Linux): cascade builds (0 warning), clangcheck 125 TUs 0, `-Wfloat-conversion` 0 on the
  Component TUs but a PRE-EXISTING base one (`Animation/AnimationChannel.hpp:277`, a `0.05` double into
  `Quaternion< float >::slerp()`, instantiated from `NodeAnimation.cpp`; base 72cb8c9, 2026-08-29 — lead for the base);
  clang-tidy 75 → 40 (on purpose, ledger). Proof of the refusal: a TEMPORARY `setParticleSize(NaN)` +
  `setChaos(inf)` in projet-alpha's particles demo logged both refusals, then was reverted. Demos particles, beams
  (MCP 1707/0, console 4448/0), terrain, lighten-marbles, animation-debug: 0 VUID, 0 error, 0 refusal on the legitimate
  paths.
- [x] (5) Pushed 2026-09-30: engine `e32b26f2`; peers asked.
  - macOS M2: PASS (AppleClang 0 warning; particles, beams — MCP 1707/0, console 4427/0, its command set —,
    lighten-marbles, terrain, animation-debug: 0 VUID, 0 UNASSIGNED, 0 error, 0 refusal, exit 0).
  - Windows RTX 3060 Laptop + AMD iGPU: PASS (MSVC /W4 /WX 0 warning; the 5 demos 0 VUID, 0 refusal in 11 runs; MCP
    1707/0, console 4439/0). One observation, unrelated to 6d: `terrain` shut down at 40 s, WHILE its act was still
    loading, logged "[UIManagerService] No default page found !" and a CEF teardown timeout of the menu web-view (0 at
    100 s) — noted in item `act-load-blocks-main-thread`.

### 6e — `Editor/`, `AVConsole/`, `Viewers/`, `EffectsToolkit/`, `Debug/` (started 2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (14 TUs, the 6e files only): **78**: 26 use-scoped-lock, 11 constant-array-index,
  9 pro-type-vararg (ImGui's printf-style API, constant formats), 6 non-private members (the gizmo base), 5
  switch-missing-default-case, 5 use-anonymous-namespace, 5 avoid-const-or-ref-data-members, 3 parentheses, 2
  const-cast, 2 no-automatic-move, singles.
- [x] (2) Review. Findings:
  - A1 **`AbstractVirtualDevice::interconnect()`** (both overloads) has NO caller in the engine or projet-alpha, and
    the first one is undefined behaviour: it range-fors over `m_outputDevicesConnected` while the `disconnect()` /
    `connect()` it calls erase from and insert into that set (plus a TODO: no rollback on a half-done insertion).
  - A2 **`WeakPtrOwnerHash` constructs a `std::shared_ptr` from the weak_ptr it hashes**: that constructor throws
    `std::bad_weak_ptr` on an EXPIRED device = an abort. PROVEN with a standalone copy of the hasher: libstdc++ caches
    the hash of a non-`noexcept` hasher, so Linux survives a rehash by accident; on a non-cached path (the same hasher
    `noexcept`, and MSVC's `unordered_set`, which does not cache) inserting into a set holding an expired peer →
    SIGABRT. A peer that dies while still connected is all it takes.
  - A3 `AbstractVirtualDevice::disconnectFromAll()` departs from the class's own pattern (`connect()` /
    `disconnect()` lock BOTH mutexes in one `std::scoped_lock` and fire their events after unlocking): it holds its
    mutex, then locks each peer's (a lock-order deadlock between two devices), fires the peers' events under both
    locks, and calls `shared_from_this()` (its comment records a past crash there).
  - V1 `ImageViewer`: an image that reads but has a zero dimension divided by zero → refused (fixed in (3)).
  - On purpose (ledger): the ImGui varargs; the `GizmoMode` switches cover every enumerator (a `default` would silence
    `-Wswitch`); the gizmo base's protected state; the subscripts (compass axes, gizmo axes).
- [x] (3) Mechanical (2026-10-01): scoped_lock ×23, parentheses ×3, `CameraPresets` helpers in an anonymous
  namespace, the two viewer scenes returned by move, a const pointee, the World / Parent transform-space branches
  merged, the ImageViewer empty-image refusal; a NON-CONST `Scene::forEachStaticEntities()` overload removes the three
  `const_cast`s that worked around it (the editor, ModelViewer, projet-alpha's asset loader). Builds clean.
- [x] (3b) Owner rulings (2026-10-01), all as recommended, APPLIED: A1 both `interconnect()` overloads REMOVED (dead API,
  latent UB; its now-unused `TracerTag` too — AppleClang `-Wunused-const-variable`); A2 the device sets are
  `std::set< std::weak_ptr< AbstractVirtualDevice >, std::owner_less<> >` and the throwing `WeakPtrOwnerHash` /
  `WeakPtrOwnerEqual` are gone; A3 `disconnectFromAll()` snapshots its peers, edits both sides of each link under ONE
  `std::scoped_lock` of the two mutexes, fires the SAME peer events after unlocking, and uses `weak_from_this()`.
  Engine caution-points § Build / Compiler (the weak_ptr hashing trap).
- [x] (4) Verified 2026-10-01 (Linux): cascade builds (0 warning), clangcheck 115 TUs 0, `-Wfloat-conversion` 0;
  clang-tidy 78 → 38 (on purpose, ledger). One launch: citadel → console `deleteScene(citadel)` → an image dropped
  (`+ImageViewer`) → a glTF dropped (`+ModelViewer`) → `Stage.loadDemo(beams)` (MCP 1707/0, console 4448/0) →
  lighten-marbles → shutdown: 0 VUID, 0 error. The animated Fox dropped, `cycleAnimation()` ×2 (the ModelViewer path
  that lost its const_cast): 0 VUID.
- [x] (5) Pushed 2026-10-01: engine `c7f7dd15`, alpha `47128cce`; peers asked.
  - macOS M2: PASS (AppleClang 0 warning; the one-launch sequence — delete citadel, image viewer, Fox 'Animation
    cycled.' ×2, beams MCP 1707/0 + console 4427/0, the scene editor opened / closed, lighten-marbles, shutdown —: 0
    VUID, 0 UNASSIGNED, 0 error, exit 0).
  - Windows RTX 3060 Laptop + AMD iGPU: PASS (MSVC /W4 /WX 0 warning, the owner_less sets and the two
    forEachStaticEntities overloads clean; the same sequence on both GPUs: MCP 1707/0, console 4439/0, clean exit, no
    crash event; only the known citadel 12325 on NVIDIA, 0 VUID on AMD). The MSVC rehash abort path is gone.
  - ✅ 6e VALIDATED on Linux, macOS M2 and Windows NVIDIA + AMD.

## Section 7 — `src/Graphics` (started 2026-10-01), seven sub-sections (owner, 2026-10-01)

| Sub | Content | Lines | Status |
|---|---|---|---|
| 7a | Resources read from disk: images and textures (`ImageResource`, `CompressedImageResource`, `KTX2Decoder`, `TextureCompressor`, `VolumetricImageResource`, `TextureResource/`, `TextureCache`), cubemaps and IBL (`CubemapResource`, `IBLTexture`), video (`MovieResource`, `CubemapMovieResource`, `VideoFrameConverter`, `ExternalInput`), `FontResource`, `CloudShapeResource` | ~13 700 | ✅ pushed base `10e23f3`, engine `4e4bda64` + MSVC hotfix `734b4422`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 7b | `Material/` (JSON material definitions) | ~14 000 | ✅ pushed engine `1ee4a6c6`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 7c | `Geometry/`, `Renderable/`, `MDI/` (grounds, terrains, seas, meshes) | ~20 500 | ✅ pushed base `dce53a7`, engine `b1541d1c`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 7d | Renderer and frame: `Renderer` (+ console), `RendererFrameScope`, `Recorder`, `FrameCapture`, `RenderDocCapture` | ~10 400 | ✅ pushed (the engine 7d commit); peers pending |
| 7e | Targets, instances, views, buffers: `RenderTarget/`, `RenderableInstance/`, `SceneRenderTarget`, `IntermediateRenderTarget`, `ViewMatrices*`, `Frustum`, `Types`, `SharedUBO*`, `BindlessTextureManager`, `VertexBuffer*`, `FramebufferPrecisions`, `SkinnedGeometryProcessor`, `Selection*`, `PathDebugOverlay` | ~22 000 | ✅ pushed engine `11e574d8`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 7f | Post-process: `PostProcessor`, `PostProcessStack` (+ console), `IndirectPostProcessEffect`, `GrabPass`, `CombinePass`, `DenoisePass`, `GIDenoiser`, `OverflowCensus`, `Effects/` Shared, Resolve, Camera, Style | ~22 000 | ✅ pushed engine `cfcb91b7`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 7g | Lighting and atmosphere: `Effects/` Lighting, Atmosphere, `IrradianceProbeVolume`, `LTC*`, `Dummy*`, `CloudShadowMap`, `OceanWaves`, `ImposterAtlas`, `Compute/` | ~22 000 | ✅ pushed engine `712bf0e7`; VALIDATED macOS M2 + Windows NVIDIA + AMD |

Leads carried: 7a `CubemapResource` `CubemapFaceNames.at(faceIndex)` (section 2, done); 7c (done) `BasicGroundResource` passes
`DefaultGeometryFlags` as the grid's UV multiplier and calls `setLoadSuccess()` without `beginLoading()` (6c); 7d (done) the
`screenshot()` "did not complete in time" wording while an asset is still uploading (6a). Outside section 7: base
`Animation/AnimationChannel.hpp:277` `-Wfloat-conversion` (6d), `SoundfontResource` unconfined `file` path (section 10).

### 7a — resources read from disk (started 2026-10-01)
- [x] (1) clang-tidy 21.1.6 baseline (21 TUs, the 7a files only): **171**: 71 parentheses, 28 constant-array-index, 17
  designated-initializers, 17 reinterpret-cast, 7 init-variables, 7 special-member-functions, 6 isolate-declaration,
  4 use-anonymous-namespace, 3 C arrays, 2 implicit-widening, 2 qualified-auto, 2 convert-to-static, and singles incl.
  a clang-analyzer `core.BitwiseShift` in `KTX2Decoder` (a shift by 4294967295).
- [x] (2) Review (trust boundary: files on disk). Findings:
  - K1 `KTX2Decoder`: `firstFittingLevel()` answers `numLevels - 1` = 4294967295 for a 0-level texture, then shifted by
    it; libktx forces ≥ 1 level but checks the count against the size with `1 << (levelCount - 1)`, itself UNDEFINED past
    32 (`lib/checkheader.c`): a hostile header can pass it and the level walks shift by up to the count.
  - K2 `TextureCache::tryLoad()`: a level's `dataSize` from the cache FILE is trusted: `resize()` up to 4 GiB × 20
    levels, and a size that does not match the dimensions hands the upload a short buffer. The cache is a file on disk
    (a truncated write, a disk error).
  - K3 `MovieResource` / `CubemapMovieResource::extractCountWidth()`: `std::stoul()` on the JSON pattern `{…}` — throws
    (abort) on "{abc}" or an out-of-range number; a huge width pads every frame name to gigabytes.
  - K4 `MovieResource::loadParametric()`: `FrameCount` from JSON is unbounded, and a missing frame image silently gets
    the store's DEFAULT image (a copy per frame): 4e9 frames = an out-of-memory abort (owner question).
  - K5 `TextureCompressor`: `srcY * stride` in 32 bits wraps from a 32k × 32k source (an out-of-bounds read).
  - K6 (cascade census of `std::sto*`, the section-2 lead): 4 sites — the two movie parsers (K3), `Console::Controller`
    (port) and base `HTTPSClient` (Content-Length), both pre-validated (digits only, bounded length).
  - K7 PRE-EXISTING, found by the hostile tests, not a 7a file: **a glTF with ONE unreadable image leaks GPU objects to
    shutdown** — a corrupt JPEG in a copy of DamagedHelmet (or a KTX2 libktx refuses): 20 `vkDestroyDevice-05137`,
    VMA "Some allocations were not freed", "device smart pointer still have 14 uses". The same asset intact: 0. The
    failed-dependency path of the resource chain (section 2 / 3 territory) — owner question.
  - Checked, sound: `CloudShapeResource` clamps every parameter; `VolumetricImageResource` has no file loader;
    `VideoFrameConverter` refuses zero / odd dimensions; the cubemap face loops are bounded (the `.at()` lead: replaced
    by `[]`).
- [x] (3) Mechanical + the evident refusals (2026-10-01): K1 a container with 0 or > 32 levels is refused (and
  `firstFittingLevel()` guards 0 itself); K2 a level whose size is not the BC7 size of its dimensions, or more than the
  file holds, is a cache miss (a warning); K3 `std::from_chars`, a width of 1 to 10 digits, else the existing
  "Invalid basename"; K5 the offsets in `size_t`; K6 `Controller` and `HTTPSClient` on `std::from_chars` too — 0
  `std::sto*` left in the cascade. clang-tidy fix-its (parentheses, designated initializers, isolated declarations,
  qualified auto) with **`--format-style=none`** (without it clang-tidy reformats the lines it touches), the
  designated-initializer spacing normalized, float temporaries initialized to 0 (the fix-it's `NAN` + `<math.h>`
  undone), `std::array` for the C arrays, copy / move deleted on the 6 texture classes and `CloudShapeResource`, the
  KTX2 helpers and `testPatternBGR` in anonymous namespaces, the `KTX2Decoder` deleted constructor public, an
  unambiguous `m_descriptorPool = nullptr`, `*` instead of `.value()`. Builds clean.
  Hostile tests (Linux): 18 texture-cache entries corrupted (`dataSize` 0xFFFFFFF0) → 17 "ignored" warnings, the
  textures recompressed, no abort, 0 VUID; a 40-level KTX2 → refused (by libktx itself here), engine alive.
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - K4 a movie frame missing from the store REFUSES the movie (`isResourceExists()` before `getResource()`, for the
    parametric pattern AND the explicit frame list, `MovieResource` and `CubemapMovieResource`).
  - K7 investigated now. ROOT CAUSE: no failure path told the parents nor released the strong child ↔ parent links, and
    `Container::getOrCreateResource()`'s async creation left a resource whose function returned false `Unloaded`
    forever. Ruling: a failure always propagates and releases every link; each parent answers the new
    `ResourceTrait::onDependencyFailed()` hook (default: fail in turn); the six texture types take their type's default
    data (`TextureResource::Abstract::takeDefaultData()`). Also: `addDependency()` of an already-failed child is handled
    as a failure, its own failing refusals release their links, the file-parse failures too, and the container fails an
    unfinished manual creation (async and sync). Contract: `docs/subsystems/resources/04-development-patterns.md`;
    caution-points § Resources.
- [x] (4) Verified 2026-10-01 (Linux): cascade builds (0 warning); base 2170/2170 Release AND ASan/UBSan (the
  `HTTPSClient` change); clangcheck 123 TUs 0, `-Wfloat-conversion` 0; clang-tidy 171 → 72 (on purpose, ledger).
  Broken assets: DamagedHelmet with a corrupt JPEG and the KTX2 lamp with a 40-level texture now RENDER (the default
  texture in place, a "goes on without its failed dependency" warning) and leak nothing (was: nothing rendered, 20
  VUIDs, VMA asserts); a corrupted texture cache → 17 entries ignored, recompressed. Regression: citadel (MCP 1707/0,
  console 4466/0, log classes = the 6b run), terrain, beams, lighten-marbles, the INTACT helmet / lamp / Fox: 0 VUID,
  0 VMA, 0 error, 0 "goes on without", 0 "fails: its dependency".
- [x] (5) Pushed 2026-10-01: base `10e23f3`, engine `4e4bda64`; peers asked (the broken helmet / lamp, a corrupted
  texture cache in a dedicated `--cache-directory`, regression demos).
  - Windows peer: 4e4bda64 did NOT compile on MSVC — two `const auto *const pairIt = std::ranges::find_if(…)` on a
    `std::array` in `KTX2Decoder.cpp` (clang-tidy's qualified-auto fix-it; MSVC's array iterator is a class). Fixed and
    pushed as engine `734b4422` (owner's order). Caution-points § Build / Compiler.
  - VALIDATED 2026-10-01 at engine `1ee4a6c6` (= `734b4422` + 7b), base `10e23f3`, alpha `47128cce`, every launch on a
    scratch `--cache-directory`. macOS M2 (AppleClang 0 warning; base 2170 = 2167 + 3 skipped) and Windows (MSVC /W4 /WX
    0 warning; NVIDIA RTX 3060 Laptop AND the forced AMD iGPU, same results): the broken helmet and lamp RENDER with
    the default texture and one "goes on without its failed dependency" warning each, shutdown 0 VUID / 0 VMA / 0
    "still have N uses"; every corrupted `.bc7cache` entry the run looks up is "ignored" (22 on both) and the helmet
    renders with its real albedo; citadel MCP 1707/0, terrain, lighten-marbles, beams, intact helmet + Fox: 0 VUID (on
    Windows NVIDIA only the known 12325 of citadel), 0 error, exit 0.
  - Windows noticed: `console-conformance` fails ONE flood / over-long-line check in 3 of 6 runs (the refusal line is
    lost, the client reads a reset). Not 7a: engine item `docs/todo/console-last-refusal-lost-on-windows.md`.

### 7b — `Material/` (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (8 TUs): **56**: 23 constant-array-index, 20 parentheses, 7 static-cast-downcast,
  and singles: an unused forward declaration, use-enum-class, an unused parameter, a C array, a dead store, a
  no-automatic-move.
- [x] (2) Review. The trust boundary (the JSON definitions of `StandardResource`, `BeamResource`, `PathResource`)
  reads every key through `FastJSON::getValue` (the 6c migration: a wrong type or a non-finite value is absent, the
  default applies), and the setters it feeds clamp their ranges (`setIOR` 1-3, roughness / metalness / opacity /
  factors 0-1, distances and strengths ≥ 0); the `set…Component(float)` creators delegate to those setters; a normal /
  height scale may be any finite value (glTF allows a negative one). The material helpers were hardened in 6c. No
  defect at the boundary; no owner question.
- [x] (3) Mechanical (2026-10-01): parentheses (fix-its, `--format-style=none`), the dead `Transmission` lookup
  removed, the albedo expression returned by move, the unused descriptor-set parameter named in a comment, the RT
  texture mapping table a `std::array`, the unused `Resources::Manager` forward declaration of `Component/Texture.hpp`
  removed.
- [x] (4) Verified 2026-10-01 (Linux): builds clean, clangcheck 107 TUs 0, `-Wfloat-conversion` 0; clang-tidy 56 → 31
  (on purpose, ledger). One launch: citadel (its material library), then the helmet, the KTX2 lamp and the Fox dropped
  after deleting it, then beams (beam / path materials): 0 VUID, 0 error, 0 VMA.
- [x] (5) Pushed 2026-10-01 as engine `1ee4a6c6`; VALIDATED with 7a (macOS M2, Windows NVIDIA + AMD, above).

### 7c — `Geometry/`, `Renderable/`, `MDI/` (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (the 7c TUs): **95**: 30 constant-array-index, 13 use-scoped-lock, 5
  parentheses, 4 each of implicit-widening, integer-division, unchecked-optional-access, C arrays and protected
  members, 3 each of designated-initializers, std-numbers, use-enum-class and qualified-auto, a virtual call from a
  destructor (`VertexGridResource`), and singles.
- [x] (2) Review (trust boundary: the ground / terrain / grid JSON of the `Grounds` store, the glTF meshes). Findings:
  - G1 no grid size had an upper bound, and base `Grid::pointCount()` = (division + 1)² is computed in the index type:
    a division above 65534 wrapped the point count (65535 → 0), and `UINT32_MAX` + 1 wrapped to 0 (owner question).
  - G2 `BasicGroundResource::load(json)` passed `DefaultGeometryFlags` (15) as the grid's UV MULTIPLIER and built its
    `VertexGridResource` without those flags (the 7c lead; owner question).
  - G3 `setLoadSuccess(false)` before `beginLoading()` (`BasicGroundResource` × 3, `TerrainResource::load(path)`, the
    head of `MovieResource::load(frames)`): a refused load stayed `Unloaded` — `failLoading()` now.
  - G4 `meshIndex.value()` (`MeshResource`, `MultiLayerMeshResource`) and `enum_cast(...).value()`
    (`TerrainResource`, the height-map modes): throwing `std::optional::value()` on data.
- [x] (3) Mechanical (2026-10-01): fix-its with `--format-style=none` (scoped_lock, parentheses, `std::numbers`,
  `std::min` / `std::max`, redundant casts and member initializers, `.data()`, designated initializers; the spacing
  normalized), `std::array` for the C arrays of `ResourceGenerator`, the 64-bit index products of `CDLODTerrainResource`
  and `VertexGridResource`, the destructor's call qualified (`VertexGridResource::destroyFromHardware(true)`),
  `std::ranges::any_of` in `Renderable::Abstract`, G3, G4 (a mesh node without a usable mesh is refused with an error;
  an unknown height-map mode is `Replace`), the two `isCreated()` De Morgan forms simplified.
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - G1 base `Grid::initializeByCellSize()` / `initializeByGridSize()` refuse a count above the new
    `Grid::MaxCellCount` (65534 for `uint32_t`, 254 for `uint16_t`) and a non-finite size (base unit tests
    `ACellCountWhosePointCountOverflowsTheIndexTypeIsRefused`, `ANonFiniteSizeIsRefused`: failing before, passing
    after). The engine JSON loaders refuse above named caps: `VertexGridResource::MaxGridDivision` 4096 (vertex grids
    and basic grounds), `TerrainResource::MaxGridDivision` 16384 and `TerrainResource::MaxClipTexels` 4096 (default
    2048). A scene definition's ground keeps its stricter 1024 (6c).
  - G2 the flags go to the constructor, as in the three other ground paths, and the UV multiplier is the default 1.0
    (the existing optional `UVMultiplier` key still sets it). No ground JSON exists in the data stores today.
- [x] (4) Verified 2026-10-01 (Linux): cascade builds (0 warning); base 2172 = 2169 + 3 skipped, Release AND
  ASan/UBSan; clangcheck 118 TUs 0, `-Wfloat-conversion` 0; clang-tidy 95 → 51 (on purpose, ledger). A scratch
  `--add-data-directory` with `Grounds/` JSON loaded through `loadResource()`: a 64-division basic ground and a
  512-division terrain → `Loaded`; a 5000-division ground, a 20000-division terrain and an 8192-texel clip → `Failed`
  with the cap named; a single-mesh `Box.glb` in `Meshes/` → `Loaded` as both `SimpleMeshResource` and `MeshResource`.
  terrain and citadel: 0 VUID, 0 error, 0 leak, exit 0.
- [x] (5) Pushed 2026-10-01 (owner's order): base `dce53a7`, engine `b1541d1c`. VALIDATED the same day: macOS M2
  (AppleClang 0 warning) and Windows (MSVC /W4 /WX 0 warning; NVIDIA RTX 3060 Laptop AND the forced AMD iGPU): base
  2172 = 2169 + 3 skipped; the five `Grounds/` JSON and `Box.glb` give exactly the Linux statuses and errors (3 refusals,
  nothing else), shutdown 0 VUID / 0 VMA / 0 "still have N uses"; terrain and citadel (MCP 1707/0) 0 VUID (Windows
  NVIDIA: only the known 12325 of citadel), exit 0.

### 7d — renderer and frame (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (6 TUs, the 7d files only): **226**: 39 implicit-widening (the BGRA→I420 row
  offsets), 39 parentheses, 28 SIMD intrinsics, 26 reinterpret-cast, 21 constant-array-index, 14 array-to-pointer
  decay, 10 anonymous-namespace, 8 isolate-declaration, 5 each of owning-memory (`FILE *`), ambiguous smart-pointer
  reset, union access (libvpx packets) and convert-to-static, and singles incl. a narrowing, a use-after-move.
- [x] (2) Review (trust boundaries: the console commands, the RushMaker settings, the pipeline-cache file, the
  output files). Findings:
  - R1 `Recorder`: no `fwrite()` result was checked (headers, packets, flush): a full disk or a removed drive gave a
    silently truncated video while encoding went on, ending "finalized: N frames written" (owner question).
  - R2 RushMaker settings unbounded: `MaxQueuedFrames` (W×H×4 bytes per frame, 10 000 = an out-of-memory abort) and
    `VideoFramerate`; `maxQueuedFrames - 2` wrapped for a value of 1 (owner question for the range).
  - R3 console `triggerRenderDocCapture(frameCount)` up to INT32_MAX (owner question); `getGPUTimings` padding wrapped
    past 20 nesting levels.
  - R4 the output `FILE *` were owning raw pointers (a path closed none of them: none today, but no RAII).
  - Checked, sound: `screenshot` / `temporalCapture` (bounded by `FrameCapture::MaxCaptureBytes`, unique stems), the
    pipeline cache (header, device, driver and hash checked, the crash marker), the frame scopes, the use-after-move
    (`m_sceneTarget` retired then tested; the dispatch requires it non-null: a false positive).
  - R5 (the 6a lead) a capture whose frames never come answered "is the window rendering?" while the window did render
    and a load held the frames back: the message now says "No frame was presented within N ms … the window is not
    rendering, or a load (an asset upload, a scene still building) is holding the frames back" (wording only; not
    reproduced here, the peers saw it on macOS).
  - Lead (cascade-wide, section 2): `std::thread` construction (Recorder ×2, 8 more sites) is a throwing std call.
- [x] (3) Mechanical: fix-its with `--format-style=none` (parentheses, isolated declarations, designated initializers,
  smart-pointer resets, parameter names), the BGRA→I420 row / column indices and strides in `size_t` (the three
  variants byte-identical before / after: scalar, SSE4.1, AVX2 × 8 sizes 2×2 to 7680×4320, a standalone harness),
  the SIMD scratch integers initialized, the helpers and `hashSamplerCreateInfo()` in anonymous namespaces, C arrays →
  `std::array`, the encoder-name ternary unnested, a redundant cast, the timebase cast, a `MiB` constant, an unused
  forward declaration removed, `RendererFrameScope` copy / move deleted explicitly, R3's padding clamped.
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - R1 the first failed write (or final `fclose()`) is traced with the OS reason and latched; the recorder STOPS the
    recording at its next capture; the session ends "TRUNCATED"; the output files are `std::unique_ptr< FILE >`.
    Found while testing: with audio ON the video's own stop left the rush's audio running and the toggle could never
    stop it (it tried to START a rush, refused by the "audio still active" guard): `Core::rushRecording()` (video OR
    audio OR voice-over) is now what the toggle and Shift+Ctrl+F12 test.
  - R2 `MaxQueuedFrames` 3-240, `VideoFramerate` 1-240, outside → warning + default (as 0 already did).
  - R3 `triggerRenderDocCapture()` refuses outside 1-100.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 112 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 226 → 91 (on purpose, ledger), 0 new on the touched `Core` lines. RushMaker
  hardware H.265 and VP9, 6 s each: 181 frames, decoded by ffprobe; with `RLIMIT_FSIZE` 4 MiB / 2 MiB (SIGXFSZ ignored)
  "Unable to write … ! The recording stops", stopped at the next capture, TRUNCATED, the files decode (66 / 64 frames);
  audio ON: the next toggle saved the WAV; settings 1000 FPS / 1 frame → warnings and the defaults. citadel:
  `triggerRenderDocCapture(101)` and `(0)` refused, screenshot + `temporalCapture(3)` saved, MCP 1707/0, console
  4466/0, 0 VUID, 0 leak; `getGPUTimings` with the profiler ON prints its table. Every test file was moved out of the
  owner's captures directory (unchanged). One unrelated crash: the known CEF MemoryInfra SIGILL (alpha item updated).
- [x] (5) Pushed 2026-10-01 (owner's order): engine `9cc3659d`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning, the scalar converter on arm64): the range warnings word for word; a VP9 rush;
    `ulimit -f 4096` + `trap '' XFSZ` → BOTH failure paths seen (a session still encoding in background after the stop,
    and the live auto-stop), engine alive, 0 VUID; the RenderDoc refusals; MCP 1707/0, console 4445/0.
  - macOS found (PRE-EXISTING, reproduced on Linux): the VP9 session reported and wrote "196 frames" in the IVF header
    for a file of 181 — `frameCount` counted the images handed to the encoder AND, again, the packets of the final
    flush (the 15 frames of the lookahead). Fixed (pushed with 7e): `EncodingSession::writtenFrames` counts the frames
    actually written (the header's count and the final message); `frameCount` stays the statistics' input count.
    Linux: log 181, header 181, ffprobe 181.
  - Windows PASS on NVIDIA RTX 3060 Laptop AND the forced AMD iGPU (MSVC /W4 /WX 0 warning, no C4267 from the size_t
    indices): the range warnings; NVIDIA H.265 181 pictures, VP9 on both GPUs (the same 196 / 181 header defect, fixed
    above); the RenderDoc refusals, screenshot + temporal capture, MCP 1707/0, console 4457/0; citadel NVIDIA only the
    known 12325, AMD 0. The write-failure test was skipped (no small volume); Linux and macOS cover it.
  - Windows found (PRE-EXISTING, not 7d): the AMD hardware H.265 rush works but raises 55 VUIDs (copies and transfer
    barriers recorded on AMD's encode-only queue family, the padded 1280x768 coded extent) → engine item
    `video-encoder-h265-transfer-on-encode-only-queue`. "IDR every 1 frames" is the intended all-intra layout.

### 7e — targets, instances, views, buffers (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (25 TUs, the 7e files and their headers): **205**: 80 constant-array-index, 38
  use-scoped-lock, 16 reinterpret-cast, 11 qualified-auto, 9 parentheses, 8 implicit-widening, 5 each of
  special-member-functions and nested conditionals, 4 each of unnecessary copies, redundant boolean literals and
  inconsistent parameter names, and singles.
- [x] (2) Review. These files are the renderer's internals: their inputs come from the components (6d, finite
  values), the console (validated there) and the renderer. Sound: the bindless table refuses an index past its capacity,
  the shared UBO banks are reserved once, a view size <= 0 is refused. Findings:
  - V1 a view distance of 0 reached the projection (far plane 0, below near): the `Core/Graphics/ViewDistance` setting
    (the toolkit's cameras) and `Camera::setPerspectiveProjection` / `setDistance` (`>= 0` accepted) / `setFar`
    (clamped UP to 0) (owner question).
  - V2 `SharedUBOManager::createSharedUniformBuffer()` (no descriptor-set creator) took a `frameCount` and dropped it: a
    caller asking for per-frame regions would get one shared by every frame in flight (no caller today).
  - V3 every render-target constructor takes a `viewDistance` nothing reads (the base ignores it, each subclass only
    forwards it): an API cleanup across every construction site, not mechanical → engine item
    `render-target-dead-view-distance-parameter`.
- [x] (3) Mechanical: fix-its with `--format-style=none` (scoped_lock, parentheses, designated initializers — spacing
  normalized —, ranges, smart-pointer resets, the `if constexpr` boolean returns, references instead of copies,
  parameter names, member initializers, a const vector), the four duplicated nested `stageFlags` conditionals → one
  `perVertexPushConstantStages()`, the skinning `tbnMode` unnested, the 64-bit instance / cascade offsets, `const`
  references (UBO, transfer manager), copy / move deleted explicitly with the engine's Doxygen block on the five
  flagged classes (and on 7d's `RendererFrameScope`), the cube-face `fov` parameter documented as ignored, V2 (the
  frame count passed through).
- [x] (3b) Owner ruling (2026-10-01), as recommended, APPLIED: V1 a camera distance / far <= 0 is ignored with a warning
  (the previous one kept, as `setNear` already did); the setting outside (0, 1 000 000] m warns and takes the default
  (`MaxGraphicsViewDistance`, `Toolkit::viewDistanceSetting()`). Doc: `subsystems/scenes/02-scenes-specific-rules.md`.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 124 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 205 → 109 (on purpose, ledger). `ViewDistance = 0` in a settings copy: sponza's
  three toolkit cameras warn "'Core/Graphics/ViewDistance' = 0 is outside (0, 1000000] m ! Using 10000 m." and render;
  the selection outline (light-and-shadow-debug, `highlightEntity(SmoothMesh4)`): 1 857 pixels change around the
  sphere vs 76 of noise between two un-highlighted shots, the orange outline on the image; citadel MCP 1707/0, console
  4466/0; terrain, particles, beams: 0 VUID, 0 error, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order, with the 7d frame-count fix): engine `11e574d8`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning): VP9 log = IVF header = frame records = 181 (the 7d fix holds); ViewDistance 0
    → exactly 3 warnings on sponza, the scene renders; the outline on `SmoothMesh4`; citadel MCP 1707/0, console 4445/0,
    the 7d console checks again; terrain, particles: 0 VUID, 0 UNASSIGNED, 0 error.
  - macOS noticed the first VP9 pts was 1 (0 in its 7d run). Not 7e: a frame's pts IS its CFR slot on the wall clock
    since the start (`Recorder::cfrSlotAt()`), and nothing fills the slots BEFORE the first capture; a first capture
    landing more than 1/30 s after the toggle leaves slot 0 empty. Timing-dependent, pre-existing, and it keeps the
    video aligned on the audio track, which starts at the toggle (a start_time of 1/30 s in ffprobe).
  - Windows PASS on NVIDIA RTX 3060 Laptop AND the forced AMD iGPU (MSVC /W4 /WX 0 warning): VP9 181 / 181 / 181;
    ViewDistance 0 → 3 warnings on sponza, renders; the outline on both GPUs; citadel MCP 1707/0 on both, console 4457/0
    on AMD and 4456/1 on NVIDIA (the known flaky flood check, item `console-last-refusal-lost-on-windows`); NVIDIA only
    the known 12325; terrain, particles 0 VUID.

### 7f — post-process (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (36 TUs, the 7f files and their headers): **192**: 131 constant-array-index, 16
  static definitions in anonymous namespaces, 9 return-const-ref-from-parameter, 7 use-scoped-lock, 6 designated
  initializers, and singles.
- [x] (2) Review (trust boundaries: the `PostProcess` console commands, the effects' quality settings). The console
  commands validate their slot / effect / lane names and say why they refuse. Findings:
  - P1 the effect quality settings had no range: a huge `DepthOfField/SampleCount` or `MotionBlur/SampleCount` or
    `Clouds/StepCount` runs the shader into the GPU timeout (device lost), a huge `DepthOfField/MaxRadius` overflowed its
    `int32_t` cast (UB), a TAA alpha outside (0, 1] (owner question).
  - P2 the "outside the range → warning + default" check was already written inline three times (7d RushMaker ×2, 7e
    ViewDistance): where should it live (owner question).
  - P3 `GIDenoiser::updateFrameData()` took a `FrameContext` it never read (SSGI, RTGI, RTR passed it).
  - Checked, sound: the nine `return-const-ref-from-parameter` are the chain's pass-through contract (an effect with
    nothing to do returns its INPUT texture, owned by a render target, never a temporary).
  - Lead for 7g: `VolumetricLight` reads its override keys (`SampleCount`…) with `settings.get()`, unbounded.
- [x] (3) Mechanical: fix-its with `--format-style=none` (`static` dropped inside anonymous namespaces, scoped_lock,
  designated initializers — spacing normalized —, parentheses, redundant member initializers — `Vector`'s `m_data{}`
  and `time_point` zero them —, isolated declarations initialized, a range-for, a smart-pointer reset), P3 (the unused
  parameter removed with its three callers), the `getStatus` nested state conditional unnested, `CounterIndex` on
  `uint8_t`, copy / move deleted on `CombinePass` and `DenoisePass`.
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - P2 `Settings::getOrSetDefaultInRange< T >(key, default, minimum, maximum, minimumExclusive = false)`: a NaN, an
    infinity or a value outside the range warns once (`'<key>' = <value> is outside [min, max] ! Using <default>.`, an
    integral float printed without its exponent) and returns the default; the 7d / 7e inline checks moved to it.
  - P1 the nine keys bounded through it, the ranges as `Min…` / `Max…` constants beside the defaults: TAA Alpha (0, 1],
    VarianceGamma (0, 10]; MotionBlur SampleCount [1, 128], SoftDepthExtent (0, 10]; Clouds StepCount [1, 512],
    LightStepCount [1, 64]; DepthOfField SampleCount [1, 256], MaxRadius [1, 128], AutoFocusSpeed (0, 100].
    Docs: engine caution-points § Settings trust boundary, `13-12-post-processing-effects/09-available-effects.md`.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 123 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 192 → 145 (on purpose, ledger). All twelve range keys out of range in one
  settings copy: sponza → the ten DoF / MotionBlur / TAA / ViewDistance (×3) / RushMaker warnings, terrain → the two
  Clouds ones; both render, 0 VUID. Sponza with TAA + DoF + motion blur: `testOverflowCensus()` PASS (the `uint8_t`
  counters), `getStatus` complete, the frame against the pre-fix-it run: mean |diff| 0.05 level, 738 of 4.67 M pixels
  over 24 (TAA noise); MCP 1707/0. citadel ScreenSpace ↔ RayTracing lane switches (the GI denoiser of SSGI / RTGI /
  RTR), console 4466/0, 0 VUID, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `cfcb91b7`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning): sponza 12 warning lines (10 keys, ViewDistance ×3, the RushMaker pair now
    tagged `SettingsService`), terrain the 2 Clouds lines plus TAA ×2 and RushMaker ×2 (both enabled in that copy too);
    both render, no hang; `testOverflowCensus()` PASS, `getStatus` complete (ScreenSpace only); citadel RayTracing
    refused as expected on MoltenVK, MCP 1707/0, console 4445/0; 0 VUID, 0 UNASSIGNED everywhere.
  - Windows PASS on NVIDIA RTX 3060 Laptop AND the forced AMD iGPU (MSVC /W4 /WX 0 warning): the same 12 + 6 warning
    lines on both GPUs (the ViewDistance one now prints no "m": the generic Settings wording), renders, no hang; the
    census self-test PASS on both; the AMD iGPU HAS the ray-traced lane (RTGI / RTR / RTAO resident); citadel lane
    switches on both, MCP 1707/0; console 4454/2 (NVIDIA) and 4455/1 (AMD): only the known flaky RST checks (item
    `console-last-refusal-lost-on-windows`); NVIDIA only the known 12325.

### 7g — lighting and atmosphere (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (23 TUs, the 7g files and their headers): **184**: 45 constant-array-index, 23
  parentheses, 17 C arrays, 17 designated initializers, 12 nested conditionals, 12 const_cast, 10
  return-const-ref-from-parameter, 7 each of smart-pointer resets and qualified-auto, 6 static in anonymous namespaces,
  5 redundant boolean literals, 4 implicit-widening, 3 unmatched `NOLINTEND`, and singles.
- [x] (2) Review (trust boundary: the ~70 numeric settings the lighting effects read). Findings:
  - L1 the counts and sizes (samples, steps, iterations, accumulations, blur radii, the probe grid and its rays) were
    unbounded: a huge one hangs the GPU or allocates without limit (owner question).
  - L2 `denoiseContribution() const` handed the denoise pass MUTABLE pointers to the effect's own targets: 12
    `const_cast` (the six effects).
  - L3 the pixel-doubling extent `pixelDoubling ? ((width > 1) ? width / 2 : 1U) : width` copied ten times (RTAO,
    RTContactShadows, RTGI, RTR, SSR).
  - Checked, sound: the ten return-const-ref are the chain's pass-through contract (7f); the probe volume's constant
    seed is its reproducible rotation sequence; its two integer divisions centre the grid on whole cells.
- [x] (3) Mechanical: fix-its with `--format-style=none` (parentheses, designated initializers — spacing
  normalized —, smart-pointer resets, boolean returns, a range-for, `std::max`, an explicit bool test, `static` dropped
  in anonymous namespaces); the three orphan `NOLINTEND` removed; L2 the hook made non-const (the caller holds non-const
  effects) and the const_casts removed; L3 one `IndirectPostProcessEffect::traceSize()`; the push-constant / UBO / GPU
  C arrays → `std::array` (same layout, the size `static_assert`s hold), the XRay corner ternaries → a corner table, the
  descriptor writes a `std::array`; 64-bit grid / cascade offsets; `sqrt1` / `sqrt2` renamed (confusable with
  `sqrtl`); `std::numbers::sqrt3_v`; two `const` locals; copy / move deleted on `IrradianceProbeVolume`.
- [x] (3b) Owner ruling (2026-10-01), as recommended, APPLIED: L1 the eighteen keys (22 read sites) bounded through
  `getOrSetDefaultInRange()` with `Min…` / `Max…` constants; `VolumetricLight/SampleCount` through the new
  `Settings::getInRange()` sibling (an override key: the demo's value is kept). Docs: caution-points § Settings trust
  boundary, `09-available-effects.md`.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 122 TUs 0, `-Wfloat-conversion` 0;
  clang-tidy 184 → 69 (on purpose, ledger). The eighteen keys out of range: citadel RayTracing lane → the eleven RT /
  shared / probe warnings, ScreenSpace lane → the ten SS / shared ones, light-and-shadow-debug → "VolumetricLight/
  SampleCount = 100000 … Using 64" (the demo's value); 0 VUID. Regression: geometry-generator XRay (option 2: 1000
  slices, 8.8 ms each, 0 VUID), water-world (ocean waves), terrain (clouds), citadel MCP 1707/0, console 4466/0, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `712bf0e7`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning; ScreenSpace lane only): citadel the 10 SS / shared warnings,
    light-and-shadow-debug the same 10 + "VolumetricLight/SampleCount … Using 64"; geometry-generator XRay 1000 slices
    (56.7 ms each), water-world, terrain, citadel MCP 1707/0, console 4445/0; 0 VUID, 0 UNASSIGNED everywhere.
  - macOS noticed (PRE-EXISTING, projet-alpha): the XRay demo traces "1000 slices saved" while its write block is
    commented out → alpha item `geometry-generator-xray-claims-a-save`.
  - Windows PASS on NVIDIA RTX 3060 Laptop AND the forced AMD iGPU (MSVC /W4 /WX 0 warning): citadel RayTracing lane
    exactly the 11 lines, ScreenSpace lane exactly the 10, light-and-shadow-debug the VolumetricLight one ("Using 64"),
    on both GPUs; renders, no hang. light-and-shadow-debug printed 22 lines on NVIDIA (both lanes' effects created) and
    12 on AMD (the RT set): which effects a device creates, every value refused correctly. XRay 20.1 ms / 74.2 ms per
    slice, water-world, terrain, citadel MCP 1707/0, console 4457/0 (NVIDIA) and 4454/2 (AMD, the known RST checks);
    NVIDIA only the known 12325.

## Section 8 — `src/Saphir` (started 2026-10-01), three sub-sections (owner, 2026-10-01)

| Sub | Content | Lines | Status |
|---|---|---|---|
| 8a | Shader core: `ShaderManager` (the on-disk shader cache: the trust boundary), `Program`, `AbstractShader`, `AbstractVertexStage`, the stage classes (vertex, fragment, geometry, tessellation, mesh, task, compute), `CodeGeneratorInterface`, `Types`, `SetIndexes` | ~9 000 | ✅ pushed engine `1f1004d4`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 8b | `LightGenerator` (+ `.PBR`, `.ShadowMap`) | ~5 000 | ✅ pushed engine `040d100f`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 8c | `Declaration/`, `Generator/` | ~13 000 | ✅ pushed engine `0709e6e3`; VALIDATED macOS M2 + Windows NVIDIA + AMD |

### 8a — shader core (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (15 TUs, the 8a files and their headers): **40**: 21 qualified-auto, 9 designated
  initializers, and singles (any_of, a nested conditional, a dead store, an unnecessary copy, an anonymous-namespace
  function, special members, recursion, an unscoped enum).
- [x] (2) Review. The trust boundary is the on-disk SPIR-V cache (`ShaderManager::checkBinaryFromCache()`): every
  header field (magic, format version, source hash, stage, size, toolchain hash, data hash), a size multiple of 4 and
  the SPIR-V magic word are checked before a byte reaches `vkCreateShaderModule`; a rejected file is erased. The rest
  generates GLSL from engine data. No defect, no owner question.
- [x] (3) Mechanical: designated initializers (the heightfield frame-override table, spacing normalized),
  `const auto * const` for the `const char *` expressions (NOT the `std::array` iterators of `FragmentShader`: MSVC's
  are classes, the 7a lesson), `preparationAlreadyDone()` through `std::ranges::any_of` (the commented-out version and
  its "check if right" TODO were right, removed), the rest-position conditional unnested, a reference instead of a
  copy, `toGLSLangShaderType()` in an anonymous namespace, `ShaderManager` copy / move deleted.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 110 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 40 → 7 (on purpose, ledger). The generated GLSL is unchanged by construction (the
  two text-generating edits produce the same strings); beams (path / beam ribbons), citadel (mesh shaders; MCP 1707/0,
  console 4466/0), terrain (heightfield fragment overrides), sponza: 0 shader compilation failure, 0 VUID, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `1f1004d4`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning, the mesh path compiled on arm64): each launch on an EMPTY `--cache-directory`
    so every shader went through the new code — beams 39, terrain 252, sponza 221, citadel 472 binaries compiled, 0
    compilation failure, 0 VUID, 0 UNASSIGNED; citadel MCP 1707/0, console 4445/0.

### 8b — `LightGenerator` (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (3 TUs + the header): **15**: 9 convert-member-functions-to-static (the light
  uniform-block accessors), and singles (a nested conditional, an anonymous-namespace function, a branch clone, an
  unused non-trivial local, a missed automatic move, a const local).
- [x] (2) Review. Generates the lighting GLSL from material and light descriptions built by the engine; no trust
  boundary, no defect beyond an unused `roughness` string in the thin-surface transmission branch (computed, never
  emitted). No owner question.
- [x] (3) Mechanical: the dead local removed, the light-block location count unnested, `insideShadowVolumeCondition()`
  in an anonymous namespace, `dielectricF0Expression()`'s local non-const so its return moves, `ambientFresnelDeclaration()`'s
  prefix `const`.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 109 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 15 → 10 (on purpose, ledger). light-and-shadow-debug (point, spot, directional
  shadows), beams (line lights), sponza (PBR), citadel: 0 shader compilation failure, 0 VUID, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `040d100f`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning): empty `--cache-directory` per launch — light-and-shadow-debug 188, beams 39,
    sponza 221, citadel 472 binaries compiled through the new generator, 0 compilation failure, 0 VUID, 0 UNASSIGNED;
    citadel MCP 1707/0, console 4445/0.
  - Windows PASS on NVIDIA RTX 3060 Laptop AND the forced AMD iGPU, 8b and 8c validated TOGETHER at `0709e6e3` (MSVC
    /W4 /WX 0 warning), binary cache ON per empty directory: light-and-shadow-debug 188, beams 39, sponza 220, terrain
    284 / 252, water-world 178 / 131, animation-debug 129, citadel 474 / 472 binaries; 0 compilation or PerModel
    failure; citadel MCP 1707/0, console 4457/0 / 4455/1 (the known RST check); NVIDIA only the known 12325.
  - Windows noticed: (a) file IO fails past MAX_PATH (a 142-character scratch cache directory pushed 118 binary paths
    to 273-282 characters; handled, traced) → emeraude-base item `windows-long-paths`; (b) on the RTX 3060 the terrain
    imposter bake is often unfinished at 100 s (0 to 20 atlases depending on the run; AMD always 20), pre-existing,
    nothing fails.
  - Windows PASS on NVIDIA RTX 3060 Laptop AND the forced AMD iGPU (MSVC /W4 /WX 0 warning): beams, citadel (MCP 1707/0;
    console 4455/1 NVIDIA, the known RST check; 4457/0 AMD), terrain, sponza: 0 compilation failure; on AMD with the
    binary cache ON per empty directory: 39 / 472 / 252 / 220 binaries; NVIDIA only the known 12325. (The shader caches do
    not live under `--cache-directory`: only CEF, downloads, the pipeline and texture caches do.)

### 8c — `Declaration/`, `Generator/` (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (31 TUs + headers): **23**: 7 reinterpret-cast, 4 qualified-auto, 4 parentheses,
  2 branch clones, 2 nested conditionals, and singles (a non-const global, an anonymous-namespace function, an unscoped
  enum).
- [x] (2) Review. The GLSL declarations and the program generators work from engine data; no trust boundary, no
  defect. One duplication: the PerModel descriptor-set-layout choice (ocean, else heightfield surface, else skinning)
  written as the same nested conditional in `SceneRendering` and `ShadowCasting`.
- [x] (3) Mechanical: one `Generator::Abstract::perModelDescriptorSetLayout()` for both generators, the shader-block
  tracer tag a `constexpr` in an anonymous namespace, `declareGizmoPushConstantBlock()` in an anonymous namespace,
  `const auto * const` on four raw pointers, parentheses.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 112 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 23 → 10 (on purpose, ledger). terrain (heightfield PerModel set), water-world
  (ocean), animation-debug (skinning), citadel (shadow casting; MCP 1707/0, console 4466/0): 0 shader compilation or
  layout failure, 0 VUID, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `0709e6e3`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning): empty cache per launch — terrain 252, water-world 131, animation-debug 129,
    citadel 472 binaries, 0 compilation or PerModel-layout failure, 0 VUID, 0 UNASSIGNED; citadel MCP 1707/0, console
    4445/0.

## Section 9 — `src/Vulkan` (started 2026-10-01), three sub-sections (owner, 2026-10-01)

| Sub | Content | Lines | Status |
|---|---|---|---|
| 9a | Instance, device, presentation: `Instance`, `DebugMessenger`, `PhysicalDevice`, `Device`, `DeviceQueueConfiguration`, `DeviceRequirements`, `Queue`, `Surface`, `SwapChain`, `Utility` (trust boundary: the settings — GPU choice, layers, present modes — and what the driver reports) | ~9 500 | ✅ pushed engine `fa06d2fd`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 9b | Memory and resources: `Buffer`, `DeviceMemory`, `MemoryRegion`, `Image`, `ImageView`, `Sampler`, `TextureInterface`, the transfer operations and `TransferManager`, the buffer objects, `AccelerationStructure` (+ builder), `VideoEncoderH265` | ~10 000 | ✅ pushed engine `03962da6`; VALIDATED macOS M2 + Windows NVIDIA + AMD (the dump step with alpha `ad00545f`) |
| 9c | Commands, pipelines, descriptors, sync: `CommandBuffer` / `CommandPool`, `ComputePipeline` / `GraphicsPipeline`, `PipelineLayout`, `RenderPass` / `RenderSubPass`, `Framebuffer`, the `Descriptor*` classes, `LayoutManager`, `ShaderModule`, `GPUProfiler`, `Sync/` | ~10 000 | ✅ pushed engine `1cbb475d`; VALIDATED macOS M2 + Windows NVIDIA + AMD |

### 9a — instance, device, presentation (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (10 TUs + headers): **59**: 12 array-to-pointer decay, 8 reinterpret-cast, 5 each
  of convert-to-static and scoped-lock, 4 constant-array-index, 3 each of implicit bool and const locals, and singles.
- [x] (2) Review (trust boundaries: the settings — validation layers, `ForceGPU`, auto-select mode, MSAA samples,
  present-mode switches — and what the driver reports). Sound: the requested layers are matched against the available
  ones, the MSAA count is clamped by `Device::checkMultisampleCount()`, the swap-chain extent is clamped to the surface
  capabilities, the frames exist before the render passes read them. Findings:
  - D1 every queue the driver reports for a used family was created and registered into a `StaticVector< Queue *, 16 >`
    (and its priorities into a `StaticVector< float, 16 >`): a family reporting 17+ queues would `abort()` at startup
    (NVIDIA's graphics family reports exactly 16) (owner question). The same for the family list
    (`StaticVector< VkQueueFamilyProperties2, 8 >`, filled with the driver's count).
  - D2 `Instance::getComputeDevice()` (the physics compute device) read `ForceGPU` and ignored it (owner question).
- [x] (3) Mechanical: fix-its with `--format-style=none` (scoped_lock, designated initializers, `std::max`, explicit bool
  tests, `VK_NULL_HANDLE` init, static calls not through `this`, `using`), two `std::ranges::any_of`, the debug-messenger
  function pointers through `reinterpret_cast`, `UUIDToString()` on a `std::span< const uint8_t, VK_UUID_SIZE >`, three
  `const` locals, an explicit `const char *`, a `StaticVector` pointer iterator qualified, `SwapChain` copy / move deleted.
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - D1 `DeviceQueueConfiguration::MaxQueuesPerFamily` (16): `addQueueFamilyToCreateInfo()` creates min(reported, 16)
    queues per family (an info line when it caps), the `StaticVector`s sized by that constant. Applied the same way to
    the family list: `PhysicalDevice::MaxQueueFamilies` (8), the first 8 read with a warning beyond (none reports more
    than six today).
  - D2 the compute device takes the forced GPU when it is compute-capable, else warns and selects by score.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 113 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 59 → 32 (on purpose, ledger). Queue families unchanged (graphics #0 ×16, transfer #1
  ×2, compute #2 ×8, video encode #4 ×1: no cap reached). Physics acceleration ON + `ForceGPU = "NVIDIA GeForce RTX 3070
  Ti"` → "Compute capable physical device '…' selected (FORCED)"; `ForceGPU = "NoSuchGPU"` → the warning and the score
  selection. That run exposed a PRE-EXISTING defect of the (off by default) physics acceleration — its transfer manager
  creates a command pool on family 0, absent from the compute device: `VUID-vkCreateCommandPool-queueFamilyIndex-01937`,
  the physics service fails, the compute device leaks — engine item `physics-acceleration-transfer-pool-on-missing-family`.
  Normal settings: beams, terrain, citadel (MCP 1707/0, console 4466/0): 0 VUID, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `fa06d2fd`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning): one queue family (#0, 1 queue); the forced / unknown compute GPU lines as
    expected; beams, terrain, citadel (MCP 1707/0, console 4445/0) 0 VUID, 0 UNASSIGNED.
  - macOS found (PRE-EXISTING): with physics acceleration ON the compute LOGICAL device fails on MoltenVK — created
    without `VK_KHR_portability_subset` (`VUID-VkDeviceCreateInfo-pProperties-04451`): `getComputeDevice()` passes no
    extension, the portability handling exists only for the graphics device → engine item
    `compute-device-missing-portability-subset`.
  - Windows PASS on NVIDIA RTX 3060 Laptop AND the forced AMD iGPU (MSVC /W4 /WX 0 warning): queue families NVIDIA
    #0 ×16 / #1 ×2 / #4 ×1, AMD #0 ×8 / #2 ×1 / #3 ×1, none capped; forced AMD → graphics AND compute "(FORCED)",
    "NoSuchGPU" → the warning and the score; the known physics-acceleration failure reproduces on both (family 0 asked by
    the transfer pool, the compute device created with 1 + 2 / 2 + 1), with its leak tail; beams, terrain, citadel (MCP
    1707/0 on both, console 4456/1 NVIDIA the known RST, 4457/0 AMD); NVIDIA only the known 12325. NVIDIA terrain shut
    down during a pending imposter bake again logged "No default page found !" + a CEF web-view teardown timeout
    (third time, pre-existing).

### 9b — memory and resources (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (16 TUs + headers): **116**: 24 reinterpret-cast, 21 smart-pointer resets, 15
  qualified-auto, 14 designated initializers, 12 scoped-lock, 5 each of missing `std::forward` and const locals, and
  singles (incl. two implicit-widening sizes).
- [x] (2) Review. Findings:
  - M1 `TransferManager::downloadImage()` sized its staging buffer from a format switch whose DEFAULT was "RGBA8, 4
    bytes": a 16-bit colour target (`R16G16B16A16_SFLOAT`, 8 bytes, reachable through the console's `dumpRenderTarget`)
    got a buffer half too small — measured with the old default: `VUID-vkCmdCopyImageToBuffer-pRegions-00183`, "8388608
    bytes … exceeds the VkBuffer total size of 6291456 bytes", and a corrupt PNG answered "dumped" (owner question).
  - M2 `Buffer` / `Image::createOnHardware()` validated nothing: a size 0, an empty extent or one past the device's
    limits went straight to the driver (VUIDs, undefined in Release) — an evident refusal (no valid creation exists).
  - M3 two 32-bit size products (`downloadImage()`'s bytes, `VertexBufferObject`'s), the five callables taken by `&&`
    and invoked without `std::forward`.
- [x] (3) Mechanical: fix-its with `--format-style=none` (scoped_lock, designated initializers — spacing normalized —,
  parentheses, smart-pointer resets, explicit bool tests, `using`), M2 (an error naming the size / extent / limits, then
  `false`), M3 (`size_t` products, `std::forward< function_t >` at the single call), raw `transferOperation` pointers
  qualified (NOT the `VkDevice` handles), the switch locals initialized, `alignUp()` in an anonymous namespace,
  `TransferManager` copy / move deleted.
- [x] (3b) Owner ruling (2026-10-01), as recommended, APPLIED: M1 an unknown format is REFUSED (an error naming it);
  the known list gained the 8-bit sRGB variants (`R8G8B8A8_SRGB`… — the old default happened to size them right, the
  first refusal test caught their absence).
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 114 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 116 → 48 (on purpose, ledger). offscreen-rendering: `dumpRenderTarget(Cubemap)`
  dumps the sRGB `SecurityCubemap`; `dumpRenderTarget(OffscreenRenderingCubemap)` (format 97) is refused, 0 VUID (the old
  default: the VUID above). terrain, water-world, animation-debug, citadel (RT acceleration structures; MCP 1707/0,
  console 4466/0), a hardware H.265 rush (121 frames): 0 VUID, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `03962da6`, base `94fc69d`; peers asked.
  - macOS M2 and Windows (NVIDIA + AMD): builds 0 warning; terrain, water-world, animation-debug, citadel (MCP 1707/0,
    console 4445/0 macOS, 4455/1 and 4456/1 Windows — the known RST checks) 0 VUID, 0 `VUID-…-pRegions-00183`; the
    Windows NVIDIA H.265 rush 121 pictures. Step 1 (the dumps) could NOT run on either: `offscreen-rendering` crashed at
    startup by itself (SIGSEGV 6/6 macOS, 0xc0000005 5/5 Windows) — a PRE-EXISTING projet-alpha use-after-scope: the "CCTV"
    material's async factory captured the block-local `dynamicTexture2D` BY REFERENCE (Linux survived by luck). Fixed in
    projet-alpha `ad00545f` (by value; `Marble::getMesh()`'s `&color` too, same pattern), Linux 3/3 runs: the sRGB dump, the format-97
    refusal, 0 VUID. The dumps are re-asked from the peers with that fix.
  - Re-test with alpha `ad00545f`: macOS M2 PASS — `offscreen-rendering` 3/3 clean launches; the sRGB cubemap face
    dumped (a valid 1024×1024 PNG), `OffscreenRenderingCubemap` refused ("format 97, which cannot be downloaded"), 0
    `VUID-…-pRegions-00183`, 0 VUID; `lighten-marbles` 0 VUID, 0 error. Windows (NVIDIA RTX 3060 + AMD) PASS: no crash
    (NVIDIA 5/5, AMD 1/1; it was 0xc0000005 5/5), the same dump / refusal, 0 VUID. The refusal names the image `''`:
    Vulkan identifiers are empty in Release by design, and the console line after it names the target. Windows found
    (not a triad regression): the FIRST dump of a session is sometimes solid blue (3/6), every later one is real — item
    `dump-render-target-first-dump-solid-blue`.

### 9c — commands, pipelines, descriptors, sync (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (18 TUs + headers): **45**: 15 reinterpret-cast, 14 scoped-lock, 3 designated
  initializers, 3 array-to-pointer decay, and singles (a duplicate include, a pointer-to-pointer streamed, an
  implicit-widening constant, a C array, two `snprintf`).
- [x] (2) Review. Internal (the renderer and Saphir drive these wrappers); sound: the GPU profiler drops the scopes past
  `MaxScopesPerFrame` with a warning and keeps its open / close bookkeeping balanced; the descriptor pool grows (the
  2026-09-22 fix). No defect, no owner question.
- [x] (3) Mechanical: fix-its with `--format-style=none` (scoped_lock, designated initializers — spacing normalized —,
  the duplicate include), the profiler label a `std::array< char, LabelCapacity >`, a `size_t` constant product, the
  immutable-sampler pointer streamed as `const void *`.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 113 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 45 → 21 (on purpose, ledger). `getGPUTimings()` with the profiler ON prints its
  table (labels at their 47-character capacity, unchanged); terrain, sponza, citadel (MCP 1707/0, console 4466/0): 0
  VUID, 0 leak.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `1cbb475d`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning): the GPU profiler works on MoltenVK ("3 query pools of 128 timestamps, period
    1 ns/tick, 64 valid bits") and prints its full table (labels cut at 47 characters, as designed); terrain, sponza,
    citadel (MCP 1707/0, console 4445/0): 0 VUID, 0 UNASSIGNED.
  - Windows PASS on NVIDIA RTX 3060 Laptop AND the forced AMD iGPU (MSVC /W4 /WX 0 warning): the profiler table on both
    (39 lines); terrain, sponza, citadel (MCP 1707/0, console 4455/1 NVIDIA the known RST, 4457/0 AMD); NVIDIA only the
    known 12325. Noted (PRE-EXISTING, unchanged by 9c — the label was already 48 bytes): labels are cut at 47 characters
    and the statistics accumulate BY LABEL, so two scopes whose names differ only past character 47 would merge (none do
    today: 0 duplicates on both GPUs).

## Section 10 — `src/Audio` (started 2026-10-01), two sub-sections (owner, 2026-10-01)

| Sub | Content | Lines | Status |
|---|---|---|---|
| 10a | The audio core: the resources read from disk (`SoundResource`, `MusicResource`, `PlaylistResource`, `SoundfontResource` — the unconfined `file` path lead of section 2), `Buffer`, `Manager` (+ console), `TrackMixer` (+ console), `Source`, `Listener`, `Ambience*`, `Recorder`, `ExternalInput`, `HardwareOutput`, `Utility` | ~12 000 | ✅ pushed engine `1320d529`; VALIDATED macOS M2 + Windows NVIDIA + AMD |
| 10b | `Effects/`, `Filters/`, `EffectSlot` (the EFX parameters, from JSON and the console) | ~7 000 | ✅ pushed engine `2544c410`; VALIDATED macOS M2 + Windows NVIDIA + AMD |

### 10a — the audio core (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (18 TUs + headers): **121**: 47 designated initializers (a note table), 24
  reinterpret-cast, 16 scoped-lock, 13 convert-to-static, and singles (incl. two missing braces, two widening sizes).
- [x] (2) Review (trust boundaries: the resource files, the playlist / ambience JSON, the audio settings, the TrackMixer
  console). Sound: the console bounds its volume (0-100), seek (the duration) and indices; the ambience channel count
  is bounded by the source pool, its radius by `setRadius()`. Findings:
  - A1 (the section-2 lead) `SoundfontResource::load(json)` opened its `file` key RAW (working-directory relative,
    absolute and `..` accepted) — and the JSON form was unreachable anyway: the container calls `load(path)` on the
    `.json`, which read it as an SF2 (owner question). A file over 2 GiB overflowed TinySoundFont's `int` size.
  - A2 audio settings unbounded: the capture buffer size times 1024 in `int32` (signed overflow, a negative size to
    OpenAL), a music chunk size of 0 (`chunkCount(0)`), the OpenAL attributes (owner question).
- [x] (3) Mechanical: fix-its with `--format-style=none` (scoped_lock, designated initializers — NOT the 48-note jingle
  table, kept positional for readability —, parentheses, `auto`, the two missing braces, a redundant cast, a boolean),
  the WAV byte rate in `uint32_t` explicitly, two `const auto * const`, `s_tsfMutex` in an anonymous namespace, a `const`
  reader builder, a switch local initialized, copy / move deleted on `Audio::Manager` and `SoundfontResource`.
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - A1 `file` resolved through `FileSystem::getFilepathFromDataDirectories("data-stores/SoundBanks", file)` (confined);
    `load(path)` hands a `.json` to `ResourceTrait::load()` (as `MeshResource` does), the read shared by both forms
    (`readSoundfont()`), a file past `INT_MAX` bytes refused.
  - A2 through `getOrSetDefaultInRange()`: Capture BufferSize 1-1024 KiB, Music ChunkSize 1024-1048576, OpenAL
    MaxMonoSources 1-256, MaxStereoSources 1-64, RefreshRate 1-1000, SyncState 0-1.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 117 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 121 → 87 (on purpose, ledger). A scratch `--add-data-directory` with
  `SoundBanks/TestSF.json` `{"file": "FluidR3.sf2"}` → `Loaded` (the owner's store file); `"../SoundBanks/FluidR3.sf2"` and
  `"/etc/hostname"` → refused by FileSystem; the six audio keys out of range → their warnings and the defaults; a MIDI
  track plays (the `.sf2` path); citadel MCP 1707/0, console 4466/0, 0 VUID.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `1320d529`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning): `TestSF` (`FluidR3.sf2`) `Loaded`; `../SoundBanks/FluidR3.sf2` and `/etc/hosts`
    refused by FileSystem ("leaves its store … refused") then by `SoundfontResource`; the five audio keys out of range →
    their warnings and the defaults; a track plays (`nowPlaying()` advances); citadel MCP 1707/0, console 4445/0; 0 VUID,
    0 UNASSIGNED everywhere. The shutdown pair "`GlobalReleaseFlush` AL_INVALID_OPERATION" + "unread problem with AL" is
    PRE-EXISTING (in ~20 logs from 6b to 9c, with or without music). Windows (NVIDIA + AMD) PASS: MSVC /W4 /WX 0
    warning; the same Loaded / refused / refused (`C:/Windows/win.ini` too); the five audio warnings; a track plays;
    citadel MCP 1707/0, console 4457/0 on both GPUs, AMD 0 VUID, NVIDIA only the known
    `VUID-VkGraphicsPipelineCreateInfo-renderPass-12325` (mesh-shader multiview, RTX 3060).

### 10b — `Effects/`, `Filters/`, `EffectSlot` (2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (15 TUs + headers): **1** — `EffectSlot::disable()` convert-to-static.
- [x] (2) Review (trust boundary: the EFX parameters from the console and the JSON definitions; every setter checks its
  range against the `AL_…_MIN_…` / `AL_…_MAX_…` constants before the `alEffect*` call). Findings:
  - E1 a NaN passed every float range check (`value < MIN || value > MAX` is false for NaN) and reached OpenAL, which
    refuses it with `AL_INVALID_VALUE` (0xa003) — no UB, but no warning naming the parameter and an AL error left in the
    queue. 75 float setters.
  - E2 `EAXReverb::setReflectionsPan()` / `setLatePan()` and their getters used `alEffectf` / `alGetEffectf` on
    `AL_EAXREVERB_REFLECTIONS_PAN` / `AL_EAXREVERB_LATE_REVERB_PAN`, which are 3-float VECTORS: OpenAL Soft answers
    `AL_INVALID_ENUM` (0xa002), so both pans never worked (owner question).
- [x] (3) Mechanical: E1 — the 75 float checks become `std::isnan(value) || value < MIN || value > MAX` (the 6 integer
  ones are unchanged). A first `!(value >= MIN && value <= MAX)` form is rejected: it drew 81
  `readability-simplify-boolean-expr`, whose fix-it reopens the NaN hole.
- [x] (3b) Owner ruling (2026-10-01), as recommended, APPLIED: E2 — the pans become `Base::Math::Vector< 3, float >`,
  set through `alEffectfv` (a non-finite component or a length over 1 refused with a warning, per the EFX guide) and
  read through `alGetEffectfv`.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 120 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 1 → 1 (on purpose, ledger). A scratch harness on the bundled OpenAL Soft (no demo
  uses `EAXReverb`): the old `alEffectf(pan)` → 0xa002, the new `alEffectfv(pan)` → 0, read back `0.5 0 -0.25`; a NaN
  density → 0xa003 (what the new check stops first). citadel: a track plays (`nowPlaying()` 5 s), MCP 1707/0, console
  4466/0, 0 VUID, 0 UNASSIGNED.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `2544c410`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning): citadel, a track plays (5.07 s advance in 5 s), MCP 1707/0, console 4445/0,
    0 VUID, 0 UNASSIGNED, 0 error.
  - Windows PASS on NVIDIA RTX 3060 Laptop and AMD (MSVC /W4 /WX 0 warning): citadel, a track plays (5.02 s / 5.04 s
    advance in 5 s), MCP 1707/0 on both; console 4455/1 and 4454/2 (the known RST flakes,
    `console-last-refusal-lost-on-windows`); AMD 0 VUID, NVIDIA only the known `renderPass-12325`. The 9b dumps again:
    6/6 real, 0/6 blue this time.

## Section 11 — `src/Physics` (started 2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (14 TUs + headers): **90**: 33 implicit-bool-conversion, 17 branch-clone (the
  `Particle` transform switches), 16 static-cast-downcast, 7 convert-to-static, 5 designated initializers, 3 unused
  parameters, 3 isolate-declaration, 2 constant-array-index, singles.
- [x] (2) Review: the trust boundaries (the properties' setters, their JSON, the environment from a scene definition)
  by hand; the collision models, `CollisionDetection`, `ContactManifold` / `ContactPoint`, `ConstraintSolver`, `Physics.hpp` and
  the base collision maths by an agent, every finding re-checked against the code. Findings:
  - P1 `MovableTrait::updateSimulation()` divided the angular velocity by an angular speed that drag can drive to 0:
    exactly with an angular drag of 1.0, and by underflow after ~500 free-spinning frames at the default 0.1. The NaN
    axis turned the orientation into NaN for good (`rotation(0, NaN)` is NaN).
  - P2 `Vector / s` is NaN for `|s| <= epsilon` (base): every `normal = mtv / depth` (9 sites in the three models) made
    a NaN normal for a 1-ulp overlap, which reached the velocities, then the positions.
  - P3 the explicit quadratic drag (`MovableTrait`, `Particle`) went past a stop when `k |v| dt > 1`, reversed, then
    diverged (a 1 g, 100 cm² particle at 40 m/s: `-nan` in 8 frames) (owner question).
  - P4 the property setters accepted NaN (and +inf for mass / surface / drag); a `Weight` animation can pass one
    (owner question). `merge()` made `1/0 = +inf` for two massless bodies. The JSON notifications carried an
    `optional< float >`, not the `float` the other overload sends. The JSON never read `AngularDragCoefficient`
    (owner question).
  - P5 base `Matrix::inverse()` tested `|det| <= epsilon` (absolute) and returned the matrix itself: the inverse inertia
    of a 1 kg, 10 cm sphere was 0.004 instead of 250 (owner question). `determinant()` skipped the terms
    below epsilon too.
  - P6 the `deltaTime` guard of `ConstraintSolver::solve()` was commented out (a 0 step made the Baumgarte bias +inf);
    `ContactPoint` stored a K <= 0 or NaN as its own inverse.
  - P7 the solver re-applies restitution at each velocity iteration (the bounce is lost; `onCollision` up to 8× per
    contact) and the position correction 3× with a stale depth (owner question).
  - Minor, recorded only (owner: no item): `Physics.hpp` atmosphere functions mix metres and km above 84.85 m (no
    caller); with coincident centres, sphere-sphere / capsule-capsule / capsule-sphere push the first body of the pair
    down, an arbitrary choice; the sphere and capsule shape merges ignore or misplace `centerOffset`.
  - Sound (checked): contact capacity (`addContact()` guards `MaxContactPoints`), the normal convention end to end, the
    divisions of the base collision maths, degenerate shapes (`isValid()`), mass 0 / static pairs, friction bounds,
    fixed iteration counts, member initialization.
- [x] (3) Mechanical: P1 (re-test the speed after damping, `> FLT_MIN`, `* (1 / speed)`), P2 (`depth > FLT_MIN`,
  `mtv * (1 / depth)`), P6 (the guard restored, `!(deltaTime > 0)`; K -> 0 when not finite and positive), `merge()`
  (the `setMass()` rule), the notification payload; fix-its: the `Particle` switches merged, designated initializers,
  `std::max`, `= nullptr`, implicit bool conversions, isolated declarations (also 7 in base headers: a header fix-it is
  applied once per TU, see the ledger), `/*other*/`. Base: `SamePrimitive.hpp` `denom` un-nested (~90 levels; 21 more
  lines in 7 base files: base item `runaway-nested-parentheses`).
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - P3 the exact step `|v'| = |v| / (1 + k |v| dt)` (`Physics::getDragVelocityFactor()`, 1 for a zero / negative /
    NaN term), in `MovableTrait` and `Particle`.
  - P4 the setters refuse non-finite values (`std::isnan(v) || v < 0 || v > 1` for the unit ones, the tidy-safe form),
    `setInertiaTensor()` a non-finite entry; the JSON reads `AngularDragCoefficient`; `Inertia` stays code-only
    (documented).
  - P5 base: a relative singularity test (`|det| <= epsilon × ∏ max |column entry|`, upper 3x3 for an affine 4x4),
    `tryInverse()` (`std::optional`), `inverse()` unchanged in contract, `determinant()` skips exact zeros only;
    `MovableTrait` takes a zero inverse inertia on a singular tensor.
  - P7 → engine item `physics-solver-restitution-and-position-correction` (a measured solver pass).
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); base unit tests 2184/2184 (Release and
  ASan/UBSan), 4 new `MathMatrix` tests; the old header printed `det(diag(1e-8, 1, 1)) = 0` and `inverse()[0] = 0.004`,
  the new one `1e-8` and `250`; clangcheck 116 TUs 0, `-Wfloat-conversion` 0; clang-tidy 90 → 27 (on purpose, ledger).
  Harnesses on the real code: the drag (explicit `-nan` at frame 8, exact 40 → 1.21 m/s; an 80 kg body equal to
  1e-6); `BodyPhysicalProperties` linked against `libEmeraude` (the JSON angular drag 0.75 read; NaN / inf refused by
  the seven setters and the tensor, the value kept). Runtime: collision, collision-debug (rotating cubes), lighten-marbles,
  particles, physics-debug — 0 VUID, 0 error, 0 property warning, the scenes intact; sponza pixel A/B for the
  `Matrix` change inside the noise (mean |Δ| 0.71 / 1.04 against 0.77 run-to-run, >8 levels 0.44-0.47 % against
  0.45 %); citadel MCP 1707/0, console 4466/0, 0 VUID.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `a777ddf7`, base `9d62d80`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning, base 2184 = 2181 + 3 skipped): the five physics demos, sponza, citadel (MCP
    1707/0, console 4445/0): 0 VUID, 0 UNASSIGNED, 0 error, 0 property warning; every root body finite
    (`getNodePhysics()`). `collision-debug` `DynTopCube10` rests at x = 18.23 on macOS, 6.86 on Linux (identical over
    2 Linux runs): a tipping cube is chaotic, read as an arm64 / x86 floating-point divergence (its tensor and drag are
    not touched by 11 beyond ~4e-4 relative per frame); Windows asked for the same reading.
  - Windows PASS on NVIDIA RTX 3060 Laptop and AMD (MSVC /W4 /WX 0 warning, base 2184 = 2181 + 3 skipped): the five
    physics demos, sponza, citadel (MCP 1707/0, console 4457/0 on both): AMD 0 VUID, NVIDIA only the known
    `renderPass-12325`; 0 property warning. The `DynTopCube10` reading CORRECTS the reading above: 4 launches of the
    same binary on one machine rest at x = 18.87, 12.60, 10.86, 2.66 (`DynBottomCube9` identical in all). The physics is
    NOT reproducible from one run to the next (Linux's two equal runs were a coincidence of regular pacing): not an
    x86 / arm64 divergence. Raised with the owner.

## Section 12 — `src/Animations` (started 2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (8 TUs + headers): **5** (prefer-member-initializer, redundant-member-init,
  integer-sign-comparison, math-missing-parentheses, use-ranges).
- [x] (2) Review (the clips and skeletons come from the loaders; `Sequence`, `RandomValue` and the flickers from code
  and animations). Findings:
  - N1 `SkeletalAnimator::computeWorldMatrices()` is one forward pass that needs parents first, but glTF allows the
    joints of a skin in ANY order and `Skeleton::isValid()` was never called (an out-of-order skin: a child reads a
    parent matrix not computed yet). `computeSkinningMatrices()` never checked that a skin joint index exists in the
    skeleton (an out-of-bounds read per frame) (owner question).
  - N2 (a section 3 gap, reached from the skins) fastgltf 0.9.0 bounds-checks nothing: `validate()` checks neither a
    buffer view inside its buffer nor an accessor inside its view, and its `span` / `iterateAccessor()` have no check
    (the type is an `assert`). Consequences: out-of-bounds reads at every accessor; heap WRITES past the vertex array
    (an attribute longer than POSITION) and past `inverseBindMatrices` (glTF allows more matrices than joints);
    `validate()` ITSELF indexes the sparse views, the channel sampler and the sampler accessors unchecked; a failed
    meshopt decode served an empty span that was then read; a JOINTS_0 value past the skin is an unchecked GPU read
    (`bones[JOINTS_0]`) (owner question).
  - N3 `RandomValue` called `asFloat()` on vectors, colors and frames (always 0 plus a type-mismatch diagnostic); its
    Windows build returned `Variant{0}` (an int) for the 8-bit types (owner question).
  - N4 base `Utility::quickRandom()` for integers: a rand() truncated to a negative `int8_t` / `int16_t` (46 % of
    `quickRandom< int8_t >(0, 10)` below 0), a signed overflow of `1 + max - min` on a wide `int32_t` range (owner
    question).
  - N5 `Sequence::addKeyFrame(float)` / `setCurrentTime(float)`: `clampToUnit(NaN)` is NaN and a NaN converted to
    `uint32_t` is UB; `LampFlicker`: `std::clamp(NaN)` is NaN, a NaN health made a NaN light intensity.
  - Sound: `Sequence` keyframes (a `std::map`: `normalizedEnd` > 0), the wrap modes, `FlameFlicker` (`std::max` drops a
    NaN), `AnimationClipResource` / `SkeletonResource` (built by the loaders only).
- [x] (3) Mechanical: N5 (NaN refused with a warning in the two `Sequence` setters and `setHealth()`; the `LampFlicker`
  constructor and helpers take a NaN health as 1, the section 11 rule); the 5 fix-its (`m_phase` in the initializer
  list after `m_randomizer`, `std::cmp_greater_equal`, `std::ranges::sort`, parentheses). The fix-its also touched
  three base headers through the TU's includes: REVERTED (outside the section, the section 3 precedent).
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - N2 refuse the FILE: `hasValidatableIndices()` before `validate()`, `hasConsistentExtents()` after the node forest
    check (views, accessors with sparse, attribute counts, SCALAR indices, skin joints, MAT4 FLOAT IBMs >= joints,
    VEC3 / VEC4 animation outputs), only the first jointCount IBMs read, a failed meshopt view served as zeros of its
    size and the file refused, a JOINTS_0 past its skin refused.
  - N1 reorder: `parentFirstJointOrder()` (the identity when already parents first; a stable sort by memoized depth
    otherwise), the `Skin` maps the glTF index to it, `loadAnimations()` the same; `SkeletalAnimator` refuses a
    skeleton failing `isValid()` and a skin that does not match it (an index out of range, an IBM count ≠ joints).
  - N3 per component (vectors, color with alpha, the frame position); the Windows special case removed.
  - N4 base: computed in the unsigned type of the same width (`bool` excluded), always in range; RAND_MAX coverage
    documented.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); base unit tests 2186/2186 (Release and
  ASan/UBSan), the 2 new `BaseUtility` tests failing before the fix; clangcheck 112 TUs 0, `-Wfloat-conversion` 0;
  clang-tidy Animations 5 → 0, `GLTFLoader.cpp` 0 new finding on the changed lines. A standalone fastgltf census
  (323 parseable glTF-Sample-Assets + 11 store assets, 192 skins): 0 out-of-order skin, 0 IBM count / type issue, 0
  view or accessor out of bounds, 0 JOINTS_0 out of its skin. The REAL loader over the 349 corpus files
  (`Core.openFiles()`): 0 refused by the new checks, 0 animator refusal; the 18 that do not load are pre-existing (15
  unsupported extensions at the parse, 3 point / line primitive modes); `NodePerformanceTest` alone (10 000 materials)
  overflows the shared material buffer (7 232 slots), a known limit. 13 hand-crafted glTFs: the valid ones load (an
  out-of-order skin, more IBMs than joints), the 10 hostile ones refused with their reason, 0 VUID. animation-debug
  (the skinned Paladins animate), citadel MCP 1707/0, console 4466/0, 0 VUID.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `29159fdf`, base `96cf9bc`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning, base 2186 = 2183 + 3 skipped): the 13 crafted glTFs exactly as on Linux;
    CesiumMan, Fox, BrainStem, RiggedFigure, SimpleSkin load and animate with 0 [SkeletalAnimator] line;
    animation-debug, citadel MCP 1707/0, console 4445/0; 0 VUID, 0 UNASSIGNED. Found (PRE-EXISTING, A/B on Linux with
    the pre-12 loader and animator: the same): the animated BrainStem lies on its side — item
    `gltf-skinned-non-joint-ancestors-misoriented`.
  - Windows PASS on NVIDIA RTX 3060 Laptop and AMD (MSVC /W4 /WX 0 warning: `quickRandom< int8_t >` accepted without
    the former Windows special case; base 2186 = 2183 + 3 skipped): the 13 crafted glTFs as on Linux (NVIDIA and AMD
    logs identical), the five skinned samples load and cycle, animation-debug, citadel MCP 1707/0, console 4457/0;
    AMD 0 VUID, NVIDIA only the known `renderPass-12325`. BrainStem as on macOS (it alone logs "1 node animation
    clip(s) attached, driving 1 node(s)"): the item above.

## Section 13 — `src/Overlay` (started 2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (8 TUs + headers): **22** (6 ambiguous smart-pointer `reset()`, 5 designated
  initializers, 3 scoped-lock, 3 redundant member init, 3 constant-array-index, singles).
- [x] (2) Review (the trust boundaries: the window system's sizes and scales, the content provider's painted size and
  dirty region — CEF, through `requestTransitionBufferResize()` and the transition buffer —, the render / main thread
  split). Sound: the upload path (the partial band only when provable, the full upload otherwise), the accelerated
  popup composite (clamped to the target), the 0-px transient. Findings:
  - O1 the render-thread passes (`updateVideoMemory()`, `onWindowResized()`, `dumpUploadStatistics()`,
    `UIScreen::processSurfaceUpdates()`) iterated `m_screens` / `m_surfaces` without the mutex that the main thread's
    `createScreen()` / `destroyScreen()` / stack operations take; `createImGUIScreen()` wrote `m_ImGUIScreens` unlocked
    while `render()` reads it locked; `surfaces()` handed out an unlocked reference; `UIScreen::operator<<` read the
    stack unlocked (owner question).
  - O2 a surface size has no upper bound: past `maxImageDimension2D` the image creation is refused (9b), but the pixmap
    is allocated first and a huge one aborts (owner question).
  - O3 `FramebufferProperties`: a NaN or infinite screen scale passed the `<= 0` test (NaN resolutions), and the pixel
    sizes converted `round(float)` to `uint32_t` / `int32_t` unchecked (UB on a NaN or negative geometry).
- [x] (3) Mechanical: O3 (a non-finite scale is 1; `roundedToInteger()` saturates, NaN = 0); the `operator<<` lock; the
  fix-its (`= nullptr` ×6, designated initializers with the spacing normalized, `std::scoped_lock` ×3 + the ImGUI
  device lock, the redundant initializers, the parameter name), run TU by TU: no header outside the section touched.
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED:
  - O1 snapshots: `Manager::snapshotScreens()` into a reused `m_screenSnapshot`, `UIScreen::m_surfaceSnapshot`, both
    copied under their mutex and processed without it; `createImGUIScreen()` locks; `surfaces()` returns a copy.
  - O2 refused before any allocation (`Surface::fitsDeviceLimits()`): error, the current buffer stays.
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 110 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 22 → 4 (on purpose, ledger). citadel with the CEF menu open
  (`Stage.openMenu()`) through 4 window resizes (1200×700, 1700×1000, 900×600, 1600×900): the menu re-laid out at
  each size, 4 transition buffers committed, 0 VUID, 0 error; a 17 000×1200 request (the compositor settled at
  7 668 px: the refusal not reached at runtime); citadel MCP 1707/0, console 4466/0.
- [x] (5) Pushed 2026-10-01 (owner's order): engine `734de41d`; peers asked.
  - macOS M2 PASS (AppleClang 0 warning, Retina scale 2): the 4 resizes re-lay the menu out (exactly 4 commits, 2400×1400
    to 3400×1870), citadel MCP 1707/0, console 4445/0, 0 VUID. The O2 refusal PROVEN at runtime: `resize(17000, 1200)`
    → a 34000×1870 framebuffer, "The surface 'ApplicationMenuCEF' would be 34000x1870 px, past the device limit of
    16384 px per side: the size is refused" (and 'Notifier'). The post-process targets at that size then exhaust the
    M2's memory → device lost → a SIGSEGV in an overlay upload submitted on the lost device (pre-existing device-loss
    handling: recorded in `renderer-fail-fast-on-device-loss`; the unbounded `Window.resize` command is for section
    15).
  - Windows PASS on NVIDIA RTX 3060 Laptop and AMD (MSVC /W4 /WX 0 warning: `roundedToInteger<>` and the
    `std::scoped_lock` on `Vulkan::Device` clean): one commit per resize, the menu re-laid out every time; Windows
    clamps `resize(17000, 1200)` to the desktop (1924×1061), no crash; the three ImGUI screens created at startup, the
    physical-camera panel toggled (Shift+F2); MCP 1707/0, console 4457/0 (AMD 4455/1, the known RST flake); AMD 0
    VUID, NVIDIA only the known `renderPass-12325`. Note for section 15: `Window.resize` answers with the REQUESTED
    size, not the size applied.

## Section 14 — `src/PlatformSpecific` (started 2026-10-01)

- [x] (1) clang-tidy 21.1.6 baseline (the 17 Linux TUs + headers; the Windows / macOS sources are not compilable here):
  **54** (12 array-to-pointer decay, 10 vararg, 9 mt-unsafe, 8 union access, 4 non-private members, singles).
- [x] (2) Review: Linux and the common code by hand; the Windows and macOS sources by an agent, every finding re-read.
  Sound: the Linux shell command lines (every user string through `escapeShellArg()`), `reproc` argv for
  `runDesktopApplication()`, the Windows file-dialog STA thread, the registry reads, the bounded dialog buffers.
  Findings:
  - Q1 VideoCapture: the YUYV conversion wrote past the output for an odd pixel count and ignored the row stride; Linux
    never checked that the driver kept YUYV; a short frame returned stale data as a success; Windows RGB32 read
    `w × h × 4` bytes unchecked, ignored a format change while streaming and `MFGetAttributeSize()`'s result; macOS
    trusted the pixel buffer (lock, base address, format, row length) and freed the delegate while its queue could
    still call it; the platform objects were an owning `void *`.
  - Q2 macOS: the engine `.mm` files were compiled WITHOUT ARC while written for it (every alert, notification and
    capture session leaked); `TextInput` set the Accessory activation policy on every prompt and never restored it; no
    camera permission check and no `NSCameraUsageDescription`; `open "…"` through `system()` (shell injection);
    nil / NULL string conversions (an abort on invalid UTF-8); a NULL CF property crashed `SystemInfo` at boot;
    `flashTaskbarIcon(false)` never cancelled; the notification reported success without a notification center.
  - Q3 Windows: `path::string()` (ANSI, throws outside the code page) for every shell path and log line; the UTF-16 →
    multibyte converters queried one length and converted another (ERROR_INSUFFICIENT_BUFFER); `GetModuleFileNameA`
    (`?` outside the code page, 1024-byte truncation unchecked) for the application directory; `OpenFile` /
    `SaveFile` COM leaks and a dereference of a failed `GetItemAt()`; an `IFileSaveDialog` in an `IFileOpenDialog *`;
    `CoInitializeEx` never balanced; a NaN progress cast to `ULONGLONG`; a trailing backslash escaping the closing
    quote; `CoTaskMemFree` missing on a failure; `GetLogicalDriveStringsW` overflow not checked.
  - Q4 Linux: `/proc/mounts` octal escapes not decoded (a mount point with a space skipped); `isdigit` on a signed
    `char`; NULL `passwd` fields into `std::string`; a throwing `.at()`; the program detection and shell escaping
    duplicated in `Notification.linux.cpp`.
  - Q5 `std::thread`'s constructor throws (an abort): `Helpers.linux.cpp` and `Notification.windows.cpp` here, about ten
    sites cascade-wide (owner question).
  - Q6 `openURL()` hands any scheme that passes `URL::isURL()` to the system (`ShellExecuteW`, `open`, `xdg-open`): any
    registered protocol handler (`file://host/share/x.exe`, `ms-*`). No caller today (owner question).
- [x] (3) Mechanical: Q1, Q3, Q4, Q2 except the rulings; clang-tidy fix-its TU by TU (starts_with, `timeval{}`,
  pass-by-value, anonymous namespaces, `std::array`), no header outside the section touched.
- [x] (3b) Owner rulings (2026-10-01), as recommended, APPLIED: Q5 → base item `non-throwing-thread-start` (a base RAII
  thread with a non-throwing start, then a cascade pass); ARC enabled on every engine `.mm` with a compile-time guard
  in each; the TextInput activation-policy line removed; `NSCameraUsageDescription` in projet-alpha's Info.plist and the
  macOS authorization check in `VideoCaptureDevice::open()`; Q6 `openURL()` opens `http://` and `https://` only (the
  raw string tested, case-insensitive; `file://`, `ms-*`, `javascript:` refused with an error);
  `NSMicrophoneUsageDescription` added to the Info.plist too (the audio capture).
- [x] (4) Verified 2026-10-01 (Linux, RTX 3070 Ti): cascade builds (0 warning); clangcheck 114 TUs 0,
  `-Wfloat-conversion` 0; clang-tidy 54 → 44 (on purpose, ledger). Harnesses under ASan / UBSan on the real function
  text: the old YUYV conversion over-reads on a 3×3 frame, the new one refuses it, is byte-identical on a packed 4×2
  frame, honours an 8-byte stride and refuses a short frame; the `/proc/mounts` decoder (`\040`, `\011`, `\134`, an
  incomplete escape kept); the `openURL()` scheme test (8 cases). Runtime (owner-approved camera test): `/dev/video0` negotiated YUYV 640×480, two real
  captures through KeyP (49 091 distinct colours), 0 VUID; citadel MCP 1707/0, console 4466/0. The Windows and macOS
  changes are UNCOMPILED here: the peers build and test them.
- [x] (5) Pushed 2026-10-01 (owner's order): engine (the 14 commit), base `d005f1c`, alpha `c043790a`; peers asked (each OS has a camera).

