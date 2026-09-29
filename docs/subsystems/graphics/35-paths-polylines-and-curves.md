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
| `Graphics::PathDebugOverlay` | the debug mode: its own per-frame SSBO (VP, eye, pixel, world points), no depth, alpha blended, after the outline |

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
- Width: `style.x × model scale` (metres), or `style.x × pixel × distance` (pixels; `pixel` = 2 / (viewport
  height · |P[1][1]|), the beam's clamp).
- The velocity pass rebuilds the ribbon from the PREVIOUS points (the render-side history) and the previous model
  matrix: a turning path (the `paths` loop) stays crisp under TAA.

### The colour (owner decision: colour + luminance)

An unlit surface writes its colour as RADIANCE (SceneRendering's unlit branch: normal facing the eye, neutral
material properties). The path's colour is a linear hue × a luminance in nits (250 by default): the exposure treats
it like any emitter — brilliant at night, modest in full sun. A colour exact ON SCREEN whatever the exposure is the
debug mode's (the exposure is not in the shaders; putting it there was rejected — the GPU auto-exposure would lag a
CPU copy by a frame).

### ⚠️ Traps and limits

- **The directory entry must always exist.** A path with no point (hidden, or drawn by the overlay) still stages an
  EMPTY entry: its vertex stage reads `pathSpans[slot]` whatever, and a slot past the end of the directory is an
  out-of-bounds read. And the collapse returns `vec3(0)` WITHOUT calling `pathPoint()`: `clamp(i, 0, count − 1)` is
  undefined in GLSL for count 0. Both found in review before any symptom.
- **The shadow-casting programs never call `prepareVertexStage()`**: they would draw a path as an ordinary mesh —
  a position attribute with no vertex buffer, garbage (seen as a spurious outline when a path was highlighted,
  0 VUID). Hence: paths cast no shadow (`disableShadowCasting()`), and `Scene::renderSelectionDepth()` skips a
  geometry without a vertex buffer — **a path cannot be outlined yet**.
- **Instance-transforms SSBO path only**: `preparePathRibbon()` refuses instancing, MDI, cubemap and CSM (the
  directory is indexed by the instance slot, `gl_InstanceIndex`). A path is absent from reflection cubemaps.
- **The vertex capacity only grows** (the render thread reads it while the logic publishes more points); the
  vertices past the frame's count collapse. A path that shrank keeps drawing collapsed vertices.
- **A material change does not reach the path**: call `markLookChanged()` (the console adapter does) — the bounds
  follow the width, and the debug look is the material's style.
- **Debug mode, round and translucent**: the two capsules overlap at every join, the joint reads darker (accepted).
- The debug overlay needs the internal scene target (`renderFrameWithInternal()`), like the outline.
