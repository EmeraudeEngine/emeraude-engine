---
id: renderer-read-state-index-stored-across-frames
title: Renderer::m_currentReadStateIndex is set in the middle of a frame and read by 16 files — the cloud shadow pass reads the previous frame's
status: open
priority: high
scope: Graphics/Renderer (m_currentReadStateIndex, currentReadStateIndex()), Graphics/IndirectPostProcessEffect (FrameContext), the 13 effects that reconstruct a view, Graphics/GIDenoiser, Graphics/PostProcessor, Scenes/Scene.lighting.cpp
opened: 2026-11-15
tags: [lifetime, defect, code-reading, refactor]
---

# Renderer::m_currentReadStateIndex is set in the middle of a frame and read by 16 files — the cloud shadow pass reads the previous frame's

## Why

The render-state slot a frame reads belongs to the SCENE: `Scene::prepareRender()` latches it
(`Scene::preparedReadStateIndex()`). The renderer copies it into a member, `m_currentReadStateIndex`, at the frame's
ray-tracing bind — after `prepareRender()`, after the shadow passes — and 16 files read it back through
`Renderer::currentReadStateIndex()`. It is the shape `m_currentTLAS` had before the ray-tracing refactor of 2026-11-15
(scenes doc 14 § *The session, the frame, the effect family*): a frame's state kept in a renderer member, valid only
for the readers that come after the line that sets it.

**One reader comes before** (by reading the code, 2026-11-15; not measured): `Scene::recordCloudShadowMap()`
(`Scenes/Scene.lighting.cpp`) passes `renderer.currentReadStateIndex()` to `CloudSet::recordShadowMap()`, and both
frame functions call it BEFORE they set the member. It records with the index of the PREVIOUS frame — while this
frame's index sits in the scene it is a member of. In the triple buffer the previous frame's read slot is no longer
the rendering thread's: the logic thread may be writing it. On the direct path the member is set only when
post-processing is active: without it the cloud shadow pass reads an older value still.

The other 15 readers (13 effects, `GIDenoiser`, `PostProcessor`) run after the line: they get this frame's index, by
position in the function and nothing else.

## What remains

- The owner's refactor rule applies (scenes doc 14): what a frame uses travels in the frame context.
  `IndirectPostProcessEffect::FrameContext` gains the frame's read-state index; the effects read the context;
  `Scene::recordCloudShadowMap()` reads its own `preparedReadStateIndex()`; the member and its getter leave the
  renderer. Group it at the level where it is common: every effect that reconstructs a view takes the index the same
  way — a function of `IndirectPostProcessEffect`, not 13 copies of one line.
- Read, for each of the 16 readers, the function it sits in and whether it has the frame context
  (`PostProcessor.cpp` builds its view data BEFORE the context: the index is then a parameter of that step).
- Proof: a scene with clouds and a moving sun — the cloud shadow map follows the frame's state (a capture of the
  shadow map against the light's published direction); ThreadSanitizer on that scene, 0 report on the view matrices'
  render state; the traced and screen-space effects pixel-identical on a static scene.

## References

- Scenes doc 12 § *View Matrix State Index — Critical Trap* (why an effect must use the frame's index).
- Scenes doc 14 § *The session, the frame, the effect family* (the refactor this continues).
- `tsan-logic-render-instance-state-races` (member 3 there is the renderer reading the view matrices' LOGIC state).
