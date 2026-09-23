---
id: rushmaker-capture-inside-the-frame
title: RushMaker copies the swap-chain image after the present; move it onto the in-frame capture hook
status: open
priority: unranked
scope: Graphics/Recorder, Graphics/Renderer
opened: 2026-09-23
tags: [capture, video, vulkan, validation]
---

# RushMaker copies the swap-chain image after the present; move it onto the in-frame capture hook

## Why

`Core::renderingTask()` calls `Recorder::captureAndSubmitFrame()` AFTER `Renderer::renderFrame()` returned, i.e.
after `vkQueuePresentKHR`: both paths (the CPU VP9 slots, `submitGPUCopy()` / `submitTransferQueueCopy()`, and the
hardware H.265 snapshot, `captureHardwareFrame()`) copy `m_frames[m_acquiredImageIndex]`, an image the presentation
engine owns until it is re-acquired, in a separate submit. It is the ownership defect `screenshot()` had, fixed on
2026-09-23 by `Graphics::FrameCapture` (`src/Graphics/AGENTS.md` § 9b). Owner decision the same day: screenshot and
temporal capture first, RushMaker in its own step.

## What remains

- Record the recorder's copy at the same point of `Renderer::renderFrame()` (after the last pass, before
  `commandBuffer->end()`, image acquired), into its existing 4 async slots / GPU snapshot slots, guarded by the
  frame's fence instead of its own submit.
- Keep its wall-clock pacing (`shouldCaptureFrame()`) and its non-blocking harvest.
- Verify on macOS (MoltenVK reports the defect, Linux does not): 0 `UNASSIGNED` during a recording.

## ⚠️ Traps

- The hardware path's `VideoFrameConverter::dispatchConversion()` calls `queue->waitIdle()` on the GRAPHICS queue
  from the encoder thread: moving the copy does not remove that stall.
