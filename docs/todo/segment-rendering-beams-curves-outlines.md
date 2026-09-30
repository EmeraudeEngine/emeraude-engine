---
id: segment-rendering-beams-curves-outlines
title: Graphics — segment rendering (selection silhouette, paths and curves, lasers and electric arcs)
status: in-progress
priority: unranked
scope: Graphics / Saphir
opened: 2026-09-28
tags: [graphics, shaders, lines, beams, photometry, owner-request]
---

# Graphics — segment rendering (selection silhouette, paths and curves, lasers and electric arcs)

## Why

The owner wants one shader family that draws **segments**, with three users:

1. **Selection silhouette**: outline the selected object as seen from the camera, so what is targeted
   in 3D is obvious. The request names the `VertexFactory::Silhouette` algorithm of emeraude-base.
2. **Paths and Bézier curves**: draw paths (AI routes, camera rails, splines) and Bézier curves.
3. **Lasers and electric arcs, Half-Life style**: GoldSrc's `env_beam` / `env_laser`, made photometric.

## Done (2026-09-28) — the beams, first pass

Lasers and electric arcs work: `Scenes::Component::Beam`, `Material::BeamResource`, the beam ribbon vertex
stage, a reactive mask for the TAA, the projet-alpha bench `beams`. Everything that must survive is in
`docs/subsystems/graphics/33-beams-lasers-and-electric-arcs.md` (usage, design, contracts, measurements, traps),
`docs/subsystems/saphir/27-the-beam-ribbon-a-vertex-stage-that-builds-a-beam.md`, the TAA resolve doc § reactive
mask and `docs/subsystems/scenes/13-instance-transforms.md` (per-slot local transformation).

⚠️ Two deviations from the decisions of the morning, reported to the owner:
- The endpoints do NOT live in the beam's material UBO: a buffer written every frame must have one copy per
  frame in flight (graphics doc 25, Rule 1), and a material UBO has one region. They travel as the instance's
  local transformation, published per render-state slot, through the instance-transforms SSBO — which also gives
  the real previous model matrix. The UBO holds the look only.
- "Primitive A" was delivered as a SINGLE straight segment (the unit strip placed by a matrix), not the general
  polyline the silhouette and the curves need.

## What remains

### A. The general polyline primitive (for B and C)

- ~~A polyline expanded to camera-facing ribbons by vertex pulling~~ DONE 2026-09-29 (the Path, graphics doc 35).
- ~~The beams on it~~ DONE 2026-09-30: a Beam follows any curve of the Path's kinds (`Math::CurveShape`), on vertex
  pulling, the arc pinned at the two ends only along a rotation minimizing frame (graphics doc 33; bench `beams`:
  `RelayLaser`, `CoilArc`). Left: an occluded-dashed look; the geometric silhouette (B.1) still to be built on it.

### B. Selection silhouette — first pass DONE (2026-09-29)

Screen-space "custom depth" outline: graphics doc 34 (`docs/subsystems/graphics/34-the-selection-outline-custom-depth.md`).
Validated on `geometry-generator` (3070 Ti): 2 px line all around, full where visible, dimmed across the occluder,
0 validation message. What remains:
- ~~Measure the composite's cost; scissor it~~ DONE 2026-09-29: graphics doc 34 § Cost and the scissor.
- ~~Several entities at once~~ DONE 2026-09-29 (a set, one style; editor Shift+click): graphics doc 34, scenes doc 25.
- The direct swap-chain frame path (no scene target there).
- B.1, the geometric silhouette (`VertexFactory::Silhouette` + primitive A), for debug / wireframe tools — later.

### C. Paths and curves — `Component::Path` — first pass DONE 2026-09-29 (graphics doc 35)

Owner decisions (2026-09-29): points reach the GPU through an SSBO read by VERTEX PULLING (no VBO: a new VBO-less
geometry); the scene holds ONE packed path SSBO per frame in flight, staged in `prepareRender()` from each path's
published state (the `SceneInstanceTransforms` model), each draw pushing `{firstPoint, pointCount}`; every point
carries `{current, previous}` (a path that moves every tick has a real velocity; a point-count change = velocity 0
+ the reactive tag that frame); the world path is an unlit OPAQUE solid colour, depth tested and written, in the
scene pass, casting no shadow; joins miter falling back to bevel past the SVG miter limit (4), round joins/caps as a
per-path option (distance in the fragment shader, Rougier); width in metres or in pixels; a DEBUG mode drawn after
the tone mapping (no depth test, display colour, translucent allowed, neither exposed nor TAA'd — like the outline).
Curves, all tessellated on the CPU into a polyline, adaptively, by a chord tolerance in metres (1 cm default):
polyline; piecewise cubic Bézier (the existing `Math::BSpline`, anchors + handles — consolidation, not a parallel
class); uniform cubic B-spline; centripetal Catmull-Rom (Yuksel, Schaefer, Keyser 2011) — every kind converted to
cubic Bézier spans, subdivided by de Casteljau until flat (`Math/CurveTessellation.hpp`, emeraude-base).
References: Rougier, "Shader-Based Antialiased, Dashed, Stroked Polylines", JCGT 2013; three.js
`LineSegments2`/`LineMaterial` (`worldUnits`); A. Klein, "Rendering thick lines with dashes".
The colour (owner decision 2026-09-29): a linear hue × a luminance in nits (250 default), like the beams; a colour
exact on screen is the debug mode's. What remains:
- ~~A path cannot be OUTLINED~~ DONE 2026-09-30 (graphics doc 35 § The selection outline). A path still casts no
  shadow and is absent from reflection cubemaps (the directory needs the instance-transforms SSBO path).
- The debug overlay on the direct swap-chain frame path (no scene target there).
- A point-count change pairs no previous points (velocity 0) but sets no reactive tag yet.
- Dashes (the arc length is already in w), arrows, per-vertex colour; a Bézier path from the console.

### D. Beams — what the first pass left out

- ~~Console / MCP adapter~~ DONE 2026-09-29 (`BeamConsoleAdapter.cpp`, 10 `SceneManager_Beam_*` tools, MCP
  conformance 1511/0): graphics doc 33 § Using it, `docs/ai-runtime-control.md`.
- A beam that lights the scene: "not now" (owner, 2026-09-29).
- A texture scrolled along the beam, a life / fade-out, HL's "strike again time" (random interval between
  arcs), JSON declaration of a beam in a scene definition.
- A beam that lights the scene — separate owner decision (point light at the impact, or a line light).
- The reactive history tag lasts one frame: a beam over a depth edge that STAYS still keeps rejecting its own
  history there (by design — it is reactive), and a sparse speckle remained around the arc in the `beams`
  captures (bilinear gather mixing tagged and untagged texels). Measure it on a scene with detail behind a beam.

## ⚠️ Traps

- The TAA without the reactive mask averages an arc's re-strikes into a straight line: graphics doc 33.
- Hardware wide lines are optional (`Vulkan/Instance.cpp` only warns without `wideLines`) and Metal has none:
  segments must be geometry, never `lineWidth` (`Saphir/Generator/GizmoRendering.cpp` draws at 1 px).
- A beam is emissive translucent geometry: out of the TLAS (`rt-blended-materials-in-tlas`).
- Pre-exposure: a beam's luminance sits in the scene-colour range — `scene-colour-pre-exposure`.

## Owner decisions

- 2026-09-28: first pass = beams (D) on a ribbon primitive; vertex stage expansion; arc noise on the GPU (fBm,
  `sin(πt)` envelope, deterministic per seed and time); no scene lighting; a dedicated `Material::BeamResource`;
  one `Unique` instance per beam; the TAA answer is a REACTIVE MASK in a dedicated `R8_UNORM` attachment
  (rejected: velocity widened to RGBA16F, a post-TAA pass, the beam's own velocity).
- 2026-09-28 (afternoon), the selection outline: **SCREEN-SPACE** (B.2), the parts hidden behind other geometry
  shown DIMMED (the visible ones full), a THIN line of constant PIXEL width, colour configurable. B.1 (geometric)
  stays for debug / wireframe tools later. Architecture: a **"custom depth" pass** (Unreal's CustomDepth): after
  the scene pass the selected instances are drawn ALONE into a dedicated depth target with the main camera
  (UNJITTERED, for a stable line) and the depth-only shadow-casting programs (skinning, alpha test, wind already
  handled); a final effect AFTER the tone mapping draws the N-pixel edge and compares that depth with the scene's —
  full where visible, dimmed where hidden. Rejected: a stencil bit in the scene pass (no hidden parts), a
  render-to-texture of the subject (a second fully shaded render + a camera rig).
- 2026-09-30: curved beams = the Path's curve kinds (not a single Bézier, not a chain of straight arcs); the Beam
  moves to vertex pulling (a straight beam is a 2-point curve, one ribbon code); the arc is pinned at the two ENDS of
  the whole curve only (not at each control point). No depth bias for paths lying on a surface, for now.
- 2026-09-29: order = bugs, then finish (beam console adapter, outline cost + scissor, multi-selection), then new
  (polyline + curves). **A beam does NOT light the scene — not now** (still emissive only). **Multi-selection = a
  SET of highlighted entities, one style** (add / remove / clear, console; Shift+click in the editor). **Paths and
  curves = a world `Component::Path`** (polyline, Bézier, B-spline as a camera-facing ribbon, depth tested, width in
  metres or pixels, proper joins) **plus an always-on-top debug mode**.

## References

- Half-Life (GoldSrc) FGD, `env_beam` / `env_laser` (start / end entity, amplitude noise, brightness, width,
  texture scroll rate, life, strike again time).
- Matt DesLauriers, "Drawing Lines is Hard" (2015) — screen-space polyline expansion, joins and caps.
- Ben Golus, "The Quest for Very Wide Outlines" (2020) — screen-space outlines, jump flood.
- Emil Persson, "Phone-wire AA", GPU Pro 5 (2014).
- AMD FidelityFX Super Resolution 2, "Reactive mask" (GPUOpen documentation); Unreal Engine "Responsive AA".
- Rong & Tan, "Jump Flooding in GPU with Applications to Voronoi Diagram and Distance Transform", I3D 2006.
- Unity `LineRenderer` / `TrailRenderer`, Unreal Niagara ribbon / beam emitter.
