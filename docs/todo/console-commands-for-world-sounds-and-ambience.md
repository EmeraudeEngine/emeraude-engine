---
id: console-commands-for-world-sounds-and-ambience
title: Console — no command plays a world sound nor drives a scene's ambience
status: open
priority: unranked
scope: Scenes/Manager.console (the scene service), Audio/Ambience
opened: 2026-11-26
tags: [console, mcp, testability, audio]
---

# Console — no command plays a world sound nor drives a scene's ambience

## Why

Two things of a scene's sound cannot be driven from the remote console nor from the MCP server, so they cannot be
measured nor tuned by ear on a running scene:

- **A world sound** (`Scene::playSoundAt()`): `getWorldSoundCount` reads them, nothing plays one. Its path to the
  listener across a sea's surface (`Scene::soundPathAt()`, audio doc 15) was proven for a sound emitter only, with a
  temporary probe.
- **The ambience** (`Scene::ambience()`): its loop's gain, a start, a stop. projet-alpha's `water-world` plays its
  sea through it with a gain nobody could tune while listening; a measurement of another sound had to switch it off
  with a temporary probe.

## What remains

Typed commands, to PROPOSE to the owner before any code (the engine API exists for each):

| Command | Arguments | Calls | State read / written |
|---|---|---|---|
| `playSoundAt` | `sound, x, y, z [, gain, pitch]` | `Scene::playSoundAt()` | one of the scene's 16 world voices; the name checked always (unknown → refused) |
| `setAmbienceLoop` | `sound, gain` | `Audio::Ambience::setLoopSound()` then a restart of the loop | the ambience's loop and its gain; an empty name removes the loop |
| `startAmbience` / `stopAmbience` | — | `Scene::startAmbience()`, `Audio::Ambience::stop()` | the ambience's state |
| `getAmbience` | — | — | answers the loop's name and gain, the state, the channel count |

- `Audio::Ambience` has no way to change the loop's gain while it plays: to add with the command.
- `tools/mcp-conformance.py` after the change.
- Then the proof left open: a world sound played under the sea while the listener is in the air, and the reverse, on
  the mix written whole.

## References

- `docs/subsystems/audio/15-the-listeners-medium.md` § *Measured — the path, the exemption, the submerged sound*.
- `src/Scenes/Manager.console.cpp` (`getWorldSoundCount`, `setSeaSound`), `src/Audio/Ambience.hpp`.
