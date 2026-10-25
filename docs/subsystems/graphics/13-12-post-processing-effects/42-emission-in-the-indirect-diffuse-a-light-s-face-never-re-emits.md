## 12. Post-Processing Effects

### Emission in the indirect diffuse — a light's face never re-emits, a free emitter always does (Oct 2026)

> [!CAUTION]
> **THE DEFECT** (measured and fixed 2026-10-25, green on the three OS). In `labyrinth` the two lanes
> disagreed about 20× on the indirect diffuse, and only when a lamp panel was on screen (GI ÷ direct light, clipped pixels
> masked, § 41's linearisation: SSGI 6.0, RTGI 0.26; panel off screen: both ≈ 0). The estimators did not read the same thing at a bounce hit:
> - **SSGI** samples the final colour at the hit pixel — a panel there is 7 800 nits of emission;
> - **RTGI** shaded the hit as `albedo / π × direct + albedo × probe feedback`, never its emission — but the
>   **irradiance probe volume**, whose feedback RTGI reads, did add it.
>
> A lamp's panel is the VISIBLE FACE of that lamp (L = Φ / (π A) of the very spot already lighting the walls): SSGI
> and the probes counted every lamp twice, and the second copy, not held by the spot's cone, flooded the upper walls —
> view-dependently in SSGI (the walls brightened when a panel entered the frame). Meanwhile RTGI let a FREE emitter (an
> exit sign, a neon, a screen) light nothing.

**The contract (owner decision 2026-10-25, option A).** Emission is a source of the indirect diffuse unless the
surface is the visible face of light sources:

| Surface | Camera, reflections (SSR, RTR) | Indirect diffuse (SSGI, RTGI, the probe volume) |
|---|---|---|
| **Light face** — `StandardResource::setLightFace(true)` | visible, emission included | emits NOTHING: its light is the lights' own |
| **Free emitter** — the default | visible, emission included | a source, in every estimator and both lanes |

How a material becomes a light face:
- `AbstractLightEmitter::linkEmissiveMaterial()` declares it (the panel that follows a lamp's intensity);
- `setLightFace(true)` for a material that several lights SHARE — one panel model for many steady lamps cannot follow
  one light (`labyrinth`: the flickering lamps' panels are linked, the steady lamps' shared panel is declared);
- the material JSON key `"LightFace": true` (`docs/material-json-format.md`).
`unlinkEmissiveMaterial()` leaves the declaration; `setLightFace(false)` turns a face back into a free emitter. This is
the per-material switch the industry exposes by hand — Unity's `MaterialGlobalIlluminationFlags`
(docs.unity3d.com/ScriptReference/MaterialGlobalIlluminationFlags.html), Unreal's per-mesh "Emissive Light Source" and
its path tracer's `PathTracingEnableEmissive`, whose documentation names this very double count of surfaces also
represented by light sources (docs.unrealengine.com, FPostProcessSettings::PathTracingEnableEmissive) — derived here from
the light link where it can be.

**The carriers** (all runtime: linking a lamp later needs no new program):
- material UBO slot 140, `lightFaceParameters.x` (the layout grew 140 → 144 floats, owner decision S1);
- the material-properties G-buffer, **R low nibble, bit 1** (bit 0 is the deferred-lighting bit, bits 2-3 reserved),
  written by the lit path (`LightGenerator::materialPropertiesExpression()`, `declareSurfaceLightFace()`) and by the
  unlit path (`Material::Interface::lightFaceExpression()`);
- the RT material flag `GPURTMaterialData::IsLightFace` (bit 11).

**The consumers:**
- **RTGI** adds `emission (× its texture)` at a hit when `IsEmissive` and not `IsLightFace` — the same term as the probe
  volume, in the same unit (nits, before the pre-exposure);
- **the probe volume** skips the emission of a light face;
- **SSGI** reads the hit's material-properties texel and brings back NOTHING from a light face; the ray still stops
  there (the panel occludes what lies behind it);
- RTR and SSR are unchanged: a mirror shows the panel.

> [!IMPORTANT]
> **SSGI drops the WHOLE hit, by intent — the one approximation of this contract (owner decision S2, 2026-10-25).** A
> screen pixel cannot be split into its emission and its reflected light, so a light-face hit loses both. The
> reflected part is negligible for a real light's face (a 7 800-nit panel reflects a few nits: < 1 %), but it is not
> zero: a light face whose lamp is OFF (emission 0) reflects nothing into SSGI either, and a dim, broad light face
> would lose more. **The exact alternative**, if it is ever needed: an emission attachment written by the scene pass
> (the per-pixel emitted radiance), from which SSGI subtracts the emission of a light face and keeps the rest — one
> more full-resolution RGBA16F target in the G-buffer, its bandwidth on every frame. RTGI has no such limit: it adds
> the emission separately and keeps the face's reflected light.

> [!WARNING]
> **Read the colour and the flag from the SAME texel.** The first build read the hit colour with a filtered
> `texture()` and the flag with `texelFetch()`: at a panel's edge the flag said "wall" while the bilinear colour already
> carried half of a 7 800-nit texel, and the panel pose still read 2.6× the direct light (against 0.04 once both reads
> used `texelFetch()` at one texel). A small, very bright target makes every edge leak count: packed flags and the data
> they qualify are fetched together, never one filtered and one not.

> [!NOTE]
> **The noise of a free emitter (owner decision S3).** A hemisphere ray finds a small bright emitter rarely: unbiased,
> noisy. Measured at labyrinth's exit (its 2 × 1 m, 2 000-nit sign), RT lane, 16 consecutive frames after the denoiser:
> peak-to-peak mean 0.26 / p99.9 5.1 / max 15 (8-bit) with the indirect diffuse, 0.18 / 4.4 / 13 without, 0 % above
> 8/255 in both. Sampling the emissive triangles as lights (next-event estimation, the RTXDI / ReSTIR family) is the
> production cure for smaller, brighter emitters: item `rt-many-lights-sampling`.

**Measurements** (`labyrinth`, exposure pinned f/2.8 · 1/30 s · ISO 400, focus pinned; GI ÷ direct light, the pixels
clipped in any state masked and the 0.9999 clamp, § 41; validation ON, 0 `VUID-` everywhere):

| Framing | Linux before: SS / RT | Linux after: SS / RT | macOS after (SS) | Windows after: NVIDIA SS / Intel SS / NVIDIA RT |
|---|---|---|---|---|
| a lamp panel on screen (`Act.setPosition(-6, 0, 67.3)` + `lookAt(-6, 3.9, 66)`) | 6.0 / 0.26 | **0.041 / 0.006** | ≈ 0 | 0.045 / 0.020 / 0.038 |
| pitched down, no panel (`lookAt(-6, 0, 65)`) | ≈ 0 / 0.08 | ≈ 0 / ≈ 0 (noise) | 0.04 (noise) | ≈ 0 / 0.03 / 0.06 |
| the exit alcove, a free emitter (`setPosition(78.6, 0, -77.5)` + `lookAt(77.6, 1, -83.7)`) | — / 0 by construction | 1.05 / 0.60 | 1.04 | 1.04 / 1.03 / 0.59 |

The masked SSGI delta at the panel pose fell from +0.17 to ≈ 0 on the three systems (Windows +0.0016 / +0.0007, macOS
−0.0007). RTGI fell there too: the probes no longer feed it the panels. At the exit the screen-space lane stays above
the traced one — its known bias (an occluder off screen is invisible to it, § 18). `sponza` (RT lane, the four poses of
§ 18): unchanged to the fourth decimal; sky-lab (macOS): unchanged per pixel. Firefly check at the exit, RT lane,
Windows: 0.29 / 5.1 / 19 with the indirect diffuse, 0.22 / 4.6 / 14 without, the pixels above 8/255 in the same floor
tile in both (the floor's own noise).

**Scenes whose look may move** (free emitters that now light their surroundings in the RT lane): JungleRuins' sky dome
(an emissive mesh: RTGI rays hitting it now return the sky's luminance), offscreen-rendering's screen,
post-processor-effect-debug's emissive band, the debug markers (DemoUtils).
