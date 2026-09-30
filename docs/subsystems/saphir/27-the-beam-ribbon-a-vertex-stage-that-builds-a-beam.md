## The beam ribbon — a vertex stage that BUILDS a beam (Sep 2026)

`AbstractVertexStage::enableBeamRibbon(shape, motion)` (Material::BeamResource): since 2026-09-30 a VERTEX-PULLING
stage, like the path ribbon (saphir: `preparePathRibbon()`, graphics doc 35) — no vertex attribute; each vertex
(`gl_VertexIndex`, 6 per segment) reads its beam STATION from the path SSBO through the directory entry of its instance
slot (two records per station: position + t, normal). `prepareBeamRibbon()` — a UNIQUE PREPARATION registered right
after the model matrix, the imposter's scheme — places it: station + arc offset along the station's frame (Saphir
`BeamGLSL.hpp`), across the local tangent facing the eye, in world space, back through `inverse(model)`. The strip
geometry and the unit-segment matrix are gone. The feature as a whole: graphics
`docs/subsystems/graphics/33-beams-lasers-and-electric-arcs.md`.

- **Expressions**: `vertexPositionExpression()` → `beamPosition`, `previousVertexPositionExpression()` →
  `previousBeamPosition` (the same ribbon with the instance-transforms PREVIOUS model matrix and the previous scene
  time and the PREVIOUS stations, emitted only when the velocity synthesis asked for it: `m_previousBeamRequired`),
  `vertexFrameExpression()` → fixed axes (an unlit emitter). `declarePositionAttribute()` and the rest position declare
  NO attribute for it (the pulled-vertex rule, graphics doc 35 § the VUID trap).
- **Outputs**: `ShaderVariable::BeamCoordinates` (t, side) and `BeamCoverage` (true width / drawn width).
- **Exclusive** with skinning, the wind, a heightfield, the imposter and the path ribbon; refused on instancing, MDI,
  cubemap and CSM, and without the instance-transforms SSBO (the directory is indexed by the instance slot). The scene
  skips a pulled-vertex instance for a cubemap target before any program generation (graphics doc 33 § Traps).

⚠️⚠️ **ORDER: the mode must be on BEFORE the velocity synthesis.** `SceneRendering` synthesizes the velocity
outputs before it calls `material->generateVertexShaderCode()`, and the synthesis writes
`vertexPositionExpression()` into the GLSL as it runs. Switched on later, the velocity reads the raw strip
coordinates. Hence `Material::Interface::prepareVertexStage()`, called just before the synthesis; the beam switches
its ribbon on there. The imposter still switches its billboard on in `generateVertexShaderCode()` — engine item
`imposter-velocity-captured-before-billboard`.
