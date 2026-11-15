---
id: console-commands-to-play-a-sound-and-set-a-light-projection
title: Console — no command makes an emitter play a sound, nor gives a light its colour projection
status: open
priority: unranked
scope: Scenes/Component/AudioConsoleAdapters (SoundEmitter), Scenes/Component/LightConsoleAdapters (the four light types)
opened: 2026-11-16
tags: [console, mcp, testability, audio, lights]
---

# Console — no command makes an emitter play a sound, nor gives a light its colour projection

## Why

Two paths of the scene cannot be driven from the remote console nor from the MCP server, so they cannot be measured:

- a sound emitter that plays a sound STILL LOADING (`SoundEmitter::play()` awaits it, `stop()` cancels the awaited
  play — scenes doc 12 § *The two emitters*, 2026-11-16: the cancellation is verified by reading only);
- a light that takes a colour projection texture while the scene runs (the registration reaches the logic cycle — same
  section: measured only on a scene that sets the projection while it is built).

`SoundEmitter` exposes `getState`, `setGain`, `setVelocityDistortion`, `replay` and `stop`; the lights expose their
photometry, their shadows and `setProjectionBoost` — nothing that changes WHAT they play or project.

## What remains

Two typed commands, proposed to the owner on 2026-11-16 (the engine API exists for both):

| Command | Arguments (after `entity, component`) | Calls | State read / written |
|---|---|---|---|
| `SoundEmitter.play` | `sound` (a resource name of the Sounds store), `gain` (≥ 0, default 1), `loop` (bool, default false) | `Resources::Container< Audio::SoundResource >::getResource(name)` then `SoundEmitter::play(sound, gain, loop, true)` | the attached sound, the gain, the loop flag, the awaited play; answers the emitter's state (`getState`) so a caller sees "awaiting" against "playing" |
| `<Light>.setColorProjection` (each light type) | `texture` (a resource name; an empty name removes the projection) | the store that fits the light — a cubemap or an animated cubemap for a point light, a 2D texture for the others (`usesCubemapColorProjection()`) — then `AbstractLightEmitter::setColorProjectionTexture()` | the projection texture and its bindless registration; answers the light's state |

- A trust boundary: the resource name is checked ALWAYS (unknown name, wrong store for the light type → an explicit
  refusal, never the default resource silently).
- `SoundEmitter.getState` gains the awaited play (`m_playAwaitsLoad`) — without it the proof below cannot read the state.
- The binding site: where `replay` / `stop` and `setProjectionBoost` are bound; the commands appear in the MCP tool list
  by themselves (`tools/mcp-conformance.py` after the change).
- Proof they are meant for: `SoundEmitter.play` of a large sound never requested, `stop` in the same console batch, then
  `getState` once the journal says the sound is loaded — not playing, the sound still attached; `replay` — playing.
  `setColorProjection` on a running scene — the projection is drawn in every frame (several captures, one launch),
  0 VUID.

## References

- `src/Scenes/Component/AudioConsoleAdapters.cpp`, `src/Scenes/Component/LightConsoleAdapters.cpp`.
- `docs/ai-runtime-control.md` (the addressing of an entity and a component; the MCP server).
