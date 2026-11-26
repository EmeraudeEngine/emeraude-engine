---
id: audio-file-output-shared-by-two-devices
title: A mix recorded to a file is written by two output devices at once
status: open
priority: unranked
scope: Audio (Manager, Recorder), measurement method
opened: 2026-11-26
tags: [audio, measurement, tooling]
---

# A mix recorded to a file is written by two output devices at once

## Why

A test instance sends its sound to a file with OpenAL Soft's wave writer (`ALSOFT_DRIVERS=wave`), which makes a sound
a measurement. The engine opens two output devices on the selected output — the audio manager's own
(`Audio::Manager`) and the passthrough of the recorder (`Audio::Recorder`: the game mixed on a loopback device, played
on a second device) — and in that mode both write the same file, each at its own position: the manager's silent
blocks overwrite 32 to 41 % of the samples of the passthrough's mix (`docs/subsystems/audio/15-the-listeners-medium.md`
§ Limits, proven on 2026-11-26). Nothing is lost on a real device.

Not a defect of the engine's sound: a limit of the measurement. Only ratios between launches can be read on such a
mix, and they spread by up to 2.5 dB from a launch to the next.

## What remains

A mix written whole. The ways seen, none chosen (the owner decides):
- a launch argument that keeps the recorder's passthrough off for a run, the game then mixed on the manager's device
  (whether the engine already runs that way without the loopback extension is to read in `Audio::Recorder`);
- the manager not keeping a device of its own open while the passthrough carries the game — a change of the audio's
  structure, to weigh on a real device too (what that device is for today is not established);
- measuring from the recorder's own capture instead of the wave writer.

Then: measure the listener's medium again on a whole mix (audio doc 15, the Linux mid-band reading that sits 2 dB
above the other machines), and correct projet-alpha's `test-instance` skill.

## References

- `docs/subsystems/audio/15-the-listeners-medium.md` § Limits.
- `src/Audio/Recorder.cpp` (the loopback device, the passthrough device), `src/Audio/Manager.cpp` (its own device).
