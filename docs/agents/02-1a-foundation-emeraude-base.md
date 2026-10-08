## 1a. Foundation: emeraude-base

The engine's foundation layer (math, image/audio/mesh factories, hashing, compression,
threading, traits, I/O — formerly `src/Libs/`, `EmEn::Libs`) now lives in the **standalone
[emeraude-base](https://github.com/EmeraudeEngine/emeraude-base) library** (`EmEn::Base`),
pulled in by `cmake/InstallEmeraudeBase.cmake` (clone-if-absent + `add_subdirectory`).

> [!IMPORTANT]
> **This is a real split with independent lifecycles.** emeraude-base evolves on its own
> (its own versioning and roadmap, new formats/capabilities); the engine **builds on a
> pinned emeraude-base package** and consumes it — it does not drive its evolution.
>
> emeraude-base is also the **single source of truth for external dependencies**: it owns
> every `Setup*.cmake` (even libs only the engine uses: ufbx, fastgltf, taglib, bc7enc,
> cpu_features, hwloc, libvpx), the `ext-deps-generator` resolution (`EMERAUDE_EXT_LIBS_*`),
> the **project-wide compile policy** (`EMERAUDE_COMPILE_*`, `EMERAUDE_CXX_VERSION`,
> `EMERAUDE_C_VERSION`), and the **shared STL hot-set precompiled header**
> (`cmake/STLPrecompiledHeaders.cmake`, which defines the `EMERAUDE_BASE_STL_PCH_HEADERS` list,
> + `cmake/EnablePrecompiledHeaders.cmake`). The engine inherits all of it via `emeraude::base`
> and adds `EMERAUDE_BASE_CMAKE_DIR` to its module path.
>
> **A `Setup<Lib>.cmake` links the TOP of the dependency chain, never the whole chain.** The
> ext-deps-generator packages ship real CMake config packages, so a wrapper already carries its
> core through `$<LINK_ONLY:…>` in its exported `INTERFACE_LINK_LIBRARIES`. Naming both in
> `target_link_libraries()` — and, worse, naming the core *first* — conflicts with the ordering
> static linking requires: CMake honours the requested position **and** re-emits the entry at the
> end of the link line, so the archive appears twice. On Apple's linker that surfaces as
> `ld: warning: ignoring duplicate libraries: '…/libfoo.a'` (harmless, but it hides real ones).
> Reference case: `cmake/SetupReproc.cmake` linked `reproc reproc++` while the sources only use
> `reproc++/run.hpp`; fixed 2026-09-07 by linking `reproc++` alone. ⚠️ The `find_package()` for the
> core must **stay**: `reproc++-config.cmake` does call `find_dependency(reproc)`, but without
> `PATHS ${EMERAUDE_EXT_LIBS_PATH} NO_DEFAULT_PATH` it cannot reach the ext-deps prefix — the
> explicit call is what makes the target exist, and `find_dependency()` then sees it as found.
> To audit a link line: `sed -n '<line>p' build.ninja | tr ' ' '\n' | grep '\.a$' | sort | uniq -c | awk '$1>1'`.
>
> **Precompiled header:** the engine target applies base's shared STL PCH via
> `emeraude_base_target_enable_pch(${PROJECT_NAME} "${EMERAUDE_BASE_STL_PCH_HEADERS}")`, like every
> other target in the cascade — the header list is always passed explicitly, as a CMake list.
> Turn the
> switch off periodically (`-DEMERAUDE_ENABLE_PCH=OFF`): the PCH masks missing `#include`s and both
> configs must stay green. **Windows is resolved (2026-07):** the old
> `WINDOWS_EXPORT_ALL_SYMBOLS` + PCH incompatibility (PCH marker symbols leaking into the
> auto-generated `exports.def` → `LNK2001`) was solved by completing the explicit-export
> migration: the public surface consumed by a consumer application carries `EMEN_LEAN_API`, and
> C4251/C4275 are disabled cascade-wide (decision "2b": emeraude-base types stay unexported,
> consumers keep their static base copy). Full MSVC cascade verified (build + link with PCH).
>
> **Explicit exports are mandatory on MSVC, and the option is now a PAIR (2026-08).** The brief
> 2026-08 revert to export-all (motivated by the longer consuming-application link and the standing
> annotation duty) died within days: the engine's symbol surface crossed the **hard PE limit of 65535
> exported ordinals per DLL** (`exports.def` at ~65.8k symbols → `LNK1189`), so export-all **no
> longer links on Windows at all**. The single `EMERAUDE_USE_EXPLICIT_EXPORTS` switch was then split
> into two **mutually exclusive** options (both On is a `FATAL_ERROR`):
> **`EMERAUDE_USE_FULL_EXPORTS`** (default On on MSVC — exports both `EMEN_LEAN_API` and `EMEN_API`)
> and **`EMERAUDE_USE_LEAN_EXPORTS`** (default Off — exports `EMEN_LEAN_API` **only**, keeping the
> ordinal count down to what an embedding application actually references; this is what a downstream application
> forces). On non-MSVC platforms both are inert (no `.def`; export via symbol visibility), hence Off.
>
> **Consequence of LEAN, and the trap it sets: `EMEN_API` is a no-op there.** A public symbol a
> consumer references out-of-line must carry **`EMEN_LEAN_API`**, or the **consumer's** link breaks
> (`LNK2019`/`LNK2001` on every member of the class) — one repository away from the change, **and on
> MSVC only**, so Linux and macOS keep building green. Let the linker name what is missing, then
> promote the class in the engine; never widen a consumer to `FULL` to silence it. Reference case:
> `Graphics::Material::StandardResource` stayed `EMEN_API` when it replaced `BasicResource` while all
> its consumed neighbours were `EMEN_LEAN_API` → 38 unresolved externals in a downstream application. The
> export-all/PCH guard stays at the `emeraude_base_target_enable_pch()` call site in `CMakeLists.txt`
> for anyone forcing both options `Off` locally (which no longer links on MSVC). **macOS Objective-C++ is handled (2026-07):** the
> base helper auto-sets `SKIP_PRECOMPILE_HEADERS` on the engine's `.mm` sources (SerialPort,
> WiFiScanner, StorageInfo, Window, Dialogs, …) — without it clang rejects the pure-C++ PCH in
> those TUs (`Objective-C was disabled in PCH file but is currently enabled`), because CMake
> classifies `.mm` as CXX when OBJCXX is not enabled. See `docs/windows-export-api.md` and
> `dependencies/emeraude-base/AGENTS.md` § 3a.
>
> **Third-party symbols never leave this library (ELF).** `libEmeraude.so` statically links the
> vendored archives, and on ELF — flat dynamic namespace — it used to **re-export them**, which made
> every plugin the process loads bind to our copies instead of theirs: cairo, FreeType and
> gdk-pixbuf behind CEF all called our libpng **1.6.58** rather than the system **1.6.48**, and
> libdecor refused its GTK3 plugin over the conflict (`uses conflicting symbol "png_free"`). The
> engine now calls
> `emeraude_base_target_hide_third_party_exports(${PROJECT_NAME} EXCEPT jsoncpp)` after its Setup
> includes — **44127 → 15518 exported symbols**, `EmEn::` and `glfw*` untouched. ⚠️ The consuming
> **executable must call it too** (an ELF exe exports what a shared object of its link closure asks
> for), and a third-party-backed codec must never be defined in a header. Full rationale, the
> measurement and the two-line check: [`dependencies/emeraude-base/AGENTS.md`](../../dependencies/emeraude-base/AGENTS.md) § 3b.
> Same root cause as the Windows export limit below, seen from the other end.
>
> **Rule:** a bug or a missing feature in the foundation layer (math, factories, I/O,
> threading, …) is fixed **in emeraude-base**, never worked around in the engine — the same
> co-development discipline the engine applies to itself vs projet-alpha.
