---
id: vertex-attribute-presence-belongs-to-geometry
title: A vertex attribute's presence is a property of the GEOMETRY, but the shader decides it from the MATERIAL
status: open
priority: medium
scope: Graphics/Material + Graphics/RenderableInstance + Saphir
opened: 2026-08-28
tags: [architecture, material, program-cache, gltf]
---

# A vertex attribute's presence is decided by the material, not by the geometry that owns it

## Why

Surfaced while wiring glTF `COLOR_0` (2026-08-28). `StandardResource`'s codegen asks
`usingVertexColors()` — a **material flag** — and, when true, `VertexShader::synthesizeVertexColor()`
declares an `InputAttribute{VertexAttributeType::VertexColor}`, i.e. a **vertex input attribute**.
But whether that attribute exists in the buffer is decided by the **geometry**
(`Geometry::Interface::vertexColorEnabled()` → `VertexColorType::RGBA` in
`IndexedVertexResource::createOnHardware()`).

So the material declares what only the geometry can provide. If the two disagree, the shader reads
an attribute the vertex input state does not describe — an invalid pipeline.

**glTF makes the disagreement the normal case, not an edge case.** `COLOR_0` is declared per
**primitive** while the material is shared across primitives. Measured on the reference
`Sponza.ktx2.glb`: 67 of 448 primitives declare `COLOR_0`, and **17 of the 22 colour-using materials
are also used by colourless primitives**.

## What was done instead (2026-08-28)

The loader creates a separate `…-vc` material resource for the materials a `COLOR_0` primitive
actually uses, and picks it **per primitive**. That is defensible on its own terms — `UseVertexColors`
changes the material's shader contract, so two materials differing in it genuinely are two
resources, which is the same reasoning that fixed the resource-key collapse the same day. It costs
17 duplicated material resources on Sponza (light objects; the textures stay shared) and pays only
for the attribute the asset actually declares (79.4 MiB rather than 123.1 MiB).

The owner's decision was to ship that and **record the refactor as a possibility to consider**.

## What the refactor would be

Move the decision to the geometry:

- add the geometry's vertex-colour flag to `Renderable::ProgramCacheKey` (it currently carries
  `materialFlags` and **no geometry flags**), so one material used by two geometries yields two
  programs;
- make the codegen consult `Generator::Abstract::geometry(LODIndex)->vertexColorEnabled()` instead
  of the material flag. The generator already exposes the geometry, so the plumbing exists.

Then no material variant is needed, and the same fix generalises to every other optional attribute
(`TEXCOORD_1+`, a second joint set, …), which is the real prize: those gaps will hit exactly this
wall.

## ⚠️ Traps

- **`StandardResource::albedoExpression()` is `const` and is called from sites with no generator in
  hand.** It routes every consumer through `SurfaceAlbedoFinal` when vertex colours are on, so it
  has to know. Making it geometry-dependent means either re-plumbing every caller or stashing
  per-generation state on the material — and hidden generation state on a resource whose programs
  are cached is precisely what the program-cache contract warns against (the cache keys on the
  descriptor layout, never on values). Solve that first; it is the whole difficulty.
- **An UNUSED vertex input attribute is legal**, a shader input with no attribute is not. That
  asymmetry is what makes the current mesh-level attribute + per-primitive material split safe: a
  colourless primitive inside a mesh that carries the attribute simply ignores it.
- Do not "simplify" by enabling the attribute on every geometry of an asset that declares it
  anywhere: measured 43.7 MiB of strictly useless vertex colours on Sponza.

## References

- `Graphics/Material/StandardResource.cpp` (`usingVertexColors()` call sites),
  `Saphir/VertexShader.cpp::synthesizeVertexColor()`,
  `Graphics/RenderableInstance/Abstract.cpp::buildProgramCacheKey()`,
  `Scenes/Loaders/GLTFLoader.cpp` (the `…-vc` variant).
- [`src/Scenes/Loaders/AGENTS.md`](../../src/Scenes/Loaders/AGENTS.md) § *Known gaps (glTF 2.0)*.
