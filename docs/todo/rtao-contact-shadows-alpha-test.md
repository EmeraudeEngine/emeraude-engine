---
id: rtao-contact-shadows-alpha-test
title: RTAO and ContactShadows rays ignore the alpha test — a leaf occludes as a solid quad
status: open
priority: medium
scope: Graphics/Effects/Framebuffer
opened: 2026-08-29
tags: [vulkan, ray-tracing, alpha-test, foliage]
---

# RTAO and ContactShadows rays ignore the alpha test

## Why

Both effects launch their occlusion rays with `gl_RayFlagsOpaqueEXT`
(`RTAO.cpp:180`, `ContactShadows.cpp:119`). That flag overrides the `FORCE_NO_OPAQUE` instance
flag a cutout material carries, so the hardware accepts every triangle of a foliage card
whole: a leaf occludes as a solid quad, and the ground under a tree is darkened by the card,
not by the leaves. RTGI and RTR were fixed on 2026-08-29 through the ONE shared rule in
`RTAlphaTestGLSL.hpp` (`rtCandidateIsSolid()`); these two are the remaining rays of the engine
that still take the shortcut.

## What remains

The shared rule needs what the fixed effects already bind and these two do not:

- **RTAO** binds the Renderer's RT set (set 0: TLAS + mesh/material/light SSBOs) but its
  shader declares only the TLAS, and its pipeline layout has NO bindless set — so it cannot
  sample an opacity texture. Add the SSBO declarations (already in the bound set), add the
  bindless layout as set 2 (as RTGI/RTR do), bind it per frame, then replace the ray flag and
  the empty `while (rayQueryProceedEXT(q)) {}` by `EMEN_RT_CONFIRM_ALPHA_TESTED_CANDIDATE`.
- **ContactShadows** uses its OWN descriptor set with the TLAS at binding 2 and nothing else.
  It has to move to the Renderer's RT set + the bindless set, which is a larger change to its
  layout and recording.

⚠️ Measure the cost: an opaque-flagged ray finishes in hardware, a candidate-judging ray
returns to the shader for every cutout triangle it crosses. RTAO shoots 8 rays per pixel at
half resolution — under dense foliage the difference may be visible in the GPU profiler
scopes. The fixed RTGI/RTR rays pay the same price and it was accepted; measure anyway.

## ⚠️ Traps

- ⚠️ The shared macros rely on names the host shader must declare: `meshSSBO`,
  `materialSSBO`, `textures2D`, `getMeshAccessor()`, `getHitUV()`, and the flag constants
  `IsAlphaTest`, `HasOpacityTexture`, `HasAlbedoTexture`. Missing one is a GLSL compile error
  at RUNTIME, not at build time.
- ⚠️ `gl_RayFlagsTerminateOnFirstHitEXT` composes with the rule — an occlusion ray still stops
  at the first CONFIRMED candidate. Do not drop it.

## References

- `dependencies/emeraude-engine/src/Graphics/Effects/Framebuffer/RTAlphaTestGLSL.hpp` — the
  rule and its usage contract.
- `dependencies/emeraude-engine/src/Graphics/Effects/Framebuffer/RTGI.cpp` — both rays fixed,
  the reference port.
