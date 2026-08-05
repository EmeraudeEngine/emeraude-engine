# Bugs and TODO-list

> Open work only. Completed items are removed once their knowledge lives in `docs/` or the
> `AGENTS.md` network — measurements, traps and owner decisions belong there, not here.

- GENERAL: Remove all invalid noexcept keyword. (WIP)
- GENERAL: Increase inlining. (WIP)
- GENERAL: Improve functions args to use "std::move" when useful. (WIP)
- GENERAL: Rewrite libs Observer/Observable pattern with the idea of static and shared objects.
- GENERAL: Replace all "std::stringstream" by "std::format" (C++20) for simple keys, names or identifiers creation. WARNING: This doesn't work under macOS for targeting older SDKs.
- GENERAL: Issue on Linux with X11, multi-monitors and NVIDIA proprietary driver. More info: https://forums.developer.nvidia.com/t/external-monitor-freezes-when-using-dedicated-gpu/265406
- RENDERING SYSTEM: Check sprite texture clamping to edges.
- RENDERING SYSTEM: GPU profiler V2 — cover the shadow map and render-to-texture passes.
  They are submitted through SEPARATE command buffers before the main one, so the V1
  single-pool-per-frame reset (recorded at the top of the main command buffer) would wipe
  their timestamps on the GPU timeline. Needs one query range (or pool) per submission.
  V1 (`Vulkan::GPUProfiler`, main command buffer only) is documented in
  `docs/ai-runtime-control.md` §6 and `src/Vulkan/AGENTS.md`.
- RENDERING SYSTEM: Remove the dead camera velocity vector from the view UBOs — uploaded
  every frame (`ViewMatrices*UBO::updateViewCoordinates()`, `VelocityVectorOffset`) and
  declared in the generated GLSL view blocks, but read by NO shader. The velocity parameter
  itself stays (it feeds the OpenAL listener/doppler on the audio side of the AVConsole
  contract). Motion vectors do NOT use it (they need the previous view-projection matrix,
  not a linear velocity).                                                       
- PHYSICS SYSTEM: Enable the rotational physics. (WIP)
- PHYSICS SYSTEM: Create a particle system using Compute Shader.
- RESOURCES SYSTEM: Merge Font from PixelFactory and FontResource.
- RESOURCES SYSTEM: Check the direct data description with the JSON resource description.
- RESOURCES SYSTEM: Check the store resource addition from the JSON resource description. (WIP)
- OVERLAY SYSTEM: Rework ComposedSurface from overlay to create a native menu.
- OVERLAY SYSTEM: Rewrite the TextWriter class.
- ANIMATION SYSTEM: Check all animatable properties for all objects.
- ANIMATION SYSTEM: Root-motion mode — extract root delta per frame from skeletal clips and feed it back to the actor as actual displacement (foot-planting, no foot-sliding, animation-driven speed). Companion to `LoaderOptions::stripRootMotion` (which kills horizontal root translation at load); the new mode keeps it and routes it through `MovableTrait`. Industry-standard locomotion. Required for production-grade runtime quality on humanoid characters.
- CONSOLE SYSTEM: Bring back a useful console behavior.
- LIGHTING AND SHADOWING: **CSM directional lights do not light their receivers — OPEN but PARKED
  BY OWNER DECISION (Jul 2026).** Three defects were fixed; a fourth remains (the CSM block's
  colour / direction / intensity are written only inside `DirectionalLight::updateCascades()`).
  The classic constructor is the right tool for the scenes at hand, so the investigation was
  closed deliberately: **do not relaunch the light-UBO host readback without a new reason.**
  Consequence to respect: prefer the classic constructor; `lighten-marbles` and `basic-scenery`
  still use CSM but at 0.5 lx moonlight, where the loss is invisible. Full defect table in
  `docs/shadow-mapping.md` (CSM status section).
- LIGHTING AND SHADOWING: the **PBR low-quality specular approximation** (`lqSpecPower` in
  `LightGenerator.cpp`) is still unnormalised and still multiplies the raw illuminance, reusing the
  raw `N.L` `finaleDiffuseFactor` inside its own `pow()`. Same treatment as the legacy specular
  (done, see `docs/caution-points.md` § "the legacy specular was not energy-normalised"), smaller
  blast radius — LQ path only, `Core/Graphics/Shader/EnableHighQuality` false.
- GRAPHICS MATERIAL: **`Reflection: { "Type": "Automatic" }` does not create the component** (Jul 2026,
  observed, STILL PRESENT Aug 2026). `setReflectionComponentFromEnvironmentCubemap()` raises
  `m_isUsingEnvironmentCubemap` and then calls `setReflectionAmount()`, which warns
  `The material 'X' has no reflection component !` because no `ComponentType::Reflection` is ever
  emplaced (verified in both `StandardResource` and `PBRResource`). Either the bindless path should
  stop going through `setReflectionAmount()`, or the warning is wrong — as it stands the log accuses
  materials that declared reflection correctly, which already sent one session chasing a false lead.
- LIGHTING AND SHADOWING: Fix the ambient light update against the render target which uses it.
- LIGHTING AND SHADOWING: Re-enable the ambient light color generated by the averaging active light color.
- LIGHTING AND SHADOWING: Check the ambient light color generated by a texture.
- LIGHTING AND SHADOWING: Shadow maps: Create a re-usable shadow map for ephemere lights.
- SHADERS CODE GENERATION: Check source and binary caches.
- SHADERS CODE GENERATION: Prepare a way to use manual GLSL sources.
- SHADERS CODE GENERATION: Re-enable normal calculation bypass when the surface is not facing a light.
- MATERIAL: Create a material editor in JavaScript (application side). EDIT: Should be a tool for the engine.
- VULKAN: Find a better way to detect the UBO max capacity. For now the limit is hard-coded to 65,536 bytes.
- VULKAN: Implement VK_KHR_synchronization2 and VK_KHR_dynamic_rendering (Vulkan 1.3), then leverage dynamic rendering to order draws by pipeline layout and reduce binding cost.
- VULKAN: Extend SharedUniformBuffer pooling strategy to short-lived entities (particles, projectiles) for UBO/VBO allocation optimization.
- **RAY TRACING: intermittent DEVICE_LOST in the `game-logic` demo — DID NOT REPRODUCE, 0/8
  (re-tested 2026-08-04, RTX 3070 Ti, Release built from `develop` at `3225123c`).**
  Filed 2026-07-26 at **2/8 runs** with `RayTracing/Enabled` true: `device_fault` reported a
  `READ_INVALID` at a low address plus an `INSTRUCTION_POINTER_FAULT`, with the last per-queue GPU
  markers a MIX of `AS-build:end` and `transfer:image-layout-transition` — the signature of BLAS
  builds racing uploads across the round-robined transfer queues. Four plausible fixes landed since
  (`Device::waitTransferQueuesIdle()`, the `DeferredDestructor`, the one-shot queue-family ownership
  fix `ebae3d4c`, the shared-UBO registry race `e8d63525`).
  ⚠️ **0/8 is NOT proof of a fix.** At the original 25% rate, eight clean runs happen by chance
  about 10% of the time. Left open deliberately; **16 consecutive clean runs would put that at ~1%**
  and justify closing it. Do NOT re-derive the protocol — it is below.
  Repro: `cd .claude-build-release/Release && for i in $(seq 1 8); do timeout 40
  ./projet-alpha --load-demo game-logic --disable-cef > /tmp/gl_$i.log 2>&1; grep -c DEVICE_LOST
  /tmp/gl_$i.log; done`.
  ⚠️ Two false positives to skip when reading those logs: `device_fault` matches the startup line
  `VK_EXT_device_fault detected and enabled`, so grep the FAULT REPORT, not the extension name; and
  `[Error][UIManagerService] No default page found !` appears in every demo, including the ones that
  never fail.
- ⚠️ **WAYLAND: a compositor protocol error kills the surface mid-load (measured 1/8, 2026-08-04).**
  Found while re-testing the entry above, and it is a DIFFERENT defect — no device loss, no GPU
  fault. One run in eight logged
  `wp_linux_drm_syncobj_surface_v1#94: error 3: Release or Acquire point set but no buffer attached`,
  then `VK_ERROR_SURFACE_LOST_KHR` at present, then a clean `User exit code: 0` shutdown (2561
  frames rendered, so it survived a while before dying). The engine shuts down gracefully, so the
  symptom reads as "the demo closed by itself".
  TIMING IS THE LEAD: the error fires inside the **RT skinned-geometry creation burst** (Fox +
  Paladin BLAS mirrors, ~4 MB of mirror buffers allocated back to back) — the same "heavy load
  starves the Wayland dialogue" family already suspected elsewhere. `explicit sync` is the
  protocol involved (`wp_linux_drm_syncobj`): the engine attaches an acquire/release point without a
  buffer, which the compositor treats as a protocol violation and kills the surface for.
  Next step: audit the swap-chain present path for a present submitted with a sync point but no
  attached buffer, most likely on a frame that raced a swap-chain recreation or a stall.
- RENDERING SYSTEM: Hi-Z Occlusion (see the GPU-driven roadmap below — the SSR Hi-Z pyramid
  already exists, occlusion culling does not).
- RENDERING SYSTEM: GPU Frustum Culling — Move frustum culling to a compute shader for scalability with high instance counts.
- RENDERING SYSTEM: Indirect Draw / Draw Call Batching — Use vkCmdDrawIndexedIndirect to batch draws by pipeline/material, reducing per-draw CPU overhead.

## Post-Processing Pipeline (Effects/Framebuffer + Effects/Lens)

- [ ] **SMAA (Subpixel Morphological Anti-Aliasing)** — Anti-aliasing post-process morphologique (complément au FXAA existant).
- [ ] **Re-light the demos that never migrated to the physical camera** (last phase of the
  photometric project — the units, the attenuation, the APEX exposure and the effect
  recalibration are all done; see `src/Graphics/AGENTS.md` § "Light Attenuation — Physical, Not
  Artistic", § "Physical Camera" and § "Background photometric contract").
  `Liminal` and `Citadel` add `ToneMapping` BY HAND with fixed parameters, so they have NO
  metering to absorb photometric values and read blown out (`liminal` measured 200.4 mean, 5.8%
  blown at f/4 1/60; `Citadel` reads ~0 nits). Their lamps and their optics have to be tuned
  against each other. This is AUTHORING, not a defect.
  ⚠️ Method: keep the auto-exposure ON while migrating a scene — absolute values span ~1 to
  ~100000 and without metering every intermediate step renders pure white or pure black.
- [ ] **Velocity on the translucent pass** (motion-vector defect 3) — the B1-B4 chain is done and
  pushed (per-instance transforms SSBO, previous-model history, RG16F velocity attachment, double
  skinning; see `src/Saphir/AGENTS.md` § "InstanceTransforms SSBO Path" and `src/Scenes/AGENTS.md`
  § "Instance Transforms"). The translucent pass still writes a wrong velocity — same family as
  the two static-camera bugs already closed.
  ⚠️ **Keep a static-camera zero-velocity check in any future motion-vector work.** The original
  B1-B4 validation only ever exercised MOVING geometry, where a constant offset is invisible next
  to real motion, and RTGI's own history validation masked the error for a day.
- [ ] **Push constant min-spec violation — observed at 144 B (owner, 2026-07-30), still open.**
  `Saphir/Generator/Abstract.cpp::declareMatrixPushConstantBlock()` still emits a
  `V(64) + M(64) + frameIndex(4)` = 132 B fallback for the advanced path when a scene has no
  instance transforms, and `generatePushConstantRanges()` lays blocks END TO END — so a second
  declared block ADDS to that offset, which is how the engine's own startup validation now reports
  144 B against the 128 B Vulkan minimum guarantee (part of the AMD/Intel fleet exposes exactly
  128). The validation itself is DONE (`Vulkan::PipelineLayout::createOnHardware()`: hard error
  above the device limit, warning above 128 B) — read the warning to identify which program and
  which second block push it over, then eliminate the fallback.
  See `docs/caution-points.md` § "Push Constants: the 128-Byte Minimum Guarantee".
- [ ] **Physical camera follow-ups** — (a) console commands for the camera optics
  (`prévoir les possibilités`: the setters exist, the bindings don't); (b) focal length
  → FOV coupling as an opt-in physical mode (sensorWidth); (c) bokeh aperture blades
  (polygonal sample distribution in the DoF gather); (d) more presets (the LensPresets
  catalog — GoldenHour, Analog80s... — can each become a full camera preset).

### GI/AO follow-ups

- [ ] **World-space GI cache** — the multi-bounce feedback is SCREEN-SPACE only: no propagation
  around corners that are never co-visible. The full fix is a world-space cache (probes /
  surface cache), budget-gated. See `src/Graphics/AGENTS.md` § "RTGI — Temporal + Multi-Bounce".

## Rendering

### Post-device-loss robustness (observed 2026-08-04, macOS)

After a `VK_ERROR_DEVICE_LOST` (probe self-sampling GPU fault on Apple M2, since fixed at the
trigger — see `docs/caution-points.md` § "Probe self-sampling"), the engine kept limping: every
later fence reset was rejected ("the fence must be destroyed" per the validation layer), every
IRT/effect/grab-pass creation failed in cascade, and the process eventually segfaulted instead
of failing stopped. A lost device invalidates fences, command buffers and every downstream
object; the services keep using them. Options to evaluate: a device-lost flag checked by the
service layer (fail-stop with diagnostics dump), or full device recreation. Low urgency — the
known triggers are fixed — but any future GPU fault will end in the same undignified crash.

## Current State (v0.9.52)

The renderer has a solid foundation:
- **State sorting** via 64-bit composite key (pipeline > material > geometry > distance)
- **Multi-Draw Indirect (MDI)** with Buffer Device Address (BDA) for per-draw data
- **Dual render strategy**: direct swap-chain path and HDR internal target (float16) path
- **Triple buffering** with double-buffered SSBO/indirect buffers per frame-in-flight
- **TLAS deferred recording** for ray-tracing acceleration structure builds
- **Post-processing pipeline** with indirect (multi-pass) and direct (in-RP) effect execution
- **Per-instance transforms SSBO** (`Scenes::SceneInstanceTransforms`) with motion history
- **Bindless texture table** (`BindlessTextureManager` + per-scene `BindlessTextureSet`)

### Roadmap toward UE5-class runtime

#### 1. GPU-Driven Rendering (highest impact)

Move culling and draw submission from CPU to GPU.

- Replace CPU frustum cull + `vkCmdDrawIndexedIndirect()` with:
    - GPU compute shader performing frustum culling
    - `vkCmdDrawIndexedIndirectCount()` where GPU decides draw count
- CPU uploads entire scene to persistent SSBOs, no longer rebuilds render list per-frame
- Foundation for Nanite-class geometry handling

#### 2. Bindless Textures — FOUNDATION DONE, the PAYOFF is not taken yet

The global table exists and is multi-scene safe (`BindlessTextureManager`, per-scene
`BindlessTextureSet`; see `src/Graphics/AGENTS.md`). What remains is USING it to stop treating
material as a batch-breaking criterion:

- Per-draw material index stored in the transforms SSBO alongside the model matrix
- Batch key reduced from `(pipeline, material, geometry)` to `(pipeline, geometry)`
- Dramatically larger MDI batches

#### 3. GPU Occlusion Culling (Hi-Z)

Reject invisible geometry before draw submission. A Hi-Z depth pyramid already exists for SSR
ray marching (`Effects/Framebuffer/SSR`), but nothing culls with it.

- Downsample previous frame depth into Hi-Z mipmap pyramid
- GPU compute tests each bounding box against Hi-Z
- Integrates into the GPU culling compute shader from step 1
- Critical for dense urban/interior scenes

#### 4. Render Graph

Automate resource management and barrier placement.

- Automatic render target lifetime management (transient allocations)
- Automatic `vkCmdPipelineBarrier` placement between passes
- Pass reordering and merging opportunities
- Scales cleanly as passes multiply (SSAO, SSR, bloom, volumetrics, motion blur...)

#### 5. Order-Independent Transparency (OIT)

Replace sorted per-object translucency with a robust algorithm.

- Weighted Blended OIT or Moment-Based OIT
- Eliminates sorting artifacts for overlapping transparents
- Better handling of particles and vegetation