---
id: rtr-push-constants-exceed-min-spec
title: RTR — move TracePushConstants (148 bytes) to a per-frame UBO
status: open
priority: unranked
scope: Graphics/PostProcessing
opened: 2026-09-02
tags: [vulkan, ray-tracing, portability, min-spec]
---

# RTR — move TracePushConstants (148 bytes) to a per-frame UBO

## Why

`RTR::TracePushConstants` is **148 bytes**, above the 128-byte Vulkan minimum guarantee for
`maxPushConstantsSize`. On a device that exposes exactly 128 — *"part of the AMD/Intel fleet"*,
`Vulkan/PipelineLayout.cpp:106` — `PipelineLayout::create()` returns `false`, so **RTR never
gets created at all** and every scene that declares it silently loses its reflections.

It is the **only** push-constant block in the engine over the floor. A scan of the 31
`*PushConstants` structs (2026-09-02):

| Struct | Bytes |
|---|---|
| `RTR::TracePushConstants` | **148** ← over the floor |
| `RTAO::TracePushConstants` | 128 (exactly at it, no headroom left) |
| `AtmosphericFog::FogPushConstants` | 116 |
| everything else | ≤ 96 |

Breakdown of the 148:

| Block | Bytes | Running total |
|---|---|---|
| `invViewProj` (mat4) | 64 | 64 |
| 3 × (`invViewColN` vec3 + `viewPosN`) | 48 | 112 |
| `maxDistance`, `intensity`, `fadeScreenEdge`, `lightCount` | 16 | 128 |
| `ambientR/G/B`, `skyLuminance` | 16 | 144 |
| `coneScale` | 4 | 148 |

It was 144 until `b32f22d8` (*RTR glossy cone — per-pixel hit distance (v2) and a 24-tap disk
gather (v3)*) added `coneScale`; the block was already over the floor before that.

## What remains

Port the trace pass to a per-frame UBO, exactly as `ContactShadows` was ported in `9a14a190`:

- [ ] `ShadowFrameUBOData`-style struct, **std140-compatible: `mat4` and `vec4` members only**,
      scalars packed into the `w` slots (patterns: `GIDenoiser::FrameUBOData`,
      `ContactShadows::ShadowFrameUBOData`).
- [ ] `getInputLayout(samplerCount, 1)` so the UBO binding lands after the samplers.
- [ ] `createPerFrameUniformBuffers(sizeof(...), ClassId, "...")`, one buffer per
      frame-in-flight, `writeUniformBufferObject()` **once** at create time (the buffer handle
      never changes — only the image bindings are rewritten per frame).
- [ ] Fill it with `IndirectPostProcessEffect::updateUniformBufferData()` in the record path and
      drop the `VkPushConstantRange` from the pipeline layout plus the `vkCmdPushConstants` call.
- [ ] Check the OTHER RTR passes while there: `PyramidPushConstants` (16 bytes,
      `RTR.cpp:1163`) is fine and must stay push constants.

## ⚠️ Traps

- ⚠️⚠️ **This cannot be reproduced on either of the owner's workstations.** NVIDIA exposes 256
  bytes, so both GPUs (RTX 3070 Ti, RTX 3500 Ada) take the `TraceWarning` portability path and
  RTR works. The min-spec failure is a `TraceError` + a failed pipeline layout. Verifying the fix
  here therefore means reading the **absence of the warning** in the log, not a visual change —
  the render must come out identical.
- ⚠️ **The warning only fires when the layout is actually created**, i.e. when a demo really
  instantiates RTR. `AbstractDemo.cpp:216` has it **commented out**, so the demos that go through
  the shared stack never trigger it. Use `reflexion-debug`, `light-and-shadow-debug` or
  `post-processor-effect-debug` (projet-alpha `src/Builtin/`).
- ⚠️ A UBO read is not a push-constant read: `GL_EXT_scalar_block_layout` is enabled in these
  shaders but a uniform block still defaults to **std140**. Declare the block `std140`
  explicitly and keep to `vec4`/`mat4` members, or the C++ struct and the GLSL block will
  disagree — a mismatch that shows up as wrong values, never as a compile error.
- ⚠️ Do not "solve" this by packing the block tighter to squeeze under 128. `RTAO` is already at
  exactly 128 and the same pressure will come back on the next field; the UBO is the engine's
  documented answer (`Graphics/IndirectPostProcessEffect.hpp`, `getInputLayout()` doc comment).

## References

- `src/Graphics/Effects/Framebuffer/RTR.hpp` — `TracePushConstants`
- `src/Graphics/Effects/Framebuffer/RTR.cpp:1163` — `PyramidPushConstants` (leave as is)
- `src/Vulkan/PipelineLayout.cpp:105-133` — the min-spec validation, error vs warning
- `src/Graphics/AGENTS.md` § *ContactShadows reads its per-frame data from a UBO* — the ported
  reference and the note that RTAO has no headroom left
- `9a14a190` — the ContactShadows port, the pattern to copy
