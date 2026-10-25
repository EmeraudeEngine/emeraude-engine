## 12. Post-Processing Effects

### The view reconstruction — one matrix path from a depth to a position (Oct 2026)

> [!CAUTION]
> **THE DEFECT** (found 2026-10-25, fixed the same day, item closed after the three OS). The chain linearised depth with
> two scalars, `FrameContext::constants.nearPlane / farPlane`, and `PostProcessor::setClipPlanes()` — their only
> writer — had **no caller**: every effect worked with 0.1 / 1000 while the real projection used the near plane of
> `computeNearPlane()` (it FOLLOWS THE FIELD OF VIEW) and far = `Core/Graphics/ViewDistance` (10 km). The error is a
> factor that depends on the lens, at 16:9:
>
> | Lens (vertical FOV) | real near | 1 m read as | 10 m | 1 km |
> |---|---|---|---|---|
> | 13 mm (85°, labyrinth's player) | 0.047 m | 2.11 m | 20.7 m | 680 m |
> | 24 mm (53°) | 0.070 m | 1.43 m | 14 m | 590 m |
> | 85 mm (16°) | 0.096 m | 1.04 m | 10.4 m | 510 m |
>
> ⚠️ **The formula was never the problem**, although the item first said so: `2nf / (f + n − (2d − 1)(f − n))`,
> OpenGL's inverse written on `depth × 2 − 1`, simplifies EXACTLY to the Vulkan inverse `nf / (f − d(f − n))`. A
> measurement (the depth of field's focus sweep, below) contradicted the first table, and the algebra confirmed it.
> Check an "obvious" formula defect against one measured number before writing it down.

**Owner decision (2026-10-25, option B): a matrix reconstruction everywhere, through ONE per-frame chain UBO.** Not
the real planes plugged into the scalar formula: a scalar formula is wrong for the orthographic camera whatever its
planes, and the scalar COPY is what went out of sync.

#### The contract

- `PostProcessor` writes, once per frame and per frame-in-flight slot, a `ViewReconstructionGLSL::Data` block
  (std140, 208 bytes): the projection, its inverse, the inverse of projection × view ROTATION (camera-relative) and
  the camera position — all of the READ state, the matrices that rendered the depth buffer. It reaches the effects as
  `FrameContext::view` (null only if the buffer could not be created or written; `Buffer::writeData()` logs that).
- `Effects/Shared/ViewReconstructionGLSL.hpp` holds the GLSL block and the helpers, spliced into an effect's source
  with `sourceGLSL(set, binding)` between its sampler declarations and its body:
  - `emViewPosition(uv, depth)` — view space (camera looks down −Z);
  - `emViewDepth(uv, depth)` — the distance along the view axis (what "linear depth" means);
  - `emRelativePosition(uv, depth)` — the world offset from the camera; `emViewDistance()` its length;
  - `emClipFromView()`, `emUVFromView()`, `emDepthFromView()` — back to clip space, the screen, the depth buffer.
- An effect declares the block with `getInputLayout(samplerCount, 1)` (uniform buffers come AFTER the samplers, so
  the block's binding is the sampler count), writes it every frame with
  `writeUniformBufferObject(binding, *context.view)` and returns its input untouched when `context.view` is null.
- `PostProcessor::PushConstants` is down to 16 bytes (frame size, time, deltaTime); `setClipPlanes()`,
  `nearPlane`, `farPlane` and `tanHalfFovY` are gone from the chain and from its two GLSL mirrors
  (`Saphir/Generator/PostProcessing.cpp`, `CombinePass.cpp`). The signed-`tanHalfFovY` contract (see
  `docs/coordinate-system.md`) died with it: the inverse projection carries the Y flip by construction.

| Effect | Binding of the block | What it reads |
|---|---|---|
| `SSAO` | 3 | reconstruction-space positions (X, Y, +Z) |
| `DepthOfField` focus / setup | 2 / 3 | `emViewDepth()`: the thin lens' object distance |
| `MotionBlur` gather | 4 | `emViewDepth()`: the soft depth comparison (`SoftDepthExtent` in true metres) |
| `TAA` resolve | 5 | `emViewDepth()`: the history's linear depth in alpha |
| `AtmosphericFog` | 3 | `emRelativePosition()` + `emView.cameraPosition` |
| `VolumetricScattering` | 5 (after the cascade block at 4) | the same |
| `SSR` trace / resolve | 2 / 6 (`SSRResolveInputView`) | positions, `emUVFromView()`, `emDepthFromView()` |
| `SSGI` horizon / trace | 2 / 3 (4 with the sky visibility) | positions, `emUVFromView()` |

`VolumetricLight` only lost two dead push fields. `RTGI` took its sky-ray distance from the frozen `farPlane`
(1000 m): it now reads the REAL far plane of the main view, so geometry between 1 km and the view distance hides the
sky. The matrix-based effects (RTAO, RTR, RTGI's trace, the contact shadows, the clouds, the aerial perspective) were
never affected. `SelectionOutline` is not a chain effect: it reads the real planes (`mainViewMatrices`) through the
scalar formula, right in perspective and wrong only for an orthographic main camera — to port if the editor ever needs
one.

> [!IMPORTANT]
> **The SSGI horizon search maps a screen direction through the matrix too.** It used to scale a UV step by
> `2·tanHalfFovY·aspect·z`. It now takes the view-space displacement of ONE whole UV unit along each screen axis at
> the centre's depth-buffer value — `reconstructPosition(uv + (1, 0)) − P` and `(0, 1)`. At a constant depth value
> the view Z is constant and the view X, Y are AFFINE in the UV, in perspective (jittered or not) as in
> orthographic, so the difference is exact, not a local approximation; the out-of-screen UV is only an operand of
> the matrix.

> [!WARNING]
> **The reconstruction space of the screen-space family** (SSAO, SSR, SSGI) is view X, Y and a POSITIVE Z (the
> distance along the axis). Their helpers convert at the boundary (`vec3(v.xy, -v.z)`); never feed a positive-Z
> point to `emUVFromView()` without negating it back.

#### Measurements

Linux, RTX 3070 Ti, 1920×1080, validation layers ON (0 `VUID-` in every run), the same binary built from the commit
before (`522e9d47`) and after, the same pose and a PINNED exposure, each slot disabled in turn in ONE launch; values
are the mean luminance LINEARISED (sRGB, then the inverse of the Narkowicz ACES fit).

> [!WARNING]
> **Two traps of the linearisation, both met on this lot (macOS-PA, Windows-PA, 2026-10-25).**
> - **A clipped pixel has no inverse.** The fit saturates, so a 255 byte maps to whatever clamp the script chose, and
>   labyrinth's panel pose holds ~1 % of clipped pixels (the panel, its glare, its defocus). Exclude the pixels clipped
>   in ANY compared state (one shared mask) and report their share.
> - **The clamp sits at the top of the fit, never lower.** Narkowicz's curve tends to 1.033: clamp the sRGB-decoded
>   value at 0.9999 (255 → 7.22 linear). These Linux runs first clamped at 0.79 (→ 0.94 linear), which also FLATTENED
>   every bright unclipped pixel: labyrinth's green-lit exit alcove read a ratio of 0.76 instead of 1.05, and the SSGI
>   delta at the panel pose +0.129 instead of +0.167 — an "unexplained platform spread" that was this clamp alone (with
>   it right: Linux +0.167 at 1920×1080, macOS +0.165 at 2560×1440, Windows +0.178 / +0.168 at 1280×720).
> The figures below use the mask and the 0.9999 clamp.

- **Depth of field** (the proof that settled the formula question): labyrinth's lamp panel 2.6 m from the eye, manual
  focus sweep at f/1.4, sharpness = 99th percentile of the panel edges' gradient. 24 mm: the old DoF is sharpest on a
  plateau 3.0–4.4 m (it believed the panel at ~3.7 m, the ×1.43 of the table); the ported one peaks at 2.6 m (74.7 at
  1.6 m, 128.7 at 2.6 m, 85 at 4.4 m). 85 mm: both peak at 2.6–2.7 m (the frozen near is right there).
- **SSAO**: the floor-wall corner band now covers its metric width (profile 0.80 / 0.91 / 0.98 against one row at 0.92
  before); sky-lab's 9 km cubes lost the staircase and the diagonals macOS-PA reported. Owner: the radius stays 0.5 m —
  it now MEANS 0.5 m (it covered ~0.23 m near the camera before: corners are visibly stronger).
  - **A defect the true distances revealed, fixed with them**: the unlit path wrote aoResponse 15 and a placeholder
    normal (0, 0, 1) (`SceneRendering.cpp`), so the emissive lamp panels lost ~36 % to SSAO. Unlit surfaces write
    aoResponse 0 (material-properties G = 15/255), and the SSAO pass outputs 1 there so its blur cannot spread it.
  - **Not this defect, still there**: a uniform darker band on the side faces of sky-lab's 9 km cubes (131/115/106
    with AO against 159/141/125 without, macOS-PA). A 0.5 m radius projects to under a pixel at 9 km and depth
    precision rules; a screen-space radius floor is the usual lever.
- **Fog** (`light-and-shadow-debug`, its medium: 0.015 /m, falloff 0.2): both occupants of the `Fog` slot integrated
  a length about twice too long with that camera's lens. VolumetricScattering's contribution −0.0247 → −0.0092,
  AtmosphericFog's +0.041 → +0.021; the near ground clears, the horizon stays white.
- **SSGI** (labyrinth, the 13 mm player lens, SS lane): the indirect diffuse +0.139 → +0.166 (+20 %; the toggle noise
  of this scene is ±3 %), the walls brighter: the bounce rays (`IndirectDiffuse/MaxDistance` 8 m) and the horizon
  search (`SkyVisibilityRadius` 16 m) now cover their stated metres instead of half of them. Sponza's four
  sky-visibility poses (roof, vault, arcade, gallery; its lens is longer) move by ≤ 3 %.
- **SSR**: labyrinth +0.041 → +0.039 (noise), sponza unchanged.
- **TAA** (the resolve gate: `relief` parked, f/8 · 1/125 · ISO 100, `temporalCapture(16)`): peak-to-peak mean 0.590 →
  0.595, p99.9 2.93 → 2.93, 0 % > 8/255 both. Its depth test is a RATIO (±1 %, 5 %): the near-plane factor cancelled,
  the far compression did not reach this pose.
- **Motion blur**: `SoftDepthExtent` (0.05 m) now means 0.05 m; it was ~half as soft with a wide lens.
- **RTGI** with its sky rays at the real far plane (10 km instead of 1 km): `RTGIEffect/trace` 0.849 → 0.875 ms median
  on `relief` (the windows overlap: 0.83–0.87 against 0.85–0.89), 0.174 → 0.175 ms on sky-lab's distant cubes.

**Three operating systems** (engine `5bd6a118`, 2026-10-25): macOS-PA (Apple M6, MoltenVK, ScreenSpace lane) and
Windows-PA (RTX 3500 Ada and Intel iGPU, MSVC /W4 /WX Release + Debug) green — 0 `VUID-` (sync validation on macOS),
no crash. Fog contributions within 3 % of Linux on the three (VolumetricScattering −0.0092 / −0.0094 / −0.0090..−0.0092,
AtmosphericFog +0.021 / +0.020 / +0.020..+0.021); the depth of field sharpest at 2.6 m everywhere; the TAA gate 0 % > 8/255
everywhere; with no panel on screen, labyrinth's levels within 5 % of Linux (macOS); at the panel pose, masked and
clamped right, the SSGI deltas agree too (the warning above). That pose was dominated by the panel double count, fixed
the same day ([`42-emission-in-the-indirect-diffuse`](42-emission-in-the-indirect-diffuse-a-light-s-face-never-re-emits.md)).

**No setting default changed**: every value these effects read in metres (`AmbientOcclusion/ScreenSpace/Radius`
0.5, `IndirectDiffuse/MaxDistance` 8, `IndirectDiffuse/ScreenSpace/Thickness` 0.5,
`IndirectDiffuse/ScreenSpace/SkyVisibilityRadius` 16, `MotionBlur/SoftDepthExtent` 0.05, and SSR's
`Parameters::maxDistance` 50 / `thickness` 0.2, not settings) was chosen as a physical quantity, not fitted to a look,
and is now honoured. A scene's medium density is the scene's.
