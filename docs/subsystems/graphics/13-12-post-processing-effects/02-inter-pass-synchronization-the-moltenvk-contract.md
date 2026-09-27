## 12. Post-Processing Effects

### Inter-Pass Synchronization — the MoltenVK contract (Aug 2026)

Chained post-process passes render into `IntermediateRenderTarget`s whose render pass declares
full (non-by-region) `VK_SUBPASS_EXTERNAL` dependencies in both directions — that is the
**Vulkan-level** guarantee, and Synchronization Validation is clean with it alone.

It is NOT sufficient on MoltenVK. Back-to-back render passes become separate Metal command
encoders, and the external-dependency translation was **measured insufficient on Apple M2**:
tile-granular stale reads in the motion-blur chain (displaced 64 px blocks around moving
objects), uninitialized reads in the bloom chain (green blocks), and implausible luminance
samples in the auto-exposure chain (`ToneMapping::meteredRejectedCount()` at ~2/s). The
corruption is timing-dependent — `MTL_DEBUG_LAYER=1` serialization suppresses it completely,
which is how it was cornered.

**The contract:** `IndirectPostProcessEffect::recordFullscreenPass()` emits an explicit
`vkCmdPipelineBarrier` (write→read, `SHADER_READ_ONLY → SHADER_READ_ONLY`, no layout change)
after `endRenderPass()`, forcing a real inter-encoder fence. On conforming desktop drivers it
is redundant with the subpass dependencies and free. **Any pass recorded OUTSIDE
`recordFullscreenPass()` that writes an image another pass samples must emit the same barrier
itself.** Full story: `docs/troubleshooting.md` § "Blocky corruption on macOS",
`docs/caution-points.md` § Vulkan Validation.
