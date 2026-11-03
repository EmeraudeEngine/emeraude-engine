---
id: temporal-discontinuity-contract
title: One temporal-discontinuity contract instead of three per-pixel large-change detectors
status: in-progress
priority: unranked
scope: Graphics (TAA, GIDenoiser, SkyEnvironment, Renderer, LightSet, Scene)
opened: 2026-11-02
tags: [performance, architecture, temporal, intel]
---

# One temporal-discontinuity contract instead of three per-pixel large-change detectors

## Why

TAA's large-change test (3c25c355) and the GI anti-lag's pooled test and reset (e2a7320e, cf4c4432) guessed, every
frame and for every pixel, a lighting step the engine already knows: Intel iGPU +0.92 ms and +0.83 ms a frame (item
`intel-igpu-frame-rate-halved`). Owner decision 2026-11-02: one signal raised by the code that knows, each consumer's
validated test run only on the frames it marks.

## Done (Linux, 2026-11-03, working tree)

The contract as designed and decided: graphics doc 13-12 § 43 (the signal `FrameContext::temporalDiscontinuity`, 25
frames; the monotonic epoch; the producers; the per-channel ×2 step rule; TAA's and the anti-lag's two pipeline
variants), with the owner's decisions (2026-11-02/03: producers, transport, name, window 8 → 25 on measurement,
per-channel steps, a flickering light marks, the unsignalled-change trade-off, TAA kept in the contract after the eye
A/B at the quay) and the Linux validation (RTX 3070 Ti: `city` jumps on both lanes, GI on and off, 25 and 40 FPS;
parked `relief` / `city` / `sponza` / `labyrinth`; `sky-lab`; 0 VUID). Gate `commit` all OK, clang `-Wdocumentation`
clean on the touched TUs.

## What remains

- The peers' round 1 (engine ef0bb2bc) is done on the three OS and the Intel gain measured (§ 43).
- **One slow TAA run out of 4 on AMD** (§ 43): unmarked-rate convergence after a jump that landed inside the capture.
  No readout said whether a frame was marked, so a missed mark could not be told from a GI switch still pending.
  Owner decision 2026-11-03: each frame's mark goes into `temporalCapture`'s JSON (`FrameCapture::FrameMetadata`);
  then LinuxB-PA repeats R2 until a jump lands inside a capture again, and reads the marks of those frames.
- Then close the item (the knowledge is in § 43 already).

## ⚠️ Traps

- **One unexplained freeze, not reproduced** (Linux, 2026-11-03): during a `labyrinth` GI-noise run (layer on, TAA
  off), right after `PostProcess.setLightingMode(ScreenSpace)`, the log stopped for good (the logic thread's per-tick
  probe lines too); the instance had to be killed. Its logs were overwritten by the next runs. Not reproduced in 24
  lane switches (layer on and off) and 12 full GI-noise runs under a stall watchdog that would have dumped every
  thread with gdb. The probe of that build traced while holding `LightSet::m_lightsAccess` (removed since); the
  Tracer forwards nothing to the console, so no deadlock through it is proven. If it comes back: `gdb -p <pid> -batch
  -ex "thread apply all bt 30"` before killing, and keep the log.
- `labyrinth`'s GI-noise runs vary from launch to launch (the dying lamp in the 16-frame window): compare medians
  (§ 43).

## References

- Graphics doc 13-12 § 43 (the contract), § 06 (the TAA resolve), § 16 (the anti-lag), § 41 (the FAST forms);
  graphics doc 42 § The environment; item `intel-igpu-frame-rate-halved`.
