---
id: audio-sources-outlive-their-context
title: The audio manager's sources and the resources' buffers are deleted after their context and device are gone
status: in-progress
priority: high
scope: engine Audio (Manager, Recorder, TrackMixer, Buffer, Source, SoundResource, MusicResource), Resources Container
opened: 2026-10-16
tags: [audio, openal, shutdown, ave-robustus-ii]
---

# The audio manager's sources and the resources' buffers are deleted after their context and device are gone

## Why

Every shutdown logged, on Linux and macOS, `AL_INVALID_NAME` at `GlobalReleaseFlush` ("There was unread problem with
AL during execution !"). The recorder's game context (loopback device) is current when the audio manager creates its
default source and pool; the recorder was terminated first, so these sources were deleted on the wrong context, the
pool after every context, the track mixer's two sources in its destructor, and every sound / music buffer once the
resource manager terminated (after both devices were closed). OpenAL Soft reported "254 Sources not deleted" and
"187 Buffers not deleted".

## Done (Linux, 2026-10-16, not pushed)

The fix and its proof are recorded in engine `docs/subsystems/audio/06-critical-points.md` (owner decisions: an
outstanding `SourceRequest` is a contract fault; the audio manager releases the resources' buffers itself):
- A/B with `ALSOFT_LOGLEVEL=3 ALSOFT_LOGFILE=<file>`: 254 sources / 187 buffers not deleted before, none after;
- the startup fault-injection sweep (ASan, 34 runs): 0 AL warning, 0 ASan error, 0 leak;
- projet-alpha with music playing: exit 0, 0 VUID, 0 AL warning;
- `EmeraudeTrackMixerStress` under TSan: 28 307 calls, 0 report;
- GCC and clang builds 0 warning; clang-tidy 0 new finding on the changed lines.

## What remains

- macOS and Windows: a projet-alpha shutdown with music playing and the recorder up, the OpenAL Soft log free of
  "not deleted", no "unread problem with AL"; a rush with the voice-over still valid.
- Then delete this item.

## References

- `src/Audio/Manager.cpp` (`onTerminate()`, `releaseSources()`, `releaseResourceBuffers()`), `src/Audio/TrackMixer.cpp`
  (`onTerminate()`), `src/Audio/Buffer.*` (`destroy()`), `src/Audio/PlayableInterface.hpp` (`releaseBuffers()`),
  `src/Resources/Container.hpp` (`resourcesSnapshot()`).
- Engine `docs/subsystems/source-tree/08-startup-fault-injection.md` (where it was seen).
