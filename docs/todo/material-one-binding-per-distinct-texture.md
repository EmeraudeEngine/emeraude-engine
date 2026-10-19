---
id: material-one-binding-per-distinct-texture
title: A material binds one descriptor per DISTINCT texture — components share sampler and fetch
status: in-progress
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

- DONE on Linux (2026-10-20, uncommitted): one binding and one sampler per distinct texture, deterministic binding
  order, the layout identifier carrying the mesh-stage binding, `declare(Sampler)` silent on an identical
  re-declaration — graphics § 39 § One binding per distinct texture (sponza 207 → 162 bindings, 0 VUID).
- The fetches: owner decision D10 (2026-10-20) — shared only for the repack's canonical textures, so that part
  moved to `material-texture-canonical-repack`; nothing left here but the peers.
- Validation by the peers (macOS, Windows) after the push.

## ⚠️ Traps

- Two components of one texture with DIFFERENT UV transforms cannot share the fetch: group on texture + transform.
- The per-material samplers share the 32-descriptor `OtherSetsSamplerHeadroom` with the other sets when bindless is
  on (`BindlessTextureManager.hpp:152`): fewer bindings helps, the limit is still not checked for materials.

## References

- `docs/subsystems/graphics/39-material-texture-packing.md`, `05-5-material-ubo-system.md`,
  `18-descriptor-set-binding-contract.md` (Saphir).
