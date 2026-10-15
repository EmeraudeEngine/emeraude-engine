---
id: jobs-owned-by-their-starter
title: Every Base::Thread owner stops its thread through the thread's std::stop_token; remove the hand-made stop flags
status: in-progress
priority: high
scope: the engine's Base::Thread owners (Audio/TrackMixer, Audio/Recorder, Audio/ExternalInput, Console/RemoteListener, Graphics/Recorder, TracerLogger, Core loops)
opened: 2026-10-08
tags: [ave-robustus-ii, concurrency, lifetime]
---

# Every Base::Thread owner stops its thread through the thread's std::stop_token; remove the hand-made stop flags

## Why

Phase P2 of Ave Robustus II, its dedicated-thread half (projet-alpha plan § 4.2). Done on Linux on 2026-10-15:
- every owner stops through the stop token, and its thread member is declared last;
- the `Core` loops use a member `std::stop_source` (owner decision 2026-10-15);
- four defects were fixed on the way.

The rules, the owners, the defects and the Linux proof are in `docs/subsystems/source-tree/07-dedicated-threads-stop-tokens.md`.

## What remains

- [ ] Windows and macOS peers:
  - the base and engine builds;
  - `EmeraudeTrackMixerStress` under TSan on macOS;
  - a 6 s rush (hardware or VP9, voice-over on, `--enable-log=<scratch path>`), with these checks: the files decode,
    the log ends with its footer, 0 VUID, exit 0;
  - a shutdown during a load;
  - on Windows, the downstream application in `OnDemand` mode, its shutdown time.

  Then this item closes.

## ⚠️ Traps

- A thread joining itself aborts in every build (base D2): an owner destroyed from its own thread is a contract fault.
- Enable a test log with `--enable-log=<path>`, never through the setting: the setting rotates the user's journal.
