---
id: list-entities-positions
title: Give entity positions in SceneManager listEntities()
status: open
priority: unranked
scope: Scenes/Manager.console.cpp
opened: 2026-09-27
tags: [console, mcp]
---

# Give entity positions in SceneManager listEntities()

## Why

`listEntities()` answers every entity's name, address and components, but not WHERE it is: a client
looking for "the light above the table" must call `getNode()` per node, and static entities have no
`getNode()` twin at all. Left over from the component adapters work (item `component-console-adapters`,
closed 2026-09-27); worth doing when a use needs it.

## What remains

1. Add the world position of each node and static entity to `listEntities()` (full precision, `null`
   for a non-finite value — item `console-json-non-finite-numbers`).
2. Mind the size: `forest` lists ~150 entities, `terrain` far more — consider an optional filter
   (component type, name prefix) before adding fields to every entry.

## References

- `src/Scenes/Manager.console.cpp` `listEntities`, `getNode`.
- `docs/ai-runtime-control.md` § "Driving an entity's components".
