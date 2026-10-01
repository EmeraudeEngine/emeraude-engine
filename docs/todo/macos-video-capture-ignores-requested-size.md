---
id: macos-video-capture-ignores-requested-size
title: macOS video capture delivers 1920x1080 frames while it logs the requested 640x480
status: open
priority: unranked
scope: PlatformSpecific/VideoCaptureDevice.mac.mm
opened: 2026-10-01
tags: [macos, video-capture, avfoundation]
---

# macOS video capture delivers 1920x1080 frames while it logs the requested 640x480

## Why

Measured by the macOS peer (triad 14, 2026-10-01, M2 + its built-in camera): the log says
`Video capture device '…' opened successfully on macOS (640x480)` (the `Core/Video/Capture/Width` / `Height`
defaults), but the three captures saved through KeyP were 1920×1080 RGBA. Either the requested size never reaches
the capture session, or the log reports the request instead of the delivered size. Linux (V4L2) and Windows (MF) both
delivered the requested 640×480.

## What remains

- [ ] Find the cause. Lead, not verified: `open()` calls `canSetSessionPreset:` / `setSessionPreset:` BEFORE
  `addInput:`, on a session that has no input yet. The preset's availability depends on the device, so the call may be
  refused silently, and the session keeps its default (`AVCaptureSessionPresetHigh`). Set the preset after the input is
  added (or pick the device's `activeFormat` inside `lockForConfiguration`), and trace when it is refused.
- [ ] Log the size actually delivered (the first frame's `CVPixelBufferGetWidth` / `Height`), on the three OS.
- [ ] Re-test on macOS: the frames have the requested size, and the log line matches them.

## References

- `src/PlatformSpecific/VideoCaptureDevice.mac.mm` `open()` (the preset block, then `addInput:`).
- Apple, `AVCaptureSession.sessionPreset` and `canSetSessionPreset:`.
- `docs/todo/triad-engine-pass.md` § 14 (6).
