## Heightfield surface — a vertex stage that BUILDS the surface (Sep 2026)

A geometry flagged `Geometry::EnableHeightfieldSurface` (`CDLODTerrainResource`, every terrain) has a
vertex buffer of flat patch positions and nothing else: the vertex stage places each point in its
quadtree node, geomorphs it and lifts it from the height clipmap, and SYNTHESIZES the normal, the
tangent frame and the UV. The contract (`Graphics/Geometry/HeightfieldSurface.hpp`, helper
`Generator/HeightfieldSurfaceHelper.{hpp,cpp}`):

- **Sets**: the program's `PerModel` set is the surface (`getHeightfieldSurfaceDescriptorSetLayout()`:
  height clipmap, normal clipmap, per-frame uniforms, vertex+fragment) — the skinning SSBO otherwise;
  a heightfield is never skinned. `RenderableInstance::Abstract::bindPerModelSet()` is the ONE binding site.
- **Switch order**: `enableHeightfieldSurface()` right after `initVertexShader()`, BEFORE
  `declareMatrixPushConstantBlock()` — the block appends `heightfieldNode` + `heightfieldCamera` for it
  and records the node's offset on the program (`Program::heightfieldPushConstantOffset()`, read by the
  draw that pushes each node). Largest block: 76 B of matrices, node at 80, camera at 96, end 112 —
  ⚠️ no room for a third per-node vec4 under the 128 B guarantee.
- **ONE site per surface expression**: `vertexPositionExpression()` (heightfield → `hfPosition`, else
  wind → skinning → attribute) and `vertexFrameExpression(Tangent|Binormal|Normal)` (heightfield →
  `hfTangent`/`hfBinormal`/`hfNormal`, else skinned → attribute; it replaced SIX copies of the skinning
  ternary). Every synthesizer declares a frame attribute through `declareFrameAttribute()`, a no-op that
  records the request on a heightfield — the vertex input then holds the position alone, which is what
  `VertexBufferFormatManager` builds the layout from. A synthesizer that spells an attribute directly
  breaks the heightfield: go through these.
- **Per-pixel frame**: `VertexShader::enableHeightfieldPixelFrame()` + `FragmentShader::enableHeightfieldPixelFrame()`
  (before `connectFromPreviousShader()`): the fragment receives the interpolated frame variables under
  other names (Vulkan links stages by LOCATION) and `main()` redefines the canonical ones from the normal
  clipmap, with the vertex stage's own formulas applied to the pixel's T/B/N (world = model · v, view =
  normalize(normal matrix · v), `svTangentToWorld` = normal matrix · mat3(T, B, N)). The material code
  reads the canonical names and is untouched. The pixel-frame outputs are declared BEFORE
  `generateMainUniqueInstructions()` — ⚠️ a preparation (normal matrix, model matrix) asked after it is
  never emitted.
- **Cache key**: both `computeProgramCacheKey()` hash `isHeightfieldSurfaceEnabled()`.

⚠️⚠️⚠️ **`PushConstantBlock::bytes()` is the std430 layout, EXACTLY** — a vec2 is 8 bytes, a vec3 12
(16 in an array). It used to add `size_bytes()`, which pads both to 16: every member after the TAA
jitter was 8 bytes later on the CPU than in the SPIR-V, harmless while the number only sized a range,
fatal the day an offset was taken from it (the terrain vanished, zero validation messages). And a
block must MIRROR the struct pushed into it: the post-processing block lacked `deltaTime` (24 B
declared, 28 pushed — `VUID-vkCmdPushConstants-offset-01795` once the size was exact).
⚠️⚠️ **Functions are emitted LAST**, right before `main()` (`AbstractShader::generateSourceCode()`): a
function may read a uniform block, a sampler or a push constant (`hfHeight()` reads all three). They
were emitted before every resource, which held only because none did.
⚠️ Read the generated GLSL (`Core/Graphics/Shader/ShowSourceCode`) before blaming the data: the
invisible-terrain defect was found by comparing the offset the CPU pushed with the block the log printed.
