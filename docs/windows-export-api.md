# Windows DLL export API (`EMERAUDE_API`) — migration guide

> Status: **scaffolding in place, migration in progress.** Default build is unchanged
> (`EMERAUDE_USE_EXPLICIT_EXPORTS=OFF` → `WINDOWS_EXPORT_ALL_SYMBOLS`, macro is a no-op).

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

### C4275 — the base-class cascade (MSVC, `/WX`)

A dll-interface class whose **base** is not itself `EMERAUDE_API` raises `C4275` — promoted to an
error here. So annotating a class pulls in its bases. Example: `Core` derives from
`ObservableTrait`, `ObserverTrait`, `ControllableTrait`, `KeyboardListenerInterface` — each must
carry `EMERAUDE_API` (or its own export macro for `emeraude-base` types) before the flip. The same
applies to a `std::` base only when the consumer instantiates it; STL bases are usually fine.

## 4. `emeraude-base`

`emeraude-base` is a STATIC library whose objects are folded into `Emeraude.dll`, and `projet-alpha`
uses some of its types **directly**. Those that are out-of-line (e.g. `Logging`, `Tracer`) need their
own `EMERAUDE_BASE_API` macro (same dllexport/dllimport logic, keyed on the engine build) so they are
exported from the DLL. Header-only base code (Math, the `PixelFactory` format templates) needs
nothing. The base export header is added during the iteration when the first unresolved base symbol
surfaces — keep its name free of any product name (public repo, see the confidentiality rule).

## 5. Migration procedure (iterative, build-driven)

The annotations are inert until the flip, so progress is measured by flipping ON in a **scratch
build** and draining the linker:

1. Configure a throwaway build with `-DEMERAUDE_USE_EXPLICIT_EXPORTS=ON -DEMERAUDE_ENABLE_PCH=ON` in
   a Claude-owned build dir (never the CLion `cmake-build-*`).
2. Build the cascade; `projet-alpha`'s link reports `LNK2019` (unresolved import) for every
   not-yet-exported symbol it references.
3. For each, annotate the **owning class/function** with `EMERAUDE_API`, adding its bases as C4275
   demands.
4. Repeat until the link is green. Then make `EMERAUDE_USE_EXPLICIT_EXPORTS=ON` the default and drop
   this guidance to a short "done" note.

Do not annotate blind — let the linker name exactly what is missing, so the exported surface stays
minimal and intentional.

## 6. Status

- [x] `emeraude_export.hpp` reworked for the staged toggle (no-op default).
- [x] `EMERAUDE_USE_EXPLICIT_EXPORTS` option + `WINDOWS_EXPORT_ALL_SYMBOLS` gating wired.
- [x] First annotated class: `Core` (pattern example).
- [ ] Engine public surface referenced by `projet-alpha`.
- [ ] `emeraude-base` out-of-line surface (`EMERAUDE_BASE_API`).
- [ ] Flip default to ON; verify the full MSVC cascade links with PCH enabled.