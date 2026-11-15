---
id: point-light-shadow-face-intermittently-empty
title: A face of a point light's shadow cube is read entirely at the far plane in some launches
status: open
priority: high
scope: Graphics/Renderer (shadow passes, their submissions), Graphics/RenderTarget/ShadowMap (the multiview cube), Vulkan/TransferManager and Scenes/Manager.console (the read-back)
opened: 2026-11-15
tags: [shadows, point-light, cubemap, depth, read-back]
---

# A face of a point light's shadow cube is read entirely at the far plane in some launches

## Why

Found with the depth read-back (`dumpRenderTarget(BulbShadowMap, layer)`, launch switch `--enable-depth-readback`) in
`light-and-shadow-debug`: a static point light ("Bulb", at (-2.2, 3.5, 1.2), range 20 m), a cube of 1024² in
`D32_SFLOAT`, a sun (4 cascades) and a spot beside it, a static scene (a ground, a sphere, a cube, a palm that sways).
When it is right, face 5 (-Z: the ground, the sphere) has 542 859 of its 1 048 576 pixels at the far plane; "empty"
below means all of them at 1 — nothing drawn in that face, not even the ground.

Every read of face 5 so far, Linux, RTX 3070 Ti, 0 `VUID-` (2026-11-15 unless said):

| Launch | Build | Lights on | Reads of face 5 | Result |
|---|---|---|---|---|
| 2026-11-14 | Debug | all | 1, after a read of face 0 | empty |
| the six faces once, 0 to 5 | Release | all | 1, about 27 s after the start line | right |
| face 5 alone, 0.7 to 2 s apart | Debug | all | 6, from 8 s after the start line | 5 empty, 1 right (the 5th) |
| the six faces in turn, three rounds | Debug | all | 3, about 8.4 s apart | 3 right (the 18 reads right) |
| face 5 alone, 1.3 s apart | Release | all | 8, from 8 s after the start line | 8 empty |
| six launches | Release | the bulb alone (sun and spot switched off 5 s before) | 2 per launch, 6 s apart | 12 right |
| six launches, faces 5, 5, 3, 0, 4, 5 | Release | all | 3 per launch | launch 2: 3 empty, and face 4 empty, faces 3 and 0 right; the five others: all right |
| launches until a bad one (the 4th) | Release | all, then the bulb alone | see below | see below |

**The bad launch, read in full** (Release, the 4th of that series). The six faces with the three lights on, in
pixels at the far plane: 566 073, 631 244, 844 734, 0, **1 048 576, 1 048 576** — faces 0 to 3 right, faces 4 and 5
empty. Then the sun and the spot switched off (KeyPad0, KeyPad2), six seconds, and the six faces again: 566 073,
631 244, 844 734, 0, **631 244, 542 859** — all right.

Beside them: face 3 (downward, the ground only) read alone 8 times in Debug and 8 times in Release, all lights on —
16 right of 16. The Windows peer (RTX 3500 Ada, Release, all lights) read face 2 once: empty; the macOS peer read
it once: right, as Linux Release did.

What this says, and does not:
- It is NOT Debug against Release (the first reading of it, and this item's first title): both builds give both
  results.
- It is decided per LAUNCH: 2 bad launches of the 16 Release launches above with the three lights on, bad or good
  for all their reads.
- **It is the MAP, not the read.** In one and the same launch the same command reads faces 4 and 5 empty, then right
  once the two other lights are off: the read-back returns what the image holds. In a bad launch the LAST TWO views
  of the cube's multiview pass (4 and 5: +Z and -Z) receive no draw while the sun and the spot render; the four
  first are right.
- So in such a launch the bulb casts no shadow towards +Z and -Z (the sphere's shadow, here). On screen it was NOT
  measurable with the three lights on: the bad launch's screenshot differs from a good one's no more than two good
  ones differ (0.006 % of the pixels over 12 levels: the animated sprite) — at the sun's exposure the bulb weighs
  almost nothing. With the bulb alone the map is right, so there is nothing to see either: the defect hides in
  exactly the conditions where it exists.
- Which of the sun and the spot matters is not known (both were switched off together), nor whether switching them
  on again breaks the map again. "Four right views" and "four cascades of the sun" is a coincidence to test, not a
  finding. On Windows the peer read face 2 empty: the count of right views may differ there.

How the cube is drawn (read in the code): ONE multiview pass for the six faces (`ShadowMap` enables multiview, the
vertex shader takes its view-projection from a UBO indexed by `gl_ViewIndex`), from ONE caster list for the whole
cube (`Scene.rendering.cpp`, the `isCubemap()` branches skip the per-volume test) — a face cannot lack casters the
others have. Each shadow map is its own command buffer and its own submission (`Renderer::renderShadowMaps()`), on a
queue of the graphics family (`Device::getGraphicsQueue()` rotates among them). The read-back runs on the console
thread: `Device::waitIdle()`, then a copy submitted on `getPrimaryQueue(High)` with two layout transitions of the
image around it (`TransferManager`), while the render thread goes on submitting.

## What remains

- [ ] A RenderDoc capture of a frame of a bad launch (`/renderdoc-capture`; classify the launch first with
  `dumpRenderTarget(BulbShadowMap, 5)`): the cube pass draw by draw — are views 4 and 5 drawn and clipped, or not
  drawn; which program, which view UBO content (the six matrices) at that draw.
- [ ] In a bad launch: switch the sun off alone, then the spot alone, then both on again, reading faces 4 and 5 after
  each step.
- [ ] What differs between a bad and a good launch at creation: the order the three shadow maps are created and
  rendered in, the order the casters' shadow programs and pipelines are generated in (item
  `program-cache-key-codegen-inputs-audit`).
- [ ] Fix at the cause; then a way to SEE it: a scene where a point light's shadow towards +Z or -Z is measurable
  beside a sun (a dim sun, or a camera exposure set for the bulb).
- [ ] Re-test: every face right in 20 reads of 20, in 6 launches of 6, Debug and Release, on the three OS.

## ⚠️ Traps

- Neither "the dump is wrong" nor "the map is wrong" is established: the solid blue image of `offscreen-rendering`
  was read for six weeks as a wrong read and was a real picture (`docs/render-targets.md` § A uniform dump is first
  of all a picture) — and here this item first concluded the opposite, too early, from one control.
- One read, and one launch, prove nothing either way: classify the launch first (several reads of face 5), and
  repeat over several launches.
- A measurement made with the sun and the spot switched off does not describe a launch with the three lights.
- Two dumps of one layer within the same second: the second is refused (item `dump-render-target-same-second-refused`).
- Each depth dump leaves a 4.2 MB `.pfm` in the captures folder.

## References

- `docs/render-targets.md` (the depth read-back, its measurements).
- projet-alpha `src/Builtin/LightAndShadowDebug.cpp` (the scene, the keys).
