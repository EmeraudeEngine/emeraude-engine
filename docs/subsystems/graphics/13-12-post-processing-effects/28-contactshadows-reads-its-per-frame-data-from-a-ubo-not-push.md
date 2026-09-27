## 12. Post-Processing Effects

### ContactShadows reads its per-frame data from a UBO, not push constants (Sep 2026)

> [!NOTE]
> Offsetting along the normal means transforming the VIEW-space G-buffer normal to world space,
> so the pass needs the inverse view ROTATION on top of the inverse view-projection. That totals
> **132 bytes — above the 128-byte Vulkan minimum guarantee** for `maxPushConstantsSize`
> (`Vulkan/PipelineLayout.cpp` warns, and the engine treats the warning as a portability defect
> to fix, never to silence). The pass therefore lost its push constant range entirely and reads
> `ShadowFrameUBOData` from **set 1, binding 2** — `getInputLayout(2, 1)`, per-frame buffers from
> `createPerFrameUniformBuffers()`, same pattern as `GIDenoiser::FrameUBOData`. std140 layout:
> `mat4` and `vec4` members only, scalars packed into the `w` slots.
>
> ⚠️ RTAO's own `TracePushConstants` is exactly 128 bytes — it has no room left either. Anything
> added to an RT trace pass from now on goes to a UBO.
