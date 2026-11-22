---
id: deferred-resolve-projected-spot-lights
title: The deferred light resolve shades a light that projects a texture (it is a forward pass per light today)
status: in-progress
priority: unranked
scope: src/Graphics/DeferredLightResolve.*, src/Scenes/LightSet (isDeferredPunctualLight), docs/subsystems/graphics/38
opened: 2026-11-22
tags: [lighting, deferred, colour-projection, performance]
---

# The deferred light resolve shades a light that projects a texture

## Why

Owner decision 2026-11-22 ("projection in the deferred resolve"). `LightSet::isDeferredPunctualLight()` keeps a light
with a colour projection OUT of the resolve: it stays a forward pass over every lit batch. Measured on a downstream
testbed's car at night (two headlights, each a spot projecting a dipped-beam pattern over a 50° cone, 100 m of
reach; Linux, RTX 3070 Ti, 1280×720 window-less, no validation layer, an idle machine, the GPU profiler):
`ScenePass` 2.5 → 4.0 ms and the frame 8.5 → 10.0 ms with the two lights (+1.5 ms, 17 % of the frame); a
flashlight of the same class at 30 m of reach, +0.28 ms. The resolve shades 160 lights in 0.12 ms.

## What remains

The design read on the code (nothing written yet):

- **The texture table.** The resolve's pipeline has ONE set (the G-buffer, the light buffer, the tile masks). It needs
  the bindless table as a second set: `Renderer::bindlessTextureManager().descriptorSetLayout()` in the pipeline
  layout and `descriptorSet()` bound at the record, the way `CloudShadowMap.cpp` does it. No table (an overlay-only
  target) = the projected lights stay forward.
- **The data.** `DeferredLightData` stays 64 bytes (the tile culling and the hot loop read it). A parallel buffer,
  one entry per light slot (`MaxLights` × 80 bytes = 80 KiB a frame in flight): the matrix VIEW → projection
  coordinates (the light's published matrix × the frame's inverse view, built in `prepare()`), the bindless index,
  the boost. `cone.z` of the light says which kind (0 none, 1 a picture, 2 a cubemap; for a cubemap the matrix is the
  inverse view's rotation, to bring the light → pixel direction back to world space).
- **The shader.** After the cone and the N·L test: the forward pass's `projectionColor`, term for term (the picture
  at the projection coordinates, the boost). The loop runs in non-uniform control flow: no implicit derivative, so an
  explicit mip level — the pixel's footprint against the texel's, from the two distances — or the base level.
- **The predicate.** `isDeferredPunctualLight()` no longer refuses a projected light when the table is bound; a
  point light's ANIMATED cubemap stays forward (a cubemap array). The forward skip follows the snapshot by itself;
  a material the resolve does not shade (clear coat, translucent) keeps the forward pass, which projects already.
- **The proof.** The same pose with the setting `DeferredPunctualLights` on and off: the picture within the noise
  (the forward pass is the reference), `ScenePass` back near its cost without the lights, 0 VUID; then the three OS
  (macOS has the resolve, MoltenVK's bindless limits apply).
- Graphics doc 38 (the predicate's row, a section for the projection), the changelog.

## ⚠️ Traps

- The CPU half and the GPU half of the eligibility must agree every frame (graphics doc 38): a light the resolve
  takes without its projection, or the forward pass keeps as well, is counted wrong or twice.
- The published matrix of a spot is refreshed only while it casts a shadow or projects
  (`SpotLight::updateLightSpaceMatrix()`).

## References

- `docs/subsystems/graphics/38-deferred-punctual-lights.md`, `docs/subsystems/saphir/14-color-projection-code-generation.md`.
- `src/Graphics/DeferredLightResolve.cpp` (`DeferredLightData`, the resolve's fragment shader, `prepare()`),
  `src/Saphir/LightGenerator.PBR.cpp` (`projectionColor`), `src/Graphics/CloudShadowMap.cpp` (the table in a raster
  pipeline).
- `docs/subsystems/scenes/16-lightset-background-derived-lighting.md` § *A traced light entry carries the light's
  colour projection* (the same term, in the traced lanes).
