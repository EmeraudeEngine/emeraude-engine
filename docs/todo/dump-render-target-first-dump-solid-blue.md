---
id: dump-render-target-first-dump-solid-blue
title: The first dumpRenderTarget of a session sometimes writes a solid blue image
status: open
priority: high
scope: Scenes/Manager.console (dumpRenderTarget), Vulkan/TransferManager (downloadImage), Graphics/Renderer (render-to-texture submission)
opened: 2026-10-01
tags: [console, render-target, transfer, synchronization, windows]
---

# The first dumpRenderTarget of a session sometimes writes a solid blue image

## Why

The Windows peer found this during the triad 9b re-test (2026-10-01, projet-alpha `ad00545f`, engine `1320d529`), in
`offscreen-rendering`. The first `dumpRenderTarget(SecurityCubemap)` of a session, about 30 s after the load, is
sometimes a solid pure blue: a 1024×1024 RGBA image where every sampled pixel is (0, 0, 255), a 6502-byte PNG. A real
dump (the night city) is ~730-790 KB.

- The first dump was blue in 3 of 6 launches (NVIDIA RTX 3060 2/5, AMD 1/1). Every later dump in the same session was
  real (5/5).
- 0 VUID. The `hasBeenRendered()` guard passed: the target had been submitted at least once.
- Not seen on Linux or macOS. Neither of them repeated the first dump on purpose.
- A second Windows series (engine `2544c410`, each launch on an EMPTY `--cache-directory`, so a slower start): 0/6
  blue first dumps (NVIDIA 3, AMD 3). It is intermittent, so a fix needs a reproduction rate first.

The command runs on the console thread. It calls `device()->waitIdle()` and then `downloadImage()`, but nothing stops
the render thread from submitting the next frame between the two. A copy that reads the target in the middle of its
next render pass, or right after its clear, would explain a uniform colour. That is a guess: nothing has measured it.

**2026-11-14, Linux (RTX 3070 Ti, Debug, validation layer loaded):** the first dump of 2 sessions of
`offscreen-rendering`, 25 s after the start line, was a real image both times (1024², about 49 000 distinct colours,
0 pure-blue pixel). The command is now `dumpRenderTarget(nameFragment, layer = 0)` and its file is named
`rt-dump-<timestamp>-layer<N>.png`. ⚠️ `SecurityCubemap` was a 2D target despite its name
(`Toolkit::generateTexture2DRenderer()`), renamed `SecurityTexture2D` the same day: this item is about a plain
single-layer image, and the command to repeat is now `dumpRenderTarget(SecurityTexture2D)`.

**2026-11-14, read in the code (nothing run):**
- the blue is NOT the target's clear colour: the renderer clears every render-to-texture with its own clear colour,
  which is black (`Renderer::setClearColor(Black)`, no other setter in the cascade);
- the demo creates its security camera with `showModel = true`: `Toolkit::generatePerspectiveCamera()` then calls
  `enableVisualDebug(VisualDebugType::Camera)` on the camera's OWN entity — a debug mesh
  (`AbstractEntity::getCameraVisualDebug()`) at the camera's position, so the camera sits inside it. Its colour was
  not established (the default standard material is grey; the mesh may carry vertex colours). Observed the same day:
  a saturated blue shape near the centre of the probe's faces, not identified — the macOS peer saw it too
  (2026-11-15, Apple M6, `dumpRenderTarget(OffscreenRenderingCubemap)` layer 0: a pure-blue blob about 70 px across
  at the centre, on the waterline): it belongs to the scene, on every OS;
- a candidate that does not involve the dump at all follows from it: a frame in which that mesh covers the camera's
  view. It is told from the thread race by one check — the `TV` cuboid of the scene shows the same target, so a
  screenshot taken beside a blue dump shows a blue screen (the target WAS blue) or a normal one (the read was wrong).

## What remains

- [ ] Reproduce: 6+ fresh launches of `offscreen-rendering`, the first dump at ~30 s (Windows first, then Linux).
- [ ] Find where the blue comes from: NOT the clear colour (black, read 2026-11-14). Take a
  `Core.RendererService.screenshot()` beside a blue dump: a blue `TV` screen means the target itself was blue (look
  at the camera's debug mesh, then at what is drawn while resources load); a normal one means the read was wrong
  (the thread race, or memory never written).
- [ ] If the console thread races the render thread, run the download on the render thread (a deferred request served
  at a frame boundary), as `Graphics::FrameCapture` does for screenshots (the copy happens inside the frame), rather
  than with `waitIdle()` from the console thread.
- [ ] Re-test: the first dump is real in 6/6 launches, 0 VUID.

## References

- `src/Scenes/Manager.console.cpp`: the `dumpRenderTarget` command (waitIdle, then `downloadImage`).
- `src/Graphics/Renderer.cpp`: the render-to-texture submission, `setRenderFinished()` / `markRendered()`.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 9b (the peer re-test).
