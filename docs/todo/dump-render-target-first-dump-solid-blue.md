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

## What remains

- [ ] Reproduce: 6+ fresh launches of `offscreen-rendering`, the first dump at ~30 s (Windows first, then Linux).
- [ ] Find where the blue comes from: the target's clear colour, the scene's background, or memory never written.
- [ ] If the console thread races the render thread, run the download on the render thread (a deferred request served
  at a frame boundary), as `Graphics::FrameCapture` does for screenshots (the copy happens inside the frame), rather
  than with `waitIdle()` from the console thread.
- [ ] Re-test: the first dump is real in 6/6 launches, 0 VUID.

## References

- `src/Scenes/Manager.console.cpp`: the `dumpRenderTarget` command (waitIdle, then `downloadImage`).
- `src/Graphics/Renderer.cpp`: the render-to-texture submission, `setRenderFinished()` / `markRendered()`.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 9b (the peer re-test).
