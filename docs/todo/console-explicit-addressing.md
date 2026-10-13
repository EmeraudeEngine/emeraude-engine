---
id: console-explicit-addressing
title: Replace the stateful targeting commands (targetScene/targetNode/…) by explicit addressing
status: open
priority: medium
scope: Scenes/Manager.console.cpp
opened: 2026-09-27
tags: [console, mcp]
---

# Replace the stateful targeting commands (targetScene/targetNode/…) by explicit addressing

## Why

`SceneManagerService` still drives nodes through a GLOBAL target (`Scenes::ConsoleMemory`):
`targetActiveScene()` / `targetScene(name)` / `targetNode(name)` / `targetStaticEntity(name)`, then
`listNodes()`, `listStaticEntities()` act on "the" target (`src/Scenes/Manager.console.cpp:886-975`). That state is shared by
every client of the console and of the MCP server: two clients steal each other's target, and a model
must remember what it targeted — the opposite of what the MCP specification recommends for a stateless
protocol (explicit handles, no hidden per-connection state). The component commands were built the other
way on 2026-09-27 (owner decision: every call names `entity` + `component`), and `listEntities()` /
`listEntityComponents(entity)` already work without any target.

## What remains

1. Give the node commands an explicit `node` parameter (`setNodePosition(node, x, y, z)` and
   `setNodeLookAt(node, …)` already do). `moveNodeTo` itself is GONE (`22aa687f5`), but one place still names
   it: the `targetNode` help text (`src/Scenes/Manager.console.cpp:933`, "for moveNodeTo()") — code. (The two doc
   leftovers, `docs/ai-runtime-control.md` and `docs/runtime-session.md`, were fixed on 2026-10-13.)
2. Decide with the owner whether `targetScene`/`targetNode`/`targetStaticEntity`/`listNodes`/
   `listStaticEntities` are deleted (superseded by `listEntities()` and explicit parameters) or kept for
   humans typing in the console — and whether a non-active scene must be addressable at all.
3. Update the docs, scripts and texts that call `targetActiveScene()` first:
   - engine: `src/Console/MCP/Protocol.cpp:693` (the MCP server's instructions), `docs/ai-runtime-control.md`,
     `docs/runtime-session.md`, `docs/subsystems/console/05-5-ai-scene-creation.md`, `tools/demo-capture-bench.py`,
     `tools/emeraude_console.py`, `tools/gltf-conformance-bench/bench.py`, `tools/remote-console.py`;
   - projet-alpha: `docs/agents/04-3b-ai-runtime-notes.md`, `docs/agents/rules/build-and-run.md`,
     `docs/caution-points.md`, `docs/subsystems/builtin/12-8-scenes-loaders-api.md`,
     `src/Builtin/PostProcessorEffectDebug.AGENTS.md`.

## ⚠️ Traps

- `Act.*` commands (projet-alpha) do NOT depend on the target despite the docs asking for
  `targetActiveScene()` first — check each command before removing the step from a script.
