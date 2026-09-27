## 12. Post-Processing Effects

### RTGI (Ray-Traced Global Illumination) — Temporal + Multi-Bounce (Jul 2026; probe-fed multi-bounce Sep 2026)

One traced diffuse bounce per frame, temporally accumulated, with the multi-bounce read from the
irradiance probe volume at every bounce hit (a feedback loop through the SCREEN history until
2026-09-12 — see the volume section for why it moved). Since Aug 2026 everything downstream of the trace lives in
the owned `GIDenoiser` instance (see the section above) and follows the SVGF order —
temporal integration of the RAW trace first, variance-guided à-trous after:

1. **Trace** (half-res, RTGI-owned): cosine-weighted hemisphere rays via TLAS ray queries;
   at each hit, direct lighting (with shadow rays gated on the raster shadow-casting flag)
   PLUS the indirect irradiance the hit surface receives from the irradiance probe volume
   (`probeFeedback()`, multiplied by the HIT albedo — see energy algebra below). The
   output is **DEMODULATED**: no receiver albedo anywhere in the traced signal (Aug 2026).
2. **Temporal resolve** (half-res, GIDenoiser): reprojects through the velocity buffer
   (3×3 depth-nearest dilation), validates history (camera-distance in history alpha +
   world-normal history), optional variance clipping (`Temporal/NeighborhoodClamp`,
   default OFF since the SVGF reorder), then EMA (`Temporal/Alpha`). Output → history
   ping-pong `[writeIdx]` (the denoiser's own temporal history — no longer the multi-bounce source).
3. **Moments** (half-res, GIDenoiser): m1/m2 of the raw trace luminance + accumulation age,
   same reprojection/validation — the temporal variance guiding the à-trous.
4. **Normal history** (half-res, GIDenoiser): current view-space normals → world space,
   retained for the next frame's validation.
5. **À-trous** (half-res, GIDenoiser, `Denoiser/Iterations` passes): variance-guided
   edge-avoiding wavelet filter — see the GIDenoiser section.
6. **Apply** (full-res): multiplies by the receiver DIFFUSE albedo (albedo G-buffer
   `emAlbedo.rgb * emAlbedo.a`, `CombineContribution::needsAlbedo`) at FULL resolution, then
   additive blend, emissive-masked via material properties G-buffer.

**Albedo demodulation (Aug 2026):** the denoise/temporal chain carries **irradiance only**
(`E/π`); the receiver albedo is re-applied at full resolution in the combine pass — the
same convention as SSGI, and the standard practice of modern GI denoisers (SVGF, Schied et
al. 2017, HPG; NVIDIA NRD). Rationale: multiplying the albedo at trace time (half-res,
before the bilateral blur) destroyed texture detail exactly where GI dominates the final
pixel (dark areas — direct light ≈ 0, so the blurry `blur(albedo × E)` term visually
REPLACES the pixel). With demodulation the final term is `albedo_fullres × blur(E)`: the
texture stays native-sharp regardless of `GIBlurRadius`/`PixelDoubling`. Validated A/B on
Sponza (energy ratio 0.996, floor texture gradient ×1.64) and the Cornell GI demo
(uniform ≤2% run-to-run drift, colour bleed hue preserved, no multi-bounce runaway).

**Frame UBO instead of push constants:** the trace parameters (invRelativeViewProj + prevViewProj +
camera data) exceed the **128-byte Vulkan push constant minimum guarantee**
(`maxPushConstantsSize`). A per-frame UBO (`FrameUBOData`, std140) is shared by the
trace/temporal/normal-history passes — created via
`IndirectPostProcessEffect::createPerFrameUniformBuffers()`, bound through
`getInputLayout(samplerCount, uniformBufferCount)` (samplers first, then UBOs).

**History rectification = variance clipping (Aug 2026):** the temporal resolve bounds the
reprojected history to mean ± gamma × sigma of the current 3×3 neighborhood (Salvi, GDC 2016 —
the same technique as the engine TAA; `Temporal/VarianceGamma`, default 1.0), replacing the
former min/max clamp. Neutral with the static noise (measured), required for any future
animated-noise work.

**Animated noise infrastructure (Aug 2026) — DEFAULT OFF, measured regression:**
`Temporal/AnimatedNoise` advances the per-pixel sample rotation along the R2 sequence
(Roberts 2018) each frame (frame index in `traceParams.w`, flag bit 1 of `temporalParams.w`,
gated on the temporal chain). ⚠ With the fixed-alpha EMA (0.1) this REGRESSED temporal
stability ×2.4 on the Sponza corridor bench (mean peak-to-peak 0.67 → 1.65, >4/255 area ×9)
with NO spatial gain: the EMA leaks ~α/(2−α) ≈ 23% of the injected variance, while a frozen
pattern has near-zero temporal variance by construction — even an NRD-style 1/N accumulation
counter would only reach parity (computed). The winning lever is cutting the per-frame noise
BEFORE the resolve (variance-guided à-trous filter, SVGF) — only then does animated noise
become viable. Owner-isolated context: the TAA jitter is what makes the static pattern
shimmer (TAA→FXAA freezes it); see `docs/caution-points.md` § "Animated GI Noise".

**Multi-bounce energy algebra:** the probes store DEMODULATED indirect irradiance
(`E/π` — the same convention as the history and the combine), so the feedback IS multiplied by
the HIT surface's albedo at consumption (`albedo * probeFeedback(hitPos, hitNormal, sampleDir)` in
the trace) — the geometric series `1/(1-albedo*strength)` stays damped by physical albedo (< 1)
and converges. `MultiBounce/Clamp` is GONE (2026-09-12): it bounded the re-injected irradiance
against the fireflies of the per-pixel history, and a converged probe average has none.
`MultiBounce/Strength` is a continuous bounce-depth dial: 0 = single bounce, 1 = full series; it is
no longer gated on the history validity (the probes are valid from their first blended frame).

**History ping-pong correctness:** 2 half-res RGBA16F history targets (+2 normal history).
Frame N reads `[1-w]`, writes `[w]`; safe on a single queue thanks to the IRT's full
(non-by-region) external subpass dependencies **plus the explicit inter-pass barrier emitted
by `recordFullscreenPass()`** (the MoltenVK contract, § 12 "Inter-Pass Synchronization"). `m_historyValid` forces alpha=1 and
strength=0 on the first frame after (re)creation — the ping-pong images load DONT_CARE.
The temporal/normal-copy pipelines are created against the `[0]` targets and record into
`[1]` via render pass compatibility (same trick as the shared blur pipeline).

**Former structural limitation (screen-space feedback) — LIFTED 2026-09-12:** bounce rays used to
pick up feedback only from surfaces visible ON SCREEN in the previous frame, so light did not
propagate around corners never co-visible with lit surfaces (measured: +21% median brightness
where lit+penumbra were co-visible, zero effect in the occluded bend, 64 → 65 on the green column's
face lit from off screen). The irradiance probe volume is the world-space cache that paragraph
called for: at a pinned exposure the same face went **38 → 135** (its reflection through RTR reads
118 — one cache, two readers). Price: the probe query at every hit sample, RTGI trace 2.46 → 3.91 ms
on `global-illumination` (3840×1990).

**Static-geometry reprojection (v1):** the temporal reprojection uses `prevViewProj` +
current world position — exact for camera motion over static geometry, wrong for moving
objects (bounded by history validation + neighborhood clamp). Per-object motion vectors
(5th MRT attachment) are a planned follow-up; they require moving scene-pass transforms
out of push constants first (see `docs/caution-points.md`, 128-byte entry).

**Settings**: `Core/Graphics/PostProcessing/IndirectDiffuse/Temporal/Enabled|Alpha|DepthTolerance|
NormalThreshold|NeighborhoodClamp` (common to both lanes since 2026-09-12) and
`IndirectDiffuse/RayTracing/MultiBounce/Enabled|Strength` (`Clamp` deleted 2026-09-12 with the
screen-history feedback; see `SettingKeys.hpp` for defaults and rationale). `Temporal/Enabled=false` skips the
temporal chain entirely (no history VRAM, apply reads blur V — the pre-Jul-2026 flow).

**Code references:**
- `Effects/Lighting/RTGI.hpp` — Parameters, owned `GIDenoiser` instance
- `Effects/Lighting/RTGI.cpp` — trace GLSL shader (inline), blur snippet, combine snippet
- `Graphics/GIDenoiser.{hpp,cpp}` — `FrameUBOData` (std140), temporal/normal-copy shaders,
  history ping-pong recording
- `Graphics/ViewMatricesInterface.hpp` — frame-history contract (previous view/projection)
