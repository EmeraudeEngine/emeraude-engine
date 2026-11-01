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
> moved to binding 8 (and to binding 9 with the anti-lag, 2026-10-25: binding 8 is the anti-lag texture). ⚠️ Outside reflection mode every path is bit-for-bit the previous
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
variance fallback must cover. The key is read by SSGI too (`IndirectDiffuse/Denoiser/DebugView`). ⚠️ The views write
nits into a chain that holds nits × pre-exposure. From pre-exposure step 6 (2026-10-24) they read white under any
exposure, until they were multiplied by the frame's pre-exposure (2026-10-25). Read the age view at a pinned exposure
where 100 nits is mid-scale (f/4, 1/8 s, ISO 100): 64 frames reads ~200, the anti-lag's cap (~6–15) ~60–100, young
pixels and the sky white.

**Stage 5 — ANTI-LAG (2026-10-25, owner decision: the fast/slow history of NVIDIA's RELAX / ReBLUR — D. Zhdan,
"ReBLUR: A Hierarchical Recurrent Denoiser", Ray Tracing Gems II, 2021).** The 1/N counter's floor, 1/64, made EVERY
lighting change take ~64 frames per e-fold, and nothing rectified the history (the neighbourhood clamp is off by
default: on the raw input it costs ~5 % of the GI). Counted in frames, the lag was 0.2 s at 390 FPS and 10–30 s on an
iGPU at 47 FPS (a lamp switched off, the dynamic sky's IBL moving). A new pass, recorded FIRST (`GIDenoiser/antiLag`),
keeps a FAST mean of the raw luminance (`Denoiser/AntiLagFastFrames`, 6) with the moments' reprojection and validation,
in an RG16F ping-pong (R the fast mean, G this frame's cap); it compares it with the SLOW mean (the moments' m1) against
the fast mean's expected noise times `Denoiser/AntiLagSigma` (3), plus a 2 % relative floor. That noise is one frame's
deviation over √(frames averaged), one frame's deviation being the LARGER of the raw estimate's 3x3 spatial deviation
and `Denoiser/AntiLagTemporalSigma` (0.5) × the pixel's temporal deviation (the slow moments). Beyond it, the slow
history's age is capped to the fast length
(a ramp over one more tolerance); the colour resolve and the moments apply the cap (flag bit 4, binding 8), restart
together, and grow back to `MaxAccumulation`. Diffuse producers only (RTGI, SSGI; the reflection mode never runs it).
`Denoiser/AntiLag` (default true) is the A/B lever.

| sky-lab, Linux, RTX 3070 Ti, ~390 FPS, exposure pinned: the floor at +0.1 / 0.25 / 0.5 / 1 s after a 0.76 → 0.80 jump | RT lane | SSGI lane |
|---|---|---|
| Anti-lag off (the former behaviour) | 39 / 21 / 8 / 2 | 32 / 19 / 8 / 2 |
| Anti-lag on, the first detector (spatial deviation alone, 711c70a7) | 19 / 4 / 1 / 0 | 21 / 5 / 1 / 0 |
| Anti-lag on, the safe detector (temporal weight 0.5) | **24 / 10 / 4 / 1** | **28 / 15 / 7 / 2** |
| `IndirectDiffuse` off (the measurement chain's own latency) | 10 / 1 / 0 / 0 | — |

**⚠️ The first detector fired on the noise of a SPARSE estimator** (macOS-PA, 2026-10-25, confirmed on Linux). Where
the screen-space lane's rays mostly miss (a dim corridor), most 3x3 windows hold no hit, so the spatial deviation reads
0 and only the 2 % floor is left. The fast mean, dipping between two hits, crossed it nearly every frame and the slow
history was held at 6 frames. SSGI's screen-space feedback amplified that small per-frame bias into an energy loss.
- `labyrinth`, SSGI lane, converged right wall: 39.8 instead of 88.5.
- The age view: mean 128.6 instead of 214.9.
- The RT lane and sky-lab were untouched: identical noise on and off, and identical age view (244.1 vs 244.5). That
  is why the steady-state checks above passed.

Any trigger correlated with the noise biases the level: firing between hits loses energy, firing on a hit's arrival
gains it (a detector on the tracked RMS of fast − slow: +33 %). The feedback amplifies either. The safe detector keeps
`labyrinth`'s level (86.4) and the age view (214.4). It gives up most of the screen-space acceleration: on SSGI, much of
the first detector's speed WAS that permanently short history. A `labyrinth` lamp switch-off reads, in the panel at
+1 / 2 / 3 / 5 s:
- off: 50 / 17 / 5.3 / 2.5;
- first detector: 3.5 / 2.4, but the scene's level after the switch was 19.4 instead of 29.7;
- safe detector: 53 / 20 / 6.1 / 2.6.

A per-pixel 3σ test on a 6-frame mean cannot separate SSGI's per-frame noise (of the order of the mean) from a full
drop. Owner decision 2026-10-25: the safe detector now, a neighbourhood-pooled test next (ReBLUR / RELAX); A-SVGF's
temporal gradients (Schied et al., HPG 2018) are the reference beyond. All the detectors measured are in § The
history of the lag, below. Cost: 0.037 ms (`GIDenoiser/antiLag`, RTX 3070 Ti, GI resolution).
0 VUID, 0 SYNC-HAZARD (sky-lab both lanes, `reflexion-debug`).

**The POOLED detector (2026-11-01, owner decision — the neighbourhood-pooled test the safe detector announced).** The safe
detector never fired on a large global change: its temporal noise came from the SLOW moments (64 frames), whose variance
during a lighting change measures the old/new mixture itself, so its threshold grew with the change it had to detect
(`city`, a sunset jump 0.55 → 0.95: the age view dipped 221 → 193 for 0.5 s and was back at 216 by 1 s, while the scene
stayed lit by the gone daylight for 8 s). Now:
- the test is POOLED over a 5×5 neighbourhood: the previous slow and fast means are averaged over the reprojected
  neighbourhood, the fast one advanced by this frame's pooled raw luminance, and compared against the noise of a mean of
  that many pixels over min(age + 1, fastFrames) frames (NRD RELAX / ReBLUR compare neighbourhood statistics the same
  way);
- the temporal noise is the FAST window's own deviation (its second moment, kept beside its mean: the anti-lag pair is
  RGBA16F since: R the fast mean, G the cap, B the root of the fast second moment), averaged over the neighbourhood: it
  forgets the old light within `AntiLagFastFrames`. One frame's noise stays the larger of the 5×5 spatial deviation and
  `AntiLagTemporalSigma` (0.5) × that temporal one.

| Measured (Linux, RTX 3070 Ti, 1920×1080, validation layer, exposure pinned) | safe detector | pooled |
|---|---|---|
| `city` sunset 0.55 → 0.95, land region at +0 / 1 / 2 / 3 / 12 s, RT lane (~42 FPS) | 231 / 187 / 139 / 88 / 15.2 | 236 / **28.7 / 15.4** / 15.1 / 15.0 |
| same, SSGI lane | 221 / 170 / 125 / 79 / 14.2 | 220 / **24.1 / 14.4** / 13.9 / 13.8 |
| `sky-lab` 0.76 → 0.80, floor R at +0.1 / 0.25 / 0.5 / 1 s, RT lane (~60 FPS, `temporalCapture`) | 39.5 / 27 / 19.5 / 13.2 | 40.4 / **17 / 6 / 2** |
| same, SSGI lane | 37.6 / 29 / 21.2 / 15 | 34.6 / **15 / 6 / 2** |
| `labyrinth` converged level (whole image, f/2.8 1/30 ISO 6400, test pose, 40 s), SSGI / RT (anti-lag OFF: 61.1 / 68.6) | 60.2 / 69.7 | 61.1 / 70.1 |
| `labyrinth` age view mean (`DebugView` 2, f/4 1/8 ISO 100), SSGI / RT | 213.8 / 211.2 | 213.5 / 211.2 |
| `GIDenoiser/antiLag` (GPU profiler, no layer, `city` day), RT / SSGI | 0.082 / 0.080 ms | 0.142 / 0.134 ms |

The level is kept (no false trigger: the age view and the converged levels are those of the safe detector and of the
anti-lag off), and a change converges in about the fast window's length instead of the 64-frame history. 0 VUID,
0 `[Error]`. ⚠️ The former protocol's right wall (x 1350–1900, y 200–800) reads 3.1 at `labyrinth`'s own exposure
(f/2.8 1/30 ISO 400) on both detectors: the measurement above uses the whole image at ISO 6400.

**The pooled detector on the three OS and AMD** (engine e2a7320e, 2026-11-01; no energy loss anywhere, 0 VUID, 0 "Unable
to compile" — RADV and MoltenVK included):
- Windows-PA, RTX 3500 Ada (~106 FPS, layer on): the `city` sunset settles by +2 s (AntiLag off: +8-9 s). Intel iGPU
  (SSGI, 24 FPS): the `reflexion-debug` picture-sky switch reaches its floor (0.0121 → 0.0034) by +6 s (October, before
  any anti-lag: 0.96 at +10 s); `labyrinth` 63.35 vs AntiLag off 62.78 (whole image, ISO 6400).
- macOS-PA, M6 (SSGI, 2560×1440, 25 FPS): `labyrinth` 55.84 = AntiLag off 55.84; age view 213.3; the `city` sunset
  187 / 88 / 37.8 / 21.6 at +1 / 2 / 3 / 4 s (AntiLag off: 222 / 198 / 174 / 150, still 24.5 at 12 s); `sky-lab`
  0.76 → 0.80 floor 39.7 / 31 / 18 / 7 at +0.1 / 0.25 / 0.5 / 1 s (off: 41.2 / 39.4 / 34.4 / 25).
- LinuxB-PA, AMD RADV Renoir (SSGI, 17-22 FPS): the `city` sunset 208 / 135 / 73 / 40 / 25 at +1 … 5 s, 14.3 at 12 s
  (off: 225 / 202 / 181 / 162 / 143, still 43.8 at 12 s); `labyrinth` within the launch-to-launch noise of off.
- ⚠️ **On a slow GPU the convergence still takes seconds**, for two reasons. Everything here is counted in FRAMES: the
  history, the fast window, TAA. And on the M6 the INPUT itself is slow: with `IndirectDiffuse` disabled the `city`
  land still reads 95 / 26.7 / 16.4 at +1 / 2 / 3 s at 25 FPS (Linux at ~42 FPS: final from +1 s), so the anti-lag
  sees no sharp change and its cap never engages (the age view slides 206 → 142 over 2.4 s; on Linux it drops to ~80
  within 0.1 s and holds). Open item `lighting-change-converges-over-frames`.

### The history of the lag (the former item `gi-history-keeps-a-switched-off-emitter-lit`, 2026-10-06 → 2026-11-01)

- **Found** by macOS-PA (Apple M2, SSGI, ~6.7 FPS, `labyrinth` test pose, 2026-10-06): the 55 flickering lamps switched
  off, their panels' emission at 0 at once, the panel interior read 230 / 167 / 80 / 27 / 10 / 4.7 / 1.4 at +5 / 10 /
  15 / 20 / 25 / 30 / 40 s; `IndirectDiffuse` off: 0.0 at +2 s. The history fed the panel's old glow back onto its
  own pixels and decayed PER FRAME. Switching on was instant.
- **The same under the sky's IBL** (owner: a defect, 2026-10-25): macOS M6 SSGI ~76 FPS, `sky-lab` 0.76 → 0.80 an
  e-fold of ~1.3 s (~100 frames), TAA on or off alike; brightening lagged too (~3 s); `IndirectDiffuse` off: final by
  +0.5 s. Windows' Intel iGPU (~47 FPS then) kept a picture sky's light on the floor 10-30 s after `setBackground()`.
- **The cause**: the 1/N accumulation's floor, 1/64, with no history rectification — Linux ~390 FPS `sky-lab`
  RT 39 / 21 / 8 / 2 at +0.1 / 0.25 / 0.5 / 1 s, `MaxAccumulation` 8 instead: 26 / 4 / 1 / 0.
- **The detectors measured** (Linux; `labyrinth` right wall; `sky-lab` floor R after 0.76 → 0.80 at +0.1 / 0.25 / 0.5 /
  1 s; the `labyrinth` lamp panel at +1 / 2 / 3 s):

  | Noise term | labyrinth | sky-lab SSGI | sky-lab RT | lamps |
  |---|---|---|---|---|
  | anti-lag OFF | 88.5 | 32 / 19 / 8 / 2 | 39 / 21 / 8 / 2 | 50 / 17 / 5.3 |
  | spatial σ (the first detector) | 39.8 | 21 / 5 / 1 / 0 | 19 / 4 / 1 / 0 | 3.5 / 2.4 / 2.4 |
  | temporal σ (slow moments) | 88.6 | 32 / 19 / 8 / 2 | 28 / 10 / 5 / 1 | — |
  | spatial + 0.5 temporal (RELAX form) | 86.4 | 31 / 16 / 7 / 2 | 27 / 10 / 4 / 1 | — |
  | max(spatial, 0.5 temporal) (the safe one) | 86.4 | 28 / 15 / 7 / 2 | 24 / 10 / 4 / 1 | 53 / 20 / 6.1 |
  | max(spatial, 0.25 temporal) | 64.1 | 24 / 10 / 4 / 1 | 24 / 10 / 4 / 1 | — |
  | tracked RMS of (fast − slow) | 117.7 (+33 %) | 28 / 13 / 6 / 1.8 | 26 / 9 / 4 / 1 | — |

  The safe detector on the three OS (49b6f570, 2026-10-25): macOS `labyrinth` 82.3 vs off 84.3, age view 214.71 vs
  214.92; Windows Intel 85.7 vs off 87.6 (`AntiLagTemporalSigma` 0: 42.5).
- **The pooled detector** (above) closed it on 2026-11-01: a sunset in `city` now converges in ~1 s on an RTX at
  40-100 FPS, with no level change.
- **Not done, recorded for later** (each a design decision): A-SVGF's temporal gradients (re-shade last frame's samples
  with this frame's light; the producers must re-evaluate their own samples); a history luminance clamp (A. Lauritzen /
  B. Karis, "High-Quality Temporal Supersampling", SIGGRAPH 2014; ReBLUR's history reset on a lighting change).
- Protocol notes: the `labyrinth` test pose is `Act.setPosition(-6, 0, 67.3)` + `Act.lookAt(-6, 3.9, 66)`; its lamp
  switch-off turns off `LabyrinthLamp<N-1>` for each `LabyrinthFlickeringPanel<N>` (55), panel region x 860-1060 /
  y 420-620 at 1080p; the age view is `Denoiser/DebugView` 2 read at f/4 1/8 s ISO 100; a lamp's panel follows it
  (`docs/subsystems/animations/02-animations-specific-rules.md`).

