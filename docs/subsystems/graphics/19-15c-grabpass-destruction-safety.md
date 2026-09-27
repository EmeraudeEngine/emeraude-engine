## 15c. GrabPass Destruction Safety (Fixed Mar 2026)

> [!WARNING]
> **`PostProcessor::configure()` must call `device->waitIdle()` before destroying the old
> GrabPass.** In-flight command buffers may still reference the old GrabPass's image/sampler.
> Destroying without waiting causes use-after-free Vulkan validation errors.
>
> **Code reference:** `Graphics/PostProcessor.cpp:configure()`
