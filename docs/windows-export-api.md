# Windows DLL export API (`EMERAUDE_API`) — migration guide

> Status: **migration complete (2026-07).** `EMERAUDE_USE_EXPLICIT_EXPORTS` defaults to **ON**
> (explicit `EMERAUDE_API` boundary, `WINDOWS_EXPORT_ALL_SYMBOLS` dropped) and
> `EMERAUDE_ENABLE_PCH` defaults to **ON**. The full MSVC cascade (base → engine → app_kernel →
> app_system executables) builds and links with the PCH enabled. New public API that a consumer
> references out-of-line must carry `EMERAUDE_API` — the consumer's linker (`LNK2019` on
> `__imp_...`) names any omission.

## 1. Why this exists

`Emeraude` is a SHARED library (LGPLv3 requires it). On Windows its exported symbol set was
produced automatically by CMake's `WINDOWS_EXPORT_ALL_SYMBOLS`, which scans every input `.obj`
and writes an `exports.def`.

That mechanism is **incompatible with precompiled headers on MSVC**. Once `EMERAUDE_ENABLE_PCH=ON`,
each translation unit pulls a PCH object (`cmake_pch.cxx.obj`) carrying compiler-internal marker
symbols (`__@@_PchSym_@00@…`). The DEF generator picks them up (`LNK4022`/`LNK4002` "no unique
match") and emits a **bogus `__` export**, which then fails to resolve:

```
exports.def : error LNK2001: unresolved external symbol __
Emeraude.lib : fatal error LNK1120: 1 unresolved externals
```

GCC/Clang are unaffected (no `.def`; ELF/Mach-O export via symbol visibility). The fix is to stop
auto-exporting and declare the public boundary explicitly with the `EMERAUDE_API` macro.

## 2. The toggle

`option(EMERAUDE_USE_EXPLICIT_EXPORTS … OFF)` in `CMakeLists.txt`:

| Mode | `WINDOWS_EXPORT_ALL_SYMBOLS` | `EMERAUDE_API` expands to |
|------|------------------------------|---------------------------|
| **OFF** (default) | `ON` | *nothing* (no-op) |
| **ON** | `OFF` | `__declspec(dllexport)` while building the DLL (`Emeraude_EXPORTS` is auto-defined by CMake for the SHARED target), `__declspec(dllimport)` for consumers, `__attribute__((visibility("default")))` elsewhere |

Because the macro is inert while the option is OFF, the public API can be annotated **one class at
a time without ever breaking the default build**. The switch is flipped to ON only once the whole
referenced surface is annotated. The macro lives in [`src/emeraude_export.hpp`](../src/emeraude_export.hpp).

## 3. What to annotate

`EMERAUDE_API` marks the boundary of what crosses the DLL edge and is referenced **out-of-line** by
a consumer (`projet-alpha`, tests, tools).

- **Annotate**: a public `class`/`struct` that has out-of-line member definitions (i.e. a `.cpp`).
  Put it on the type — `class EMERAUDE_API Foo` — which exports every member.
  ```cpp
  #include "emeraude_export.hpp"

  class EMERAUDE_API Foo { … };
  EMERAUDE_API bool freeFunction (int);   // out-of-line free function
  ```
- **Do NOT annotate**: header-only / fully-inline classes, function/class **templates** (instantiated
  in the consumer — `Math/*`, most of `Base::PixelFactory`), `constexpr`/`inline` helpers, and
  anything purely internal to the engine. Annotating them is harmless but noise.

### C4275 / C4251 — disabled by decision (no base-class cascade)

MSVC raises `C4275` when a dll-interface class derives from a non-dll-interface base, and `C4251`
when it holds a `std::` data member. Both are **disabled project-wide** (`/wd4275 /wd4251` in
emeraude-base's `EMERAUDE_COMPILE_OPTIONS`) rather than driving an annotation cascade:

- The whole cascade (DLL + every consumer) is built with the **same toolchain and CRT** — the
  layout/allocator mismatches those warnings guard against cannot occur.
- `EMERAUDE_API` classes may derive from emeraude-base traits that stay unexported **by design**
  (see § 4). Annotating the base hierarchy would only serve the warning, not a real need.

Consequence: annotating a class does **not** pull in its bases. Annotate only what the linker
names. The one residual risk C4275 used to flag — a `static` data member in a duplicated base
whose state could diverge between the DLL copy and the executable copy — must now be caught in
code review instead.

## 4. `emeraude-base` — decision: no `EMERAUDE_BASE_API` (static copies preserved)

`emeraude-base` is a STATIC library consumed **twice** in the cascade: its objects are folded into
`Emeraude.dll`, *and* the final executables get their own static copy (propagated through the
engine's / app_kernel's `PUBLIC` link). Executables therefore resolve every base symbol from
**their own embedded copy** — they import nothing base-related from the DLL. This duplication is
the long-standing status quo and works.

**Decision (2026-07, "option 2b"):** keep that model. **No `EMERAUDE_BASE_API` macro is created.**
Base symbols are neither exported from the DLL nor imported by consumers; the C4275 pressure that
would have forced base-trait annotation is removed by disabling the warning (see above). Base
stays completely untouched by this migration.

The alternative (a single source of truth where executables import base symbols from the DLL and
stop linking base statically) remains possible later as a separate project — it would require the
dllexport/dllimport macro, a much larger exported surface, and a linkage rework across the cascade
(app_kernel's `Kernel` and the executables). Do not introduce a no-op `EMERAUDE_BASE_API` in the
meantime: an inert macro with ambiguous semantics is worse than none.

## 5. Migration procedure (iterative, build-driven)

The annotations are inert until the flip, so progress is measured by flipping ON in a **scratch
build** and draining the linker:

1. Configure a throwaway build with `-DEMERAUDE_USE_EXPLICIT_EXPORTS=ON -DEMERAUDE_ENABLE_PCH=ON` in
   a Claude-owned build dir (never the CLion `cmake-build-*`).
2. Build the cascade; the consumer's link reports `LNK2019` (unresolved import) for every
   not-yet-exported symbol it references.
3. For each, annotate the **owning class/function** with `EMERAUDE_API` (bases are NOT pulled in —
   C4275 is disabled, see § 3).
4. Repeat until the link is green. Then make `EMERAUDE_USE_EXPLICIT_EXPORTS=ON` the default and drop
   this guidance to a short "done" note.

Do not annotate blind — let the linker name exactly what is missing, so the exported surface stays
minimal and intentional.

## 6. Status

- [x] `emeraude_export.hpp` reworked for the staged toggle (no-op default).
- [x] `EMERAUDE_USE_EXPLICIT_EXPORTS` option + `WINDOWS_EXPORT_ALL_SYMBOLS` gating wired.
- [x] First annotated class: `Core` (pattern example).
- [x] Decision "2b": no `EMERAUDE_BASE_API`, static base copies preserved, `/wd4251 /wd4275`
      added to `EMERAUDE_COMPILE_OPTIONS` (MSVC) in emeraude-base.
- [x] Engine public surface referenced by app_system (~120 annotations over 2 linker-driven
      iterations; includes the `FramebufferProperties` friend `operator<<` pattern — the DLL
      linkage attribute must be on the FIRST namespace-scope declaration, before the class).
- [x] Flip defaults to ON (`EMERAUDE_USE_EXPLICIT_EXPORTS`, `EMERAUDE_ENABLE_PCH`); full MSVC
      cascade verified (build + link with PCH).
- [x] MSVC PCH guard in emeraude-base's `EnablePrecompiledHeaders.cmake`: **kept deliberately**
      — it now protects the `EMERAUDE_USE_EXPLICIT_EXPORTS=OFF` fallback (anyone reverting to
      export-all gets the PCH silently disabled instead of `LNK2001`).