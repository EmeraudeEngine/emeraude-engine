---
id: component-console-adapters
title: Expose the remaining component types to the console and MCP (adapters)
status: open
priority: unranked
scope: Scenes/Component
opened: 2026-09-27
tags: [console, mcp, components]
---

# Expose the remaining component types to the console and MCP (adapters)

## Why

The owner wants to DRIVE an entity's components from the console and the MCP server ("piloter une
lampe, par exemple", 2026-09-27). The mechanism exists and the three light types are done
(`src/Scenes/Component/ConsoleAdapter.hpp`, `LightConsoleAdapters.cpp`; pattern in
`src/Scenes/AGENTS.md` § "Exposing a Component to the Console / MCP"; operator view in
`docs/ai-runtime-control.md` § "Driving an entity's components").

## What remains

1. **Lights, second batch**: shadow casting (`enableShadowCasting()` only works when the shadow map was
   requested at light creation — the command must say so instead of silently doing nothing), PCF radius,
   shadow bias, colour projection.
2. **The other types**, one adapter each, in an order to agree with the owner: `Camera`, `SoundEmitter`,
   `Microphone`, `Visual` / `MultipleVisuals`, `ParticlesEmitter`, `NodeAnimation`, `CloudVolume`,
   `SunCourse`, `SkyFollowsSun`, `Weight`, the modifiers (`DirectionalPushModifier`,
   `SphericalPushModifier`). Decide per type what is worth driving — not every setter.
3. Positions of entities are not in `listEntities()` yet (static entities have no `getNode()` twin):
   worth adding when a use needs it.

## ⚠️ Traps

- A setter that is only valid at creation must be refused with the reason, never applied halfway.
- Validate ranges before `act()`: a refused call must not take the exclusive scene lock.
- Capture a frame only ~1 s after a change (logic tick + render state), and compare hues rather than
  luminance under auto-exposure (`docs/ai-runtime-control.md`).
