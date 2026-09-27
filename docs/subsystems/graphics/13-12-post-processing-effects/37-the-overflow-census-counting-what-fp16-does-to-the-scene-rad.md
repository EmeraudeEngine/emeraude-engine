## 12. Post-Processing Effects

### The overflow census — counting what fp16 does to the scene radiance (Sep 2026)

Scene-colour pre-exposure, step **B1a** (engine `docs/todo/scene-colour-pre-exposure.md`). A physical
luminance above 65 504 nits cannot live in an RGBA16F target: it becomes +Inf, and the first filter
that multiplies it by 0 makes it NaN. `Graphics::OverflowCensus` (`OverflowCensus.hpp/.cpp`) is the
instrument that says **how often, where and by how much**, before anything is changed to prevent it.

- **Channels** (one compute dispatch each, `MaxChannels = 8`): `SceneColour` (the post-processor's
  grab-pass copy the chain starts from), `ToneMapInput` (the chain colour the tone mapper receives —
  or the chain output, `toneMapped = false`, when none ran), the RAW TRACES the effects declare through
  **`IndirectPostProcessEffect::radianceTargets()`** (`RTGI_Trace`, `RTR_Trace` — a passive hook the
  executor reads once an effect passed every gate, so no effect class is ever named), `ProbeIrradiance`
  (the INTERIOR texels of the probe atlas, `IrradianceProbeVolume::irradianceCensusView()`, a 2D-array
  variant of the shader; the tile borders repeat the octahedral wrap), and `SelfTest` on request.
- **The predicate is integer-only**: `worst = max(|R|,|G|,|B|)` taken on the IEEE BITS (sign cleared,
  where the bit order IS the numeric order); `> 0x7F800000` NaN, `== 0x7F800000` Inf, `>= 0x477FE000`
  (65 504, the largest finite binary16) **ceiling**, else it feeds `peakFinite`. No `isnan()`/`isinf()`
  and no float compare: a fast-math compiler (Metal) may fold them. Alpha is never read.
- **ONE batch per frame**, in the closing phase of `PostProcessor::executeIndirectPostProcessEffects()`,
  right before the first effect whose slot is `>= EffectSlot::ToneMapping` (after the overlay group has
  been flushed), or after the final flush when no such effect runs. Every counted image is final there.
  Only on the HDR chain (`m_cachedRequiresHDR`): an 8-bit grab cannot overflow. The two halves of a cut
  frame share `Renderer::renderedFrameSerial()`, so `prepareFrame()` is idempotent per serial and the
  RTGI trace noted by the pre-translucency half is still counted by the closing batch.
- **Buffer layout**: per frame in flight, a device-local counter buffer of `1 + MaxChannels` regions at
  the storage offset alignment — region 0 a `uint64` **serial header** (`vkCmdUpdateBuffer`), region
  c + 1 channel c — and **one descriptor set per channel whose storage binding covers ONLY its own
  region**. Consecutive dispatches therefore touch disjoint ranges and need no barrier between them
  (atomics are writes to synchronization validation). Barriers: one before the dispatches (CAO |
  FRAGMENT | TRANSFER | COMPUTE → COMPUTE, which also chains the IRTs' final-layout transition), one
  after (COMPUTE | TRANSFER → TRANSFER | FRAGMENT | CAO: the copy, and the next WRITERS of every counted image),
  then the copy to a persistently mapped readback and a **TRANSFER → HOST** buffer barrier.
- **Harvest** in `Renderer::beginFrame()` right after the slot's fence (`PostProcessor::onFrameSlotRetired()`,
  beside the frame capture's), whether or not the chain runs that frame. A header that does not match
  the recorded serial is a batch recorded but never executed: `staleSlots`, never counted as that frame.
- **Positive control**: a 16×16 RGBA16F image of raw half bit patterns (qNaN, −qNaN, a NaN with the
  smallest payload, NaN+Inf in one texel, ±Inf, ±65 504, alpha-only NaN, 65 472, subnormals, integers),
  uploaded once by a buffer copy (no float conversion on the way). `runSelfTest()` counts it in the next
  batch; the expected tuple is **tested 256, NaN 21, Inf 12, ceiling 8, peak 65 472** (bits
  `0x477FC000`). ⚠️ A count read on a machine means something only once the self-test answered PASS
  there — any other tuple means the census is blind on that platform.
- **Lifetime**: created ONCE at the first `PostProcessor::configure()` — ⚠️ NOT in `onInitialize()`,
  which runs from `Renderer::initializeSubServices()` before the frame scopes exist
  (`framesInFlight()` is 0 there). **Disarmed by default** (`Core/Graphics/PostProcessing/OverflowCensus/Enabled`,
  read once; `Core.RendererService.setOverflowCensus(1|0)` live). Disarmed: no dispatch, no barrier, one
  branch per chain call. A creation failure is traced once and never retried.
- **Publication**: the render thread composes a `FrameDiagnostics` (`FrameDiagnostics.hpp`, plain data,
  no export macro, names COPIED) once per rendered frame after the frame's recording
  (`Renderer::publishFrameDiagnostics()`: the census snapshot + the camera tone mapper's metering) and
  copies it into the renderer's board and into the scene stack's, each behind a mutex. The console
  reads only those copies: `Core.RendererService.getFrameDiagnostics()` (JSON) and the `Metering:` /
  `Overflow census:` blocks of `Core.SceneManagerService.PostProcess.getStatus()`.
- ⚠️ **`SceneColour` vs `ToneMapInput`: the gap IS the SSR/TAA guards.** Both scrub non-finite values
  before the tone mapper (`TAA.cpp`, `SSR.cpp`), so `ToneMapInput = 0` does not mean the scene colour
  was clean.
- ⚠️ **`ceiling` is not `Inf`**: a GPU may round an overflowing conversion to the largest finite value
  instead of +Inf; the two are counted apart at no extra cost.
- ⚠️ **The linear-blit caveat**: `SceneColour` is the grab copy, made with `vkCmdBlitImage(...,
  VK_FILTER_LINEAR)`. At a 1:1 scale a driver may still weight a neighbour by 0, and 0 × Inf = NaN, so
  it may count NaN around a texel that is Inf in the scene target (not verified on hardware). It is
  still what the chain itself reads.
- **Cost** (computed, not measured — measure it with the GPU profiler's `OverflowCensus` zone inside
  `PostFXChain`): 8 B read per counted texel, ≈ 95 MB per frame at 2880×1620 with half-resolution
  traces, ≈ 0.2-0.3 ms on an RTX 3070 Ti, serial before the tone map.
