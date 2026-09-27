## Critical: Deferred destruction contract (`DeferredDestructor`)

> [!CRITICAL]
> **Never destroy a GPU-visible Vulkan object in place at runtime.** With N frames in
> flight, a command buffer submitted at frame F still executes while the CPU prepares
> frame F+1: destroying a descriptor set, buffer, image or acceleration structure "as soon
> as the CPU is done with it" pulls it from under the GPU — validation errors at best,
> `VK_ERROR_DEVICE_LOST` (Xid 109) or segfaults at worst.
>
> **The contract:** route every runtime destruction through the renderer-owned queue
> `Vulkan::DeferredDestructor` (`Renderer::deferredDestructor()`, header
> `Vulkan/DeferredDestructor.hpp`):
> - `retireObject(std::unique_ptr<T> / std::shared_ptr<T>)` — keeps the object alive and
>   destroys it after `framesInFlight` render ticks.
> - `retireAction(std::function<void()>)` — for objects needing an explicit tear-down call
>   (e.g. `RenderTarget::Abstract::destroyRenderTarget()`); capture via `std::shared_ptr`.
> - `tick()` is called once per frame by `Renderer::renderFrame()` right after the frame
>   fence wait; `flush()` runs at `Renderer::onTerminate()` after the final device idle.
> - Retiring is **thread-safe** (logic or render thread); `tick()`/`flush()` belong to the
>   render thread.
>
> **Do NOT** use `vkDeviceWaitIdle()` as a destruction guard in per-frame code paths — it
> stalls the whole GPU and silently proceeds on device-loss errors.
>
> Migrated call sites (2026-07-05): `SceneMetaData` (TLAS + build requests + per-frame RT
> SSBOs — both the empty-instances path and the per-rebuild retirement), the
> `PostProcessor::configure()` grab pass + per-frame descriptor sets (previously a
> mid-frame `waitIdle`), `Renderer::recreateSceneTarget()` (previous target retired — its
> in-place destruction freed the view-matrices descriptor set/UBO under in-flight frames),
> and the renderer scene-target disable path (previously a local vector + countdown).
> **Any new runtime destruction path MUST use this service.**
>
> Migrated 2026-08-05 (VUID `vkDestroyAccelerationStructureKHR`/`vkDestroyBuffer` "in use"
> seen live on scene switches): `Geometry::Interface` destructor (per-geometry BLAS — a
> geometry resource can unload while ray queries reference it) and
> `RenderableInstance::Abstract` destructor (the whole per-instance skinned GPU set: refit
> BLAS, skinned mirror + scratch buffers, skinning SSBO, descriptor sets then their pool —
> an instance dies at runtime on actor death). Both keep the destructor pointer from their
> creation site (`serviceProvider().graphicsRenderer()` / `prepareSkinningResources()`).
>
> Known candidates NOT yet migrated: `Overlay::Surface` framebuffer recreation,
> material/shared-UBO teardown paths, `LightSet::terminate()` (scene teardown — currently
> in-place).
