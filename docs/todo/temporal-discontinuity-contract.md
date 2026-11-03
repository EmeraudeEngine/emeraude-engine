---
id: temporal-discontinuity-contract
title: One temporal-discontinuity contract instead of three per-pixel large-change detectors
status: open
priority: unranked
scope: Graphics (TAA, GIDenoiser, SkyEnvironment, Renderer, LightSet)
opened: 2026-11-02
tags: [performance, architecture, temporal, intel]
---

# One temporal-discontinuity contract instead of three per-pixel large-change detectors

## Why

The item `lighting-change-converges-over-frames` (closed 2026-11-02) was solved by three mechanisms, each one GUESSING,
every frame and for every pixel, an event the engine already KNOWS:

| Mechanism | Commit | What it detects | Cost every frame |
|---|---|---|---|
| TAA's large-change test | 3c25c355 | 5×5 mean exposed luminance, this frame against the history (9 + 9 bilinear taps) | Intel +0.92 ms, RTX +0.10 |
| GI anti-lag, pooled 5×5 + reset ratio | e2a7320e, cf4c4432 | pooled raw / slow / fast GI means, `AntiLagResetRatio` 4 | Intel +0.83 ms, RTX +0.06 |
| SkyEnvironment's whole IBL cycle in one frame | 2cdc4112 | the star moved > 5° or its illuminance changed ×2 (CPU, exact) | none (CPU) |

Their causes were all discrete: a time-of-day jump (city's sunset, sky-lab), a lamp switched on. The owner (2026-11-02):
"take a step back": one contract, raised by the code that knows, honoured by every temporal integrator; the selective
per-pixel tests run only on the frames it marks. Measured costs of the per-frame form: item `intel-igpu-frame-rate-halved`.

## Proposed design (nothing written yet)

Owner decisions (2026-11-02): **unify the WHEN, keep each effect's HOW** ("sometimes divide to rule, sometimes unite
to be stronger: a middle ground") — one signal, each consumer keeps its own response and validated thresholds in its
own class, no shared rejection mechanism. **Producers: all three below.** **The GI denoiser off-event keeps the
per-pixel detector** of before e2a7320e (the 6-frame cap for moderate continuous changes); the pooled 5×5 and its reset
run on marked frames only. **Selection: measure first** (below), then a push constant or pipeline variants.

- **The signal** (owner decisions 2026-11-03): `bool temporalDiscontinuity` in `IndirectPostProcessEffect::FrameContext`
  (it names what the consumers do, not the cause: VolumetricLight, RTR and clouds can join), chosen by the `Renderer`
  when it records the frame, TRUE for **8 frames** after the last raise (the IBL published a logic tick later, the
  frames in flight, the GI lane reading the previous frame).
- **The transport: a monotonic epoch** (owner decision 2026-11-03). The scene keeps a counter, published with the
  render state slot; the logic side increments it when a light is added, removed, enabled, disabled or its luminance
  steps by ≥ ×2 between two publications, on `AmbientLightChanged`, on a background swap, and when
  `Scene::updateEnvironmentIBL()` takes a set completed by an immediate cycle. The renderer compares the epoch of the
  slot it reads with the one of its previous frame (robust to skipped slots); the sky's immediate cycle is a flag of
  the renderer itself (`renderSkyAtmosphere()` records before the post-process of the same frame). No per-light table
  on the render thread. Rejected: a per-light comparison of the published blocks in the renderer (a new virtual
  accessor, the intensity offset differs per light type, a cost per light per frame).
- **Producers** (CPU, exact, no per-pixel work):
  - the sky: `SkyEnvironment::largeChange()` (already 5° / ×2), the frame it records an immediate cycle;
  - the light set: a light added, removed, enabled, disabled, or its published luminance stepped by ≥ a ratio between two
    frames (a per-light comparison in the renderer, so game code raises nothing by hand: the city's lamps are covered);
  - the ambient light (`LightSet::AmbientLightChanged`) and a background swap.
- **Consumers**: TAA (its large-change test runs only on marked frames), the GI denoiser (the pooled test and its
  reset only on marked frames), later VolumetricLight, RTR accumulation, clouds.
- **Selection**: a pipeline variant per consumer (the FAST-forms precedent, graphics doc 13-12 § 41) — measured
  2026-11-03 (What remains): the push-constant uniform branch switched off still costs TAA ~0.32 ms on the Intel (a
  per-call uniform branch in a march loop had cost +0.85 ms there).
- **Validated look kept**: on marked frames the tests are the ones the owner validated (5×5), unchanged.

## What remains

- The signal's name, window and transport, and the unsignalled-change trade-off: decided 2026-11-03 (above, ⚠️ Traps).
- The selection. Intel, Windows-PA 2026-11-02, `reflexion-debug`, layer off, alternated: the existing uniform branch
  switched off (`TemporalAA/LargeChangeThreshold` = 1) TAA 4.77 ms against 5.44 at 0.25 (−0.67; 4 and 6 clean values;
  a second half of the series discarded, the machine loaded by another application). ~0.2 ms short of the 4.57 measured
  before 3c25c355 (another session). **Settled, Windows-PA 2026-11-03** (round 7, idle machine: a 5 s load sample
  before every launch, 0.09-0.89 CPU-s; same scene and driver; alternated T25 → T100 → N → T100 → N, 3 launches each;
  N = the GLSL block REMOVED at compile time, a throwaway patch): TAA medians T25 5.48, T100 4.82 / 4.82, N 4.49 /
  4.50 ms — N below the switched-off branch by 0.33 and 0.32 ms in both pairs (threshold set at 0.15), Frame −0.30 ms;
  ScenePass flat (9.23-9.29) as the control; 0 VUID, 0 `[Error]` in the 20 runs (layer off). **A uniform branch
  switched off still costs ~0.32 ms on this iGPU: the consumers take pipeline variants** (the FAST-forms precedent),
  not a push-constant branch. Total recoverable on TAA: −0.99 ms on unmarked frames.
- Implementation, then the validation of 3c25c355 / e2a7320e / cf4c4432 again: city jumps (both lanes, GI on and off,
  25 and 40 FPS), sky-lab, parked labyrinth / sponza / relief / city (no false trigger, shimmer unchanged), Intel timings,
  0 VUID, three OS.

## ⚠️ Traps

- A change nobody signals (an emissive material switched, an explosion's flash) converges at the pre-2026-11-01 rate
  (TAA ~20 frames per e-fold, the GI's 6-frame cap): **accepted by the owner (2026-11-03)** — the validated behaviour of
  before 2026-11-01; an explosion's flash is a light, so the epoch covers it. A public `Scene` call raising the epoch
  for game code comes only with a real case.
- The anti-lag's fast moments are STATE: they must still be updated on unmarked frames; only the pooled test is skipped.
- Frames in flight: the signal is a per-frame value chosen at record time, never a shared mutable flag read by the GPU.

## References

- Item `intel-igpu-frame-rate-halved` (the per-frame costs), graphics doc 13-12 § 16 (the anti-lag), § The TAA resolve,
  graphics doc 42 § The environment, item 7 (the immediate IBL cycle).
