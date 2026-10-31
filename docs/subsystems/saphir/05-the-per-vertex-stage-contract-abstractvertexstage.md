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

### The model-view-projection matrix has ONE spelling — `modelViewProjectionMatrixExpression()` (2026-10-31)

The matrix that projects a vertex depends on the path: the pushed MVP, or a local `svModelViewProjectionMatrix`
prepared for vertex-attribute instancing, MDI, the instance-transforms SSBO, the advanced matrices, a cubemap or the
cascades (`prepareModelViewProjectionMatrix()`). `AbstractVertexStage::modelViewProjectionMatrixExpression()` is the
one place that chooses it: `synthesizeVertexPositionInScreenSpace()` goes through it, and so must any generator that
projects MORE than the vertex. ⚠️ `TBNSpaceRendering` spelled its own instanced MVP (`ViewUB(ViewProjectionMatrix)`
× the model-matrix attribute) and every instanced TBN program stopped compiling when the view-projection matrix moved
from the view block to the push constants (426 programs on projet-alpha's `city 32`, glslang "'viewProjectionMatrix'
: no such field in structure 'ubView'"). It now projects the vertex and its tangent frame in the vertex stage through
this accessor (`svTangentClipSpace`, `svBinormalClipSpace`, `svNormalClipSpace`), and its geometry stage reads clip-space
data with no matrix. The extraction is codegen-neutral: the 547 GLSL sources of `city 32` are byte-identical before and
after (`EnableSourceCodeDump`, both caches off, `--cache-directory` per run, `diff -r`).

