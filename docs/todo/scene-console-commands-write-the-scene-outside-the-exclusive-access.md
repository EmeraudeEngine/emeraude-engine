---
id: scene-console-commands-write-the-scene-outside-the-exclusive-access
title: The scene service's console commands run under the scene's exclusive access — done on Linux, awaits the peers
status: in-progress
priority: high
scope: Scenes/Manager.console.cpp
opened: 2026-11-16
tags: [concurrency, console, defect]
---

# The scene service's console commands run under the scene's exclusive access — done on Linux, awaits the peers

## Why

A console (or MCP) command runs on the MAIN thread. The commands of the scene service read and wrote the running
scene on a mere snapshot of it, while the logic thread ran its cycle and the rendering thread built its frame: nine
ThreadSanitizer reports on one scenario, and a drawn entity left out of 150 to 161 list builds when a component was
linked to it that way (a widened window).

Done on 2026-11-17 (owner decisions; the rule and its three helpers are scenes doc 07 § console adapters, the proof
closes scenes doc 12 § *Plain state shared by the logic and rendering threads*): 16 commands whole under the access,
5 that prepare first and write the scene under it, the highlight style, the target family's reads. Left on a snapshot
on purpose, each with its reason in the code: the physics recorder's commands, `writeImposterAtlases`, the physical
sky's commands (objects that guard themselves), `getActiveSceneName`, `targetActiveScene`.

## What remains

- **The peers' round** (macOS, Windows): Release and Debug with 0 warning (the file-local helpers take `const auto &`
  parameters and a generic lambda with a default argument: first compile on Apple clang and MSVC); the console
  scenario of scenes doc 12's proof — the same replies, no hang, exit 0, 0 VUID; the slowest reply.
- Record the verdicts in scenes doc 12 and the ledger, then delete this file.

## ⚠️ Traps

- ThreadSanitizer misses most of these in a plain run (scenes doc 12): what the peers verify is the build on their
  compiler, the unchanged replies and the absence of a hang — a command that asked a scene-manager access INSIDE the
  exclusive one would wait for itself.
- The same family, another site: `scene-controllers-move-nodes-outside-the-logic-cycle` (the input listeners move a
  node from the main thread). Not covered here.

## References

- `src/Scenes/Manager.console.cpp` (`inActiveScene`, `inSameActiveScene`, `inTargetedScene`), scenes doc 07, scenes
  doc 12.
