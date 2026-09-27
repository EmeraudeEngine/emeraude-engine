## 12. Post-Processing Effects

### RTR glossy cone — per-pixel hit distance (v2) and a disk gather over the pyramid (v3), Aug 2026

**The width.** The blur of a glossy reflection is `tan(θ) · hitT · f_px / (dCam + hitT)` texels
(half-width), with θ = 2·atan(0.6436 α) the half-max half-angle of the GGX lobe in
reflected-direction space (α = roughness²), hitT the traced hit distance, dCam the camera distance of
the reflecting point and f_px the focal length in trace texels (`|P[1][1]| · traceHeight / 2`,
`TracePushConstants::coneScale`). A contact reflection (hitT → 0) stays sharp; a distant one tends to
the lobe's angular size. Computed in the TRACE fragment shader, which owns every term, and written
per pixel to a **cone width map** — an R16F storage image (`m_coneImage`, set 1 binding 4 of the
effect's own `RTR_TraceInput` layout). `imageStore()` from a fragment shader needs the
`fragmentStoresAndAtomics` device feature, enabled in `Vulkan/Instance.cpp` for it: without it the
SPIR-V validation rejects the pipeline (`VUID-RuntimeSpirv-NonWritable-06340`) and the effect fails
to create. UNDEFINED → GENERAL before the trace, GENERAL → SHADER_READ_ONLY after, like the pyramid.
Misses and every early return write 0: the prefiltered environment is already roughness-filtered.

**The filter.** The combine reads the width (bilinear) and gathers the pre-convolved pyramid over a
DISK: 24 taps on a Vogel spiral rotated per pixel (interleaved gradient noise, Jimenez 2014),
Gaussian weights with FWHM = cone width, the disk cut at 2.5 σ, sampled on the mip ~2× finer than
the kernel (`log2(radius) − 1`). The taps are premultiplied (colour·confidence, confidence), so the
gathered colour is renormalized by the gathered confidence — while the FINAL blend weight is the
pixel's OWN trace confidence: a mean confidence dimmed every reflection whose kernel overlapped a
non-reflective neighbour (energy −12 % at r = 0.4, −51 % at 0.6 on the bench, fixed). The sharp
trace ↔ gather cross-fade runs from `GlossyCone/BlendStartTexels` 2 to `BlendFullTexels` **6**
(24 until Aug 2026: it kept 78 % of the sharp trace where a roughness-0.1 metal already asks for a
7-texel kernel). Per-pixel rotation only — no frame term — so a static frame is deterministic, which
the bench relies on.

**Why not the single mip lookup (v1/v2).** `textureLod(pyramid, uv, log2(width))` picks the mip whose
texel IS the kernel: a 64-128 px block whose width depends on where the reflected feature falls in
the texel grid (±30 % measured, the "pâtés"), and the pyramid's last mip (6 mips on a 720 × 405
base) capped the blur at σ ≈ 76 px where the roughness asked for 93-277.

**Measured** (projet-alpha `post-processor-effect-debug`, flush wall, 2.4 m panels, kernel σ in px at
2880 × 1620; GGX row = the formula's own expectation):

| r = | 0.1 | 0.2 | 0.3 | 0.4 |
|---|---|---|---|---|
| v1 (uniform cone), emitter 6 m | 0 | 8 | 27 | 52 |
| v3, emitter 6 m | 5.5 | 22.7 | 50.4 | 84.9 |
| GGX, 6 m | 6 | 23 | 53 | 93 |
| v3, emitter 16 m | 7.4 | 29.1 | 57* | 90* |
| GGX, 16 m | 8 | 31 | 70 | 124 |

`*` = the kernel leaves the 2.4 m panel (energy −8/−16 %). Far/near ratio 1.35 / 1.28 at r = 0.1 /
0.2 (physics 1.33). Energy conserved ≤ 2.4 % up to r = 0.3. Residual grain 1.1-1.6 / 255 with 24 taps
(3.0 with 16, 4.4 with 16 on a 4×-finer mip); combine pass 0.22 → 0.38 ms at 2880 × 1620.

- ⚠️ One hit distance per pixel cannot represent a depth discontinuity: a ray next to a floating
  bright band hits the far wall and sizes its cone from THAT distance, pulling the band's energy
  with a wide kernel (bench option 3 = 1 measures it). Inherent; a min-hitT neighbourhood or
  per-sample hit distances would be the next step if it shows in content.
- ⚠️ A flat reflector is assumed. A curved one compresses its reflected image; the proper term is
  the normal's screen-space derivative — not done.
- ⚠️ `RayTracing/Reflection/Enabled` was REMOVED (Aug 2026); ray tracing is a global switch. To
  compare against no reflections, leave the `Reflections` slot EMPTY in a test stack. Disabling ray
  tracing does not remove the reflections: the demos fall back to SSR, which shimmers (half-res,
  2-texel blur, rays leaving the screen) — compare RTR against RTR, never against the fallback.
