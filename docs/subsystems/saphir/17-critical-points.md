## Critical Points

- **Strict checking**: Material requirements MUST be satisfied by geometry
- **Hash-based cache**: Identical inputs → same shader (performance)
- **Fail-safe integration**: Failures logged but app continues (no crash)
- **Y-up convention**: the world is Y-up; the projection carries the single Y flip that reconciles it with Vulkan's Y-down NDC
- **Thread safety**: Cache protected, generation can be parallel
- **Used by Graphics and Overlay**: Graphics (3D), Overlay (2D) use Saphir
- **Runtime generation**: Shaders generated on demand during resource loading
- **Alpha preservation**: Lighting calculations use `.rgb` only, never modify alpha channel. See: `LightGenerator.cpp:603-661`
- **Color space conversion** (3D only): sRGB↔Linear functions apply gamma only to RGB, alpha passes through unchanged. See: `FragmentShader.cpp:generateToSRGBColorFunction()`, `generateToLinearColorFunction()`. Note: Overlay system does NOT use color space conversion - swap-chain format (UNORM vs SRGB) determines final handling.
