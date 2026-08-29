---
id: indirect-diffuse-before-translucency
title: Indirect diffuse must be applied BEFORE the translucent pass, so a transmissive surface transmits it
status: open
priority: high
scope: Graphics/PostProcessing
opened: 2026-08-29
tags: [vulkan, post-processing, rtgi, ssgi, transmission]
---

# Indirect diffuse must be applied BEFORE the translucent pass

## Why

The indirect-diffuse effects (SSGI, RTGI) run in `executeIndirectPostProcessEffects()`, which the
Renderer calls **after** `renderTranslucentGB()` and after the material-facing grab pass. So the
grab a transmissive material samples carries the scene **without** its indirect diffuse, and what
is seen through a glass is lit by the direct pass alone.

That was survivable while the raster ambient pass added its own diffuse IBL to everything. It is
no longer: since the indirect-diffuse OWNERSHIP contract (Aug 2026), an enabled RTGI switches that
raster leg OFF (`iblDiffuseWeight = 0`), so a surface seen through a glass now gets **no indirect
diffuse at all** — neither from the raster, which gave it up, nor from RTGI, whose G-buffer at that
pixel belongs to the glass in front.

**Measured** on `asset-loader --demo-options 11,0,1,0,0,0` (ChronographWatch under
AutumnFieldPureSky), the three GI modes captured in ONE run through KeyPad9 so the framing is
identical, mean level of the region vs the no-GI control:

| region | SSGI | RTGI |
|---|---|---|
| opaque case + strap | −0.46 | **−4.64** |
| dial, behind the crystal | −0.41 | **−6.11** |

The opaque delta is legitimate — traced sky WITH visibility is darker than an unoccluded ambient
probe. The extra ~1.5 on the dial is this defect: it lost the raster leg and got nothing back.
⚠️ It reads small **only because this dial is black** (albedo ~0.03, so its diffuse response is
tiny whatever lights it). Behind a crystal, a light-coloured subject loses its whole indirect fill.

## What remains

Split the indirect chain in two phases around the translucent pass:

1. **Phase A — indirect diffuse + AO** on the opaque G-buffer, composited back into the scene
   colour target;
2. the material-facing **grab pass** (it now captures a fully-lit opaque scene);
3. the **TranslucentGB** pass (a transmissive surface transmits the indirect diffuse);
4. **Phase B — reflections (SSR/RTR), contact shadows, volumetric light, TAA** as today.

Points to settle while doing it:

- `PostProcessor::executeIndirectPostProcessEffects()` currently runs ONE ordered list against the
  post-process grab. Phase A has to write back into `m_sceneTarget`'s colour attachment before the
  Renderer's grab-pass blit — a new entry point on the post-processor, not a reordering of the
  existing list.
- Which phase an effect belongs to must be **declared by the effect**, never hardcoded in the
  Renderer (a `phase()` accessor next to `providesIndirectDiffuse()`).
- ⚠️ **Measure the cost of the extra blit** before and after (`GPUProfiler` scope), on the 3070 Ti.
  A second full-resolution copy per frame is not free and the owner's GPU budget is deliberate.
- The reflections stay in phase B **on purpose**: the water of `WaterWorld` reflects through
  SSR/RTR from the translucent pass. Reflections OF the opaque scene seen THROUGH a glass stay
  missing — the same limitation HDRP has without transparent SSR. Separate item if it ever matters.

## ⚠️ Traps

- ⚠️⚠️ A TranslucentGB material must NOT be re-lit by phase A: its own ambient pass already ran,
  and its G-buffer footprint replaces the opaque surface behind it (translucent materials get
  `blendEnable = FALSE` on the G-buffer attachments — the "flat water reflections" fix). Phase A
  runs before that pass, so this is naturally true today; keep it true.
- ⚠️ The raster diffuse IBL leg is gated by the view UBO's `iblDiffuseWeight`, which is
  scene-wide. If a demo ever needs the glass path lit while RTGI owns the rest, the exemption
  belongs to the SHADER GENERATION of the TranslucentGB programs, not to a second UBO value.

## References

- `dependencies/emeraude-engine/src/Graphics/AGENTS.md` § "Indirect-diffuse OWNERSHIP" (the
  contract, its measurements and the state of the art it follows).
- `dependencies/emeraude-engine/src/Graphics/Renderer.cpp` — `renderFrameWithInternal()`, the grab
  pass and `renderTranslucentGB()` around `executeIndirectPostProcessEffects()`.
- `dependencies/emeraude-engine/src/Graphics/PostProcessor.cpp` — the combine-group scheduler.
