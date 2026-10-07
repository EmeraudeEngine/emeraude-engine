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

## Critical: an object released while its UPLOAD runs (queue timelines, 2026-10-07)

> [!CRITICAL]
> The frame delay above does not cover uploads. `TransferManager` returns as soon as a copy is **submitted**; the
> operation's fence is read only when the pool next looks for a free operation. A buffer or image released in
> between (a loader that refuses its file after creating its textures, a LOD level dropped right after its
> upload) was destroyed under the copy: `VUID-vkDestroyBuffer-buffer-00922` / `VUID-vkDestroyImage-image-01000`,
> and since the validation layer then SKIPS the destroy, as many `VUID-vkDestroyDevice-device-05137` at shutdown.
>
> **The mechanism (owner's choice, 2026-10-07: timeline + deferred retirement):**
> - Every `Vulkan::Queue` owns a **timeline semaphore** (Vulkan 1.2 core; the device requires `timelineSemaphore`).
>   `SynchInfo::tracksCompletion(value)` makes a submission also signal it, with a value taken under the device
>   lock that serializes `vkQueueSubmit()` — increasing per queue. `Queue::isReached(value)` /
>   `waitUntilReached(value, timeout)` read it.
> - `ImageTransferOperation` / `BufferTransferOperation` track BOTH submissions of a transfer (the copy on the
>   transfer queue, the ownership acquire on the graphics queue) and record them on the destination:
>   `Image::recordPendingSubmission()` / `Buffer::recordPendingSubmission()` (`Vulkan::PendingSubmissions`, one
>   point per queue).
> - `Image` / `Buffer::destroyFromHardware()` hand the handle and its memory (VMA allocation, or `DeviceMemory`)
>   to `Device::destroyAfter()` while a point is unreached. With the renderer's queue registered
>   (`Device::setDeferredDestructor()`, done when the renderer gets its device, undone at the top of
>   `Renderer::onTerminate()` and in its destructor) it becomes a `DeferredDestructor::retireActionWhen()` entry:
>   destroyed after the frame delay AND once the points are reached. Without it, the calling thread waits for the
>   points (10 s bound), then destroys.
> - Any other path that writes a buffer or an image asynchronously must do the same: `tracksCompletion()` on its
>   submission, `recordPendingSubmission()` on the object.
>
> Proof (Linux, RTX 3070 Ti, validation on): citadel with `Core/Graphics/LOD/EnableAutomaticGeneration` true, 60 s:
> **40 VUIDs → 0** (20 × 00922 + 20 × 05137), the same 94 LOD levels ready. citadel, sponza, asset-loader,
> light-and-shadow-debug: 0 validation error (⚠️ the first record named `gltf-loader`, a demo id that no longer exists:
> that run loaded nothing; re-measured with `asset-loader`). **ACCEPTED on the three OS (engine 3c498654,
> 2026-10-07):** macOS M2 (MoltenVK exposes the timeline) citadel LOD 0 VUID / 0 SYNC-HAZARD, 94 levels; Windows
> NVIDIA RTX 3060 + AMD iGPU citadel LOD 94 levels, 0 upload VUID, and the 2026-10-02 citadel teardown (12 VUIDs)
> 0 / 6 runs.
> The culprit of that run: `MultiLayerMeshResource::generateLODLevel()` dropped every generated level of a mesh that
> carried its own levels, right after its upload — such a mesh is now skipped (graphics doc 20-15b, 2026-10-07).
> ⚠️ NOT covered by this mechanism, and fixed separately (2026-10-07): an image destroyed by another thread while
> it was still being CREATED (`vkBindImageMemory` on `VK_NULL_HANDLE`) — `onDependenciesLoaded()` ran twice at once
> (resources doc 07, "`onDependenciesLoaded()` runs ONCE").
