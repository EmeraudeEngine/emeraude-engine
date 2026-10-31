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

## Optimising Saphir

- Saphir generates GLSL; it does not run per frame. Its cost is LOAD time: program creation and every program-cache
  miss. Optimisations target string allocation and copying, never ALU.
- Known waste pattern: building a `std::stringstream`, copying it with `.str()`, then streaming that into `Code`
  (which owns its own stream). Stream or `+=` directly into `Code`.
- Keep `stringstream` where it formats a raw float: `std::to_string(0.5f)` gives `"0.500000"`, which changes the
  generated GLSL bytes.
- An optimisation pass keeps the generated GLSL byte-identical and never touches program-cache keys. Validate with a
  pixel diff against a reference capture; differences of a few LSB on a handful of pixels are the capture's temporal
  noise, not a change.
- clang-tidy over `src/Saphir/` mostly reports included emeraude-base headers (`HeaderFilterRegex: 'src/.*'`):
  filter the output on `/src/Saphir/`.
