## Queue Family Ownership Transfer (buffer uploads)

Buffers are created `VK_SHARING_MODE_EXCLUSIVE`. When the transfer queue and the graphics
queue belong to different families, moving an uploaded buffer from one to the other requires a
**two-sided ownership transfer**: a release on the source queue, a matching acquire on the
destination queue.

> [!CRITICAL]
> **A release is a ONE-SHOT token, and both halves belong to the SAME operation.** Do NOT
> leave the acquire to "whatever reads the buffer first on the graphics queue": the second
> reader then acquires with nothing to match, `vkQueueSubmit()` is rejected
> (`VUID-vkQueueSubmit-pSubmits-02207`, `VK_ERROR_VALIDATION_FAILED_EXT`) and the whole
> submission is lost. That is exactly what happened (Aug 2026) when two instances of the same
> skeletal mesh each built their own refit-able BLAS from the same vertex buffer.

`BufferTransferOperation::transfer()` therefore performs BOTH halves, in the two-step shape
already used by `ImageTransferOperation`:

1. **Transfer queue** — copy + release barrier, submission signals the operation semaphore.
2. **Graphics queue** — acquire barrier (same buffer, same range, same families), submission
   waits on that semaphore and signals the operation fence, so the operation only becomes
   reusable once BOTH command buffers are idle.

```cpp
/* Release (transfer queue), then acquire (graphics queue) — BufferTransferOperation.cpp */
ownershipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;   /* release */
ownershipBarrier.dstAccessMask = 0;
ownershipBarrier.srcQueueFamilyIndex = transferFamilyIndex;
ownershipBarrier.dstQueueFamilyIndex = graphicsFamilyIndex;
/* ... same barrier, other half ... */
ownershipBarrier.srcAccessMask = 0;                              /* acquire */
ownershipBarrier.dstAccessMask = VERTEX_ATTRIBUTE | INDEX | UNIFORM | SHADER | AS_READ;
```

**The invariant this buys:** *once uploaded, a buffer belongs to the GRAPHICS family.* Every
later consumer — vertex input, shaders, acceleration structure builds — reads it with no
ownership barrier of its own. `AccelerationStructureBuilder::buildBLAS()` consequently records
a plain `VkMemoryBarrier` only, never an acquire.

On a single-queue-family device (`Device::hasBasicSupport()`) there is no ownership to
transfer AND no graphics command pool: both conditions travel together, and the operation
falls back to a single submission.

**Code references:**
- `BufferTransferOperation.cpp:transfer()` — the release/acquire pair, semaphore-ordered
- `BufferTransferOperation.hpp` — `m_graphicsCommandBuffer`, `m_semaphore` (only created when the families differ)
- `TransferManager.cpp` — passes both command pools to the operation
- `AccelerationStructureBuilder.cpp:buildBLAS()` — memory barrier only, NO acquire
