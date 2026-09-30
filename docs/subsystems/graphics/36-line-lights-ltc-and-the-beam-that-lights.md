## Line lights — LTC, and the beam that lights the scene (Sep 2026)

A fourth light type: a Lambertian TUBE of light along a polyline (a fluorescent tube, a neon, an electric arc), shaded
in closed form with Linearly Transformed Cosines. Owner decisions (2026-09-30): a true linear light (not a point light
at the impact), the full LTC (diffuse AND GGX specular, not a representative point), the intensity derived from the
beam, NO shadow in the first pass, a polyline (not one segment) seen by the traced effects too, and the coupling "the
beam drives a LineLight". Bench: projet-alpha demo `beams` (`ArcPylons`, `CoilArc`).

### Using it

```cpp
const auto light = entity->componentBuilder< Scenes::Component::LineLight >("Tube").build();
light->setPolyline(points);                  // entity space, 2 to 9 points (more: resampled by arc length)
light->setColor(color);                      // a CHROMATICITY, like every light (post-process doc 03)
light->setLuminance(10000.0F);               // nits (cd/m²) of the tube's surface — or:
light->setLuminousFluxPerMetre(2800.0F);     // lumens per metre = π × L × 2πR (a 36 W, 1.2 m tube ≈ 2 800 lm/m)
light->setTubeRadius(0.013F);                // metres (the luminance stays, the flux follows)
light->setRadius(8.0F);                      // the REACH: smooth window to 0 there, culled beyond (0 = unbounded)
```

**A beam that lights** (`Scenes::Component::Beam::setLight(light, scale = 1)`): the beam pushes, on change only, its
enabled state, its TESSELLATED curve (the `Math::CurveShape` polyline, resampled to 9 points), its tube radius (the
material half width) and the equivalent tube luminance `equivalentTubeLuminance() × scale`:

    L_tube = luminance × Y(colour) × √π Γ(k+1) / (2 Γ(k+3/2))        k = the core exponent

the mean of the ribbon's cross-section profile `(1 − s²)^k` (0.406 at k = 4, 0.457 at k = 3) times the luminance of
its colour (Rec. 709). Measured on `beams`: `ArcPylons` 50 000 nits, colour (0.55, 0.7, 1), k = 4 → 14 014 nits,
3 320 lm/m at a 12 mm half width; `CoilArc` 40 000 nits, k = 3 → 12 613 nits, 9 points. Hiding the beam switches the
light off (`LineLight.getState` → `enabled: false`) — on the beam's NEXT logic tick (`Beam::processLogics()` pushes
the state): a `getState` sent in the same instant may still answer `true` (seen on the Windows peer, 2026-09-30).

Console / MCP: `Core.SceneManagerService.LineLight.*` (`getState`, `setEnabled`, `setColor`, `setPoints("x y z; x y z;
...")`, `setLuminance`, `setLuminousFluxPerMetre`, `setTubeRadius`, `setRadius`) — `docs/ai-runtime-control.md`
§ Driving an entity's components. ⚠️ On a light driven by a beam, the beam overwrites the enabled state, the points,
the luminance and the tube radius on its next change: drive the BEAM.

### The integral (why it is exact for a thin tube)

`ltcLineDiffuse(p1, p2)` (Heitz & Hill 2017, `ltc_line.fs`) integrates the clamped cosine distribution D = cos θ / π
over the segment, in a frame whose z is the normal (clipped to the horizon first). For a thin Lambertian tube of
radius R and luminance L, **`R × ltcLineDiffuse` is the solid-angle integral of D over the tube** — checked offline
against a brute-force cylinder integral: ratio 1.000 ± 0.002 (2026-09-30). Therefore:

- illuminance `E = π × L × R × I`, Lambert radiance `albedo × L × R × I` (I = `ltcLineDiffuse`);
- GGX specular: the same integral of the segment transformed by `M⁻¹` (the fitted LTC of the GGX lobe for this
  roughness and view angle), × R, weighted `F0 × t2.x + (1 − F0) × t2.y` (the fitted magnitude and Fresnel terms).

ONE copy of the GLSL: `Graphics/Effects/Shared/LineLightGLSL.hpp` — the three function bodies as macros, used by the
Saphir raster pass (`LightGenerator::generateLineLightFunctions()`) and, with the world-space helpers
(`emLineIrradiance`, `emLineClosest`, `emLineReach`), by the RTGI, RTR and irradiance-probe literals.

### The LTC tables

`Graphics/LTCTables.{hpp,cpp}`: `ltc_1.dds` (inverse matrix m00, m02, m20, m22 → `mat3(vec3(x,0,y), vec3(0,1,0),
vec3(z,0,w))`) and `ltc_2.dds` (magnitude, Fresnel, 0, sphere) of https://github.com/selfshadow/ltc_code, 64 × 64
RGBA16F, converted VERBATIM (no refit) by a script that reads the DDS. `Graphics/LTCTexture` uploads them as ONE
2-layer `sampler2DArray` (linear, clamp), owned by the Renderer (`Renderer::LTCTables()`), bound at binding 1 of the
line-light descriptor set beside the light's UBO. Lookup: `uv = (roughness, √(1 − N·V)) × 63/64 + 0.5/64`.
⚠️ The BSD-style licence of the tables asks for its notice in the documentation of a BINARY distribution:
`docs/third-party-notices.md`.

**Fit accuracy vs a brute-force GGX** (offline, 2026-09-30): 1-8 % L1 near the normal, 10-30 % at grazing view —
the known accuracy of the 64² LTC fit, not a defect. The SWAPPED indexing (θ on x) is far worse on every sample,
which confirms the table orientation.

### How it is wired

| Piece | Job |
|---|---|
| `Scenes::Component::LineLight` | An `AbstractLightEmitter`. UBO (44 floats): colour 0-3, luminance 4, reach 5, tube radius 6, point count 7, points 8 + 4i (WORLD space, the entity's model matrix applied in `updateWorldPoints()`). Published per render-state slot (`publishedLine()`). `touch()` = distance to the polyline ≤ reach. No shadow map. |
| `Graphics::RenderPassType::LineLightPass` / `LightType::Line` | One additive forward pass per line light and lit instance, after the spot lights (`Scene.rendering.cpp`); no shadow variant. |
| `Saphir::LightGenerator` (PBR) | Vertex: a flat `lineViewMatrix`, the view-space position. Fragment: the polyline in view space, the closest point (the direction of the secondary lobes and the reach), the LTC diffuse + specular summed over the segments. |
| `Scenes::LightSet` | Owns the line lights, their UBO (`lineLightBuffer()`), the descriptor set layout (UBO + LTC tables); RT packing: ONE `GPULightData` entry of type 3 per SEGMENT (position = start, direction = start → end unnormalized, radius = reach, `innerCosAngle` = tube radius). |
| RTGI / RTR / `IrradianceProbeVolume` | Type 3 branch: GI and probes add `colour × emLineIrradiance × reach` (exact); RTR takes the exact diffuse and a GGX lobe toward the CLOSEST point × the illuminance (approximate). No shadow ray. |

### Measured (Linux, 3070 Ti, 2026-09-30)

`beams`, exposure pinned (f/2.8, 1/50 s, ISO 400): the ground under the arcs averages sRGB (85, 98, 119) with both
beams on, (3, 3, 3) with both hidden — the blue-white of the arcs; 0 validation message, the RT lane runs.

### ⚠️ Limits and traps

- **No shadow**, raster or traced: the tube lights through walls. The next pass (owner decision pending).
- **The light follows the CURVE, not the arc's wander**: the arc is displaced on the GPU (graphics doc 33); the light
  sees the undisplaced tessellated curve. For an arc of amplitude A the lit pattern is off by ≤ A near the arc.
- **9 points** (`LineLightMaxPoints`, the UBO array): a longer polyline is resampled by arc length — a tight coil
  loses its corners in the LIGHT (never in the drawn beam).
- The secondary lobes (clear coat, sheen, transmission) use the representative direction toward the closest point
  with the LTC diffuse as their irradiance (`π × I / max(N·L, 0.05)`); anisotropy is ignored for a line light.
- RT: a line light costs one `GPULightData` per segment out of `MaxRTLights` (128), packed AFTER the point and spot
  lights, so it is the first dropped when the budget is full.
- `Renderer::LTCTables()` must exist before any line-light descriptor set: a line light without its descriptor set is
  skipped at render time ("Unable to bind the LTC tables" was the symptom of a missing texture).

### References

- E. Heitz, J. Dupuy, S. Hill, D. Neubelt, "Real-Time Polygonal-Light Shading with Linearly Transformed Cosines",
  ACM TOG 35(4) (SIGGRAPH 2016). Tables and code: https://github.com/selfshadow/ltc_code (BSD-style).
- E. Heitz, S. Hill, "Real-Time Line- and Disk-Light Shading with Linearly Transformed Cosines", SIGGRAPH 2017
  Physically Based Shading course (`ltc_line.fs`).
- Unreal Engine source-length point lights (`SourceLength`), Frostbite "Moving Frostbite to PBR" (Lagarde & de
  Rousiers 2014) § tube lights — the representative-point alternative that was rejected.
