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
| 6 | `src/Scenes` (the rest, by sub-group: 6a-6e below) | ~59 000 | 🟠 6a + 6b + 6c ✅, 6d next |
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
| 6d | `Component/` | 18 617 | ⬜ |
| 6e | `Editor/`, `AVConsole/`, `Viewers/`, `EffectsToolkit/`, `Debug/` | ~8 500 | ⬜ |

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

