---
id: material-merge-lot7-remove-basicresource
title: Material merge — Lot 7, remove BasicResource (a chantier, not a cleanup)
status: open
priority: medium
scope: Graphics/Material
opened: 2026-08-12
tags: [material, shaders, blast-radius]
---

# Material merge — Lot 7, remove BasicResource

**Owner decision D6 (2026-08-12):** `StandardResource` is the single concrete material in the
engine; "quick cases" become configurations/presets of the one class (a colour-only Standard binds
zero texture samplers already — descriptor layouts are keyed per declared components,
`Interface.cpp:88-97`).

A four-way investigation found the original estimate far too optimistic: this is a chantier.

## Prerequisites already BUILT (do not redo)

- **Vertex colours on StandardResource.** Only the vertex-shader half existed (`:2451-2464`) and
  was unreachable. Added `enableVertexColor()` and, per the owner's contract, a SINGLE folded
  albedo variable `SurfaceAlbedoFinal = albedo * svPrimaryVertexColor` declared once at the top of
  the fragment shader, with `albedoExpression()` routing the light generator, `fragmentColor()`
  and the alpha test through it.
- **`emissionMultiplier()` on StandardResource.** It was implemented ONLY by BasicResource:
  deleting Basic would have deleted the engine's entire unlit-emission mechanism, and the skybox
  would render its raw [0,1] texel — near-black under photometric exposure, with ZERO logs.
- **`isComplex()` already met** — feature-derived on StandardResource (`:1628-1631`). Remaining
  nit: fold the ad-hoc normal-mapping term at `SceneRendering.cpp:212-219` into the predicate.
- **The UNLIT flag** (owner-approved): `MaterialFlagBits::UnlitEnabled = 1U << 17` +
  `Interface::isUnlit()` + `StandardResource::enableUnlit()`; the decision point moved into
  `SceneRendering::isLightingRequested()` (scene light set enabled + instance asked for lighting +
  the material does not veto it). glTF `KHR_materials_unlit` semantics: content carrying its own
  radiance is never re-lit, whatever the instance asked for.
- **The two API gaps**: `setAlbedoComponentFromRenderTarget(TextureInterface, enableAlpha)` (no
  resource dependency — the CALLER owns the lifetime and must outlive the material), and
  `setAlbedoComponent(texture)` now taking an `enableAlpha` flag (default false) and propagating
  `PrimaryTextureCoordinatesUses3D` for cubemap albedos.

## What remains

- [ ] Migrate the blast radius: **103 engine + 39 app code sites, 15 mesh JSONs** carrying
  `"MaterialBasicResource"`.
- [ ] Then delete the legacy lighting machinery: `m_usePBRMode` and its 14 branches, the
  (n+2)/(8π) normalisation, the Gouraud per-vertex path, ~898 lines of
  `LightGenerator.PerFragment*.cpp`, plus ~48 doc lines.

## ⚠️ Traps — the dangerous part, all SILENT

- A surface migrated off Basic **without `enableVertexColor()` loses its colours with NO log**
  (`VertexBufferFormatManager` emits `declareJump(VertexColor)` and discards the attribute).
- `FastJSON::getValidatedStringValue` returns nullopt **with no trace**, so the 15 mesh JSONs
  would silently fall back to another material class.
- **`Shininess` means two different things**: Basic reads it as a RAW Blinn-Phong exponent,
  Standard as a glossiness [0,1] — migrating the 3,918 material files visibly changes them.
- **Sprite alpha differs**: Basic gives the texture alpha priority over the uniform opacity;
  Standard replaces the whole alpha.
- `BasicResource`'s `DynamicColorEnabled` gate is deliberately NOT ported: it existed only because
  Basic's default diffuse is Grey; Standard's default albedo is White, so multiplying
  unconditionally IS the contract.
- The light generator concatenates swizzles onto the name (`+ ".rgb"`, `+ ".a"`), so it must
  receive a NAME, never a compound expression — an unparenthesised one still compiles and silently
  swizzles the wrong operand.
- ⚠️ MDI eligibility and bindless enablement are gated on `isLightingEnabled()` /
  `useEnvironmentCubemap()`: swapping the cheap material changes which draws take the MDI path and
  which sampler-binding model they use. Verify both before and after.
- ⚠️ **CMake must be RECONFIGURED** after a file rename/removal (`GLOB_RECURSE` caches the file
  list) — full entry in `docs/caution-points.md` § Build / Compiler.
