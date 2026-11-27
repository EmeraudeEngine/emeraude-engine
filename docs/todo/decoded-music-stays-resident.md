---
id: decoded-music-stays-resident
title: Host RAM — every loaded music track is held fully decoded (16-bit PCM) for its whole life
status: open
priority: medium
scope: Audio/MusicResource, Audio track mixer / streaming
opened: 2026-10-03
tags: [memory, audio]
---

# Host RAM — every loaded music track is held fully decoded (16-bit PCM) for its whole life

## Why

`MusicResource` decodes the whole file into `m_localData` (`Base::WaveFactory::Wave< int16_t >`) and keeps it. The
memory census (`Core.ResourcesManagerService.memoryCensus()`, 2026-10-03, Linux) counts **367 MiB for 32 tracks** in
citadel, forest and terrain alike: the playlist loads every track up front (`TrackMixer::loadPlaylist()` asks the
container for each one, which starts its load), and one plays at a time.

Split from the CPU-copy release work (`docs/subsystems/resources/11-cpu-copy-release.md`) by the owner (2026-10-03): streaming decode is a different mechanism
from the release / reload of GPU-uploaded copies.

**Done, 2026-11-27 (step 1, owner decision the same day)**: the SECOND copy is gone — a music resource no longer
copies its samples into OpenAL; the track mixer streams them through a ring of 8 buffers per source
(`docs/subsystems/audio/16-music-streaming.md`). The 367 MiB of decoded samples remain.

## What remains

Step 2, decided by the owner on 2026-11-27 — **the playlist loads only the track being played and the next one**:

1. The playlist holds NAMES (or unloaded handles), not loaded resources: the mixer loads the current track,
   preloads the next one (the shuffle order must then be drawn ahead) and lets the previous one go. Check first what
   keeps a `MusicResource` alive once the mixer drops it (the container's own reference, its unloading policy).
2. **The durations**: `MusicResource::duration()` reads the samples, so a track not loaded has none, and the UI lists
   them. For an audio file TagLib gives the length without decoding; a MIDI or a script music has no header — its
   duration is only known after a first rendering (to decide: shown as unknown, or cached).
3. A far jump (the 17th entry) waits for the decoding: the mechanism exists (`m_loadingTrack`,
   `playPendingTrack()`); a MIDI track renders for seconds.
4. Proof: `memoryCensus()` before and after on the same playlist; the console scenario of doc 16 (play, natural end,
   `next` ×3, `previous`, shuffle, `loadPlaylist`) with 0 error; the TSan stress `tools/stress/TrackMixerStress.cpp`.

Step 3, NOT decided (only if two or three decoded tracks are still too much): decoding on the fly from the file —
incremental decoders in emeraude-base, file reading off the mixer's thread, no answer for MIDI and script musics.

## ⚠️ Traps

- A decoder running on the audio thread must never block it (file I/O off that thread).
- The mixer's event thread feeds the streams every 16 ms under `m_stateAccess`: nothing slow may run under that lock
  (a load, a notification).

## References

- `src/Audio/MusicResource.hpp` (`m_localData`, `memoryOccupied()`), `src/Audio/TrackMixer.cpp` (`loadPlaylist()`,
  `openStream()`), `docs/subsystems/audio/16-music-streaming.md`, `docs/subsystems/resources/11-cpu-copy-release.md`
  § Phase 0.
