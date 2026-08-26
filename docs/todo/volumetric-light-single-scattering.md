---
id: volumetric-light-single-scattering
title: Volumetric light — replace the screen-space god rays with a real single-scattering pass
status: open
priority: high
scope: Graphics/PostProcessing
opened: 2026-08-26
tags: [shaders, shadow-map, participating-medium]
---

# Volumetric light — replace the screen-space god rays with a real single-scattering pass

> [!IMPORTANT]
> **Owner decision: the target is a WORLD-SPACE single-scattering march sharing the scene's
> medium, not a tweak of the existing effect.** Two cheaper couplings were offered and refused:
> multiplying the shafts by the fog transmittance (physically backwards — thick fog makes shafts
> MORE visible, not less, because the shaft IS the light scattered by the medium) and scaling
> their gain by the optical depth. The state of the art is unambiguous that the two are one
> system: *"visible rays of sunlight in misty air fall out of the same fog system, as long as the
> fog density and the shadow map are both available to the same compute shader"* — Wronski,
> *Volumetric Fog*, SIGGRAPH 2014 (Assassin's Creed 4); standardised by Hillaire,
> *Physically-based & Unified Volumetric Rendering in Frostbite*, SIGGRAPH 2015.

## Why — what `VolumetricLight` actually is today

The screen-space radial-blur god ray (Mitchell, GPU Gems 3), in two passes. The occlusion mask is
a depth threshold at 0.9999 — literally "this pixel is sky" — and the second pass marches in
SCREEN space toward the sun's projected position with a geometric decay. `density` is a
screen-space step multiplier, `decay` an ad-hoc falloff, `exposure` an arbitrary gain converting
the light's **LUX** into the **nits** buffer. **There is no participating medium in it at all.**

⚠️ It is structurally incapable of putting a shadow INSIDE a shaft: its mask knows sky from
not-sky and nothing about what occludes the volume. That is the test below.

## Ground already in place (do not re-derive)

- **Runtime override keys** (`7dc859af`): `Core/Graphics/VolumetricLight/{Density,Decay,Exposure,
  SampleCount,TemporalAlpha}`, read with `settings.get(key, m_parameters.x)` — an override that is
  ABSENT by default, deliberately breaking the TAA/MotionBlur contract because five demos pass
  hand-tuned values an engine-wide default would silently double.
- **The medium has a scene-level owner** (`0d4b0e12`, `a24c8ecb`, alpha `08c37df`):
  `Scenes::ParticipatingMedium` beside `EnvironmentPhysicalProperties`, reaching effects through
  `FrameContext::medium` the way `skyLuminance` already does. It had to move first: an effect's
  `Parameters` are private to one instance, and `AtmosphericFog` is used by ONE demo while
  `VolumetricLight` is used by EIGHT.
- **Shadow access needs NO work.** Everything is already public:
  `context.lightSet->mainDirectionalLight()` returns the CONCRETE `DirectionalLight`
  (AtmosphericFog already calls it), `shadowMap()` / `cascadeCount()` are public,
  `ViewMatricesCascadedUBO::cascadeViewProjectionMatrix(i)` / `splitDistance(i)` are public behind
  one downcast whose precedent is `DirectionalLight.cpp:428`, and
  `writeCombinedImageSampler(binding, image, imageView, sampler)` takes the shadow map's three
  public accessors with no adapter.
  ⚠️ Verify a claim of absence before designing around it: three greps undid an audit that had
  reported "zero precedent, nothing shadow-related reaches the post-process chain".

## What remains — Lot 1, the pass itself, on ONE scene

- [ ] **A NEW effect beside `VolumetricLight`, not a replacement.** Keeps the A/B alive and lets
  the eight demos migrate one at a time instead of switching together. Target
  `light-and-shadow-debug`, the only scene that declares a medium today.
- [ ] Reconstruct the world-space view ray from depth; march it with a per-pixel **STATIC** dither
  of the march origin (`docs/caution-points.md` — never ship uniform steps).
- [ ] Per step: density from the medium's height profile (`ParticipatingMedium::densityAt()`),
  shadow-map sample with **cascade selection per step** (a single ray crosses cascade boundaries —
  seams if done naively; the screen-space effect never faced this), Henyey-Greenstein phase driven
  by `phaseAnisotropy()`, accumulate in-scattering and transmittance.
- [ ] The 4 cascade matrices are 256 bytes, past the 128-byte push-constant floor: they go in a
  per-frame UBO via `getInputLayout(samplerCount, uniformBufferCount)` +
  `createPerFrameUniformBuffers()`.
- [ ] Half-res + bilateral upsample if the cost demands it (per pixel × steps × cascade PCF).

## The test, defined BEFORE building

**The palm's shadow must appear as a dark lane INSIDE the light shaft.** The current effect cannot
produce that at any setting; a march sampling the shadow map produces it necessarily. Binary,
falsifiable, and it depends on no threshold that could be tuned afterwards to flatter the result.

Protocol: exposure **PINNED** (an auto-exposing camera makes any "X changed the look" reading a
claim about the sensor), the scene's run-to-run noise floor established from two launches of the
SAME binary, and the known sun-facing pose `setPosition(0, 2, 8)` / `lookAt(63, 34, 71)`.

## Known consequences, none of them bugs

- **The combine changes shape.** Today a pure add (`em_Color.rgb += ...`); single scattering needs
  `em_Color.rgb = em_Color.rgb * T + inscatter`, a MULTIPLY inside a shader GENERATED and SHARED
  with the other overlay effects — the ordering of contributions becomes semantically load-bearing.
- **The eight demos lose their calibration.** A physical integral yields nits directly, so
  `exposure` becomes meaningless. Seven of the eight declare no medium and would render nothing
  until they do.
- **The off-screen gate disappears.** `lightOnScreen` kills the current effect when the sun is
  behind the camera; a world-space march has no such restriction, so shafts will appear in poses
  where the owner has never seen any. Expect visual changes that are not regressions.

## Deferred — Lot 2, froxel grid, only if the cost demands it

The Wronski/Hillaire 3D-texture approach amortises the cost but needs ground this engine does not
have: **no 3D STORAGE image exists anywhere** (the single `VK_IMAGE_TYPE_3D` site,
`TextureResource/Texture3D.cpp`, is SAMPLED and CPU-uploaded, and no GLSL declares `image3D` or
`sampler3D`), and **no temporal helper reprojects a VOLUME** (`GIDenoiser` is entirely 2D). The
compute-inside-an-effect precedent is complete in SSR's Hi-Z pyramid but is ~200 lines of private
layout/pipeline/pool per pass, with storage images written by raw `vkUpdateDescriptorSets`. The
industry reached froxels after per-pixel marching, and so should this.

## Open questions for the owner

- [ ] Does the concurrent shadow-map work have further chantiers in view? This pass depends on the
  CSM closely.
- [ ] New effect in the engine from the start, or prototyped first to judge the cost?
