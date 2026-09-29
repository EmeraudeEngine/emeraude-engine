## Beams — lasers and electric arcs (Sep 2026)

Half-Life's `env_beam` / `env_laser`, made photometric: a ribbon between two points, straight (a laser) or
wandering and re-striking (an electric arc). Owner decisions and the open work: engine
`docs/todo/segment-rendering-beams-curves-outlines.md`. Bench: projet-alpha demo `beams`.

### Using it

```cpp
entity->componentBuilder< Scenes::Component::Beam >("Arc")
	.setup([] (Scenes::Component::Beam & beam) {
		beam.setStart({-2.0F, 0.0F, 0.0F});      // entity space
		beam.setEnd({2.0F, 0.0F, 0.0F});         // or setEndTarget(otherEntity, offset)
		const auto material = beam.material();   // one Material::BeamResource per beam
		material->setColor(color);                // LINEAR, a hue: the brightness is the luminance
		material->setLuminance(50000.0F);         // nits (cd/m²), Graphics/Photometry.hpp
		material->setHalfWidth(0.015F);           // entity units
		material->setArc(0.35F, 3.0F, 6);         // amplitude, noise cycles along the beam, octaves (0 = laser)
		material->setArcMotion(7, 12.0F, 0.4F);   // seed, re-strikes per second (0 = never), drift
	})
	.build(resources, 96U);                       // segments along the beam: 1 for a laser
```

A beam does NOT light the scene (owner decision): pair it with a light component when it must.

**Live, from the console or an MCP client** (`src/Scenes/Component/BeamConsoleAdapter.cpp`): every setter above is a
command of `Core.SceneManagerService.Beam` (MCP `SceneManager_Beam_*`), addressed by `entity, component` and answering
the beam's NEW state as JSON — e.g. `Beam.setArc(ArcPylons, Arc, 0.35, 3, 6)`, `Beam.setEndTarget(LaserTurret, Laser,
Head, 0, -0.5, 0)`. The list: `docs/ai-runtime-control.md` § Driving an entity's components. `setEndTarget` resolves the
beam and its target under ONE exclusive access to the scene (never a second lock inside `act()`).

### How it is built — five pieces, each with one job

| Piece | Job |
|---|---|
| `Scenes::Component::Beam` | Owns its material and its `RenderableInstance::Unique`; turns the endpoints into the **segment matrix** (x = end − start, y/z two unit axes across, origin = start) every logic tick and publishes it per render-state slot. Render bounds = the segment ± (half width + arc amplitude); no collision extent. |
| `RenderableInstance::Abstract::publishTransformationMatrix()` | The local transformation, **one copy per render-state slot** (triple buffer). The endpoints thus travel through the frame-buffered instance-transforms SSBO, which gives the beam a real previous model matrix. `setTransformationMatrix()` still writes all slots (build-time constants). |
| `Geometry::ResourceGenerator::beamStrip(N)` | A shared strip of `N + 1` stations × 2 vertices at `(t, ±1, 0)`, position only. Never uploaded again. |
| `Material::BeamResource` | A second concrete `Material::Interface` (owner decision: structurally different, NOT a mode of Standard). UBO = the LOOK only (radiance = linear colour × nits, shape, motion). Unlit, `BlendingMode::Add`, `writesGeometryBuffer() = false`, writes the reactive mask. |
| `AbstractVertexStage::enableBeamRibbon()` + Saphir `BeamGLSL.hpp` | The vertex stage places each strip vertex on the beam's centre line displaced by the arc, then across the local tangent facing the eye, in WORLD space, and brings it back through `inverse(model)`. |

### The arc

Two independent fBm of 1D gradient noise (Perlin 2002 quintic fade, Wellons' lowbias32 hash), amplitude halved
per octave, under a `sin(πt)` envelope that pins both ends. Deterministic per (seed, time): a re-strike rate `r`
changes the seed every `1/r` s; the drift scrolls the noise between re-strikes. The clock is the scene time pair
of the instance-transforms header (`windTimes.xy`: this frame, the previous one — the wind and the beams share it).

### Thin beams: one pixel, dimmer (Persson, "Phone-wire AA", GPU Pro 5)

The drawn half width is `max(halfWidth, one pixel at that distance)` — `2 / (viewport height · |P[1][1]|)`, times
the distance under a perspective projection — and the light is scaled by `halfWidth / drawn`. A 1 cm laser 120 m
away stays a continuous one-pixel line instead of breaking into sub-pixel triangles.

### ⚠️ Contracts it added to the engine

- **`Material::Interface::prepareVertexStage()`** — called by `SceneRendering` BEFORE the velocity synthesis. The
  synthesis captures `vertexPositionExpression()` / `previousVertexPositionExpression()` as it runs, so a material
  whose vertex stage builds the position must switch its mode on there. (The imposter switches its own on in
  `generateVertexShaderCode()`, i.e. AFTER: engine item `imposter-velocity-captured-before-billboard`.)
- **`Material::Interface::writesGeometryBuffer()`** — `false` masks normals, material properties, albedo and
  velocity (write mask 0, like a light pass). An additive overlay is not a surface: stamping its rectangle into
  the G-buffer handed the reflections, the GI and the TAA a flat wall where the beam passed.
- **`Material::Interface::reactiveMaskExpression()`** and the **reactive mask attachment** — next section.

### The reactive mask (owner decision 2026-09-28)

Measured on `beams` before it: the TAA averaged 12 re-strikes a second into the MEAN arc — a near-straight white
line between the two fixed ends, saturated blobs at the ends, ghosts, the real arc dimmed. TAA off: one clean arc.

- A dedicated `R8_UNORM` MRT attachment (location 5, after the velocity; exists exactly when the velocity does —
  FSR 2 / DLSS layout, handed as is to a future upscaler). Cleared to 0. Every pipeline masks it except a material
  with a `reactiveMaskExpression()`, which REPLACES it (red channel).
- Copied into the post-process grab pass like the velocity (`GrabPass::reactiveImage()`), exposed as
  `FrameContext::reactive`, read by the TAA at binding 4 (`reactiveEnabled` push constant: 0 = stand-in bound).
- TAA: `blendAlpha = mix(blendAlpha, 1, reactive)` — the current frame wins — and the history is TAGGED reactive
  with **alpha = 0** (a linear depth is never 0). Next frame a reactive-tagged history is rejected
  UNCONDITIONALLY. ⚠️ The motion marker (negative depth) was not enough: it is kept on a depth EDGE, and the ghost
  line survived exactly on the horizon. With the zero tag: TAA on and TAA off leave the same persistent light
  (20 116 vs 20 461 pixels lit in all of 5 captures 1.1 s apart, glare camera-owned in both).
⚠️⚠️ **Two rules found by the macOS peer (2026-09-29), both needed** — the first version protected the beam's CURRENT
pixels only, and a sweeping laser left a fan of red streaks under TAA (macOS, then reproduced on Linux on the
ScreenSpace lane; the 2026-09-28 measurement above looked at the arc only and could not see it):
1. **The mask is read over the SAME 3x3 footprint as the reconstruction** (max of the 9 taps): the resolve's current
   colour is a Mitchell-Netravali blend of the 3x3, so a pixel NEXT to the beam borrows its light; read at the centre
   only, that pixel accumulated the light into an untagged history nothing ever rejected.
2. **A reactive-tagged history is REPLACED (blendAlpha = 1), not clipped**: a pixel next to the beam's new position has
   the beam in its 3x3 neighbourhood, the variance box spans it, and the old light survived the clip.
Validated: 10 captures over a full turn of the laser (ScreenSpace lane, TAA on), no streak, no trail, 0 validation
message.
- `SceneRenderTarget::clearValues(palette)` builds the clear values of the target's own attachments; the two
  hand-written ladders of `Renderer.cpp` (one branch per attachment combination, twice) are gone.

### ⚠️ Traps

- One program set per beam: the renderable name enters the program cache key and every beam owns its material
  (tens of beams: fine; hundreds: batch them in a `Multiple` — the rejected alternative).
- The material UBO lives in ONE frame region: only the look goes there. Endpoints never — they would race the
  frame in flight (`docs/subsystems/graphics/25-16-…` Rule 1).
- A zero-length or disabled beam collapses its x column; `BeamGLSL::beamCorner()` returns the origin for it
  (`inverse()` of a singular matrix is NaN).
- The arc is frozen (time 0) in cubemap and CSM passes: no instance-transforms clock there.
