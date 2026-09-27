## The per-vertex stage contract — `AbstractVertexStage` (Sep 2026)

The material, light and shadow generators address **`AbstractVertexStage &`**, never `VertexShader &`:
`Material::Interface::generateVertexShaderCode()` / `generateShadowVertexCode()`, the three
`Material/Helpers` texture-coordinate checks, and `LightGenerator::generateVertexShaderCode()` /
`generatePBRVertexShader()` / `generateVertexShaderShadowMapCode()`. The synthesis machinery lives in that
base, so a second per-vertex stage — the MESH shader, engine item `mesh-shader-displaced-surface` — gets
every synthetic variable without a second copy (owner decisions, 2026-09-22: an abstract stage interface,
and **overloads, not virtuals**, where the stages diverge: connection to the fragment stage, output
declaration, `gl_Position`, the draw).

⚠️ It was extracted by MOVING `VertexShader.cpp` (now `AbstractVertexStage.cpp`), not rewriting it, and
proven inert: the generated GLSL of `relief`, `light-and-shadow-debug`, `forest`, `terrain` and
`animation-debug` — **410 sources** across SceneRendering, ShadowCasting, PostProcessing and
OverlayRendering — is byte-identical before and after (`--settings-filepath` on a copy with
`EnableSourceCodeDump`, `--cache-directory` per run, `diff -r`). Use the same A/B for any change meant to
be codegen-neutral.

What stays VertexShader-only on purpose: `FragmentShader`/`GeometryShader`/`TesselationControlShader::connectFromPreviousShader(const VertexShader &)`,
`VertexBufferFormatManager::getVertexBufferFormat()` (only a vertex shader has vertex attributes) and
`Program::vertexShader()`.
