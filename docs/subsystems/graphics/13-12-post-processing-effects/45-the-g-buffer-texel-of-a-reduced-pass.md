## 12. Post-Processing Effects

### The G-buffer texel of a reduced pass — a half-resolution pixel names ONE texel, and unprojects from its centre (Nov 2026)

> [!CAUTION]
> **THE DEFECT** (found 2026-10-18 by the Windows peer on `jungle-ruins` option 1, explained and fixed 2026-11-24).
> At 1280 × 720 — projet-alpha's default window — SSGI drew a regular lattice over sloped geometry: a thin line every
> 10 px across, another every 45 px down, locked to the SCREEN, on Windows and on Linux alike; nothing at 1024 wide,
> nothing clear at 1920 × 1080. SSAO drew the same one, far weaker.

#### The mechanism

A pass rendered at half resolution reads the full-resolution G-buffer for the centre of each of its pixels. That centre
is, in the G-buffer, exactly the CORNER four texels share: `vUV × size = 2x + 1`. Two reads sit on it:

- `texture(depthTex, vUV)` — the grab pass gives the depth, the normals and the packed attachments a NEAREST sampler
  (`GrabPass.cpp`), so one of the four texels comes back;
- `texelFetch(depthTex, ivec2(vUV * size), 0)` — the same corner, floored.

Which texel is returned hangs on the last bit of the interpolated `vUV`. Then the position is unprojected at `vUV`
with a depth that belongs to a texel half a full-resolution pixel away: off the surface by that offset times the depth
slope, on one side or the other with the pick. A march or a horizon search that starts there finds its own surface
in front of it, or not. The rounding repeats across the screen, so the error is a lattice whose period is **twice the
odd factor of the reduced size**: 640 = 2⁷ · 5 gives 10 px, 360 = 2³ · 45 gives the rows every 45 px; 512 gives
nothing. The sign of a line follows the slope: bright on one side of the frame, dark on the other.

#### The rule (owner decisions, 2026-11-24)

One shared helper, `Graphics/Effects/Shared/GBufferTexelGLSL.hpp`, for every pass whose target is smaller than the
G-buffer:

- **The reduction is stated ONCE by the effect, in C++** (`constexpr uint32_t Reduction{2}`):
  `GBufferTexelGLSL::reducedSize(fullSize, Reduction)` sizes the targets, `GBufferTexelGLSL::sourceGLSL(Reduction)`
  gives the shader the same factor (`emGBufferReduction`) and the two functions, spliced after the sampler
  declarations.
- **`emGBufferTexel(gbufferSize)`** — the texel this fragment stands for: `ivec2(gl_FragCoord.xy) × reduction`,
  integer arithmetic, clamped to the attachment. The FIXED choice: the first texel (lowest x and y) of the block the
  pixel covers. One fetch, no rounding.
- **`emGBufferTexelUV(texel, gbufferSize)`** — that texel's centre. The centre's depth, normal and material
  properties are read from THAT texel with `texelFetch`; whatever is unprojected or marched from the centre starts at
  this UV, never at `vUV`.

⚠️ What the fixed choice costs, by intent: a reduced pass computes its result for a point half a full-resolution texel
up and left of its pixel's centre, and the combine reads it back as if it stood at the centre. Negligible for a diffuse
term (measured below); to be weighed again before a sharp one uses the helper. The closest of the four depths
(`textureGather`, four reads) was the other option shown, and not taken.

Applied to `SSGI` (the horizon search and the trace) and to `SSAO` (the occlusion pass). `GIDenoiser` reads the same
attachments the old way in every stage and was left alone: the lattice does not need it (it was gone with the denoiser
untouched). The other reduced passes — `RTGI`, `RTAO`, `RTR`, `SSR`, `RTContactShadows`, `GIDenoiser` — are engine
item `reduced-passes-name-their-gbuffer-texel`.

#### Measured (2026-11-24, Linux, RTX 3070 Ti, `jungle-ruins --demo-options 1`, window-less, screen-space lane, exposure pinned at f/11 – 1/250 s – ISO 100, validation layer loaded: 0 VUID, no shader that fails to compile)

The column profile is the mean luma of rows 400-700; "×N" is the spectrum of its first difference at a period over
the spectrum's median; "folded" is the peak-to-peak of the profile folded on a period.

| size | measure | before | fixed |
|---|---|---|---|
| 1280 × 720, SSGI on | 10 px / 5 px | ×32.2 / ×33.4 | ×4.4 / ×2.8 (the pyramid, below) |
| | folded on 10 px, the far slope (x 880-1280, rows 400-475) | 1.573 | **0.009** (7 to 13 px: 0.003 to 0.027) |
| | folded on 10 px, the near ground (rows 476-700) | 0.077 | **0.008** (SSGI off: 0.007) |
| 1280 × 720, SSGI off (SSAO alone) | 10 px | ×6.3 | **×1.0** (×1.2 with every scene effect bypassed) |
| 1024 × 576, SSGI on | rows, 18 px / 9 px | ×8.4 / ×8.1 | **×4.1 / ×1.3** (SSGI off: ×3.9 / ×1.3) |

- **The size decides** (before the fix, `Core.WindowService.resize()`): columns at 10 px and 5 px at 1280 wide, none
  at 1024 nor 2048 (half widths 512, 1024); rows at 18 px and its harmonics at 576 and 1152 high (288 = 2⁵ · 9,
  576 = 2⁶ · 9). ⚠️ Not confirmed at 1600 × 900 nor 1920 × 1080: ×2.4 at 50 px, ×1.6 at 30 px, no clear lattice.
- **The picture changes a little, as it must** (the centre no longer sits off its surface): `jungle-ruins` frame mean
  36.77 → 38.34 (SSAO alone +0.64, SSGI alone +1.25); per pixel, mean |difference| 1.81, p99 7.9, max 31, four pixels
  of 921 600 off by more than 16. `sponza`, same lane: 25.37 → 25.55, mean |difference| 0.66, p99 4.6; 1 139 pixels
  (0.12 %) off by more than 16, all one-pixel lines along geometric edges — the texel of an edge pixel is now always
  the same one.
- NOT run: macOS, Windows, AMD (the peers, after the push); the ray-traced lane's reduced passes (`jungle-ruins`
  option 1 has no main directional light: its four ray-traced effects fall back to the screen-space ones).

⚠️ **Two traps of this measurement.**
- A spectrum over the whole band takes regular CONTENT for a lattice: `jungle-ruins`' stepped pyramid sits in the
  middle of the frame and keeps ×4.4 at 10 px after the fix (folded on 10 px over x 400-880: 0.081 before, 0.133
  after, 0.09 at 11 and 13 px). Fold by region and compare with the neighbouring periods before calling a residue a
  lattice.
- With the temporal chain off (`…/IndirectDiffuse/Temporal/Enabled = false`) the noise is FROZEN: a folded profile
  then carries the pattern of that one noise image, larger than with the chain on (0.042 against 0.011).

**Code references:** `Graphics/Effects/Shared/GBufferTexelGLSL.hpp`, `Graphics/Effects/Lighting/SSGI.cpp`
(`Reduction`, the two centres), `Graphics/Effects/Lighting/SSAO.cpp` (`Reduction`, the centre),
`Graphics/GrabPass.cpp` (the NEAREST samplers), `docs/reflection-pipeline.md` § 3.2 (RTR unprojects at the texel it
fetched since 2026-09-13).
