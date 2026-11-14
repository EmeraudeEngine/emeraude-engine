---
id: toolkit-2d-renderer-target-named-cubemap
title: Toolkit::generateTexture2DRenderer() names its 2D render target "<name>Cubemap"
status: open
priority: unranked
scope: Scenes/Toolkit.hpp (generateTexture2DRenderer)
opened: 2026-11-14
tags: [naming, render-target, console]
---

# Toolkit::generateTexture2DRenderer() names its 2D render target "<name>Cubemap"

## Why

`Toolkit::generateTexture2DRenderer(name, …)` creates its target with `createRenderToTexture2D(name + "Cubemap", …)`:
a plain 2D texture whose id reads like a cubemap (`SecurityCubemap` in projet-alpha's `offscreen-rendering`). The
name is what `Core.SceneManagerService.dumpRenderTarget(fragment)` matches, and what a reader of a log sees: on
2026-11-14 the six "faces" of `SecurityCubemap` were asked for before its single layer gave the name away. The
sibling `generateEnvironmentCubemapRenderer()` uses the same suffix for a real cubemap, so the fragment `Cubemap`
matches both kinds.

## What remains

- **Owner decision 2026-11-14: rename it if that is clearer.** Proposed: `<name>Texture2D`, the word of the function
  and of `createRenderToTexture2D()`. The ids that tools and documents quote are updated with it: `SecurityCubemap` in `docs/todo/dump-render-target-first-dump-solid-blue.md`,
  `docs/caution-points.md`, projet-alpha's reports.
- The render target id is visible to a downstream application that looks a target up by name: say so in the CHANGELOG.

## References

- `src/Scenes/Toolkit.hpp` — `generateTexture2DRenderer()`, `generateEnvironmentCubemapRenderer()`.
