## Critical: Dedicated Device Memory on MoltenVK (Sep 2026)

> [!CAUTION]
> `Buffer::setDedicatedMemory(true)` (before `createOnHardware()`) gives a buffer its own
> `VkDeviceMemory` (`VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT`). It exists for ONE measured
> reason: on MoltenVK a VMA block is a single `MTLBuffer` whose argument-buffer residency is
> tracked per buffer, so a host-visible SSBO read through a descriptor set can be left
> non-resident for the vertex stage by an unrelated set sharing its block (MoltenVK#1870). The
> skinning SSBO carries it when `physicalDevice()->hasPortabilitySubset()` — the extension, never
> the platform. Do not spread it to every buffer: `maxMemoryAllocationCount` is finite. See
> `docs/caution-points.md` § Skinned meshes collapsed on MoltenVK.
