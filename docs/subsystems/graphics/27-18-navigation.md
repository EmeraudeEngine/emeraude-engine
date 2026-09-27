## 18. Navigation

-   **Base Class**: `Renderable::Abstract`
-   **Main Entry**: `Renderer` (Central coordinator)
-   **Scene Bridge**: `Components::Visual`
-   **Shader Cache**: [`src/Saphir/AGENTS.md`](../../../src/Saphir/AGENTS.md) - 3-level cache system
-   **On-disk Caches**: See [Section 4](#persistent-on-disk-caches--the-renderer-owns-the-pipeline-cache-io) - `Renderer` does the `VkPipelineCache` disk I/O; SPIR-V binary cache ON by default
-   **Swap-Chain/VSync**: [`src/Vulkan/AGENTS.md`](../../../src/Vulkan/AGENTS.md) - Present mode selection
-   **Pattern Examples**: [`docs/development-patterns.md`](../../development-patterns.md)
-   **Material JSON format**: See `docs/development-patterns.md#material-json-format-unified`
-   **Shadow Mapping**: [`docs/shadow-mapping.md`](../../shadow-mapping.md) - PCF, global control, per-light settings
-   **Animated Cubemaps**: See [Section 11](#11-animated-texture-cubemap-system) - CubemapMovieResource + AnimatedTextureCubemap
-   **Post-Processing**: See [Section 12](#12-post-processing-effects) - RTR, SSR, ContactShadows, SSAO, VeilingGlare, DoF, AtmosphericFog, VolumetricLight, LensFlare, ToneMapping
-   **Overflow census** (NaN / Inf / fp16-ceiling texel counts, `OverflowCensus`, `FrameDiagnostics`): See [Section 12](#the-overflow-census--counting-what-fp16-does-to-the-scene-radiance-sep-2026)
-   **Instance Program Cache**: See [Section 15](#15-instance-local-program-cache-renderableinstance) - Per-instance resolved program cache
-   **Frame Sync**: See [Section 16](#16-frame-synchronization--double-buffering-gpu-and-the-logic-triple-buffer) - Per-frame buffers, view matrix state index
-   **Compute Shaders**: See below - GPU compute pipeline for non-rendering workloads
-   **Raw Geometry**: See [Section 19](#19-raw-geometry-system) - Direct GPU upload from raw buffers
