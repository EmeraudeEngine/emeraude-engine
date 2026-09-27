## 12. Post-Processing Effects

### The screen-space sky visibility — the GTAO horizon search that made SSGI an owner (Sep 2026)

> [!CAUTION]
> **THE DEFECT.** The two lanes are supposed to be two costs of ONE lighting. In an enclosed space
> they were two lightings: measured on `sponza` (2026-09-13, RTX 3070 Ti, 2880×1620, exposure
> PINNED at f/11 · 1/250 s · ISO 100, upper gallery `setPosition(4.8, 8.4, -0.3)` +
> `lookAt(-6, 0, 0.5)`), the same frame read **11.2/255 mean in the RayTracing lane and 52.8 in the
> ScreenSpace lane** — the galleries lit as if the courtyard had no roof.
>
> **The cause was a contract, not a knob.** RTGI claims the indirect diffuse, so the scene switches
> the raster's diffuse IBL leg off and the sky reaches every surface through rays that measure its
> real visibility. SSGI claimed nothing and had no sky term, so the weight stayed 1 and the ambient
> pass applied the **full irradiance cubemap** — the sky as if nothing stood in the way — attenuated
> only by the short-range SSAO, with the SSGI bounce added on top.

**The mechanism** (`Effects/Lighting/SSGI.cpp`, pass `SSGI_SkyVisibility`, half-res RGBA16F):
a **GTAO horizon search** — Jimenez, Wu, Pesce, Jarabo, *"Practical Realtime Strategies for Accurate
Indirect Occlusion"* (SIGGRAPH 2016 courses); implementation reference **Intel XeGTAO** (MIT). Per
pixel, `SkyVisibilitySliceCount` planes through the view vector are walked on both sides; each depth
sample raises that side's horizon; the cosine-weighted arc between the two horizons is the
visibility **V**, and the mean direction of that arc is the **bent normal**. The trace pass then adds

```glsl
indirectLight += texture(texturesCube[IrradianceCubemapSlot], bentNormalWorld).rgb * skyLuminance * V;
```

right where it normalises its bounce estimator — so the sky rides the SAME demodulated convention
(an irradiance `E/PI`, the receiver albedo applied ONCE at the combine) and the SAME SVGF denoiser
as the bounce, exactly as RTGI's sky does. `SSGI::providesIndirectDiffuse()` then returns true and
the raster leg goes to 0.

> [!IMPORTANT]
> **Why the pass lives in SSGI and not in SSAO**, where the first plan put it: the slot order is
> `ContactShadows → IndirectDiffuse → Reflections → AmbientOcclusion`, and `SSR` flushes the combine
> group before it runs (`readsChainColorUpstream()`). A sky composited at the `AmbientOcclusion`
> slot lands AFTER the reflections have sampled the chain colour, so every screen-space reflection
> would reflect a world with no sky light — trading one incoherence for another. Composited at
> `IndirectDiffuse`, the reflections see it. Owner decision, 2026-09-13.

**Acceptance measurements** (Sponza, SS lane, pinned exposure, validation layers on, **0 VUID**,
values linearised through the gamma and the ACES fit before any ratio):

| Test | Sky term OFF | Sky term ON | Reading |
|---|---|---|---|
| **Open sky** (Sponza's ROOF from above, `setPosition(-9, 22, 0)` + `lookAt(-9, 0, 2)`, four tile crops — REAL geometry under an open sky, see the caution below) | raster 0.178-0.243 | SSGI 0.150-0.198 | **0.81-0.90 of the raster leg it replaces**, and RTGI reads 0.78-0.83 on the same crops: with V ≈ 1 the two lanes deliver the same sky, at the `IndirectDiffuse/Intensity` = 0.8 the concept applies to both |
| **On-screen occluder** (the vault above, `setPosition(-9, 0, -2.5)` + `lookAt(-9, 1.2, 6)`) | 0.02643 | 0.01170 | ×0.44 — the arches occlude the sky |
| **Arcade floor** (`setPosition(-9, 5, 0)` + `lookAt(-9, 0, 0.6)`, seen from ABOVE) | 0.1031 | 0.0449 | implied V = 0.44 where the surface really sees **V ≥ 0.036** — the occluder is the arcade ceiling, BEHIND the camera. This line is the blind spot, measured; see the warning below |
| **Upper gallery** (the defect's own pose) | 0.0479 | 0.0212 | the gap to the traced lane falls from **8.4× to 3.7×** (RT 0.0057) |

> [!CAUTION]
> ⚠️⚠️⚠️ **The first published version of this table used "the grass outside Sponza" as its open-sky
> test and reported a bit-identical "ratio 1.0000". That grass is the SKYBOX.** The `sponza` demo
> builds no ground (it never calls `enableBasicGround()`) and the Intel asset is the building alone,
> so everything below the edge of its stone platform is the Kloppenheim05 cubemap — its own
> photographed meadow, rocks and flowers included. A background is drawn unlit, writes no albedo and
> no material-properties G-buffer, and no lighting term of any lane can move it: an A/B taken on it
> returns a perfect ratio for EVERY term under test, forever. It also explains the 0.21/255 of
> residual movement that made it look merely "insensitive" — that was the TAA reprojecting an edge,
> nothing more.
> **Two rules, in order**: identify WHAT SURFACE the crop holds — geometry or background — then check
> that it MOVES with the term under test before reading any ratio off it. A ratio of exactly 1.0000
> is not a result, it is a warning.

**Cost**: `SSGIEffect/internal` 6.02 ms → 7.02 ms average at 2880×1620 (RTX 3070 Ti, GPU profiler,
same pose sequence) — **≈ 1.0 ms** for 3 slices × 6 steps × 2 sides at half resolution. For scale,
`SSAOEffect/trace` is 0.40 ms and `RTGIEffect/trace` 39 ms on the same frame.
**Temporal stability**: static camera, consecutive frames, mean |Δ| 0.5-0.7/255, p99.9 ≤ 7/255,
0.001-0.013 % of pixels above 16 — the animated R2 noise is absorbed by the SVGF accumulation.

> [!WARNING]
> **What it still cannot see, by construction: an occluder that is OFF SCREEN.** A roof above and
> behind the camera does not occlude anything in a depth-buffer search. That is what the remaining
> 3.7× at the gallery pose is made of, and it is the structural limit of the lane, not a defect of
> this pass. ⚠️ Out-of-frame samples are **skipped, never clamped**: a clamp-to-edge sampler recycles
> the border depth and paints a false occlusion band along the frame (the trap the SSAO kernel
> already documents).
>
> ⚠️ **Do NOT "fix" the residual by scaling the lane down.** The missing quantity is a per-pixel
> visibility, not a level: a flat factor would darken the open bench that is now exactly right.
>
> ⚠️⚠️ **SETTLED, 2026-09-14 — and it went the other way.** This section first read that the traced
> lane "delivers 0.05 of the unoccluded sky where the geometry says 0.45", and opened an item on the
> traced lane under-lighting. **The 0.45 was a bad premise**: it assumed that floor sat at the bottom
> of an open 10 m slot. It does not — it is under the arcade. Attribution, in order:
> - `RTAO` is NOT the cause: switching the `AmbientOcclusion` concept off moves that floor by **1 %**
>   (0.00475 → 0.00479). `Reflections` carried 34 % of it, `ContactShadows` nothing. **RTGI alone:
>   0.00312**, i.e. 3.1 % of the unoccluded sky.
> - **The true sky visibility of that surface, measured**: put the camera AT the point and look
>   straight up (`setPosition(-9, 0, 0)` + `lookAt(-9, 10, 0.01)`), then integrate the sky pixels of
>   the capture weighted by `cos⁴θ` (the cosine-weighted solid angle of a rectilinear image plane).
>   The frame covers 62 % of the cosine-weighted hemisphere and the sky fills 3.6 % of it:
>   **V ≥ 0.036**. Six ground points probed the same way across the courtyard read V ≤ 0.054 — the
>   whole ground floor there is covered.
> - **Verdict: RTGI (0.031 of the unoccluded sky) matches the measured visibility (≥ 0.036). The
>   traced lane is right, and the residual gap is the screen-space lane still OVER-estimating** where
>   the occluder is off screen — exactly the structural limit above, and a 12× over-estimate in that
>   framing. On an open-sky surface the two lanes agree to ~6 %.
> - ⚠️ **The method is the transferable part**: a look-up capture measures the visibility of a
>   surface point directly, in 30 seconds, and settles which estimator is right. Reasoning from a
>   floor plan ("a courtyard is open") is what produced the false premise.

**Settings** (`Core/Graphics/PostProcessing/IndirectDiffuse/ScreenSpace/`): `SkyVisibilityEnabled`
(the A/B of the whole defect — off restores the raster's unoccluded leg), `SkyVisibilityRadius`
(**16 m**, and ⚠️ NOT `IndirectDiffuse/MaxDistance`: that one is the BOUNCE range, 8 m, and the two
are independent in the traced lane too — RTGI casts its ray to the FAR PLANE and only reads a bounce
from a hit closer than `MaxDistance`, because *"nothing hit within 8 m"* does not mean *"sees the
sky"*), `SkyVisibilitySliceCount` (3), `SkyVisibilityStepCount` (6), `SkyVisibilityFalloffRange`
(**0.2**, ⚠️ not XeGTAO's 0.615: that value keeps an ARTISTIC occlusion local, while a roof must
occlude the sky at full strength however far it is — the fade only smooths the last fifth of the
range, the same choice as the SSGI range fade).

> [!NOTE]
> **Two deliberate deviations from XeGTAO**, both documented in the shader: the slice frame uses a
> NORMALISED tangent (XeGTAO rotates the raw screen direction, an approximation exact only at the
> centre of the frame), and `FinalValuePower` (2.2) is NOT applied — it is a look control for an
> artistic AO, and this V is a physical visibility that multiplies a light source.
> **`SSAO` was deliberately left untouched** (owner decision, 2026-09-13): it keeps its short-range
> hemisphere kernel for its own multiply. Sharing one visibility between the two effects is a
> separate item, `docs/todo/ssao-consumes-the-sky-visibility-lane.md`.
