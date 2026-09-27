## 12. Post-Processing Effects

### The irradiance probe volume — the engine's radiance cache (Sep 2026)

`Graphics/IrradianceProbeVolume.{hpp,cpp}` + `Effects/Shared/IrradianceProbesGLSL.hpp`. Owner
decision 2026-09-12 ("go pour le cache"), chosen against a spatial-hash cache (SHaRC: only holds what
camera rays touched) and a Lumen-style surface cache (an engine in the engine): a **DDGI irradiance
field** — Majercik, Guertin, Nowrouzezahrai, McGuire, *Dynamic Diffuse Global Illumination with
Ray-Traced Irradiance Fields*, JCGT 8(2), 2019; production extensions in Majercik, Marrs, Spjut,
McGuire, JCGT 10(2), 2021; the infinite scrolling volume follows NVIDIA's RTXGI SDK; octahedral
mapping per Cigolle et al., JCGT 3(2), 2014.

**What it answers.** "What diffuse indirect light arrives at THIS world position ?" — for a position
that is on screen or not. That is the question a ray-traced effect asks at its hits and nothing else
in the engine could answer: the RTGI history only knows the visible surfaces. Measured motive, on
`global-illumination`: the green column's face that the mirror sees is lit by bounce light alone
(RGB (0, 65, 0) in the direct view with RTGI, (0, 0, 0) without) and reflected as **0.07** in the
RTR mirror, while the lit corridor around it in the same mirror read 95–166. With the volume the
same face reflects at **(0, 214, 0)**, stable over 90 s; the reflected ceiling, black before, reads 204.

**How it works — one renderer-owned object, four compute passes a frame.**

| Piece | What |
|---|---|
| Ownership | `Renderer::m_irradianceProbeVolume`, created with the `AccelerationStructureBuilder` (so it exists exactly when the traced lane is resident), released before it. `Renderer::irradianceProbeVolume()` for consumers. |
| Grid | `ProbeCountX/Y/Z` (16 × 8 × 16) probes, `ProbeSpacing` (1.5 m) apart: a 24 × 12 × 24 m box **centred on the camera's grid cell horizontally**, the camera at `CameraHeightFraction` (0.35) of its height (centred vertically, half the probes slept under the floor and a 6 m ceiling sat on the top plane where the influence fades to 0 — measured black in the mirror), probes on integer multiples of the spacing (they do not move while the camera wanders inside a cell). When the camera crosses a cell the volume **scrolls**: slot = (grid index + scroll) mod count, only the plane that wrapped around is reset (`resetPlanes`), a jump of more than one cell resets everything. |
| Storage | Two 2D-array atlases, one layer per Y plane, tiles of N×N interior texels + a 1-texel octahedral border: irradiance RGBA16F (N = 8, stores **E/π** — the RTGI history convention, so `albedo × texel` is outgoing radiance) and distance RG16F (N = 16, mean distance and mean squared distance, clamped to 1.5 × the cell diagonal). Both live in `GENERAL` for their whole life. A ray buffer (probes × rays × vec4: radiance, distance; negative distance = back-face hit). Per-frame parameters UBO. |
| Pass 1, trace | One invocation per (probe, ray), `RaysPerProbe` (128) spherical-Fibonacci directions under ONE random rotation per frame (Shoemake quaternion). Set 0 = the Renderer's RT set, set 1 = bindless, set 2 = the volume. A hit is shaded like an RTGI bounce: Lambert under the scene's lights with a shadow ray per shadow-casting light (the shared alpha-test rule on every ray), the raster's EFFECTIVE ambient, the emission, **plus the volume's own irradiance at the hit × `BounceFeedback`** (1.0 = the full DDGI feedback that turns one traced bounce into the converged multi-bounce over the frames, 0 = single bounce, the A/B lever against RTGI). A miss is the sky (environment cubemap slot 0 × sky luminance). A back-face hit carries no light and DDGI's shortened distance (−0.2 × t). |
| Pass 2-3, blend | One workgroup per probe, the ray set staged in shared memory. Irradiance texel = cosine-weighted mean of the rays' radiance (= E/π over a sphere of uniform directions); distance texel = power-50 weighted moments. Both blended with `Hysteresis` (0.97), taken whole on a reset probe. |
| Pass 4, borders | The 1-texel border is the octahedral wrap of the interior (corners take the opposite corner, edges the mirrored first row/column) so hardware bilinear filtering stays continuous. |
| Query | `probeIrradiance(worldPos, normal, viewDir)` in the shared macro: bias the point along the normal (`NormalBias`) and toward the viewer (`ViewBias`), trilinear over the 8 surrounding probes × a back-face ("wrap shading") weight × a **Chebyshev visibility** test on the distance moments (the leak stopper) × a fade to zero over the last cell before the volume boundary × the **IndirectDiffuse `Intensity`** the primary surfaces receive (read from that concept's key, so the reflected indirect follows the same artistic multiplier; the volume's own recursion divides it back out — energy is not a look). Returns E/π; multiply by the DIFFUSE albedo. `probeVolumeWeight(worldPos)` exposes the coverage: RTR cross-fades the IBL diffuse leg back in where the volume does not reach. |
| Sync | Recorded by `Renderer::recordIrradianceProbeUpdate()` on both frame paths right after `updateRTDescriptorSet()` — after the TLAS build, before any traced effect — under the GPU profiler zone `IrradianceProbes`. Memory barriers: previous readers → trace, ray buffer → blends, interiors → borders, atlases → fragment/compute readers. |
| Settings | `Core/Graphics/RayTracing/IrradianceProbes/{Enabled, ProbeCountX, ProbeCountY, ProbeCountZ, ProbeSpacing, CameraHeightFraction, RaysPerProbe, Hysteresis, BounceFeedback, NormalBias, ViewBias}`, read ONCE at renderer initialization. `Enabled = false` keeps the volume allocated and bound (a consumer's pipeline layout needs its set) but skips the update and makes every query return zero through the shader-side flag. |

**Cost** (GPU profiler zone `IrradianceProbes`, 262 144 rays/frame): **0.27 ms** on `global-illumination`
(14.2 ms frame), **0.69 ms** on Sponza (73 ms frame, of which RTGI 48). Memory: 1.6 + 2.6 MB of atlases,
4 MB of rays.

**Consumers — two, and that is what makes the volume the engine's ONE multi-bounce estimator (Sep 2026).**
`probeIrradiance()` returns ENERGY (E/π, unscaled); the IndirectDiffuse intensity the primary surfaces
get (`probeVolume.ambientColor.w`) is applied only by a consumer that composes an image.
- **RTR** (set 3): at every hit `litColor += diffuseAlbedo × probeIrradiance(hitPos, hitNormal, hitV) × probeVolume.ambientColor.w`,
  and while the volume is enabled the raster IBL diffuse leg at the hit (slot 1, the unoccluded sky)
  is NOT added — the probes integrate the sky with visibility, adding both would count it twice (the
  `iblDiffuseWeight` ownership rule of the primary surfaces, applied at the hit). RTR **requires** the
  volume: `create()` fails without it and the stack falls back to SSR.
- **RTGI** (set 3, since 2026-09-12): its multi-bounce term at every bounce hit is
  `albedo_hit × probeIrradiance(hitPos, hitNormal, -sampleDir) × MultiBounce/Strength` (`probeFeedback()`),
  which REPLACED the screen-history reprojection `historyFeedback()` — a screen quantity that had no entry
  for a hit lit from off screen (measured 64 → 65 with it on/off on such a face). The RTGI bounce is now
  "exact direct at the hit + cached indirect at the hit", exactly what the probes do for their own hits, so
  the primary surfaces and the reflected ones read the same cache. RTGI **requires** the volume too
  (falls back to SSGI). Binding 2 of its trace input layout (the former history sampler) is left unbound
  — a shader that does not statically use a binding needs no descriptor there. `MultiBounce/Clamp` died
  with the history (a converged probe average has no fireflies; `bounceParams.y` is a dead slot). Cost of
  the query at every hit SAMPLE: RTGI trace **2.46 → 3.91 ms** on `global-illumination` at 3840×1990,
  within noise on Sponza (36.6 → 39.1 ms, traversal-bound); the probes' own update is unchanged.
The miss branch of every traced effect is the remaining follow-up.

> [!CAUTION]
> ⚠️⚠️ **The two ray-query descriptor set layouts declare FRAGMENT | COMPUTE since this volume.**
> `Renderer::createRTDescriptorSet()` and the bindless layouts were fragment-only; a compute pipeline
> may only bind a set whose bindings declare its stage. Keep both stages when adding a binding.
>
> ⚠️⚠️ **`#version 460`, not 450, on any shader that uses `GL_EXT_ray_query`.** glslang keeps the
> extension disabled below 4.60 and reports `'rayQueryEXT' : undeclared identifier` at the first use
> — a message that points at the wrong line and says nothing about the version. The RTGI/RTR fragment
> shaders were already 460; the first compute trace was written 450 and failed exactly so.
>
> ⚠️ **`DescriptorSet::writeCombinedImageSampler(binding, image, view, sampler)` writes the image's
> CURRENT layout** — UNDEFINED at creation for an image the GPU has not touched yet: 12 ×
> `VUID-VkWriteDescriptorSet-descriptorType-04150` on the first run. An image that lives in GENERAL
> uses the explicit-layout overload `writeCombinedImageSampler(binding, view, sampler, layout)` and
> `writeStorageImage(binding, view)` — both added to `Vulkan::DescriptorSet` for this volume (the
> earlier storage-image writes of RTR and the IBL baker still call `vkUpdateDescriptorSets` by hand).
> The volume also allocates its sets from its OWN pool: the renderer's main pool declares no storage
> image capacity.
>
> ⚠️ **Explicit `textureLod(…, 0.0)` in the update passes and in the query**: a compute shader has no
> derivatives to pick a mip, and the atlases have none. The shared alpha-test rule still calls
> `texture()`; in a compute pass that resolves to the base level in practice.
>
> ⚠️⚠️ **Compare the two indirect estimators (or the two lanes) at a PINNED exposure only** —
> `Core.SceneManagerService.Camera.setExposure(entity, component, aperture, shutterSeconds, iso)` (the pair `Camera.getActive()` answers) pins the triad and switches
> the metering off. The first probes-vs-RTGI comparison on this bench ("mirror 214 vs direct 64; 107 vs
> 64 at one bounce, the probes 1.7× RTGI") was taken on TWO auto-exposed frames — the mirror pose and
> the direct pose meter differently — and its single-bounce half was wrong IN DIRECTION. Re-measured
> 2026-09-12 at f/2.8 · 1/39 s · ISO 100, same green face, G channel, RTR mirror (white metal, F ≈ 1)
> against the direct view:
>
> | configuration | direct (RTGI) | in the mirror (RTR ← probes) |
> |---|---|---|
> | probe-fed multi-bounce (current) | **135** | **118** |
> | RTGI single bounce, probes recursive (= the screen-history era: that feedback added nothing off screen) | 38 | 116 |
> | everything single bounce (`BounceFeedback = 0`, `MultiBounce/Enabled = false`) | 38 | 30 |
>
> So the two estimators now agree within ~15 % (the traced side brighter — one exact, visibility-tested
> bounce refines the interpolated cache, the expected direction), and at ONE bounce the probes read
> ~20 % BELOW RTGI (a 1.5 m grid interpolated 0.75 m off a wall, the probes inside the column
> discounted by the back-face rule), not 1.7× above. The 3× gap of the screen-history era is closed.
> Residual ACCEPTED as the price of a cache (owner decision 2026-09-12): no calibration, and neither
> estimator is ever scaled to the other — the traced bounce refines the cache exactly where it
> matters, on the primary surfaces. On this
> bench (albedo exactly 1.0, ambient 0) the equilibrium tint near a coloured column is strongly that
> colour — the only absorbers in the maze are the four columns and the two mirrors — and that is
> physics, not a defect: the corridor walls next to the green column read G/R ≈ 1.2 in both lanes.
>
> ⚠️ GLSL `%` is undefined on a negative operand: the scroll offsets are kept in [0, count) on the C++
> side and every modulo of the mapping functions is fed non-negative values. Change one side, check
> the other.
