## The beam ribbon — a vertex stage that BUILDS a beam (Sep 2026)

`AbstractVertexStage::enableBeamRibbon(shape, motion)` (Material::BeamResource): each vertex of a shared strip is a
`(t, side, 0)` coordinate, and `prepareBeamRibbon()` — a UNIQUE PREPARATION registered right after the model
matrix, the imposter's scheme — places it: centre line + arc offset (Saphir `BeamGLSL.hpp`), across the local
tangent facing the eye, in world space, back through `inverse(model)`. The feature as a whole: graphics
`docs/subsystems/graphics/33-beams-lasers-and-electric-arcs.md`.

- **Expressions**: `vertexPositionExpression()` → `beamPosition`, `previousVertexPositionExpression()` →
  `previousBeamPosition` (the same ribbon with the instance-transforms PREVIOUS model matrix and the previous scene
  time, emitted only when the velocity synthesis asked for it: `m_previousBeamRequired`), `vertexFrameExpression()`
  → the unit segment's axes. `declareFrameAttribute()` declares nothing: the strip is position only.
- **Outputs**: `ShaderVariable::BeamCoordinates` (t, side) and `BeamCoverage` (true width / drawn width).
- **Exclusive** with skinning, the wind, a heightfield and the imposter; refused on instancing and MDI (one beam,
  one model matrix, one draw).

⚠️⚠️ **ORDER: the mode must be on BEFORE the velocity synthesis.** `SceneRendering` synthesizes the velocity
outputs before it calls `material->generateVertexShaderCode()`, and the synthesis writes
`vertexPositionExpression()` into the GLSL as it runs. Switched on later, the velocity reads the raw strip
coordinates. Hence `Material::Interface::prepareVertexStage()`, called just before the synthesis; the beam switches
its ribbon on there. The imposter still switches its billboard on in `generateVertexShaderCode()` — engine item
`imposter-velocity-captured-before-billboard`.
