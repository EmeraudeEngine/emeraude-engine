## Multi-Draw Indirect Support

The command buffer supports `drawIndexedIndirect()` for GPU-driven rendering. Device features enabled in `Instance.cpp`:
- `multiDrawIndirect`, `drawIndirectFirstInstance` (VK 1.0)
- `shaderInt64` (VK 1.0) — `uint64_t` for BDA address reconstruction
- `fragmentStoresAndAtomics` (VK 1.0, Aug 2026) — `imageStore()` from a FRAGMENT shader: the RTR trace writes its per-pixel glossy-cone width map (storage image) beside its colour attachment. ⚠️ Without it the SPIR-V validation rejects the pipeline (`VUID-RuntimeSpirv-NonWritable-06340`) and the effect silently fails to create — the demo then loads no scene.
- `shaderDrawParameters` (VK 1.1) — `gl_DrawID` in vertex shaders

**Buffer types for MDI:**
- `IndirectBuffer` (`IndirectBuffer.hpp`) — `VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT`, host-visible
- Per-draw SSBO — Created via `Buffer` directly with `VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT` (NOT via `ShaderStorageBufferObject` which lacks the BDA flag)

> [!WARNING]
> **`ShaderStorageBufferObject` does NOT include `VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT`.**
> For BDA-accessible SSBOs, use `Buffer` directly with both `STORAGE_BUFFER_BIT` and `SHADER_DEVICE_ADDRESS_BIT`.

**Code references:**
- `CommandBuffer.cpp:drawIndexedIndirect()` — Wraps `vkCmdDrawIndexedIndirect`
- `IndirectBuffer.hpp` — Convenience buffer subclass
- `Instance.cpp` — Feature enablement (MDI + shaderInt64 + shaderDrawParameters)

### `DescriptorSet` write helpers — the layout they write (Sep 2026)

`writeCombinedImageSampler(binding, image, view, sampler)` writes the image's **CURRENT** layout
(`Image::currentImageLayout()`), which is `UNDEFINED` for an image the GPU has not touched yet —
measured as `VUID-VkWriteDescriptorSet-descriptorType-04150` on every set of the irradiance probe
volume, whose atlases are cleared and written only once a frame has been recorded. Two overloads
exist for images whose layout is a design decision rather than a runtime state:
`writeCombinedImageSampler(binding, view, sampler, layout)` and `writeStorageImage(binding, view,
layout = GENERAL)`. ⚠️ The earlier storage-image writes of `RTR` and `IBLBaker` still call
`vkUpdateDescriptorSets` by hand from `Graphics/` — the helper they lacked now exists.
