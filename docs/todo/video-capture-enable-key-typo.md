---
id: video-capture-enable-key-typo
title: The video capture enable key is "Core/Audio/Capture/Enable", the audio capture's key
status: open
priority: unranked
scope: SettingKeys.hpp, Graphics/ExternalInput, the settings of existing installs
opened: 2026-10-01
tags: [settings, video-capture, audio-capture]
---

# The video capture enable key is "Core/Audio/Capture/Enable", the audio capture's key

## Why

`SettingKeys.hpp` declares `VideoCaptureEnableKey{"Core/Audio/Capture/Enable"}`, the same path as
`AudioCaptureEnableKey`. Its own comment already calls it a likely typo. As a result, one setting enables or disables
both the webcam (`Graphics::ExternalInput`) and the microphone (`Audio::ExternalInput`), and
`Core/Video/Capture/Enable` is never read. The macOS peer reported it again in triad 14 (2026-10-01).

## What remains

- [ ] Ask the owner whether an existing `Core/Audio/Capture/Enable = true` must keep the webcam enabled after the
  rename (a one-time migration of the value) or not (the webcam then defaults to off).
- [ ] Rename the key to `Core/Video/Capture/Enable`, remove the NOTE in `SettingKeys.hpp`, and update every doc that
  names it (`/usr/bin/grep -rn 'Capture/Enable'` in the cascade's docs).
- [ ] Test: the audio key alone no longer opens the camera, and the video key opens it (KeyP).

## ⚠️ Traps

- projet-alpha never resets its settings: the old key stays in an existing `settings.json`, where it still drives the
  audio capture, which is correct.

## References

- `src/SettingKeys.hpp` (`VideoCaptureEnableKey`, `AudioCaptureEnableKey`), `src/Graphics/ExternalInput.cpp`,
  `src/Audio/ExternalInput.cpp`.
- projet-alpha `docs/plans/triad-engine-pass-report.md` (per-section record) § 14 (6).
