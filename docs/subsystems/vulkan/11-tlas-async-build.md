## TLAS Async Build (Inline Command Buffer Recording)

> [!CRITICAL]
> **TLAS builds MUST be recorded inline into the render command buffer, NOT submitted synchronously.**
>
> The old `buildTLAS()` method used a dedicated command buffer with a fence wait per frame,
> causing massive scheduler overhead (`sched_yield` dominated profiles). The new async API
> splits TLAS building into CPU-side preparation and GPU-side command recording.
>
> **API:**
> - `prepareTLAS(instances, instanceCount)` — CPU-side: creates/resizes buffers, uploads instance data. Returns `TLASBuildRequest`.
> - `recordTLASBuild(commandBuffer, request)` — GPU-side: records `vkCmdBuildAccelerationStructuresKHR` + barrier into an external command buffer.
>
> **TLASBuildRequest** owns the TLAS + instance buffer + scratch buffer for the current build.
> After recording, the request is retired through the central `Vulkan::DeferredDestructor`
> (see the dedicated section below) for frames-in-flight safety.
>
> **Buffer lifetime:** TLAS buffers are per-request (not persistent). Each build creates fresh
> buffers. Retired requests are frame-stamped and destroyed by the deferred destructor once
> `framesInFlight` render ticks have elapsed. (History: a count-capped deque — "keep at most
> 3" — was used before 2026-07-05; it under-covered rebuild bursts during scene streaming and
> caused GPU use-after-free → Xid 109 CTX SWITCH TIMEOUT → `VK_ERROR_DEVICE_LOST`.)
>
> **Pipeline barrier:** The barrier after TLAS build uses `FRAGMENT_SHADER_BIT | COMPUTE_SHADER_BIT`
> as destination stage (NOT `RAY_TRACING_SHADER_BIT_KHR`). The engine uses **ray queries**
> (`GL_EXT_ray_query`) in fragment/compute shaders, not RT pipelines. Using
> `VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR` requires `VK_KHR_ray_tracing_pipeline` which
> is not enabled.
>
> **Measured impact:** `sched_yield` -79%, scheduler overhead -88% (perf profiling).
>
> **Code references:**
> - `AccelerationStructureBuilder.hpp` — `TLASBuildRequest` struct, `prepareTLAS()`, `recordTLASBuild()`
> - `AccelerationStructureBuilder.cpp` — Implementation
> - `Scenes/SceneMetaData.cpp:recordTLASBuild()` — Delegates to builder
> - `Graphics/Renderer.cpp:renderFrameWithInternal/renderFrameDirect` — Calls `scene->recordTLASBuild()` after `prepareRender()`, before `beginRenderPass()`
