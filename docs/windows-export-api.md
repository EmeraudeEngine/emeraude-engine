# Windows DLL export API (`EMERAUDE_API`) — migration guide

> Status: **migration complete (2026-07).** `EMERAUDE_USE_EXPLICIT_EXPORTS` defaults to **ON**
> (explicit `EMERAUDE_API` boundary, `WINDOWS_EXPORT_ALL_SYMBOLS` dropped) and
> `EMERAUDE_ENABLE_PCH` defaults to **ON**. The full MSVC cascade (base → engine → a consumer library →
> consumer executables) builds and links with the PCH enabled. New public API that a consumer
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
engine's / the consumer library's `PUBLIC` link). Executables therefore resolve every base symbol from
**their own embedded copy** — they import nothing base-related from the DLL. This duplication is
the long-standing status quo and works.

**Decision (2026-07, "option 2b"):** keep that model. **No `EMERAUDE_BASE_API` macro is created.**
Base symbols are neither exported from the DLL nor imported by consumers; the C4275 pressure that
would have forced base-trait annotation is removed by disabling the warning (see above). Base
stays completely untouched by this migration.

The alternative (a single source of truth where executables import base symbols from the DLL and
stop linking base statically) remains possible later as a separate project — it would require the
dllexport/dllimport macro, a much larger exported surface, and a linkage rework across the cascade
(the consumer library's `Kernel` and the executables). Do not introduce a no-op `EMERAUDE_BASE_API` in the
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

## 6. Exported pimpl — out-of-line destructors, and the header hygiene it unlocks

An `EMERAUDE_API`-exported class forces MSVC to instantiate its **destructor at the class
definition point in every TU**. If such a class holds a `std::unique_ptr< T >` to a
forward-declared (incomplete) `T`, `= default`-ing the destructor in the header instantiates
`std::default_delete< T >` on an incomplete type → hard error. The fix is the **exported pimpl**
pattern: declare the destructor in the header, define it out-of-line in the `.cpp` where `T` is
complete.

```cpp
// Foo.hpp
class EMERAUDE_API Foo final
{
    public:
        ~Foo ();                       // declared only — NOT '= default'
    private:
        std::unique_ptr< Bar > m_bar;  // Bar is only forward-declared
};

// Foo.cpp
#include "Bar.hpp"
Foo::~Foo () = default;                // deleter sees the complete Bar here
```

Current users: `Graphics::Renderer`, `Graphics::PostProcessor`, `Graphics::BindlessTextureManager`.

**The lever this unlocks.** Once the destructor is out-of-line, *every* smart-pointer and
reference member may point at an incomplete type, so a hot header can trade `#include` for
forward declaration. That is a real compile-time knob: the engine's heaviest headers fan out to
dozens or hundreds of TUs.

> [!IMPORTANT]
> **`Graphics/Renderer.hpp` is under an explicit no-regrowth contract (2026-07).** It is included
> by ~77 TUs directly and propagates through `Core.hpp` and `Overlay/UIScreen.hpp`, so a single
> `#include` added here is paid by most of the engine. The following are **deliberately
> forward-declared, not included** — do not "fix" a missing type by re-adding the header:
>
> | Forward-declared | Why it is safe |
> |---|---|
> | `Vulkan::SwapChain` | `shared_ptr` member only; `setSwapChainDegraded()` / `isSwapChainDegraded()` are **out-of-line on purpose** (they were the only inline bodies needing the complete type). Re-inlining them drags back `Window.hpp`, `Vulkan/Framebuffer.hpp`, `Vulkan/CommandBuffer.hpp` and the ViewMatrices UBOs. |
> | `Vulkan::Instance` | reference member + reference-returning getter |
> | `Window` | reference member + reference-returning getters |
> | `Resources::Manager` | reference member + by-reference parameters |
> | `GrabPass`, `MDI::BatchBuilder` | `unique_ptr` member + raw-pointer getter (relies on the out-of-line destructor) |
> | `SceneRenderTarget`, `TextureResource::TextureCubemap` | `shared_ptr` member + `const shared_ptr &` getter |
>
> When a consumer TU breaks after touching this header, **add the include to the consumer**, never
> back into `Renderer.hpp`.

### Finding the consumers before the build — and the method's blind spot

Walking the quoted-include graph with and without the removed edges gives the **header-level**
delta per TU, and that part is reliable: it is how the 2026-07 pass found `Graphics/PostProcessor.cpp`,
`Vulkan/SwapChain.cpp` and the five `Graphics/TextureResource/*.cpp` before compiling anything.

> [!WARNING]
> **Do not try to narrow that delta by grepping for the type name — it is unsound.** A TU can
> require a complete type it never spells, via `auto` plus a member call:
> ```cpp
> const auto & viewMat = viewMatrices.viewMatrix(readStateIndex, false, 0);
> ```
> `Graphics/Effects/Framebuffer/RTAO.cpp` broke exactly this way on `ViewMatricesInterface`
> (reached through `Vulkan/SwapChain.hpp` → `ViewMatrices{2D,3D}UBO.hpp`) and no name-based
> filter could have seen it. Same trap for `Manager`/`Abstract`/`Interface`/`Surface`: the names
> are ambiguous across namespaces, so a symbol grep produces both false negatives and a flood of
> false positives.
>
> Two further detectors were tried during the 2026-07 pass and **both silently under-reported**:
> - an accessor sweep matching `accessor()` immediately followed by `.`/`->` — missed
>   `Scenes/Scene.rendering.cpp`, where the result is parked in a variable first
>   (`auto * b = renderer.MDIBatchBuilder();` … `b->isReady()`). Covering that needs a second
>   step: capture the assigned name, then look for `name->`/`name.`;
> - a *method*-name sweep ("call a method declared only in a lost header") — its regex parsed
>   **zero** methods out of `MDI/BatchBuilder.hpp`, because the codebase puts the return type on
>   its own line, so the "0 regressions" it printed was a broken oracle, not a clean result.
>
> **Lesson: validate a detector against a known-broken TU before trusting its silence.** Only the
> header-level delta is sound enough to bound the surface; for the rest, **let the compiler
> enumerate it** and budget several build rounds.

## 7. Status

- [x] `emeraude_export.hpp` reworked for the staged toggle (no-op default).
- [x] `EMERAUDE_USE_EXPLICIT_EXPORTS` option + `WINDOWS_EXPORT_ALL_SYMBOLS` gating wired.
- [x] First annotated class: `Core` (pattern example).
- [x] Decision "2b": no `EMERAUDE_BASE_API`, static base copies preserved, `/wd4251 /wd4275`
      added to `EMERAUDE_COMPILE_OPTIONS` (MSVC) in emeraude-base.
- [x] Engine public surface referenced by a consumer application (~120 annotations over 2 linker-driven
      iterations; includes the `FramebufferProperties` friend `operator<<` pattern — the DLL
      linkage attribute must be on the FIRST namespace-scope declaration, before the class).
- [x] Flip defaults to ON (`EMERAUDE_USE_EXPLICIT_EXPORTS`, `EMERAUDE_ENABLE_PCH`); full MSVC
      cascade verified (build + link with PCH).
- [x] MSVC PCH guard in emeraude-base's `EnablePrecompiledHeaders.cmake`: **kept deliberately**
      — it now protects the `EMERAUDE_USE_EXPLICIT_EXPORTS=OFF` fallback (anyone reverting to
      export-all gets the PCH silently disabled instead of `LNK2001`).