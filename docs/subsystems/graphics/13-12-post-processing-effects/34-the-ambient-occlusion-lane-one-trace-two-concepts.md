## 12. Post-Processing Effects

### The ambient-occlusion LANE — one trace, two concepts (Sep 2026)

> [!CAUTION]
> **RTAO no longer casts rays when an indirect-diffuse producer is enabled: it READS the
> occlusion the producer reduced in its own ray loop.** Measured on `sponza` (2880×1620,
> RTX 3070 Ti, validation ON, 0 VUID): `RTAOEffect/trace` **7.22 → 0.06 ms** (×118),
> `RTGIEffect/internal` 43.45 → 43.77 ms — inside the 43.45–43.96 envelope of five runs, so the
> lane costs **nothing measurable** — and the whole frame **69.14 → 62.77 ms, −9.2 %**.

**Why it is free.** Both effects sample a **cosine-weighted hemisphere** at the **same** working
resolution with the same sample count, and the GI ray already commits a hit distance
(`RTGI.cpp`, `rayQueryGetIntersectionTEXT`). The occlusion is therefore a reduction of data the
producer already holds: one add and one clamp per sample, written into the **ALPHA lane** of its
half-res RGBA16F trace target.

⚠️ **That lane was verified free, not assumed free.** The producer used to write a constant `1.0`
there, which proves nothing about readers. Every consumer of that target was read: `GIDenoiser`
samples `giTex`/`rawTex` as `.rgb` only. The `.a` uses inside the denoiser belong to the HISTORY
and MOMENTS textures (reprojection distance, variance), not to the trace.

**The derived term is the MORE accurate of the two.** RTAO traces with
`gl_RayFlagsTerminateOnFirstHitEXT`, so its hit distance is *a* hit inside its range; the GI ray
commits the **closest** hit over the whole sky distance. Same occlusion set (`closest < D` ⟺
`∃ hit < D`), exact distance weight. Measured direction: the derived AO is darker on 35.9 % of
pixels, brighter on 13.1 % (signed mean −0.68/255) — slightly MORE occlusion, which is what a
larger distance weight gives.

**The protocol** (`IndirectPostProcessEffect`, six virtuals) is a **producer/consumer pairing
between slots**, wired by `PostProcessStack::syncSlotPairings()` — called from `Renderer` every
frame, right after `syncCameraEffects()`:

| Side | Members |
|---|---|
| Producer | `providesOcclusionLane()`, `setOcclusionLaneEnabled(bool, float)`, `occlusionLaneTexture()` |
| Consumer | `consumesOcclusionLane()`, `occlusionMaxDistance()`, `setOcclusionLaneSource(const TextureInterface *)` |

- **Bidirectional on purpose**: the producer cannot reduce an occlusion without the consumer's
  range, so the stack reads `occlusionMaxDistance()` off the consumer and pushes it into the
  producer. ONE value carries the whole state — a range of `0` means *disarmed*, and the shader
  then publishes the neutral `1.0`. There is no second flag to keep consistent.
- **The pairing runs unconditionally, every frame, and DISARMS both sides when it cannot hold.**
  Not on a change event: what it mirrors (which occupant of a slot is enabled, whether it is
  created) changes without passing through `addEffect()`/`removeEffect()`. ⚠️ The disarm is the
  load-bearing half — it is what forbids the consumer's borrowed texture pointer from outliving
  the producer that owns it.
- **The consumer keeps a fully working standalone path**, selected by `m_occlusionLaneSource ==
  nullptr`: no producer, a disabled one, an un-created one, or a sibling that publishes no lane
  (SSGI has no closest-hit distance to offer). **Verified by measurement**, not by reading: with
  the pairing forced to fail, `RTAOEffect/trace` returns to 7.37 ms and the run stays at 0 VUID.
- **Two pipelines, ONE layout and ONE per-frame descriptor set.** The traced and derived fragment
  shaders write the same `vec2(ao, depth)` into the same target, so the bilateral kernel, the
  blur targets and the combine snippet downstream are **untouched** and a variant switch is a
  pipeline bind and nothing else. The lane binding (set 1, binding 2) exists in both and is
  written every frame — with the depth texture standing in when there is no lane — so the set is
  never left with an unwritten binding; the traced shader does not declare that sampler.
- **The consumer resamples on NORMALIZED uv**, never `texelFetch` on `gl_FragCoord`: the
  producer's working resolution comes from ITS own pixel-doubling setting and is not required to
  match. Equal extents (the default) make the linear filter return the exact texel; unequal
  extents degrade to a bilinear resample instead of reading the wrong pixel.

⚠️ **Two behaviour deltas to know about:**
- **`Core/Graphics/PostProcessing/AmbientOcclusion/RayTracing/SampleCount` is INERT while the pairing holds** —
  the derived term is reduced over the PRODUCER's sample count. `MaxDistance`, `Intensity`,
  `BlurRadius`, `NormalSigma` and the material `aoResponse` nibble all keep working.
- **The origin offset becomes the producer's** (GI bias 0.02 against AO bias 0.005), so very
  tight creases are marginally less occluded. Not separately measurable on `sponza` above the
  scene's own noise floor.

⚠️ **On a CUT frame the lane describes the OPAQUE G-buffer only.** The indirect diffuse is a
pre-translucency slot and the ambient occlusion is not, so on a scene holding grab-pass
materials the producer runs in the first half and the consumer in the second: the derived
occlusion is the one measured before the TranslucentGB pass rewrote the G-buffer under the
glass. The traced path had the post-translucency depth. Not exercised by `sponza` (no
TranslucentGB object). **OPEN**, and the reason `isPreTranslucencySlot()` is worth re-reading
before adding a second consumer.
