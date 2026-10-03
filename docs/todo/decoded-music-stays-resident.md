---
id: decoded-music-stays-resident
title: Host RAM — every loaded music track is held fully decoded (16-bit PCM) for its whole life
status: open
priority: unranked
scope: Audio/MusicResource, Audio track mixer / streaming
opened: 2026-10-03
tags: [memory, audio]
---

# Host RAM — every loaded music track is held fully decoded (16-bit PCM) for its whole life

## Why

`MusicResource` decodes the whole file into `m_localData` (`Base::WaveFactory::Wave< int16_t >`) and keeps it. The
memory census (`Core.ResourcesManagerService.memoryCensus()`, 2026-10-03, Linux) counts **367 MiB for 32 tracks** in
citadel, forest and terrain alike: the playlist loads every track up front, and one plays at a time.

Split from `cpu-copies-retained-after-upload` by the owner (2026-10-03): streaming decode is a different mechanism
from the release / reload of GPU-uploaded copies.

## What remains

1. Find how the track mixer consumes `m_localData` (whole buffer or chunks queued to OpenAL) and who else reads it
   (`seconds()`, the playlist's durations).
2. State of the art first: streamed decoding (a ring of a few OpenAL buffers refilled from the decoder, the way
   OpenAL Soft's examples, SDL_mixer and FMOD stream music), versus keeping the compressed file in memory and decoding
   on the fly.
3. Owner decision on the approach; the duration must survive as metadata.

## ⚠️ Traps

- A decoder running on the audio thread must never block it (file I/O off that thread).

## References

- `src/Audio/MusicResource.hpp` (`m_localData`, `memoryOccupied()`), `docs/todo/cpu-copies-retained-after-upload.md`
  § Phase 0.
