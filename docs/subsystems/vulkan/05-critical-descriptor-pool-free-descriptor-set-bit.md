## Critical: Descriptor Pool FREE_DESCRIPTOR_SET_BIT

> [!CRITICAL]
> **Any `DescriptorPool` whose descriptor sets are freed individually (via destructor or explicit
> `vkFreeDescriptorSets`) MUST be created with `VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT`.**
>
> Without this flag, `vkFreeDescriptorSets` triggers `VUID-vkFreeDescriptorSets-descriptorPool-00312`
> at shutdown. The `DescriptorPool` constructor accepts this as the 4th parameter:
> ```cpp
> auto pool = std::make_shared< DescriptorPool >(
>     device, poolSizes, maxSets,
>     VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT  // Required for individual free
> );
> ```
>
> ⚠️ This applies to **transient** pools too. `DescriptorSet::destroyFromHardware()` frees its
> set unconditionally, so a pool you intended to throw away whole still gets a
> `vkFreeDescriptorSets` call when its sets unwind — being short-lived is not an exemption.
>
> **Known cases (all carry the flag):** skinning SSBO pool in
> `RenderableInstance::Abstract::createSkinningResources()`; the BRDF-LUT and per-environment
> bake pools in `Graphics/Compute/IBLBaker.cpp`; the pool in `Graphics/Compute/XRayAnalyzer.cpp`.
> The last three were fixed in Jul 2026 — see `docs/caution-points.md` § Vulkan Validation.
