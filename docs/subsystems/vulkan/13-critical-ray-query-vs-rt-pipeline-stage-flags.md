## Critical: Ray Query vs RT Pipeline Stage Flags

> [!CRITICAL]
> **The engine uses `GL_EXT_ray_query` (ray queries in fragment/compute shaders), NOT `VK_KHR_ray_tracing_pipeline`.**
>
> This means:
> - TLAS access happens in **fragment shaders** and **compute shaders**
> - Pipeline barriers must use `VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT`
> - **NEVER use** `VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR` — it requires enabling the RT pipeline extension
> - Access mask: `VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR` is correct for both approaches
