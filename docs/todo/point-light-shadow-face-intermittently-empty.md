---
id: point-light-shadow-face-intermittently-empty
title: A face of a point light's shadow cube is read entirely at the far plane, most of the time in Debug
status: open
priority: high
scope: Scenes/Scene.rendering (shadow caster lists, the isCubemap() branches), Graphics/RenderTarget/ShadowMap, Graphics/Renderer (shadow passes)
opened: 2026-11-15
tags: [shadows, point-light, cubemap, depth, debug-build]
---

# A face of a point light's shadow cube is read entirely at the far plane, most of the time in Debug

## Why

Found with the depth read-back (`dumpRenderTarget(BulbShadowMap, layer)`, launch switch `--enable-depth-readback`) in
`light-and-shadow-debug`: a static point light ("Bulb"), a cube of 1024² in `D32_SFLOAT`, a static scene (a ground,
a sphere, a cube, a palm tree that sways).

- **Linux, Debug** (2026-11-15, RTX 3070 Ti, engine `6e101587`, validation layer loaded, 0 `VUID-`): face 5 read
  six times between 8 and 16 s after the start line — ENTIRELY at the far plane (1 048 576 of 1 048 576 pixels at 1)
  five times, right once (depths 0.106236 to 1, 542 859 pixels at the far plane). The same face read entirely at 1
  on 2026-11-14 already (Debug, one read), and was written down then as "layer 5, entirely at 1" without a question.
- **Linux, Release** (the same day): the six faces right in one pass — face 0: 566 073 pixels at the far plane;
  1: 631 244; 2: 844 734; 3: 0; 4: 631 244; 5: 542 859.
- **It is not a wrong read, and not the whole cube**: face 3 (downward, the ground: no pixel at the far plane) read
  eight times in Debug and eight times in Release in the same conditions — right 16 times of 16 (depths 0.126213
  to 0.302912). A read racing the render thread, or a cube not drawn at all, would empty that face too.
- **Windows, Release** (the peer, RTX 3500 Ada, engine `a909e0d2`): face 2 entirely at 1 (one read). **macOS** (Apple
  M6): face 2 right (depths 0.172937 to 1, 845 117 pixels at the far plane — the palm's crown seen from below), as
  on Linux Release (844 734).
- When it is right, face 5 holds the ground and objects on 48 % of its pixels: "entirely at the far plane" means
  NOTHING was drawn in that face, not even the ground it shares with face 3.

So, on some frames — most of them in a Debug build — a face of the cube is cleared and receives no caster. If the
light pass samples the map at such a frame, the bulb casts no shadow in that direction: not looked at on screen yet.

## What remains

- [ ] Read how the faces of a cubic shadow map are drawn (one pass per face or multiview) and where each face's
  casters come from (`Scene.rendering.cpp`, the `renderTarget->isCubemap()` branches): what can leave one face
  without any draw while another has them.
- [ ] Measure per face: the six faces in series, Debug and Release (the dumps of one layer must be more than a second
  apart: the file name holds the second, and a second dump of the same layer within it is refused "already exists").
- [ ] Look at the bulb's shadows on screen while a face is empty (the sun and the spot switched off: KeyPad0,
  KeyPad2): is the shadow missing, does it flicker?
- [ ] Fix, then re-test: every face right in 20 reads of 20, Debug and Release, on the three OS.

## ⚠️ Traps

- The read-back is the instrument here and it was checked (face 3, 16 of 16): do not start from "the dump is
  wrong". The solid blue image of `offscreen-rendering` was the same lesson (`docs/render-targets.md` § A uniform
  dump is first of all a picture: the picture was real).
- One read proves nothing either way: the face is right about once in six in Debug.
- Each depth dump leaves a 4.2 MB `.pfm` in the captures folder.

## References

- `docs/render-targets.md` (the depth read-back, its measurements).
- projet-alpha `src/Builtin/LightAndShadowDebug.cpp` (the scene, the keys).
