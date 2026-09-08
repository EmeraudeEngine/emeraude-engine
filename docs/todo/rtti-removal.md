---
id: rtti-removal
title: Build the engine without RTTI — replace every dynamic_cast, typeid, type_index and std::any
status: open
priority: unranked
scope: cascade-wide (Resources, Scenes, Graphics/Renderable, Vulkan/TextureInterface, Observer payload)
opened: 2026-09-08
blocked-by: [rtti-free-observer-payload]
tags: [architecture, cpp, rtti, cross-platform]
---

# Build the engine without RTTI — replace every dynamic_cast, typeid, type_index and std::any

## Why

**Owner goal (2026-09-08):** the cascade already builds without exceptions; RTTI is the last
C++ feature to remove, in the engine and in projet-alpha. The knob is emeraude-base's
`EMERAUDE_DISABLE_RTTI` (`-fno-rtti` / `/GR-`). CEF itself already builds with
`-fno-rtti` on Linux/macOS and `/GR-` on Windows, so the engine is the layer that resists.

Beyond the owner's discipline argument, the measurable benefits are binary size (typeinfo and
mangled type-name strings) and the removal of **cross-casts to mixin traits on hot paths**: an
Itanium-ABI `dynamic_cast` to a non-primary base walks the hierarchy comparing `type_info`, and
`Graphics/RenderableInstance/Abstract.cpp:503/607/629` do it per renderable instance.

Inventory (2026-09-08, excluding comments):

| Construct | Sites | Where |
|---|---|---|
| `dynamic_cast` / `dynamic_pointer_cast` | 31 in 17 files | see the groups below |
| `typeid` | 34 registrations + 2 lookups | `Resources/Manager.cpp:535-568`, `Resources/ResourceTrait.hpp:215,247` |
| `typeid` exact-type comparisons | ~25 | `Scenes/AbstractEntity.cpp:313-395` (link/unlink dispatch) |
| `std::type_index` | 1 map key + 2 virtuals | `Resources/Manager.hpp:520`, `ResourceTrait.hpp:315,336` |
| `std::any` / `any_cast` | 36 files, 24 casts | Observer payload (contract owned by emeraude-base) |

Not affected: `std::function` (only `target()` needs RTTI), `std::variant`, `shared_ptr` other
than `dynamic_pointer_cast`. Third-party headers compiled in our TUs are clean: nlohmann json,
VMA (documented RTTI-free), imgui, magic_enum, glslang/SPIRV-Tools public headers, fastgltf,
ktx, meshoptimizer, reproc++; tinyusdz's `typeid` lives in its Python bindings, not included.
Prebuilt archives compiled with RTTI stay linkable: we derive from none of their polymorphic
classes.

## What remains

### Group A — cross-casts to a mixin trait → virtual accessor on the interface (returns `nullptr` by default)

- [ ] `Renderable::Abstract::skeletalData()` (and const) overridden by `MeshResource` and
      `MultiLayerMeshResource` (`class … final : public Abstract, public SkeletalDataTrait`).
      Sites: `RenderableInstance/Abstract.cpp:503,607,629`, `Saphir/Generator/Abstract.hpp:652`,
      `Scenes/Component/Visual.cpp:73`, `Graphics/Renderable/MeshResource.cpp:141`,
      `MultiLayerMeshResource.cpp:199`, `Scenes/Loaders/GLTFLoader.cpp:569`,
      `FBXLoader.cpp:358`, `Scenes/Viewers/ModelViewer.cpp:180`, plus `Loaders/AGENTS.md:906`
      (documented pattern to rewrite) and 6 projet-alpha sites that follow automatically.
- [ ] `Vulkan::TextureInterface::asResource()` → `Resources::ResourceTrait *`. Sites:
      `Scenes/Component/AbstractLightEmitter.cpp:309,393`, `PointLight.cpp:219`,
      `SpotLight.cpp:202`, `DirectionalLight.cpp:212`.
- [ ] `RenderTarget::Abstract::asTexture()` → `Vulkan::TextureInterface *`. Sites:
      `Scenes/Scene.rendering.cpp:1055`, `Scenes/Manager.console.cpp:1232`.
- [ ] `AVConsole::AbstractVirtualDevice::asRenderTarget()` → `RenderTarget::Abstract *`. Site:
      `Scenes/Scene.rendering.cpp:1920` (the `shared_ptr` is then rebuilt with the aliasing
      constructor, never a second ownership).
- [ ] Ground/sea level interfaces expose `asRenderable()` → `Renderable::Abstract`. Sites:
      `Scenes/Scene.cpp:55,57`, `Scenes/Scene.hpp:1322,1415`.

### Group B — down-casts to a concrete class → `ClassId` check + `static_cast`, wrapped as `as< T >()`

- [ ] `Component::Abstract::as< T >()` on top of `isComponent(T::ClassId)`
      (`Scenes/Component/Abstract.hpp:385`). Sites: `Scenes/Viewers/ModelViewer.cpp:472,495`
      (Visual, NodeAnimation), `Material/StandardResource.cpp:2644,2797` (`Component::Texture`,
      a material component — same idea, its own base).
- [ ] `Resources::ResourceTrait::as< T >()` for `Graphics/BindlessTextureManager.cpp:424`
      (AnimatedTexture2D) and `MeshResource.cpp:354` / `MultiLayerMeshResource.cpp:555`
      (`IndexedVertexResource`).
- [ ] `AbstractEntity::getComponent< T >()` / `getComponents< T >()`
      (`Scenes/AbstractEntity.hpp:498,527`) rewritten on `as< T >()`; **owner decision 2**
      fixes whether they keep the is-a semantics of `dynamic_pointer_cast`.

### Group C — `typeid` exact-type dispatch

- [ ] `Scenes/AbstractEntity.cpp:313-395` (`linkComponent` / `unlinkComponent` specific
      notifications): replace the `typeid(*pointer) == typeid(X)` chain with `isComponent(X::ClassId)`
      (or a per-leaf `classUID()` compare, decision 3). The comment there already notes that
      `std::any` typeinfo consistency across the DLL boundary forced this code into the `.cpp`.

### Group D — `Resources::Manager` type key

- [ ] Key `m_containers` (`Resources/Manager.hpp:520`) on `Base::Hash::FNV1a(resource_t::ClassId)`
      (all 34 registered resource classes carry `static constexpr auto ClassId`), computed at
      compile time in `ResourceTrait::container< T >()`; `getContainerInternal()` takes a `size_t`.
- [ ] **Assert uniqueness at registration** (`emplace` result checked): two identical `ClassId`
      strings would alias two containers in silence — the same failure shape as the glTF bench
      resource-key bug (`87d13636`).

### Group E — Observer payload

- [ ] After `rtti-free-observer-payload` lands: mechanical migration of the 36 files / 24
      `any_cast` sites to the base type (`std::shared_ptr< Component::* >`, `std::string`,
      `std::shared_ptr< Node >`, `std::vector< std::filesystem::path >`, `int`, pairs).

### Verification and flip

- [ ] Configure with `-DEMERAUDE_DISABLE_RTTI=On`, build the whole cascade (Linux first), base
      unit tests green, Vulkan validation layers ON for the runtime check.
- [ ] Windows build: `/GR-` turns every leftover `dynamic_cast`/`typeid` on a polymorphic type
      into **C4541**, an error under `/WX` — it is the residue detector. macOS after.
- [ ] Measure before/after: binary size of `libemeraude-engine` and `projet-alpha`, count of
      `_ZTS*`/`_ZTI*` symbols (`nm`), and a frame-time A/B on a skeletal scene (Group A hot path).
- [ ] Docs before deletion: `docs/cpp-conventions.md` (forbidden constructs and their
      replacements), `src/Scenes/AGENTS.md`, `src/Scenes/Loaders/AGENTS.md:906`,
      `src/Resources/AGENTS.md`, `src/Graphics/AGENTS.md`; projet-alpha's
      `.claude/rules/coding-style.md` and `docs/cef-integration.md` § Two compile policies
      (the `list(REMOVE_ITEM CEF_COMPILER_FLAGS /GR-)` stays: the base remains the single
      authority, it merely agrees with CEF from now on).

## Owner decisions pending

1. Payload type (A/B) — recorded in `rtti-free-observer-payload`.
2. **Is-a semantics of `as< T >()` / `getComponent< T >()`.** `dynamic_pointer_cast` accepts
   derived classes (asking for `AbstractLightEmitter` returns any light). Recommendation:
   `isComponent()` implemented as `own ClassId || Parent::isComponent(id)` so the hierarchy is
   preserved. Exact-match only is cheaper but changes behaviour.
3. **One type mechanism per component.** `Component::Abstract::classUID()` returns the hash of
   `"Component"` for every component (`Scenes/Component/Abstract.hpp:143`), i.e. not per leaf,
   while `isComponent()` does a `strcmp` on `ClassId`. Recommendation: make `classUID()` per
   leaf and implement `isComponent` on it (one integer compare), retiring the `strcmp`.

## ⚠️ Traps

- **`typeid(T)` on a non-polymorphic type is also an error under GCC/Clang `-fno-rtti`**: the
  static `typeid(resource_t)` in `ResourceTrait.hpp` must go too, not only the polymorphic uses.
- **Never replace a cross-cast with a `static_cast`** (`Abstract*` → `SkeletalDataTrait*` are
  unrelated subobjects): the accessor pattern is the only correct form.
- Group A accessors must be **`virtual` on the interface, overridden by the concrete class**,
  never a flag plus an unchecked cast: `HasSkeletalAnimation`
  (`Renderable/Abstract.hpp:82`) is a rendering flag, not a type witness.
- A Linux-only green build proves nothing for `std::any` (libstdc++ compiles it without RTTI);
  the Windows build is part of the acceptance.

## References

- Base item (blocker): `emeraude-base/docs/todo/rtti-free-observer-payload.md`.
- projet-alpha item: `projet-alpha/docs/todo/actor-trait-queries-without-rtti.md`.
- Existing type-identity contracts: `Base::ObservableTrait::classUID()/is()`
  (`emeraude-base/src/ObservableTrait.hpp:117,126`), `Component::Abstract::getComponentType()/
  isComponent()` (`src/Scenes/Component/Abstract.hpp:377,385`), `Base::Hash::FNV1a` (constexpr).
- Compile policy: `emeraude-base/CMakeLists.txt:223,258,296` (the three `EMERAUDE_DISABLE_RTTI`
  generator expressions).
