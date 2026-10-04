---
id: mrt-single-pass-deferred
title: MRT single-pass deferred — 4 draws per object down to 1
status: open
priority: high
scope: Graphics/Renderer
opened: unknown
tags: [performance, deferred, gpu]
---

# MRT single-pass deferred — 4 draws per object down to 1

## Why

P1 of the Graphics optimization roadmap (`src/Graphics/AGENTS.md` § Optimization Roadmap): the
G-buffer is filled through multiple subpasses, **4 draws per object**, where a single-pass MRT
G-buffer needs **1**. Expected impact: −75 % geometry draws. UE5 does it in one pass.

## Verified state (2026-09-08)

- **The MRT attachments already exist.** `Renderer.cpp:1143-1176` creates normals, material
  properties, albedo and velocity, and the shader generator detects the layout from
  `colorAttachmentCount` against a **FIXED** MRT layout. The groundwork is not missing.
- **What is still multi-pass is the LIGHTING, and that is where the draws go.** `RenderPassType`
  (`src/Graphics/Types.hpp:90-111`) enumerates 16 values — `AmbientPass` plus one family per light
  type and shadow/colour-map combination — and an object is re-rasterised **once per pass**. So
  "4 draws per object" is ambient + 3 lights, not four G-buffer subpasses. Going to 1 means
  resolving the lighting in screen space from the G-buffer, not merely merging attachment writes.

## Measured cost (2026-10-04, Sponza with its 22 lamps)

The evidence this item lacked. projet-alpha `--load-demo sponza`, launch pose untouched, alpha
10bbd1fc / engine 89ba38fa / base 1e37f0b, Release, engine GPU profiler
(`Core.RendererService.getGPUTimings()`), lamps switched with `SceneManager_PointLight_setEnabled`,
every scene effect bypassed (`PostProcess.setLightingMode("None")` + `bypassSceneEffects(true)`):

| Lamps on (point, radius 10 m, no shadow) | 0 | 1 | 11 | 22 |
|---|---|---|---|---|
| `ScenePass`, RTX 3070 Ti, 2880×1620 (ms) | 8.9 | 10.9 | 27.0 | 45.0 |

- **Linear, ≈ 1.65 ms per lamp**: 36 of the 45 ms are lamp passes. Full frame 112 ms (9 FPS,
  GPU-bound: CPU frame 2.3 ms without validation, 23 ms with it — validation does not move the
  GPU). Apple M2 (macOS-PA, screen-space lane): `ScenePass` 265 ms of a 343 ms frame.
- The RT traces do NOT depend on the lamps (RTGI trace 37 ms, RTR trace 16 ms either way): the
  unshadowed lamps cost no shadow ray.
- View list 290 batches / 10.5 M triangles.

**The lamp cost is GEOMETRY, not fill** (2026-10-04, three OS): the per-lamp cost barely follows
the pixel count — RTX 3070 Ti 1.43 ms per lamp at 1440×810 against 1.64 at 2880×1620 (≈ 87 %
pixel-independent); Apple M2 7.6 ms at 1280×720 against 9.2 at 2560×1440 (≈ 75 %); RTX 3060
Laptop `ScenePass` 86 ms at 640×360 against ~100 at 1280×720 (≈ 86 %). A depth pre-pass, an
ambient-first order or a per-light scissor only cut the fragment share — ≈ 5 ms of the 36 here.
The geometry is re-submitted per lamp, and its heaviest part is **`IvySim_Leaves`** (alpha-tested
ivy cards spread over the whole building, so every 10 m lamp touches its bounding sphere): hiding
it alone (`SceneManager_Visual_setDrawDistance` near 50 000) takes `ScenePass` from 45.1 to
28.3 ms at 22 lamps, 8.9 → 7.9 ms at 0 lamps — ≈ 0.75 ms per lamp pass. The other meshes still
cost ≈ 0.95 ms per lamp. `Core/Graphics/LOD/EnableAutomaticGeneration = true` brings the view
from 10.5 to 7.8 M triangles but `ScenePass` only from 45 to 40 ms: the ivy's levels stall
(LOD 1, 2 and 3 all at 2 134 434 triangles — disconnected cards; the "N %" of the "LOD n ready"
log is the TARGET ratio, not the achieved one).

Why each lamp costs that much, read in the code (2026-10-04):

- `Scene::renderLightedSelection()` (`Scenes/Scene.rendering.cpp`) is **batch-major**: batch i gets
  its ambient pass, then one draw per directional light, then one per touching point light, before
  batch i+1 is drawn. The depth buffer is never complete when a light pass runs, and there is no
  depth pre-pass, so every lamp pass shades hidden fragments too (light pipelines: depth LEQUAL,
  no write, additive).
- The lamp cull is `light->touch(instanceWorldSphere)` with the sphere of the **whole renderable**
  (`renderable()->boundingSphere()`), not of the batch's sub-geometry: a large Sponza mesh is
  touched by almost every 10 m lamp.
- The opaque sort key is pipeline → material → geometry, distance only in the low 16 bits: not
  front-to-back. Alpha-tested materials `discard` in every pass (early-Z lost for them).
- Each lamp pass re-runs the full material fragment code (texture fetches) into up to 6 colour
  targets.

## What remains

**Owner decision (2026-10-04):** go straight to the deferred resolve. The in-multi-pass fixes
(ambient-first order, per-light scissor, per-sub-geometry light bounds) were weighed and dropped:
they only touch the fragment share (≈ 5 ms of the 36 measured above). Scope: the punctual lights
WITHOUT shadow nor colour projection are resolved from the G-buffer; the forward light passes stay
for shadowed / projected lights and for translucent lists.

**Design decisions (owner, 2026-10-04)** — G-buffer audit in § Measured cost's sources:

1. **Eligibility per MATERIAL, at compile time**: lit, opaque, no clear coat / sheen / subsurface /
   anisotropy / iridescence / transmission / IOR / KHR specular (F0 = 0.04). An eligible material
   sets a bit in the reserved low nibble of `materialProperties.R`; every other material keeps the
   forward passes for every light (rendering strictly unchanged). Sponza's 37 materials carry no
   material extension: all 35 opaque ones are eligible (the ivy is OPAQUE).
2. **Lights**: point + spot without shadow nor colour projection first; line lights next. The sun
   stays forward (its shadow) — moving it is the following step toward one geometry pass.
3. **Roughness: the G-buffer's SAA-widened value** (`normals.a`), the one SSR/RTR already read —
   no new channel; accepted difference against the raw forward value on smooth curved surfaces.
4. **Per-pixel light loop first** (distance² early-out), measured; tiled compute culling
   (Andersson 2011, Lauritzen 2010) only if the resolve exceeds ~1 ms on Sponza or scales badly.
5. **Placement: split the scene render pass** — opaque (CLEAR), end the pass, depth to
   `DEPTH_STENCIL_READ_ONLY_OPTIMAL` (SAMPLED usage added), fullscreen resolve additive into the
   HDR colour, then translucent in a LOAD variant. Before the pre-translucency chain
   (RTGI write-back, contact shadows). Subpass input attachments rejected (collides with
   `vulkan-sync2-dynamic-rendering`). ⚠️ As built: NOT the post-process LOAD variant (render-pass
   incompatible, 20 VUIDs) but a RESUME variant of the CLEAR pass (`docs/caution-points.md`).
6. **Forward fallback** wherever the target has no G-buffer (render-to-texture, swap-chain path
   without post-processing).
7. Light data: proposed as the `LightSet` SSBO made one copy per frame in flight. ⚠️ As built: the
   resolve owns its OWN buffer per frame in flight, filled from each light's PUBLISHED block of the
   frame's render state (the forward UBO's bytes) in view space — the shared RT SSBO and its consumers
   are untouched; its own race is item `rt-light-ssbo-rewritten-while-in-flight`.

**Step 1 DONE (2026-10-04, engine 8c461d12; Linux, macOS M2, Windows NVIDIA + AMD validated — numbers in the reference doc)** — point and spot lights:
`Graphics::DeferredLightResolve`, reference `docs/subsystems/graphics/38-deferred-punctual-lights.md`.
Sponza (RTX 3070 Ti, 2880×1620): `ScenePass` 45.5 → 13.8 ms (resolve 0.64 ms), frame 111 → 80 ms,
0 VUID; night fidelity and its decomposition in the reference doc. A/B: `setDeferredPunctualLights`.

What remains, in order:

- [ ] The forward lamp passes LEFT on Sponza (Linux 4.0 ms, M2 ≈ 29 ms) are the cypress's `LeafSpring` leaves:
      a glTF `BLEND` material, so translucent, drawn forward by design (hidden: 4.0 → 0.7 ms). Leaves authored as a
      coverage mask belong in `MASK` (alpha test → opaque → deferred; `docs/subsystems/graphics/31-alpha-coverage-a-mask-is-not-a-gradient.md`):
      an owner decision on the asset or on a loader policy, not a resolve change.
- [ ] Line lights (LTC) in the resolve (the LTC tables as one more binding).
- [ ] The SUN (directional, CSM shadow) in the resolve: the step that brings an eligible batch to ONE
      geometry pass (ambient only) — the item's title. Sponza's `ScenePass` at 0 lamps is 8.9 ms for
      2 passes per batch.
- [ ] Shadowed / projected point and spot lights in the resolve (shadow maps sampled per light).
- [ ] Tiled light culling only if the per-pixel loop is measured to need it.

## ⚠️ Measurement protocol (mandatory, `src/Graphics/AGENTS.md`)

1. Before: `/renderdoc-capture` on the test scene, record metrics.
2. Implement.
3. After: `/renderdoc-capture` again, compare.
4. Visual output identical or better (read the thumbnail).
5. Report the delta in draw calls, render passes, vertex throughput.

No blind optimization.
