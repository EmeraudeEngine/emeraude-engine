---
id: audio-recorder-stream-shared-with-its-thread-behind-a-flag
title: Audio::Recorder — the output stream and its byte count are shared with the recording thread behind an atomic flag alone
status: open
priority: high
scope: Audio/Recorder (startRecording(), stopRecording(), renderThreadFunc()), Core::startAudioVideoRecording() / stopAudioVideoRecording()
opened: 2026-11-15
tags: [concurrency, defect, tsan, audio, recording]
---

# Audio::Recorder — the output stream and its byte count are shared with the recording thread behind an atomic flag alone

## Why

ThreadSanitizer, 2026-11-15, a run in which an audio / video recording was started (`Core::startAudioVideoRecording()`
on the main thread): 4 data races, all between `Audio::Recorder::startRecording()` (main thread) and
`Audio::Recorder::renderThreadFunc()` (the recorder's own thread) — on `m_streamByteCount`
(`Recorder.cpp`, `m_streamByteCount += …` against `m_streamByteCount = 0`) and inside `m_outputFileStream` (its buffer and
its file, `std::ofstream::write()` against `open()` and the header write).

The mechanism, by reading the code:

- `startRecording()` opens `m_outputFileStream`, writes the placeholder header, resets `m_streamByteCount`, then sets
  `m_recording` (a `std::atomic< bool >`). The thread tests `m_recording.load(std::memory_order_relaxed)` and then writes
  the stream and the count. A relaxed load orders nothing: the plain members are not published by that flag (a flag does
  not protect non-atomic data by itself — publication is "write all, store-release; load-acquire, then read").
- `stopRecording()` is worse than a memory order: it clears the flag, then seeks, patches the two header sizes and
  closes the stream — while the thread, which tested the flag before, may still be inside `write()`. Two threads in one
  `std::ofstream` at once: a corrupted file at best, undefined behaviour in general. The count it patches the header
  with may miss the block being written.

Not observed as a crash; the run that showed the races ended for another reason before `stopRecording()` ran.

## What remains

- An owner decision on the shape. Two that fit the cascade's doctrine (a class owns what its thread uses):
  1. **The thread owns the file.** `startRecording()` / `stopRecording()` only post a request (a path under a mutex and
     an atomic state: idle → starting → recording → stopping → idle); the thread opens the stream, writes, patches the
     header and closes it. `stopRecording()` waits for the thread's acknowledgement (bounded), so the file is complete
     when it returns. No member is touched by two threads.
  2. **A mutex around the stream and its count**, taken by the thread for each block and by `startRecording()` /
     `stopRecording()` for the whole open / patch / close. Smaller; the audio thread then takes a lock every 5 ms.
- Whatever the shape: `m_dataSizePos`, `m_outputPath` and `m_streamByteCount` follow the stream's protocol; the
  `Tracer` line of `stopRecording()` reads the count after the thread stopped writing.
- The same reading for the video side (`Graphics::Recorder`: its encoder thread and its file) — not read yet.
- Proof: ThreadSanitizer on a run that starts and stops a recording three times: 0 report naming `Audio::Recorder`; the
  WAV header's sizes equal the file's (a check of the two fields against the file length).

## ⚠️ Traps

- The recorder's thread is the loopback RENDER thread (`ALC_SOFT_loopback`): it renders the mix whether a recording runs
  or not. Blocking it (a long lock, a slow disk under the lock) starves the audio output.
- `stopRecording()` is also reached from the shutdown path: the bounded wait must not hang a stop.

## References

- `src/Audio/Recorder.cpp` — `startRecording()`, `stopRecording()`, `renderThreadFunc()`; `src/Audio/Recorder.hpp` —
  `m_recording`, `m_outputFileStream`, `m_streamByteCount`.
- `docs/plans/ave-robustus-ii.md` (projet-alpha) — a class owns, stops and waits for every effect it starts.
