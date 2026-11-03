## 12. Post-Processing Effects

### The temporal-discontinuity contract — the engine says WHEN, each integrator keeps its HOW (Nov 2026)

**Why.** On 2026-11-01/02 three mechanisms made a lighting change converge at once (the former item
`lighting-change-converges-over-frames`): TAA's large-change test (§ 06), the GI denoiser's pooled anti-lag and its
reset (§ 16), the sky's immediate IBL cycle (graphics doc 42 § The environment). The first two GUESSED, every frame and
for every pixel, an event the engine already knows — and paid for it every frame: on an Intel Raptor Lake iGPU
(1280×720, `reflexion-debug`) TAA +0.92 ms and the anti-lag +0.83 ms (item `intel-igpu-frame-rate-halved`). Owner
decision 2026-11-02: **unify the WHEN, keep each effect's HOW** — "sometimes divide to rule, sometimes unite to be
stronger: a middle ground". One signal, raised by the code that knows; each consumer keeps its own validated test, run
only on the frames the signal marks.

**The signal.** `IndirectPostProcessEffect::FrameContext::temporalDiscontinuity` (a `bool`), chosen by
`Renderer::latchTemporalDiscontinuity()` when the frame is recorded (after `renderSkyAtmosphere()`, before the chain),
handed through `PostProcessor::setFrameTemporalDiscontinuity()`. Never a flag the GPU reads back: frames in flight
each carry their own value; `Renderer.temporalCapture` writes each frame's value (`"temporalDiscontinuity"` in its
JSON, `FrameCapture::FrameMetadata`, owner decision 2026-11-03) — the only readout. It stays TRUE for `Renderer::TemporalDiscontinuityFrames` = **25 frames** from the last
raise, the raising frame included.

**The producers** (CPU, exact, no per-pixel work):

| Event | Where it is raised | Thread |
|---|---|---|
| A light added or removed (every type, a refused light too, `removeAllLights()`) | `LightSet::add()` / `remove()` → `m_temporalDiscontinuityEvents` | any |
| A light's EMISSION stepped: switched on or off, or a channel of `chromaticity × intensity` by ≥ ×2 | `AbstractLightEmitter::takeEmissionStep()`, collected by `LightSet::takeEmissionSteps()` in `Scene::publishStateForRendering()` | logic |
| The ambient's emission stepped (same rule) | `LightSet::setAmbientLightColor()` / `setAmbientLightIntensity()` → `noteAmbientStep()` | logic |
| A background swapped | `Scene::setBackground()` | any |
| A dynamic sky's IBL set completed by an IMMEDIATE cycle, taken by the scene | `Scene::updateDynamicSkyIBL()` (`SkyEnvironment::Publication::immediate`) | logic |
| A picture sky's IBL baked (it replaces the previous one at once) | `Scene::updateEnvironmentIBL()` | logic |
| The sky recorded an immediate cycle THIS frame (the frame its direct light jumps) | `SkyEnvironment::recordedImmediateCycle()`, read by `renderSkyAtmosphere()` | render |
| Another scene than the previous frame's | `latchTemporalDiscontinuity()` (a `weak_ptr`'s control block: a new scene at a reused address never compares equal) | render |

**The transport: a monotonic epoch** (owner decision 2026-11-03). `Scene::publishStateForRendering()` writes, with each
render state slot, the scene's own counter plus the light set's (`Scene::temporalDiscontinuityEpoch(slot)`); both are
monotonic, so is their sum. The renderer compares the slot it latched with its previous frame's value: a different value
raises, so a raise survives slots the renderer skipped. No per-light table on the render thread. Rejected: a per-light
comparison of the published uniform blocks in the renderer (a new virtual accessor, the intensity's offset differs per
light type, a cost per light per frame).

**The step rule** — `AbstractLightEmitter::emissionStepped(before, now)`: a channel above
`TemporalDiscontinuityFloor` (1 %) of the brighter of both emissions steps by at least `TemporalDiscontinuityRatio`
(×2), either way. Per CHANNEL (owner decision 2026-11-03): a colour switched at the same luminance counts (`city`'s
traffic signals, red → green). Switching on or off counts; a non-finite value never does; a light's first check only
remembers its emission (its addition already raised).

**The consumers** — each one builds TWO pipeline variants and records the one of the frame (owner decision 2026-11-03,
measured below; the FAST-forms precedent, § 41):

| Consumer | Unmarked frames `[0]` | Marked frames `[1]` |
|---|---|---|
| `TAA` (`m_pipelines`, define `EMEN_TAA_LARGE_CHANGE`) | the resolve without the large-change test | the 5×5 large-change test of 2026-11-01 (§ 06); `TemporalAA/LargeChangeThreshold` = 1 still keeps `[0]` |
| `GIDenoiser` anti-lag (`m_antiLagPipelines`, define `EMEN_GI_ANTILAG_POOLED`) | the PER-PIXEL test of before 2026-11-01 (3×3 spatial deviation, the slow moments' temporal one; the 6-frame cap), which still advances the fast window's mean and second moment | the pooled 5×5 test and its reset (§ 16) |

Later consumers (VolumetricLight, RTR accumulation, the clouds) read the same field.

**Why variants, not a uniform branch** — Intel UHD (Raptor Lake-S, driver 32.0.101.7088), `reflexion-debug`,
1280×720, layer off, Windows-PA 2026-11-03, idle machine, alternated T25 → T100 → N → T100 → N, 3 launches each, TAA
medians: threshold 0.25 **5.48 ms**; the block behind its uniform branch switched off (threshold 1) **4.82 / 4.82**; the
block REMOVED at compile time (a throwaway patch) **4.49 / 4.50**. A uniform branch switched off still cost 0.32 ms;
ScenePass flat (9.23-9.29) as the control; 0 VUID in the 20 runs. (A per-call uniform branch in a march loop had cost
+0.85 ms on the same iGPU, § 41.)

**The window, measured in frames** — the GI histories converge per FRAME. Linux, RTX 3070 Ti, `city` sunset jump
0.55 → 0.95, `FrameRateLimit` 25, TAA off, layer on, the land region's 8-bit mean at +100 / 500 / 1000 ms:

| RTGI | +0.1 s | +0.5 s | +1 s | +2 s |
|---|---|---|---|---|
| Per-frame tests (engine cf4c4432) | 16.3 | 15.8 | 15.6 | 15.3 |
| Contract, 8 frames | 20.5 | 17.9 | 17.1 | 15.4 |
| **Contract, 25 frames** | **16.0** | **15.6** | **15.5** | **15.2** |

SSGI is the same in the three (16.7-16.9 at +0.1 s, 13.8-14.2 at +1 s). Owner decision 2026-11-03: 25 frames.

**Validated** (Linux, RTX 3070 Ti, 2026-11-03, layer on, 0 VUID, 0 `[Error]`, 0 "Unable to compile"):
- TAA, `city` sunset at 40 FPS with GI off: 15.6 at +0.1 s, 14.9 at +1.4 s (the per-frame test of 2026-11-01: 15.95
  at +0.2 s); with RTGI 15.5 at +1 s.
- Parked (no raise but `city`'s traffic signals every 6-8 s): `relief` shimmer 0.612 (0.61 before); `city` day 5.31,
  the level of the test switched off (5.30) instead of 7.94 with the per-frame test — the sea's sun glints are averaged
  again. **Owner verdict 2026-11-03, eye A/B at the quay (pose (150, 12, 245) → (−150, 0, 300), f/8 · 1/125 · ISO 100,
  40 FPS), the contract against the mark forced on every frame: "no difference"** — TAA stays in the contract.
- GI parked, TAA off, both lanes: `sponza` age view 214.9 / 214.6, level 113.2 / 113.6, RTGI shimmer > 8: 1.64 %
  (1.66 % before, § 16); `city` day 111.8 / 113.9, shimmer 10.1 / 11.2 (9.7-10.4 / 11.0-12.1 before); `labyrinth` SSGI
  28.2-28.8 / 2.5-4.2 over 11 runs (28.3-28.6 / 3.5-4.1 before).
- `sky-lab` 0.76 → 0.80 (the sky's immediate cycle as the producer), both lanes, f/8 · 1/125 · ISO 800: the floor
  (R) 0.1 → 0.0 from the first captured frame — converged before the capture starts (its own console command after the
  jump; the reset converges in 2-3 frames), so this protocol no longer resolves the lag; 0 VUID, 0 `[Error]`.
- **The Intel gain, measured on the implementation** (Windows-PA 2026-11-03, Intel UHD Raptor Lake-S, driver
  32.0.101.7088, `reflexion-debug`, 1280×720, SSGI, layer off, idle machine, A = engine 95160d41 / B = ef0bb2bc
  alternated A B A B A B, 8 s windows after a 15 s settle): TAA **5.48 → 4.52 ms**, `GIDenoiser/antiLag` **1.21 →
  0.39 ms** (`SSGIEffect/internal` 14.48 → 13.68), ScenePass 9.26 → 9.28 (the control), the frame 45.99 → 44.29 ms
  (20.5 → 21.2 FPS); spread within an arm ~0.04 ms, the arms do not overlap — the frames after the load are unmarked.
  MSVC 14.44 Release and Debug 0 warning at `/W4 /WX`; on the RTX 3500 Ada (layer on) `city` RTGI 15.93 / 15.52 at
  +0.08 / 1 s against 15.01 at +8 s, SSGI 15.22 / 13.84 against 13.78, TAA alone 15.96 / 15.18 against 14.99;
  `sky-lab`, `relief` clean; 0 VUID, 0 `UNASSIGNED-`, 0 "Unable to compile" in every run.
- **macOS** (macOS-PA 2026-11-03, Apple M6, MoltenVK, AGXG18G 362.5.1, engine ef0bb2bc, 2560×1440, layer on): Release
  0 warning, AppleClang `-Wdocumentation -Werror` clean over the 264 TUs including a touched file; `city` sunset, SSGI
  (22 FPS, under the 25 cap) — the land box 14.95 / 14.07 / 13.87 / 13.83 at +0.1 / 0.5 / 1 / 2 s against 13.82 at
  +8 s (+0.4 % at 1 s); TAA alone at 34 FPS 14.35 / 13.84 at +0.1 / 1 s against 13.60 (+1.8 % at 1 s); `sky-lab`,
  `relief` clean; 0 VUID, 0 `UNASSIGNED-`, 0 "Unable to compile", 0 `[Error]` in the four runs.
- **AMD** (LinuxB-PA 2026-11-03, RADV RENOIR iGPU, Mesa 26.0.8, engine ef0bb2bc, 1280×720, layer on, ScreenSpace
  only): Release 0 warning; `city` sunset, SSGI, the land box 20.5 / 15.4 / 14.4 / 14.1 at +0.1 / 0.5 / 1 / 2 s against
  13.8 at +8 s; TAA alone, 4 runs: three fast (14.4 at +0.1 s, +0.7-2.2 % at 1 s), **one slow** (17.3 / 16.1 / 15.1 at
  +0.1 / 0.5 / 1 s, +9.4 % at 1 s: about 27 frames per e-fold, the UNMARKED rate) — the run whose jump landed inside
  the capture (frame #0 still day) and whose `getStatus` 10 ms after `disable(IndirectDiffuse)` still showed the
  pending switch; Linux's jump inside a capture was fast (240.2 day → 15.6 at +0.1 s). Not explained; `sky-lab`,
  `relief` clean; 0 VUID, 0 `UNASSIGNED-`, 0 "Unable to compile" in the 8 runs.
- The readout (Linux, RTX 3070 Ti, layer on, `city` TAA alone at 40 FPS): a parked capture 60 frames unmarked;
  three sunset jumps, each 26 frames marked then unmarked (the sky's raise, then the lamps' and the sun's one tick
  later), the land box 16.1 → 15.6 over the first 8 frames; the third jump landed inside its capture (frame #0 at
  144.6, already marked) and converged as fast; 0 VUID.
- Raises observed: `city` — the load, each signal change (every 6-8 s), each time-of-day jump (the sky's immediate
  cycle, then the lamps and the sun stepping one tick later); `labyrinth` — the dying lamp (`Animations::LampFlicker`,
  71 ↔ ~1200 cd) about 8 times a second.

> [!NOTE]
> **A flickering light marks** (owner decision 2026-11-03): a flicker IS a discontinuity, and TAA and the GI follow it
> sharply, as since 2026-11-01. In a scene with a flickering light on screen the marked variants run most of the time:
> no gain there, the look of today. A change nobody signals (an emissive material switched without a light, a
> camera cut) converges at the pre-2026-11-01 rate — TAA ~20 frames per e-fold, the GI's 6-frame cap: the accepted
> trade-off (owner decision 2026-11-03); a public `Scene` call raising the epoch comes only with a real case.

> [!WARNING]
> `labyrinth`'s GI noise runs vary from launch to launch (the dying lamp's flicker inside the 16-frame window: max
> ~189, the level dips — RTGI 14.8-18.0 in 3 runs of 14, SSGI 19.3 / 21.5 in 2 of 13). Compare its median over several
> runs, never one run.

References: item `temporal-discontinuity-contract` (while open), § 06 (the TAA resolve), § 16 (the anti-lag), § 41
(the FAST forms), graphics doc 42 § The environment.
