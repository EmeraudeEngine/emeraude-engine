## The mesh-shading surface (Sep 2026)

A geometry flagged `EnableMeshShadingSurface` (`Graphics::Geometry::DisplacedGridResource`) draws through task + mesh
stages when `Generator::Abstract::isMeshShadingSurfaceEnabled()`: that is, the geometry is such a surface, a
material exists, and the device enabled `VK_EXT_mesh_shader`, which is latched at the start of
`generateShaderProgram()` and enters the cache key. Otherwise it draws its flat grid through the vertex stage, and
the log says so once.
- `SceneRendering::generateMeshShadingStages()` creates the mesh stage (117 vertices / 256 primitives, workgroup 32)
  and the task stage. It sets the modes `initVertexShader()` would set (advanced matrices, cubemap), then runs the
  SAME `configurePerVertexStage()` as the vertex path: instance transforms, push constants, view UBO, velocity,
  material and light code.
- `Generator::generateMeshShadingSurface()` (`MeshShadingSurfaceHelper.cpp`) then writes:
  - the task code: one workgroup per 1 m tile. It picks the subdivision from the camera distance (≈ 4 mm per metre,
    a power of two up to 128 quads per side) and emits one flat quad beyond the material's handover band;
  - the mesh vertex source: grid and SKIRT vertices, the flat grid's own UV and frame, and the depth from
    `Material::Interface::generateSurfaceDisplacementCode()`;
  - the primitive source: the flat grid's winding, skirts in both windings.
- Push constants: the matrices block gains `meshSurfaceGrid` (origin, tile size, UV per metre) and `meshSurfaceView`
  (camera, then the InstanceTransforms slot as raw bits — a mesh stage has no `gl_InstanceIndex`) at
  `Program::meshSurfacePushConstantOffset()` (80, 112 B in all). The draw is
  `RenderableInstance::…::drawMeshShadingSurface()`: `drawMeshTasks(tileCountX, tileCountZ, 1)`.
- FRUSTUM CULLING in the task stage (2026-09-23): `declareMeshSurfaceCullingMatrix()` declares in the task stage what
  its local-to-clip matrix needs and returns that matrix's expression. It follows the branches of
  `declareMatrixPushConstantBlock()`: P from the view UBO × the pushed V × the InstanceTransforms model in the lit
  passes, the pushed MVP in a classic shadow map. A tile's box, from the plane down to the deepest relief, is
  rejected when all 8 corners are out on the same SIDE plane or behind w = 0; near/far are never tested (depth
  convention, shadow depth clamp). Cubemap and CSM targets are NOT culled: their matrices come from a UBO
  array (per `gl_ViewIndex` for a cubemap, per the pushed `cascadeIndex` for a CSM — one pass per cascade since
  2026-09-24, so a CSM task stage could now cull against its cascade; not done). `Abstract::declareInstanceTransformsBlock()` is now the single declaration of that SSBO layout
  (scene pass, shadow pass, task stage).
- The SHADOW program takes the same stages (`ShadowCasting::generateMeshShadingStages()`, 2026-09-23): the shadow map
  sees the displaced geometry, subdivided for the MAIN camera (the receiver's), like a heightfield's levels. The
  shadow draw passes that position, not the light's: from the light, every tile would be past the handover and flat.
- ⚠️ The first run failed on `pcMatrices.viewMatrix`: a mesh stage created without `enableAdvancedMatrices()` gets
  the classic VP-only block while the lit passes synthesize view-space vectors. Any new per-vertex mode the vertex
  shader takes from `initVertexShader()` must be mirrored in `generateMeshShadingStages()`.
