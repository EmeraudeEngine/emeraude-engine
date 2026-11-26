---
id: graphics-recorder-file-sharing-not-read
title: Graphics::Recorder — whether its file is shared with its encoder thread was never read
status: open
priority: unranked
scope: Graphics/Recorder (its encoder thread, its output file)
opened: 2026-11-26
tags: [concurrency, recorder, video]
---

# Graphics::Recorder — whether its file is shared with its encoder thread was never read

## Why

`Audio::Recorder` shared its output stream and its byte count with its render thread behind an atomic flag alone
(4 data races under ThreadSanitizer, 2026-11-15); since 2026-11-26 that thread owns the file
(`docs/subsystems/audio/11-recorder.md` § *The render thread owns the recording file*). The item that tracked it asked
the same reading for the video side — `Graphics::Recorder`, its encoder thread and its file — and that reading was
not done. `Audio::ExternalInput` was read: it opens its file before its thread starts and completes it after the
join, no fault there.

Not known to be a defect: nothing was observed, nothing was read. Unranked until it is.

## What remains

- Read `Graphics::Recorder`: which members `startRecording()` / `stopRecording()` and the encoder thread both touch,
  and what orders them (a thread start / join, a mutex, a queue).
- A ThreadSanitizer run that starts and stops a VIDEO recording three times (the audio one ran with the video off).
  ⚠️ The software device has no hardware encoder: `Core/RushMaker/ForceCPUEncoding` in the settings copy.
- If a fault is found: the audio recorder's shape (the thread owns the file, requests behind a mutex), ranked high.

## References

- `src/Graphics/Recorder.hpp`, `src/Graphics/Recorder.cpp`.
- `docs/subsystems/audio/11-recorder.md`.
