---
id: material-one-binding-per-distinct-texture
title: A material binds one descriptor per DISTINCT texture — components share sampler and fetch
status: open
priority: unranked
scope: Graphics/Material (StandardResource), Saphir
opened: 2026-10-20
blocked-by: [material-source-channel-ignored-by-hard-coded-components]
tags: [material, shaders, descriptors, texture-packing]
---

# One binding per distinct texture

## Why

Decision D1 (graphics § 39): a component reads `(texture, channel)` and the material binds each distinct texture
once. Today every component owns its binding, sampler name and fetch: a glTF ORM image is bound three times and
fetched three times, and the layout / UBO bucket is keyed by the component count.

## What remains

- Group the texture components by texture identity (the same `TextureResource` and the same UV transform / UV set);
  one binding, one sampler name and ONE `texture()` per group in the generated fragment code, each component taking
  its channel from the shared fetch.
- The descriptor-set layout identifier must describe the bindings that really exist (count AND stage flags:
  Displacement adds the mesh stage under the same identifier today); the binding order must be deterministic (it
  follows `unordered_map` iteration today).
- The shared UBO bucket changes with the binding count (`shared-material-buffer-capacity`).
- Measure the gain against the reference of `texture-vram-accounting-per-texture` (bindings, frame time).

## ⚠️ Traps

- Two components of one texture with DIFFERENT UV transforms cannot share the fetch: group on texture + transform.
- The per-material samplers share the 32-descriptor `OtherSetsSamplerHeadroom` with the other sets when bindless is
  on (`BindlessTextureManager.hpp:152`): fewer bindings helps, the limit is still not checked for materials.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `05-5-material-ubo-system.md`,
  `18-descriptor-set-binding-contract.md` (Saphir).
