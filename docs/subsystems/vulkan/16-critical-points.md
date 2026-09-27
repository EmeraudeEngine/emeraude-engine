## Critical Points

- **Ordered destruction**: Destroy resources in reverse creation order
- **Thread safety**: CommandPool per thread, CommandBuffers not shared
- **Memory barriers**: Correct state transitions for images
- **Queue family ownership**: release AND acquire in the SAME operation (BufferTransferOperation) — a release is a one-shot token
- **TLAS barriers**: Use `FRAGMENT_SHADER_BIT | COMPUTE_SHADER_BIT`, NOT `RAY_TRACING_SHADER_BIT_KHR` (ray queries, not RT pipelines)
- **Present semaphores**: indexed by **acquired swap-chain image**, never by frame in flight — no fence observes a present (see Synchronization above)
- **Validation layers**: Always active in development (note: ~6% CPU overhead, ~41% when combined with rwlock)
- **Never direct calls**: Graphics, Resources, Saphir use Vulkan abstractions
- **VMA mandatory**: All GPU allocation via VMA, never direct vkAllocateMemory
- **Y-up setup**: the projection carries the Y flip for Vulkan's Y-down NDC; the world itself stays Y-up
- **`VK_ERROR_SURFACE_LOST_KHR` is NOT a GPU fault** — the window is gone, the device is fine. Do not investigate it alongside `VK_ERROR_DEVICE_LOST`. On Wayland it means the compositor killed the `wl_display` connection over a protocol error, printed on stderr by libwayland just above (a `wl_`/`wp_` interface name); nothing is recoverable, since every Wayland object of the process dies with the connection. `vkResultDiagnosticHint()` (`Vulkan/Utility.hpp`) carries that reading and both reporting sites print it — `Queue::present()` and `SwapChain::acquireNextImage()`. Add a case there rather than re-explaining a misread code in a comment. To attribute an occurrence, do not read the trace by hand: `tools/wayland-protocol-trace.py --capture -- <command>` then `--analyse <log>` names the committer. Open occurrence: [`docs/todo/wayland-surface-lost-protocol-error.md`](../../todo/wayland-surface-lost-protocol-error.md).
