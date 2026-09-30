## Beams — lasers and electric arcs (Sep 2026)

Half-Life's `env_beam` / `env_laser`, made photometric: a ribbon along a curve — straight between two points by
default, or any curve of the Path's kinds (2026-09-30) —, steady (a laser) or wandering and re-striking (an electric
arc). Owner decisions and the open work: engine
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
	.build(resources, 96U);                       // segments over the whole beam, at least: 1 for a laser
```

A CURVED beam (owner decisions 2026-09-30: the Path's curve kinds, the beam on vertex pulling, the arc pinned at the
two ends only): `beam.setPolyline(points)` (a laser through relays), `setBezierPath(Math::BSpline)`,
`setUniformBSpline(points)`, `setCatmullRom(points)`, `setTolerance(t)` — the kinds and the tessellation are
`Base::Math::CurveShape`, shared with `Scenes::Component::Path`. `setStart()` / `setEnd()` move the curve's first / last
point, `setEndTarget()` makes its last point follow an entity. `stationCount()`, `length()`, `curve()` read it back.

A beam lights the scene through a LINE LIGHT it drives (owner decision 2026-09-30): `beam.setLight(lineLight, scale)`
— the beam pushes its curve, half width, enabled state and equivalent tube luminance to the light. Graphics doc 36.

**Live, from the console or an MCP client** (`src/Scenes/Component/BeamConsoleAdapter.cpp`): every setter above is a
command of `Core.SceneManagerService.Beam` (MCP `SceneManager_Beam_*`), addressed by `entity, component` and answering
the beam's NEW state as JSON — e.g. `Beam.setArc(ArcPylons, Arc, 0.35, 3, 6)`, `Beam.setEndTarget(LaserTurret, Laser,
Head, 0, -0.5, 0)`. The list: `docs/ai-runtime-control.md` § Driving an entity's components. `setEndTarget` resolves the
beam and its target under ONE exclusive access to the scene (never a second lock inside `act()`).

### How it is built — five pieces, each with one job

| Piece | Job |
|---|---|
| `Scenes::Component::Beam` | Owns its material, its `Geometry::PulledVertexResource` and its `RenderableInstance::Unique`. Its curve (`Math::CurveShape`) is tessellated (chord tolerance, 1 cm), every segment split so that the whole beam has at least `segmentCount` pieces (`CurveTessellation::subdivided()`: the corners kept exactly), and each STATION gets its position, its normalized arc length t and a rotation minimizing normal (`CurveTessellation::rotationMinimizingNormals()`, the arc's axes). Rebuilt on change only (a followed end: when the target moved); published per render-state slot on change only. Render bounds = the stations ± (half width + arc amplitude); no collision extent. |
| `RenderableInstance::PathPoints` + `SceneInstanceTransforms` bindings 1-2 | The Path's machinery (graphics doc 35): the stations are published per render-state slot, staged into the frame-buffered path SSBO beside the instance's entry with the previous frame's (the render-side history). TWO vec4 records per station — `(position, t)` then `(normal, 0)` — in the entity's space. The model matrix is the entity's own. |
| `Geometry::PulledVertexResource` | No vertex buffer: 6 vertices per segment (a quad, triangle list), the capacity grows only; a vertex past the last segment collapses. |
| `Material::BeamResource` | A second concrete `Material::Interface` (owner decision: structurally different, NOT a mode of Standard). UBO = the LOOK only (radiance = linear colour × nits, shape, motion). Unlit, `BlendingMode::Add`, `writesGeometryBuffer() = false`, writes the reactive mask. |
| `AbstractVertexStage::enableBeamRibbon()` + Saphir `BeamGLSL.hpp` | The vertex stage pulls its station (`beamStation()`: the point displaced by the arc along the normal and `cross(tangent, normal)`), faces the eye across the local tangent (the neighbour stations), in WORLD space, and brings it back through `inverse(model)`. The instance-transforms path only (never instanced, MDI, cubemap nor CSM), like the path ribbon. |

### The arc

Two independent fBm of 1D gradient noise (Perlin 2002 quintic fade, Wellons' lowbias32 hash), amplitude halved
per octave, normalized by their ENERGY (√Σa²: the same spread for 1 to 8 octaves), through `tanh(ArcGain × fbm)`, under
a `sin(πt)` envelope that pins both ends — t the normalized ARC LENGTH, so a curved arc is pinned at
the two ends of the whole curve, not at its control points (owner decision 2026-09-30). The two offsets run along the
station's normal and binormal: a ROTATION MINIMIZING frame (double reflection, Wang, Jüttler, Zheng, Liu, ACM TOG
2008), which does not twist around the curve; its first normal is cross(tangent, +Y) (+X near vertical), so a
straight beam keeps exactly the axes it had before curves.

**The amplitude means what Cascade's does** (owner decision 2026-09-30; Unreal's `UParticleModuleBeamNoise`
`NoiseRange`: each noise point displaced within ± the range): the arc NEVER exceeds it and its mean offset is HALF of
it — the statistics of a uniform draw, smooth. `BeamGLSL::ArcGain` = 3.4, chosen offline (40 000 samples per octave
count: mean |offset| 0.495-0.506 of the amplitude, 99th percentile 0.988-0.990). Before, the fBm was normalized by the
SUM of its octave amplitudes: σ 0.165 at 6 octaves, 99 % of the offsets under 0.43 of the amplitude — a 0.35 m arc
wandered 2-3 px at 42 px/m and read as a smooth line (Windows peer, 2026-09-30). Measured after (Linux, straight 4 m
arc seen from 5.5 m, 147 px/m, 6 octaves, re-strike 0): 104 px of the 118 px an amplitude of 0.8 m allows (31 before),
46 px at 0.35 m (≈ 13 before). ⚠️ Half of the wander runs along the axis that points at the eye for a beam across the
view (the frame is the beam's, as in Cascade): a curve seen edge-on shows it all, one seen face-on only the other axis. Deterministic per (seed, time): a re-strike rate `r`
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

- **A beam of TWO stations cannot wander.** The minimum segment count is fixed at construction (`build(resources,
  n)`, `Beam::segmentCount()`); a straight beam of 1 segment has 2 stations and no interior one to displace, so an arc
  amplitude is ignored. A curve adds a station per corner (a relay laser of 1 segment has one per relay: an arc there
  moves the corners only). The console `Beam.setArc` refuses a beam of fewer than 3 stations (`getState` shows
  `stationCount`); the C++ `material()->setArc()` does not know the beam and stays silent. Found by the macOS and Windows peers on 2026-09-29: the Linux capture of a
  "wandering" 1-segment laser was a straight line, validated too fast.

- One program set per beam: the renderable name enters the program cache key and every beam owns its material
  (tens of beams: fine; hundreds: batch them in a `Multiple` — the rejected alternative).
- The material UBO lives in ONE frame region: only the look goes there. Endpoints never — they would race the
  frame in flight (`docs/subsystems/graphics/25-16-…` Rule 1).
- A disabled or zero-length beam publishes NO station: its directory entry is empty, `isDrawnInScene()` keeps it out
  of the render lists, and `beamCorner()` collapses without reading anything.
- ⚠️ **A beam is ABSENT from reflection cubemaps** (since 2026-09-30; before, it was drawn there with a frozen arc):
  the stations are read through the instance-transforms SSBO, which a cubemap pass does not use. A pulled-vertex
  instance is SKIPPED for a cubemap target (`Scene::checkRenderableInstanceForRendering()`); without that skip the
  vertex stage refused the program, the instance was marked broken, and EVERY beam and path of the scene was
  removed as soon as a reflection probe existed (reproduced on `beams` with a temporary probe, 2026-09-30).
