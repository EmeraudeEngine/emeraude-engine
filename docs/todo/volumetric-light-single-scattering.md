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

- **Runtime override keys** (`7dc859af`): `Core/Graphics/PostProcessing/VolumetricLight/{Density,Decay,Exposure,
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

## Built 2026-09-08 — the pass exists, and the owner decisions behind it

`Graphics::Effects::Atmosphere::VolumetricScattering` (+ its two shared GLSL rule headers) is in
the engine and runs on `light-and-shadow-debug`: the world-space march executes, samples the
cascaded shadow map per step, Vulkan validation is silent (**zero VUID**, layers active), and the
hand-authored GLSL — both spliced macros included — compiles at RUNTIME, which the C++ build cannot
prove.

**Owner decisions, do not re-litigate:**

- **It takes `EffectSlot::Fog`, not `EffectSlot::VolumetricLight`.** A slot holds ONE enabled effect
  (`disableSlotSiblings()`; only `EffectSlot::Custom` is multi-occupant), so it is **structurally
  impossible** to integrate the same medium's extinction twice — the alternative was trusting
  discipline not to enable AtmosphericFog alongside it. It also leaves the `VolumetricLight` slot's
  documented "pure add, never samples the chain" contract TRUE for the legacy god rays, which keeps
  the A/B alive exactly as this item wanted.
- **Both medium integrators stay.** AtmosphericFog becomes the cheap analytic tier (no shadow map),
  the march the physical one. Choosing between them is the slot's normal behaviour.
- **In the engine from the start, no throwaway prototype** — the plumbing (layout, pipeline,
  per-frame UBO, output target) is the same either way; ContactShadows is the ~200-line precedent.
- **It follows AtmosphericFog's composition pattern**, not VolumetricLight's: it owns its pass and
  composites `sceneColor * T + inscatter` internally, so it emits NO `combineContribution()` snippet
  and the shared generated combine is untouched. The concern this item raised about a multiply
  inside the shared combine therefore does not arise — and note that multiplies were already the
  norm there anyway (`SSAO`, `RTAO`, `ContactShadows` all emit `em_Color.rgb *= …`).

**Reuse, per the owner's standing rule that reuse is what grows the base class** — two conventions
were extracted rather than re-typed a fifth time:

- [`../../src/Graphics/Effects/Shared/CSMSamplingGLSL.hpp`](../../src/Graphics/Effects/Shared/CSMSamplingGLSL.hpp)
  — THE cascaded-shadow-map sampling rule for post-process effects, and the home of the
  **"outside the map = lit"** convention this item asked to keep in one place. It carries the C++
  `CSMCascadeBlock` in the same file as the GLSL that reads it, so the std140 layout cannot drift
  from its reader.
- [`../../src/Graphics/Effects/Shared/MarchDitherGLSL.hpp`](../../src/Graphics/Effects/Shared/MarchDitherGLSL.hpp)
  — THE march-origin dither. Four effects still open-code the identical expression;
  migrating them is [`march-dither-single-source.md`](march-dither-single-source.md).

## ⚠️⚠️ Two traps this work paid for

- **The source term takes the sun's ILLUMINANCE in lux, NOT
  `ParticipatingMedium::resolveLuminance()`.** That accessor is the contract of an effect which
  composites a LUMINANCE directly (AtmosphericFog's model) and returns E/pi with no authored
  luminance. Feeding it to an integral that already carries its own sigmaS and phase function is
  dimensionally wrong and under-reports by exactly pi. Corollary to keep rather than paper over: an
  authored `setLuminance()` has **no meaning** for this integrator — it overrides a composited
  result, and this pass computes the result.
- **`light-and-shadow-debug` could not exercise this item as written.** It owns the testbed's only
  `ParticipatingMedium`, but its sun was a CLASSIC shadow map, so the pass no-op'd on its own guard.
  No scene had both a medium and a CSM sun. The sun is now cascaded (owner decision:
  `2048, 4, 0.5F, csmScale 80` — 10 km view distance / 80 = 125 m of covered depth, matching the
  100 m box the classic map covered). ⚠️ That switch also broke the scene's option 0, which passed
  resolution 0 to the CSM overload and collapsed the whole frame — see
  [`csm-light-zero-resolution-kills-lighting.md`](csm-light-zero-resolution-kills-lighting.md).
- ⚠️ The medium's `phaseAnisotropy` had **never been set by anything**, because nothing read it
  (AtmosphericFog has its own `inscatterExponent`). Its 0.0 default is ISOTROPIC, which a physical
  integrator renders as a uniform glow with no shaft at all. The scene now declares 0.7.

## ✅ Acceptance test PASSED (2026-09-08) — and two more traps it paid for

Pose `setPosition(-8, 1, -9)` / `lookAt(18.7, 14.3, 17.7)`, which puts the palm's canopy exactly on
the camera→sun line (sun direction is `-normalize(0.5, 0.25, 0.5)`; the item's original pose
`(0, 2, 8)` → `(63, 34, 71)` looks AWAY from the palm at `(0, 0, -1)` and cannot show it). With the
legacy god rays and the lens flare disabled — the march alone, zero VUID — **the palm's fronds cut a
dark silhouette INTO the haze above the trunk**, and the cube, the sphere and the sprite each cast
their own dark lane through the volume. Binary criterion met. The height profile reads correctly
too: a bright band near the ground, a dark sky above, as `densityAt()` predicts.

- ⚠️⚠️ **Cap the march by the medium's OPTICAL reach, never by `maxDistance` alone.** The step is
  `marchLength / steps`, and for a sky pixel `marchLength` IS `maxDistance` — 10 km on this scene. At
  32 steps that is **312 m per step** against a transmittance that falls to 1/e in 67 m: the whole
  contribution lands inside the first step and the origin dither (a fraction of one step, by
  design) scatters that single sample anywhere over 312 m per pixel. Owner saw it as *"a pattern of
  black dots everywhere in the fog"*. It is **undersampling, not noise** — no denoiser fixes it.
  Now capped at six extinction lengths of the density at the camera (T = 0.25 %). Measured on the
  haze region, |Δ| between neighbours **2.50 → 0.73** at unchanged luminance (162.6 → 165.5).
- ⚠️⚠️ **The two slots DOUBLE-COUNT the sun's in-scattering.** `EffectSlot::Fog` (this march) and
  `EffectSlot::VolumetricLight` (the legacy god rays) are different slots, so the slot exclusivity
  does NOT protect, and both add scattering from the same sun. Measured at the acceptance pose:
  frame mean **143.91 with both, 42.51 with the march alone** — the white-out the owner reported on
  the sun-facing pose was mostly the LEGACY effect stacked on top. **Owner decision pending**: a demo
  that takes the march should drop the god rays (the eight-demo migration this item planned), or
  the two slots need a mechanical exclusion the table does not express today.

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

- [x] **Does the concurrent shadow-map work have further chantiers in view? ANSWERED 2026-09-08 —
  no blocking one.** The CSM rework was verified done against the code and retired: all four root
  causes are fixed and pushed (`c7eef938`, `8c6417f0`, `882e5f29`, `864e6582`), and so are texel
  snapping, the rotation-invariant fit, inter-cascade blending and the shader-derived per-cascade
  bias. **This pass is therefore UNBLOCKED.** One shadow item remains —
  [`light-space-transform-single-source.md`](light-space-transform-single-source.md) — but it
  touches the CLASSIC map's light-space recipe, not the cascade matrices this pass consumes, so it
  does not gate the march. ⚠️ It does own the *"outside the map = lit"* convention, so agree on that
  convention with it rather than open-coding a fifth copy here.
- [ ] New effect in the engine from the start, or prototyped first to judge the cost?
