# Windows DLL export API (`EMEN_API`) — migration guide

> Status: **migration complete and verified (2026-07); ON by default on MSVC (since 2026-08-05),
> OFF elsewhere.** The full MSVC cascade (base → engine → a consumer library → consumer
> executables) builds and links with `EMERAUDE_USE_EXPLICIT_EXPORTS=ON`.
>
> **Why ON is now mandatory on MSVC — export-all is dead.** The default had been reverted to OFF
> in early 2026-08 (consumer link time, annotation duty — see the history in § 7). Within days the
> engine's symbol surface crossed the **hard PE limit of 65535 exported ordinals per DLL**: the
> auto-generated `exports.def` reached ~65.8k symbols and the link fails with
> `LNK1189 (library limit of 65535 objects exceeded)` on the import library. This is a file-format
> ceiling, not a toolchain setting — no flag raises it, and trimming a few hundred symbols only
> postpones the failure by a few commits. `WINDOWS_EXPORT_ALL_SYMBOLS` is therefore **no longer a
> viable mode for this engine on Windows**, and the two costs of `ON` below are now simply the
> price of linking at all:
>
> 1. **Link time.** With `ON`, the consuming application takes **longer to link** — paid on every
>    single link of the downstream project (explicit `dllimport`/`dllexport` puts the consumer's
>    linker through a far larger import-resolution surface than a compact `.def`).
> 2. **Maintenance.** Every new public symbol a consumer references out-of-line must carry
>    `EMEN_API`, or the **consumer's** link breaks (`LNK2019` on `__imp_...`) — a failure that
>    surfaces one repository away from the change that caused it. Let the linker name what is
>    missing (§ 5).
>
> On non-MSVC platforms the option stays OFF and is inert: there is no `.def`; ELF/Mach-O export
> via symbol visibility, unaffected by the ordinal limit.
>
> Side effect of ON: the engine target regains its MSVC PCH (the export-all/PCH incompatibility
> guard no longer bites). The guard itself is kept in `CMakeLists.txt` for anyone forcing the
> option OFF locally — but be aware that a forced OFF **no longer links** on MSVC.

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
auto-exporting and declare the public boundary explicitly with the `EMEN_API` macro.

## 2. The toggle

`option(EMERAUDE_USE_EXPLICIT_EXPORTS …)` in `CMakeLists.txt` — default **ON on MSVC** (export-all
exceeds the 65535 PE export limit, see the status note), **OFF elsewhere** (inert):

| Mode | `WINDOWS_EXPORT_ALL_SYMBOLS` | `EMEN_API` expands to | Engine PCH on MSVC |
|------|------------------------------|---------------------------|--------------------|
| **OFF** (default off-MSVC; **no longer links on MSVC** — `LNK1189`) | `ON` | *nothing* (no-op) | disabled (guard at the call site) |
| **ON** (default on MSVC) | `OFF` | `__declspec(dllexport)` while building the DLL (`Emeraude_EXPORTS` is auto-defined by CMake for the SHARED target), `__declspec(dllimport)` for consumers, `__attribute__((visibility("default")))` elsewhere | enabled — but a longer consumer link |

Because the macro is inert while the option is OFF, the public API can be annotated **one class at
a time without ever breaking the default build**. The switch is flipped to ON only once the whole
referenced surface is annotated. The macro lives in [`src/emeraude_export.hpp`](../src/emeraude_export.hpp).

## 3. What to annotate

`EMEN_API` marks the boundary of what crosses the DLL edge and is referenced **out-of-line** by
a consumer (`projet-alpha`, tests, tools).

- **Annotate**: a public `class`/`struct` that has out-of-line member definitions (i.e. a `.cpp`).
  Put it on the type — `class EMEN_API Foo` — which exports every member.
  ```cpp
  #include "emeraude_export.hpp"

  class EMEN_API Foo { … };
  EMEN_API bool freeFunction (int);   // out-of-line free function
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
- `EMEN_API` classes may derive from emeraude-base traits that stay unexported **by design**
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
3. For each, annotate the **owning class/function** with `EMEN_API` (bases are NOT pulled in —
   C4275 is disabled, see § 3).
4. Repeat until the link is green. Then make `EMERAUDE_USE_EXPLICIT_EXPORTS=ON` the default and drop
   this guidance to a short "done" note.

Do not annotate blind — let the linker name exactly what is missing, so the exported surface stays
minimal and intentional.

## 6. Exported pimpl — out-of-line destructors, and the header hygiene it unlocks

An `EMEN_API`-exported class forces MSVC to instantiate its **destructor at the class
definition point in every TU**. If such a class holds a `std::unique_ptr< T >` to a
forward-declared (incomplete) `T`, `= default`-ing the destructor in the header instantiates
`std::default_delete< T >` on an incomplete type → hard error. The fix is the **exported pimpl**
pattern: declare the destructor in the header, define it out-of-line in the `.cpp` where `T` is
complete.

```cpp
// Foo.hpp
class EMEN_API Foo final
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

Current users: `Graphics::Renderer`, `Graphics::PostProcessor`, `Graphics::BindlessTextureManager`,
`Graphics::Compute::ProbeConvolver`.

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

### Same mechanism, other special members — delete copy explicitly on move-only holders

The destructor is not the only implicit member the export materializes: MSVC defines **every**
implicitly-declared special member of an exported class at its definition point. So an exported
class holding a move-only member (`std::unique_ptr`, or a container of them) must **delete its
copy operations explicitly** — leaving them implicit is not equivalent.

Why "implicit" is not enough here: an implicit copy assignment is defined as deleted only when a
member's own copy assignment is *deleted or inaccessible*. `std::vector::operator=(const vector&)`
is neither — it is declared for **any** `T`, move-only included, and is not SFINAE-constrained. It
is merely ill-formed **in its body**. The class's copy assignment is therefore a normal (non-deleted)
implicit member, the export forces its definition, and the error lands deep inside
`std::vector::_Assign_counted_range` on `std::unique_ptr::operator=(const unique_ptr &)`:

```
vector(1461): error C2280: 'std::unique_ptr<T> &std::unique_ptr<T>::operator =(const std::unique_ptr<T> &)':
              attempting to reference a deleted function
  ProbeConvolver.hpp(130): see reference to class template instantiation 'std::vector<std::unique_ptr<T>>'
  ProbeConvolver.hpp(138): see the first reference to 'std::vector<...>::operator =' in 'ProbeConvolver::operator ='
```

Read that trace carefully: the "first reference" points at the class's **closing brace** — the
implicit-definition location — not at any call site. **Nothing in the codebase copies the class**;
the export alone is enough.

```cpp
class EMEN_API ProbeConvolver final
{
    public:
        ProbeConvolver () noexcept = default;
        ProbeConvolver (const ProbeConvolver &) = delete;              // required by the export
        ProbeConvolver (ProbeConvolver &&) = delete;                   // already suppressed by the dtor
        ProbeConvolver & operator= (const ProbeConvolver &) = delete;  // required by the export
        ProbeConvolver & operator= (ProbeConvolver &&) = delete;
        ~ProbeConvolver () noexcept;                                   // out of line (exported pimpl)
    private:
        std::vector< std::unique_ptr< Vulkan::DescriptorSet > > m_mipDescriptorSets;
};
```

The move operations were already suppressed by the user-declared destructor, so deleting them
changes no contract — declare them anyway: it is the style of every other exported RAII holder
(`MDI::BatchBuilder`, `Scenes::SceneMetaData`, `Scenes::SceneInstanceTransforms`, …), and it turns
`ProbeConvolver a = std::move(b);` into an error that names the move instead of one that names the
deleted copy overload it silently fell back to.

What immunizes a class is having **any base or member whose copy assignment is genuinely deleted** —
the class's own is then deleted too, and a deleted member is never defined. That covers most engine
services (non-copyable base), but also, *by accident*, some standalone holders:
`Graphics::SharedUniformBuffer` is exported, standalone, and holds two
`std::vector< std::unique_ptr< … > >` — yet never trips this, only because of its
`mutable std::mutex` member. Do not read that silence as "a vector of `unique_ptr` is fine here".

The genuinely exposed case is the standalone exported RAII holder with no such member:
`Graphics::Compute::ProbeConvolver` was the first (2026-08).

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
- [x] **`EMERAUDE_USE_EXPLICIT_EXPORTS` reverted to OFF by default (2026-08)** — the mechanism is
      done and stays in the tree, but `ON` makes the consuming application's **link much longer, on
      every link** (decisive), on top of the standing annotation duty (a missing `EMEN_API`
      breaks the *consumer's* link, one repo away). Neither was worth the engine's own PCH.
      Cost of OFF: that single target's PCH on MSVC. Flipping back is a one-liner.
- [x] MSVC PCH guard: **kept deliberately** — it protects the `EMERAUDE_USE_EXPLICIT_EXPORTS=OFF`
      fallback (anyone reverting to export-all gets the PCH silently disabled instead of `LNK2001`).
      **Moved (2026-08) from emeraude-base's `EnablePrecompiledHeaders.cmake` to the engine's own
      `emeraude_base_target_enable_pch()` call site.** Rationale: this target is the only one in the
      cascade using export-all, so a guard in the shared helper needlessly stripped the PCH from
      every *other* target (base, the consumer libraries, the consumer binaries — all
      `STATIC`/`OBJECT`/executables, never export-scanned) as soon as the option went OFF. It also
      removed an engine-specific concept from the foundation helper.
- [x] **Default flipped back to ON, on MSVC only (2026-08-05)** — the engine crossed the hard PE
      limit of 65535 exported ordinals per DLL (`exports.def` at ~65.8k symbols → `LNK1189`), so
      the OFF/export-all mode of the previous item no longer links on Windows at all. The default
      is platform-conditional in `CMakeLists.txt` (ON under MSVC, OFF elsewhere where the option
      is inert). Verified: full cascade (base → engine DLL → projet-alpha) builds and links with
      zero new `EMEN_API` annotations needed — the 2026-07 annotated surface still covers
      everything the consumer references.