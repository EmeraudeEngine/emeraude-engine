---
id: light-layers
title: Light layers — decide which lights light which surfaces (a mask on the lights and on the instances)
status: open
priority: unranked
scope: Scenes (LightSet, AbstractLightEmitter, renderLightedSelection), Graphics (RenderableInstance, DeferredLightResolve, G-buffer, RT lanes)
opened: 2026-11-01
tags: [lighting, design, deferred, ray-tracing]
---

# Light layers — decide which lights light which surfaces

## Why

Owner, 2026-11-01: "a smarter system will be needed to decide what is lit by what, like a layer system."

The first need came from `city`: its island sea, a full-screen translucent surface lit forward, was redrawn once per
street lamp and signal in reach. The answer that day is a single on/off switch per instance,
`RenderableInstance::Abstract::setPunctualLightingEnabled(false)` (for a scene's sea `Scene::setSeaLevelPunctualLighting(false)`): the instance gets no point, spot or line light pass, only
the directional lights, the ambient and the sky (graphics doc 38). A switch is all-or-nothing. A layer system says
WHICH lights light WHICH surfaces: for example, the city's lamps light the streets but not the sea, while a harbour
light still lights the water by the quay.

## What remains

- Owner decisions first:
  - **the shape**: a bitmask on each light (the layers it lights) and on each renderable instance (the layers it
    belongs to); the instance is lit when the two masks intersect;
  - **the width**: 8, 16 or 32 layers;
  - **the scope**:
    - lighting only, or shadows too (a light casting shadows only from some layers);
    - the RT lanes (the TLAS instance mask, the RT light SSBO);
  - **the default**: everything on layer 0, so a scene that never sets a mask stays lit as today.
- **The forward path**: `Scene::renderLightedSelection()` skips the pair when the masks do not intersect. This is the
  place `setPunctualLightingEnabled()` uses today.
- **The deferred resolve** (`DeferredLightResolve`): the G-buffer must carry the instance's mask for each pixel (a few
  bits) so that the resolve can skip a light for a pixel. Without that, an opaque instance whose material the resolve
  shades still receives every resolved light. That limit already holds for `setPunctualLightingEnabled()`.
- **Migrate** `setPunctualLightingEnabled()` / `setSeaLevelPunctualLighting()` to a layer: the sea on a layer that no punctual light lights. Then retire
  the flag, or keep it as a shorthand.
- Measure the cost: a mask test per light per batch in forward; a few G-buffer bits and a test per tile/light in the
  resolve.

## ⚠️ Traps

- A translucent surface is lit FORWARD, once per light. The cost of a layer that lets many lights onto a large
  translucent surface is the cost the sea had: 294 ms a frame at side 128 before the skip (graphics doc 38).
- The G-buffer is the VRAM budget of the 8 GB target card: find spare bits before adding a target.

## References

- Unreal Engine, "Lighting Channels" (three channels on lights and primitives; forward and deferred, a G-buffer
  stencil-like mask).
- Unity HDRP, "Light Layers" / URP "Rendering Layers" (a 32-bit mask on lights and renderers, stored in the G-buffer
  for the deferred path).
- Godot 4, `VisualInstance3D.layers` and `Light3D.light_cull_mask`.
- `RenderableInstance::Abstract::setPunctualLightingEnabled()`, `Scene::setSeaLevelPunctualLighting()`; graphics doc 38 (deferred punctual lights);
  `Scene::renderLightedSelection()`.
