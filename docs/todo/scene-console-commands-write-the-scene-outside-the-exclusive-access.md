---
id: scene-console-commands-write-the-scene-outside-the-exclusive-access
title: The scene service's console commands change the running scene from the main thread with a mere snapshot of it, outside the exclusive access the component adapters take
status: open
priority: high
scope: Scenes/Manager.console.cpp (every command that takes activeSceneSnapshot())
opened: 2026-11-16
tags: [concurrency, console, defect]
---

# The scene service's console commands change the running scene from the main thread with a mere snapshot of it, outside the exclusive access the component adapters take

## Why

A console (or MCP) command runs on the MAIN thread. The rule for one that touches the running scene exists since
2026-09-27: the component adapters go through `ConsoleAdapter::act()` → `withComponent()` →
`Scenes::Manager::withExclusiveActiveScene()`, so the command lands between two rendering frames and two logic cycles
(scenes doc 07 § console adapters, doc 08 § writer preference). The commands of the scene SERVICE itself
(`Scenes/Manager.console.cpp`) mostly do not: they take `activeSceneSnapshot()` — a shared pointer, no access at all —
and then read or write the scene while the logic thread runs its cycle and the rendering thread builds its frame.
`enableTBN` was one of them and takes the exclusive access since 2026-11-16 (scenes doc 12, the configuration rows).

Read on 2026-11-16, never run under ThreadSanitizer (no sweep sends console commands to a scene):

| Kind | Commands that take only the snapshot |
|---|---|
| create or destroy entities and components | `createNode`, `destroyNode`, `attachCamera`, `attachMicrophone`, `addMesh`, `addTexturedShape`, `loadGLTF`, `setGround`, `setBackground`, `toggleCompass` |
| move or drive a live entity | `setNodePosition`, `setNodeLookAt`, `setCharacterVelocity`, `setVehicleInput`, `characterJump` |
| other writes | `recordNodePhysics`, `stopPhysicsRecording`, `savePhysicsRecording`, `writeImposterAtlases`, `targetActiveScene` |
| reads of a state the logic thread writes | `getNode`, `getNodePhysics`, `getGroundLevel`, `getRenderStatistics`, `getStateSyncStatistics`, `getPhysicsRecordingStatus`, `getWorldSoundCount`, `getActiveSceneName` |

**Measured for one of them** (2026-11-16, Release, RTX 3070 Ti, `animation-debug`, a temporary pause of 400 ms injected
between the two writes of an entity's renderable flag in `AbstractEntity::updateEntityProperties()`):
`attachMicrophone(Fox, …)` links a component to the drawn node `Fox` on the main thread; during the pause the logic
thread's publication read the half-written flag and PUBLISHED "draws nothing" — 150 and 161 raster list builds left the
entity out (two commands). The same count before and after the frame's test moved to the render state
(`isPublishedRenderable()`): the slot protects a frame from the logic thread's own recomputation, not from a third
thread that changes the entity in the middle of a cycle.

## What remains

1. Read EVERY command of the file (the table is the list of `activeSceneSnapshot()` users; the commands that reach
   the scene another way — the highlights, the dynamic sky, the node and static-entity listings — are to be read
   too) and give each one the access its body needs: `withExclusiveActiveScene()` for a write, and for a read of
   logic state either the exclusive access or a published value.
2. Before wrapping a command, check what the exclusive access would hold:
   - **how long**: `loadGLTF`, `addMesh`, `addTexturedShape`, `setBackground`, `writeImposterAtlases` create resources
     — a synchronous load under the exclusive access freezes the frames and the logic for its duration. Split: prepare
     outside, link inside;
   - **re-entrance**: a shared access asked inside (`withSharedActiveScene()`, `hasActiveScene()`) is a deadlock once a
     writer is announced; `enableScene()` / `disableActiveScene()` / `deleteScene()` take the write lock themselves;
   - **notifications**: what the command emits runs under the lock (never emit while holding a mutex a handler takes).
3. Prove each one: a ThreadSanitizer run that sends the command to a running scene (the script of the 2026-11-16 lot
   sends `enableTBN` and `addTexturedShape`: one clean run proves little, the windows are a few microseconds — widen
   them, or repeat on a scene with many entities), then the functional check of the command.

## ⚠️ Traps

- ThreadSanitizer misses most of these in a plain run: a mutex both threads take a little later orders the two
  accesses after the fact. `enableTBN` wrote the flag word of every instance while frames read it, 16 commands in a
  row: ONE report, on the renderer's switch — none on the words.
- `withExclusiveActiveScene()` is a contract fault on the thread that holds the cycle lock (inside the logic cycle or
  a `betweenLogicCycles()` change): a command reachable from the logic thread (a script, a game rule calling the
  console) cannot use it.
- The same family, another site: `scene-controllers-move-nodes-outside-the-logic-cycle` (the input listeners move a
  node from the main thread). The fix may differ — the console's rule is decided, the controllers' is not.

## References

- `src/Scenes/Component/ConsoleAdapter.cpp` (`withComponent()`), `src/Scenes/Manager.hpp`
  (`withExclusiveActiveScene()`, `betweenLogicCycles()`).
- Scenes doc 12 § *Plain state shared by the logic and rendering threads* (the configuration rows, 2026-11-16).
