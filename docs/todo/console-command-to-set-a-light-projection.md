---
id: console-command-to-set-a-light-projection
title: Console — no command gives a light its colour projection; the awaited play of an emitter is not proven yet
status: open
priority: unranked
scope: Scenes/Component/LightConsoleAdapters (the four light types), Scenes/Component/AudioConsoleAdapters (the proof only)
opened: 2026-11-16
tags: [console, mcp, testability, audio, lights]
---

# Console — no command gives a light its colour projection; the awaited play of an emitter is not proven yet

## Why

A light that takes a colour projection texture while the scene runs cannot be driven from the remote console nor
from the MCP server, so it cannot be measured (the registration reaches the logic cycle — scenes doc 12 § *The two
emitters*, 2026-11-16: measured only on a scene that sets the projection while it is built). The lights expose their
photometry, their shadows and `setProjectionBoost` — nothing that changes WHAT they project.

The item's other half, `SoundEmitter.play`, exists since 2026-11-26 (audio doc 15, `docs/ai-runtime-control.md`):
its proof on a sound STILL LOADING is what remains of it.

## What remains

One typed command, proposed to the owner on 2026-11-16 (the engine API exists):

| Command | Arguments (after `entity, component`) | Calls | State read / written |
|---|---|---|---|
| `<Light>.setColorProjection` (each light type) | `texture` (a resource name; an empty name removes the projection) | the store that fits the light — a cubemap or an animated cubemap for a point light, a 2D texture for the others (`usesCubemapColorProjection()`) — then `AbstractLightEmitter::setColorProjectionTexture()` | the projection texture and its bindless registration; answers the light's state |

- A trust boundary: the resource name is checked ALWAYS (unknown name, wrong store for the light type → an explicit
  refusal, never the default resource silently).
- The binding site: where `setProjectionBoost` is bound; the command appears in the MCP tool list by itself
  (`tools/mcp-conformance.py` after the change).
- Proof: `setColorProjection` on a running scene — the projection is drawn in every frame (several captures, one
  launch), 0 VUID.
- The proof `SoundEmitter.play` was meant for, not run yet: `play` of a large sound never requested, `stop` in the
  same console batch, then `getState` once the journal says the sound is loaded — `awaitingLoad` false, not
  playing, the sound still attached; `replay` — playing. (Seen on 2026-11-26: `play` of a sound never requested
  answers `awaitingLoad` true, then the sound plays by itself.)

## References

- `src/Scenes/Component/AudioConsoleAdapters.cpp`, `src/Scenes/Component/LightConsoleAdapters.cpp`.
- `docs/ai-runtime-control.md` (the addressing of an entity and a component; the MCP server).
