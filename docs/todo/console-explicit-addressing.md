---
id: console-explicit-addressing
title: Replace the stateful targeting commands (targetScene/targetNode/…) by explicit addressing
status: open
priority: unranked
scope: Scenes/Manager.console.cpp
opened: 2026-09-27
tags: [console, mcp]
---

# Replace the stateful targeting commands (targetScene/targetNode/…) by explicit addressing

## Why

`SceneManagerService` still drives nodes through a GLOBAL target (`Scenes::ConsoleMemory`):
`targetActiveScene()` / `targetScene(name)` / `targetNode(name)` / `targetStaticEntity(name)`, then
`listNodes()`, `listStaticEntities()`, `moveNodeTo(x, y, z)` act on "the" target. That state is shared by
every client of the console and of the MCP server: two clients steal each other's target, and a model
must remember what it targeted — the opposite of what the MCP specification recommends for a stateless
protocol (explicit handles, no hidden per-connection state). The component commands were built the other
way on 2026-09-27 (owner decision: every call names `entity` + `component`), and `listEntities()` /
`listEntityComponents(entity)` already work without any target.

## What remains

1. Give the node commands an explicit `node` parameter (`setNodePosition(node, x, y, z)` and
   `setNodeLookAt(node, …)` already do): `moveNodeTo` becomes redundant with `setNodePosition`.
2. Decide with the owner whether `targetScene`/`targetNode`/`targetStaticEntity`/`listNodes`/
   `listStaticEntities` are deleted (superseded by `listEntities()` and explicit parameters) or kept for
   humans typing in the console — and whether a non-active scene must be addressable at all.
3. Update projet-alpha docs and scripts that call `targetActiveScene()` first (many do: AGENTS.md § 3b,
   the camera bullet, `tools/` benches).

## ⚠️ Traps

- `Act.*` commands (projet-alpha) do NOT depend on the target despite the docs asking for
  `targetActiveScene()` first — check each command before removing the step from a script.
