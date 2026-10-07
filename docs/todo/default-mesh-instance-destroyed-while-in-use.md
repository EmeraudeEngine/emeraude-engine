---
id: default-mesh-instance-destroyed-while-in-use
title: An entity drawn with a container's DEFAULT mesh destroys a GPU buffer still in use when it goes away
status: open
priority: high
scope: Graphics / Resources (default MultiLayerMeshResource, RenderableInstance teardown)
opened: 2026-10-07
tags: [vulkan, validation, resources, deferred-destruction]
---

# An entity drawn with a container's DEFAULT mesh destroys a GPU buffer still in use when it goes away

## Why

Found 2026-10-07 in projet-alpha's `animation-debug`, validation ON (Linux, RTX 3070 Ti), with the owner firing the
canon: each shell produced one `VUID-vkDestroyBuffer-buffer-00922` ("currently in use by VkCommandBuffer") and the
device later reported the same `VkBuffer` never destroyed (`VUID-vkDestroyDevice-device-05137`). The shell asked for a
mesh that did not exist (`getResource("Grenade40mm")`, missing its `Weapons/` prefix), so `Container::getResource()`
returned the container's DEFAULT resource. With the name fixed (projet-alpha, same day) the shells use the real mesh and
the VUIDs are gone (0 in the owner's next runs) — so the defect is tied to an entity built on the DEFAULT mesh, and
stays latent for any missing resource name. Reproduced on the build without the day's other changes (2 shots → 2 × 00922
+ 2 × 05137).

## What remains

- [ ] Reproduce without projet-alpha's typo: an entity built from `getResource("<missing name>")`, removed after a few
      frames, validation ON (a console-driven demo or a tiny scene).
- [ ] Find which buffer goes: the default resource's own geometry (shared, should never be destroyed per entity), or a
      per-instance buffer whose teardown bypasses `Renderer::deferredDestructor()` on this path
      (`docs/subsystems/vulkan/12-critical-deferred-destruction-contract.md`).
- [ ] Fix in the engine; the leaked buffer at `vkDestroyDevice` must go too.

## References

- `src/Resources/Container.hpp` `getResource()` (default fallback), `docs/subsystems/vulkan/12-critical-deferred-destruction-contract.md`.
- projet-alpha `src/Actor/Shell.cpp` (the trigger, fixed 2026-10-07).
