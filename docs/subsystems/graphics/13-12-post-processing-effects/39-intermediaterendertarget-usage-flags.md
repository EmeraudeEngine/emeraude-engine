## 12. Post-Processing Effects

### IntermediateRenderTarget usage flags (Jul 2026)

`IntermediateRenderTarget::create()` gives every target `COLOR_ATTACHMENT_BIT | SAMPLED_BIT |
TRANSFER_DST_BIT` — enough to render into it, sample it, and clear it. Anything beyond that must
be requested through the trailing `extraUsageFlags` parameter.

**Every IRT starts ZERO-CLEARED (Aug 2026).** `create()` clears the image to `(0,0,0,0)` before
the initial transition to `SHADER_READ_ONLY_OPTIMAL`. This is a hard guarantee, not a nicety:
temporal effects sample their history IRT before the first write (the VolumetricLight
occlusion-mask EMA, the GIDenoiser ping-pong). Fresh device memory is UNDEFINED — desktop
drivers happen to return zeroed pages, **Metal/MoltenVK returns real garbage**, and in float
formats garbage bit patterns contain NaNs. A NaN entering an EMA feedback loop
(`mix(history, current, alpha)`) never leaves it (`NaN * 0 = NaN`) and spreads to the whole
frame through the additive combine. Measured on macOS (Apple M2): R/B channels NaN-flushed to 0
at the UNORM swapchain write — every demo with VolumetricLight rendered as a green-only frame.
A temporal effect whose history carries a validity marker (GIDenoiser: `history.a > 0.0`, false
for NaN) is defended in depth; one that mixes blindly relies entirely on this clear.

> [!WARNING]
> **An image can only be TRANSITIONED to a layout its usage flags support.** If your effect reads
> a target back with `vkCmdCopyImageToBuffer`, it MUST be created with
> `VK_IMAGE_USAGE_TRANSFER_SRC_BIT`, or the barrier to `TRANSFER_SRC_OPTIMAL` is silently
> rejected and every later command runs against a **stale tracked layout** — the failure surfaces
> as four unrelated-looking VUIDs pointing at the copy, not at the creation. `ToneMapping`'s
> auto-exposure adaptation targets and `DepthOfField`'s rack-focus targets (both 1x1 per-frame
> readbacks) are the reference cases; see `docs/caution-points.md` § Vulkan Validation.

```cpp
m_adaptTargets[index].create(renderer, 1, 1, lumFormat, "AdaptLum0", VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
```

The same rule applies outside `IntermediateRenderTarget`: any `SceneRenderTarget` attachment
copied to the grab pass by `PostProcessor::recordBlit()` needs `TRANSFER_SRC_BIT` at creation
(all six attachments — color, normals, material properties, albedo, velocity, depth — have it).

**A second write-direction case (Aug 2026):** the pre-translucency write-back blits a CombinePass
target INTO the scene colour image, so the target needs `TRANSFER_SRC_BIT` and the scene colour
`TRANSFER_DST_BIT` (`SceneRenderTarget.cpp`, colour only). ⚠️ On NVIDIA the blit without them ran
and produced a plausible image — only the four VUIDs told; see § "The frame is CUT around the
translucent pass".

**And it applies in the WRITE direction too (Aug 2026).** `RenderTarget::ShadowMap` clears its
depth image to 1.0 through `TransferManager::clearDepthImage()` at creation, so that image needs
`VK_IMAGE_USAGE_TRANSFER_DST_BIT` alongside `DEPTH_STENCIL_ATTACHMENT_BIT | SAMPLED_BIT`. It was
missing for a while: the clear and both surrounding barriers were rejected, the image never left
`UNDEFINED`, and **`vkQueueSubmit` refused the shadow pass on every frame** — directional shadows
gone, silently, on the NVIDIA driver. Full cascade in `docs/caution-points.md` § Vulkan
Validation; per-target flag table in `docs/render-targets.md`.

**Readback is a declared capability, not a fallback.** `TransferManager::downloadImage()` now
**refuses** an image that lacks `TRANSFER_SRC_BIT` (it used to attempt a blit through
`VK_IMAGE_LAYOUT_GENERAL`, which is invalid by construction — `vkCmdBlitImage` requires the usage
bit on `srcImage` whatever the layout). Consequently `RenderTarget::Texture` declares
`TRANSFER_SRC_BIT` unconditionally on its **color** image so `capture()` and the
`dumpRenderTarget` console command work; its **depth** image deliberately does not, because
nothing reads it back and `TRANSFER_SRC` is the flag that costs depth-compression metadata on
some architectures. Grant the flag at the creation site, never work around its absence.
