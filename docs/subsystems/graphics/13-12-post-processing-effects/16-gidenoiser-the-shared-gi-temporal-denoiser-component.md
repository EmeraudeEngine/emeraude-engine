## 12. Post-Processing Effects

### GIDenoiser — the shared GI temporal denoiser component (Aug 2026, SVGF work site)

`Graphics/GIDenoiser.{hpp,cpp}` — the temporal machinery extracted VERBATIM from RTGI
(stage 0 of the SVGF plan; measured non-regression: Sponza corridor ptp 0.733/0.790 within
the 0.67–0.80 baseline envelope, identical GPU timings). **One instance is OWNED by each
producer** (RTGI, SSGI, and RTR since 2026-09-13): the code is shared, the histories are NOT —
two producers reprojecting into one history would corrupt each other.

> [!IMPORTANT]
> **Reflection mode (2026-09-13, `Parameters::reflectionMode`, flag bit 3).** The temporal and
> moments passes share ONE GLSL reprojection (`GIDENOISER_REPROJECTION_GLSL`) with two paths:
> the SURFACE path through the velocity buffer (the GI producers' only path, unchanged), and the
> VIRTUAL path — the reflected content moves with its mirror image `P + V·hitT`, projected with
> `prevViewProj` (Stachowiak 2015, ReBLUR 2021). Both histories are fetched and blended by their
> weights (virtual share fading over roughness 0.15 → 0.45; an invalid path hands its weight to
> the other); the signal is accumulated on FOUR channels (premultiplied colour + confidence); the
> validation distance is the virtual one, `cameraDistance + hitT`, stored in the MOMENTS alpha
> (the colour history's alpha carries the confidence). The owner hands the hit data to
> `recordResolve(cb, raw, context, &hitData)` (binding 7, RG16F: cone width, hitT — 0 = no
> reflection, `maxDistance` on a sky miss so the environment reprojects as a far point); the GI
> producers pass nothing and the binding holds their raw input as a placeholder. The frame UBO
> moved to binding 8. ⚠️ Outside reflection mode every path is bit-for-bit the previous
> behaviour (verified on the direct image: cube 0.54, wall 0.34 as before). Numbers and the
> integration in RTR: `docs/reflection-pipeline.md` § 3.2. Like `DenoisePass`, it
extends `IndirectPostProcessEffect` purely to reuse the fullscreen-pass infrastructure and
is never inserted into a stack.

The component owns: the resolved-irradiance history ping-pong (RGBA16F, A = camera
distance), the world-normal history pair, the temporal-resolve and normal-copy pipelines,
and the per-frame `FrameUBOData` UBO (moved from RTGI — the owner binds it into its own
trace pass via `frameUBO(f)` and fills it via `updateFrameData(f, data)`, which also
advances the animated-noise R2 index). Owner-facing flow, in `recordPre/PostDenoisePasses`:

1. `setTemporalEnabled(flag)` then `create(w, h)` at the OWNER's working resolution
   (UBOs always allocated; history VRAM and pipelines only when the temporal chain is on).
2. RTGI's trace no longer binds `historyReadTexture()` (Sep 2026: its multi-bounce feedback reads the
   irradiance probe volume, set 3); the history is consumed inside the denoiser only.
3. `m_combineSource = recordResolve(cb, noisyInput, context)` — records temporal resolve +
   normal history, flips the ping-pong, returns the texture the combine must consume
   (the noisy input unchanged when the temporal chain is off).

The SVGF stages are built INSIDE this component — settings live under the mirrored
`RayTracing/GlobalIllumination/{Temporal,Denoiser}/` +
`ScreenSpace/GlobalIllumination/{Temporal,Denoiser}/` groups (owner decisions,
2026-08-06). **CHANTIER COMPLETE**: RTGI stages 0–4 owner-validated live ("the only thing
left vibrating is the VolumetricLight streak" — a separate subject), SSGI wired the same
day — its FIRST temporal accumulation. The component assembles its own frame UBO
(`updateFrameData(frameIndex, context, FrameInputs)` — matrices from the renderer's view
state, temporal parameters from `GIDenoiser::Parameters`; the producer only supplies its
trace scalars, zeroed when it has no feedback loop / sky term) and serves the debug views
to any producer via `debugCombineContribution(prefix, mode)`.

**SSGI wiring (Aug 2026):** SSGI owns its GIDenoiser instance (histories are per-producer),
gained `requiresVelocity()`, left the shared H/V blur, and its trace noise was upgraded
from the banding-prone `fract(sin(dot))` hash to PCG + the R2 animated sequence
(`noiseFrameIndex` push constant, < 0 = frozen). Measured (Sponza corridor, RT off, double
runs): ptp 0.367–0.395 → **0.306–0.338**, area > 2/255 halved, energy preserved
(mean luma 24.30 vs 24.35). The `ScreenSpace/GlobalIllumination/BlurRadius` key is inert.

**Stage 3 — per-pixel 1/N accumulation counter (Aug 2026):** the temporal blend weight is
`max(1/(age+1), 1/MaxAccumulation)` instead of the fixed `Temporal/Alpha` (flag bit 2,
`Denoiser/AccumulationCounter` default true, `Denoiser/MaxAccumulation` default 64). The
age lives in the moments history B channel (stage 1); the colour resolve samples it at the
reprojected UV so colour and moments integrate with the SAME weight. Fast convergence after
a disocclusion (1, 1/2, 1/3…), steady-state variance leak 1/N ≈ 1.6% at N=64 versus
α/(2−α) ≈ 23% at fixed α=0.1 — the factor that sank the first animated-noise attempt.
`Temporal/Alpha` only rules when the counter is off (A/B lever).

**Stage 4 — animated noise DEFAULT ON (Aug 2026, owner decision):** with the à-trous +
1/N in place, `Temporal/AnimatedNoise` flipped to default true. Measured (Sponza corridor,
double runs): energy restored (mean luma 24.9 vs 22.9 frozen — a frozen pattern turns
stable bright outliers into "converged signal" the luminance guide protects, i.e.
fireflies), best distribution tails (>8/255: 0.33%), ptp mean 0.55–0.57 versus the
0.67–0.83 marbled baseline. Owner-validated live: the GI shimmer is extinguished.

**Stage 2 — SVGF reorder + variance-guided à-trous (Aug 2026):** the GI producers LEFT the
shared H/V `DenoisePass` (`usesSharedDenoise()` back to false — a multi-iteration à-trous
does not fit the two-pass separable MRT shape; RTAO/CS/RTR keep merging theirs) and record
their whole chain in `recordOverlayPasses()`. New order (canonical SVGF): trace → temporal
resolve **of the RAW trace** + moments → à-trous 5×5 B3-spline, `Denoiser/Iterations`
passes (default 4), footprint doubling (stride 1,2,4,8), edge-stopping on depth, view-space
normal and **luminance normalised by the local standard deviation**
(`Denoiser/LuminanceSigma`, default 4 — the SVGF auto-dosage), variance filtered alongside
with the w² rule; first iteration falls back to a 3×3 SPATIAL variance estimate where the
accumulation age < 4 (freshly disoccluded pixels — silhouettes under the TAA jitter, the
animated foliage). The combine consumes the à-trous output. (Until Sep 2026 the TEMPORAL output also served RTGI
as its multi-bounce colour history; that feedback now reads the irradiance probe volume and the
history is the denoiser's alone.)
`Temporal/Enabled=false` now means RAW passthrough (diagnostic only: no spatial filter
without its variance guide). The `BlurRadius` key is inert for RTGI.

Measured (Sponza corridor bench, double runs): temporal ptp mean 0.67–0.83 → **0.46–0.50**,
area > 2/255 divided by 4, GPU +2.9 ms half-res on the 3070 Ti (4 iterations, optimisation
candidates: single-channel gathers, fewer taps on late iterations). Two structural findings:
(1) `Temporal/NeighborhoodClamp` **default flipped to false** (owner decision) — clipping
the history against the RAW 3×3 statistics costs ~5% GI energy for no stability gain
(designed for the pre-blurred input that no longer exists; SVGF uses the double disocclusion
validation alone); (2) the FROZEN noise seed turns stable bright outliers into
"converged signal" the luminance guide protects — visible cyan fireflies. Exploratory
stage-4 test (AnimatedNoise=true, settings only): fireflies dissolve, energy restored
(luma 24.8 vs 25.2 old chain), ptp 0.69–0.73 with better tails than baseline — the ×2.4
regression is gone; the residual leak is the fixed-alpha EMA (α/(2−α) ≈ 23%), exactly what
the stage-3 1/N counter replaces (steady-state leak at N=64 ≈ 0.8%).

**Stage 1 — per-pixel moments + variance (Aug 2026):** a third ping-pong pair
(`_GIMoments`, RGBA16F) integrates the first/second raw moments of the **RAW estimate's
luminance** (the owner passes its unfiltered trace as `rawInput` to `recordResolve()` —
variance of the already-blurred signal would underestimate the noise the spatial filter
must remove). Channels: R = m1, G = m2 (temporal variance = `max(m2 − m1², 0)`, derived at
the read site via `momentsTexture()`), B = accumulation age in frames (saturates at 64,
reset on disocclusion — the future 1/N counter), A = camera distance (validity marker).
The moments pass duplicates the colour resolve's velocity reprojection + 3×3 depth-nearest
dilation + double disocclusion test VERBATIM — both passes MUST agree on which pixels have
a valid history; `alpha >= 1.0` also routes to the reset path (covers the first frame
after (re)creation AND a user-set alpha of 1). Measured: +0.196 ms half-res on the 3070 Ti
(RTGIEffect/temporal 0.266 → 0.462 ms), visually neutral (Sponza corridor ptp 0.830 within
the baseline envelope, identical mean luma).

**Denoiser debug views** (`RayTracing/GlobalIllumination/Denoiser/DebugView`, default 0,
read by RTGI at create): the combine draws the denoiser internals INSTEAD of the GI —
1 = temporal variance (amplified ×1e6, bounded — a LINEAR scale is unreadable under the
photometric exposure), 2 = accumulation age (white = young/disoccluded, < 4 frames).
Validated on Sponza: the variance map matches the owner's shimmer cartography
(floor near the lit door + curtains brightest, penumbra structured, true darkness black);
the age map shows saturation on stable surfaces and permanent per-frame resets exactly on
the wind-animated ivy and on silhouette edges under the TAA jitter — the first direct
visualisation of the "jitter is the vibrator" mechanism, and the zone the stage-2 spatial
variance fallback must cover.
