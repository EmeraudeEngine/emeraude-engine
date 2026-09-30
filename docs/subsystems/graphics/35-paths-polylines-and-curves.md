## Paths — polylines and curves drawn by vertex pulling (Sep 2026)

`Scenes::Component::Path`: a route, a trail, a zone border, an AI path — a polyline or a curve drawn as a
CAMERA-FACING RIBBON. Owner decisions 2026-09-29 (engine `docs/todo/segment-rendering-beams-curves-outlines.md`
§ C): points reach the GPU through an SSBO read by VERTEX PULLING; one packed path SSBO per scene and frame in flight;
{current, previous} per point; opaque unlit solid colour at a luminance in nits; miter joins falling back to bevel,
round as an option; width in metres or pixels; a DEBUG mode after the tone mapping. References: Rougier,
"Shader-Based Antialiased, Dashed, Stroked Polylines", JCGT 2013; three.js `LineSegments2`/`LineMaterial`
(`worldUnits`); A. Klein, "Rendering thick lines with dashes".

### Using it

```cpp
entity->componentBuilder< Scenes::Component::Path >("Route")
	.setup([] (Scenes::Component::Path & path) {
		path.setCatmullRom(points, /*closed*/ false);  // or setPolyline(), setUniformBSpline(), setBezierPath(Math::BSpline)
		path.setTolerance(0.01F);                      // the chord tolerance of the tessellation (1 cm)
		const auto material = path.material();         // one Material::PathResource per path
		material->setColor(color);                     // LINEAR, a hue: the brightness is the luminance
		material->setLuminance(250.0F);                // nits (a lit screen)
		material->setWidth(0.05F, false);              // half width: entity units, or pixels (true)
		material->setJoins(false, 4.0F);               // mitered (bevel past the SVG limit 4), or round
		path.markLookChanged();                        // after a material change: bounds + debug look
		path.setDebugMode(true);                       // always on top, display colour (setDebugColor(sRGBA))
	})
	.build(resources);
```

Console / MCP: `Core.SceneManagerService.Path.*` (`SceneManager_Path_*`) — `getState`, `setPoints(kind, "x y z; …",
closed)` (Polyline / UniformBSpline / CatmullRom; a Bézier path is set from code), `setTolerance`, `setColor`,
`setLuminance`, `setWidth(halfWidth, inPixels)`, `setJoins(round, miterLimit)`, `setEnabled`, `setDebugMode`,
`setDebugColor(r, g, b, opacity)`. Demo: projet-alpha `paths`.

### How it is built — the pieces

| Piece | Job |
|---|---|
| `Base::Math::CurveTessellation` (emeraude-base source-tree doc 16) | every curve → a polyline within the chord tolerance (adaptive de Casteljau) |
| `Component::Path` | the source curve, the tessellated polyline (w = arc length), bounds, publication per slot ON CHANGE only (a version per slot) |
| `RenderableInstance::PathPoints` | the published points per render state slot, the RENDER-side history (previous frame's points), the debug look; attached to the instance by `Abstract::setPathPoints()` (null for every other instance: one null check) |
| `SceneInstanceTransforms` bindings 1-2 | the path DIRECTORY (a `uvec4 {first, count}` per instance slot) and the POINTS ({current, previous} vec4 pairs), staged beside the entries (scenes doc 13) |
| `Geometry::PulledVertexResource` | a geometry WITHOUT a vertex buffer: nothing bound, the draw is `subGeometryRange(0)` = the vertex CAPACITY (grows only) |
| `Saphir::PathGLSL` | the ribbon: 9 vertices per segment, shared by the Saphir scene program and the debug overlay's hand-written shader (`rawFunctions()`) |
| `AbstractVertexStage::enablePathRibbon()` | the vertex source `pathPosition` / `previousPathPosition`, the path blocks, `svPathCoordinates` |
| `Material::PathResource` | the look UBO (radiance, style), opaque unlit, the round-cap discard |
| `Graphics::PathDebugOverlay` | the debug mode: its own per-frame SSBO (view, projection, eye, viewport, world points), no depth, alpha blended, after the outline |

### The ribbon (PathGLSL)

- 9 vertices per segment (a triangle list): the quad (0-5) and the BEVEL triangle of the join at its start (6-8),
  collapsed when unused. `s = gl_VertexIndex / 9`; past the last segment, collapsed onto the origin (no read).
- World space, facing the eye (`across = cross(segment, point → eye)`, as the beam), then `inverse(model)`.
- Joins: MITER while `1 / cos(half the turn) ≤ limit` (SVG `stroke-miterlimit`, 4), else square ends and the
  bevel triangle on the OUTER side of the turn. A segment and its neighbour compute their shared join from the SAME
  two normals: the miter edges coincide — watertight (verified on the `paths` zigzag: bevels on the sharp turns,
  a miter on the gentle one, butt caps).
- Round: the quad grows by the half width at both ends and the fragment discards outside the capsule
  (`PathGLSL::roundDiscard()`): round joins AND caps for no extra geometry.
- Width in METRES: `style.x × model scale`, the ribbon built in world space as above.
- Width in PIXELS: built in SCREEN space (Rougier's screen-space polyline). The segment ends go to view space (clipped
  to z = −1 mm in front of the eye), are projected to pixels (`pathScreen()`: clip.xy / w × viewport / 2), the 2D
  normals and the miter are computed there, and the corner offset in pixels is LIFTED back to view space at the
  point's own depth (`pathLift()`: ndc × w / P[0][0], ndc × w / P[1][1]), then to world through the view rotation.
  The half width is `style.x` pixels at ANY distance and slope (the first version scaled a world offset by the
  distance to the eye: 11.5 px near, 6.5 px far for 6 requested — Windows peer, 2026-09-29). The view matrix is the
  advanced path's push constant, or `inverse(P) × VP` on the classic path (the view UBO carries no view matrix);
  the viewport is `ViewProperties.xy`.
  Measured (Linux, TAA off, half width 3): 6.0 px at 7, 14, 28 and 50 m; the 1-pixel mast 2.0 px at every distance.
- The velocity pass rebuilds the ribbon from the PREVIOUS points (the render-side history) and the previous model
  matrix: a turning path (the `paths` loop) stays crisp under TAA.

### The colour (owner decision: colour + luminance)

An unlit surface writes its colour as RADIANCE (SceneRendering's unlit branch: normal facing the eye, neutral
material properties). The path's colour is a linear hue × a luminance in nits (250 by default): the exposure treats
it like any emitter — brilliant at night, modest in full sun. A colour exact ON SCREEN whatever the exposure is the
debug mode's (the exposure is not in the shaders; putting it there was rejected — the GPU auto-exposure would lag a
CPU copy by a frame).

### The depth offset (2026-09-30)

Owner decisions: an offset TOWARD THE EYE, in metres, off by default — `PathResource::setDepthOffset(m)`, console
`Path.setDepthOffset(entity, component, m)`, the third UBO vec4 (`PathPlacement.x`). `pathCorner()` moves each corner
along its own ray toward the eye (its screen position unchanged, so a pixel width stays exact), by at most half the
distance; the scene pass, the velocity and the selection depth share it (one `preparePathRibbon()`), the debug overlay
passes 0 (it draws on top).

⚠️ **A fixed offset cannot hold a path lying on the ground at every distance.** Measured (Linux, the `paths` zigzags
5 cm above the ground, 3-pixel half width, camera 1.7 m high looking at the horizon, TAA off):

| Offset | 14 m | 28 m | 50 m |
|---|---|---|---|
| 0 | 6.0 px | 5.0 | 4.0 |
| 0.1 m | 6.0 | 5.0 | 4.0 |
| 1 m | 6.0 | 6.0 | 5.0 |
| 3 m | 6.0 | 6.0 | 6.0 |

The ribbon's lower edge dips a half width BELOW the surface (camera-facing), and the ray grazing the ground descends
only `height / distance` per metre: the depth to win is `half width / sin(grazing angle)` — about the square of the
distance for a pixel width (a metre at 28 m, three at 50 m). An offset that large lets any object within it stop
hiding the path. Open (owner decision): a ribbon LYING in the surface's plane, an offset derived from that geometry,
or this offset for near and steep views only.

### The selection outline (2026-09-30)

A highlighted path is outlined like any entity (graphics doc 34): the custom depth pass draws it with the depth-only
shadow-casting program, which builds the SAME ribbon.
- `Saphir::Generator::ShadowCasting::isPulledVertexGeometry()` (a geometry without a vertex buffer) switches the vertex
  stage to the INSTANCE-TRANSFORMS path (`enableInstanceTransforms()` before the push block: VP + jitter + frameIndex),
  and adds the view set (projection, eye, viewport for the pixel width) and the instance-transforms set (the model
  matrix, the directory, the points). In the program key.
- `Material::PathResource::requiresAlphaTestedShadows()` is ALWAYS true: the material set joins the layout,
  `generateShadowVertexCode()` enables the ribbon and declares the style UBO, `generateShadowAlphaTestCode()` discards
  outside the round capsule — the silhouette of a round path has round caps and joins.
- `castShadows()` passes `useInstanceTransforms`: `Unique::pushMatricesForShadowCasting()` pushes the UNJITTERED VP
  (zero jitter) and the draw's firstInstance is the instance's entry slot, as in `render()`.
- ⚠️ **Only a path staged THIS frame is outlined**: the slot is frame-linear, and a culled path keeps the one of an
  older frame, which names ANOTHER instance's entry now. `SceneInstanceTransforms::frameSerial()` (incremented by
  `beginFrame()`) is recorded with the slot; `RenderableInstance::Abstract::isInstanceTransformsSlotStaged()` gates
  `Scene::renderSelectionDepth()`, with `isDrawnInScene()` (a hidden, empty or debug-drawn path has no outline).
- Verified (Linux, `paths`, 2026-09-30): the six paths outlined, the 2-pixel mast and the pixel zigzags included, round
  caps round, the turning loop followed; nothing drawn with every path behind the camera; a hidden and a debug-drawn
  path not outlined; `geometry-generator`'s figurine unchanged; 0 VUID. The dumped depth program has no vertex input.
- A 1-2 px "step" in the rim at the pixel mast's elbow (macOS peer, 2026-09-30) is the RIBBON's own staircase: the arm,
  2 px thick and slightly sloped in perspective, rises one pixel row a few pixels past the corner, and the rim follows
  the ribbon's pixels exactly (pixel dump, TAA off and on). A join defect would show in the ribbon, not in the rim.

### ⚠️ Traps and limits

- ⚠️⚠️ **A pulled-vertex pipeline must declare NO vertex input** (found by the macOS peer, 2026-09-29, fixed the same
  day). Two independent leaks: the position syntheses (`synthesizeVertexPositionIn*Space()`, the rest position)
  declared the position attribute UNCONDITIONALLY — `vaVertex`, declared and never read — and `Generator::Abstract`
  built every pipeline's vertex input from the vertex format, so the path pipeline declared binding 0 (stride 12,
  location 0). Nothing is bound for a path: the pipeline read the PREVIOUS draw's buffer, and
  `VUID-vkCmdDraw-None-04007` fired only while no draw had bound one yet in the command buffer — on the M2 until the
  ground loaded (300-390 messages), on Linux never (the ground loads first): latent everywhere, visible on one
  machine. Now: `AbstractVertexStage::declarePositionAttribute()` declares nothing for the path ribbon, and a geometry
  without a vertex buffer gets `configureEmptyVertexInputState()`. ⚠️ Checking a dumped shader for inputs: the
  generator writes `layout (location = N) in` WITH a space — a grep for `layout(location` finds nothing and lies
  (it did, the day of the finding).
- **A hidden, empty or debug-drawn path issues no draw**: `RenderableInstance::Abstract::isDrawnInScene()` keeps it
  out of the render lists AFTER its staging (the overlay's copy comes from that staging).

- ⚠️ **Measuring a pixel width**: switch TemporalAA OFF (`PostProcess.disable(TemporalAA)`) — the resolve softens
  the edges and a saturated line (250 nits at a night exposure clips to white) turns that softening into +1.5-2 px
  at half maximum (7.8 px measured for 6). And lift the path off the ground: a camera-facing ribbon lying ON a
  surface sinks its lower half into it at grazing angles — the paths demo's zigzags (5 cm above the ground) lose a
  row at 28 m and two at 50 m to the DEPTH TEST, not to the width (lifted 60 cm: 6.0 px at 50 m). Depth, not a
  defect; a path drawn on a surface needs a small lift (a depth bias is not implemented).
- **The directory entry must always exist.** A path with no point (hidden, or drawn by the overlay) still stages an
  EMPTY entry: its vertex stage reads `pathSpans[slot]` whatever, and a slot past the end of the directory is an
  out-of-bounds read. And the collapse returns `vec3(0)` WITHOUT calling `pathPoint()`: `clamp(i, 0, count − 1)` is
  undefined in GLSL for count 0. Both found in review before any symptom.
- **The shadow-casting programs never call `prepareVertexStage()`**: before 2026-09-30 they drew a path as an
  ordinary mesh — a position attribute with no vertex buffer, garbage (a spurious outline, 0 VUID). A path casts no
  shadow (`disableShadowCasting()`); the selection outline now builds its ribbon (§ The selection outline below).
- **Instance-transforms SSBO path only**: `preparePathRibbon()` refuses instancing, MDI, cubemap and CSM (the
  directory is indexed by the instance slot, `gl_InstanceIndex`). A path is absent from reflection cubemaps: ⚠️ the
  scene SKIPS a pulled-vertex instance for a cubemap target (`Scene::checkRenderableInstanceForRendering()`, since
  2026-09-30) — refused by the vertex stage, the instance was marked broken and REMOVED from the scene as soon as a
  reflection probe existed (reproduced with the beams, which share the route: graphics doc 33 § Traps).
- **Shared with the beams** (2026-09-30): the curve kinds and their tessellation are `Base::Math::CurveShape`
  (`Path::Kind` = `Math::CurveKind`), the SSBO, the directory, `RenderableInstance::PathPoints` and
  `PulledVertexResource` carry a beam's stations too (graphics doc 33).
- **The vertex capacity only grows** (the render thread reads it while the logic publishes more points); the
  vertices past the frame's count collapse. A path that shrank keeps drawing collapsed vertices.
- **A material change does not reach the path**: call `markLookChanged()` (the console adapter does) — the bounds
  follow the width, and the debug look is the material's style.
- **Debug mode, round and translucent**: the two capsules overlap at every join, the joint reads darker (accepted).
- The debug overlay runs on both frame paths (since 2026-09-30): on the direct swap-chain path it draws in the
  swap-chain's post-process pass, after the outline (graphics doc 34 § Limits).
