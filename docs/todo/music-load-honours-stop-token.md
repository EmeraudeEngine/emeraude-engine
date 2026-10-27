---
id: music-load-honours-stop-token
title: A music load holds the shutdown until its decode ends
status: open
priority: high
scope: src/Audio/MusicResource.cpp
opened: 2026-10-18
tags: [audio, shutdown, ave-robustus-ii, defect]
---

# A music load holds the shutdown until its decode ends

## Why
Windows peer, 2026-10-18 (engine 39cec761): `TrackMixer` stop / playlistClear / playlistAdd("Kyrandia/Medley") / play,
then `Core.shutdown()` within ~3 s: the shutdown took 6.58 s (normally ~0.8 s) —
`[ResourcesManagerService] stopBackgroundWork(): the 'Music manager' tasks took 4278 ms to stop`. The music load job
decodes and resamples the whole file (`WaveFactory::FileIO::read()`, `Processor::resample()`) without a stop token: the
owner's stop request waits for the end of the decode. Every user who quits while a playlist track loads waits.

## What remains
- The base API exists since 2026-10-27 (base item `wavefactory-decode-stop-token`, closed): `ReadOptions::stopToken`
  (`FileIO::read()` and every format's `readStream()`; libsndfile decodes in blocks of 16 384 frames) and
  `Processor::resample(frequency, stopToken)` (streamed, the samples of the one-shot `src_simple()`; measured stop
  0.35-2.2 ms on a 328 s stereo track). `MusicResource::load()` (and `SoundResource`) pass their job's token; a stopped
  load fails silently (the 2026-10-15 rule for cancelled loads: one summary line).
- Verify with the repro above: the shutdown back to ~0.8 s, the "tasks took … to stop" line under 100 ms.

## References
- `src/Audio/MusicResource.cpp` `load(const std::filesystem::path &)`; `Resources::Manager::stopBackgroundWork()`.
