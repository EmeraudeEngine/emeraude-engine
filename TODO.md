# Bugs and TODO-list

> Open work only. Completed items are removed once their knowledge lives in `docs/` or the
> `AGENTS.md` network — measurements, traps and owner decisions belong there, not here.

## Y-UP RESIDUES — sites the Aug 2026 Y-up flip missed (found 2026-08-24)

Three sites still carry the **retired Y-DOWN** convention in their *behaviour*, not just in their
prose. Each was found during the documentation sweep of the Y-up migration and is **deliberately not
fixed yet** (owner decision): each needs a check the compiler and the unit suite cannot perform.
Every site carries a `🔴 KNOWN DEFECT — OPEN` comment pointing back here.

⚠️ **None of the three is visible to `-Werror` or to the emeraude-base unit suite.** Two of them are
invisible to a screenshot as well. Do not close any of them on "it compiles and the tests pass".

- [ ] **AUDIO — listener UP vector inverted.** `Audio/HardwareOutput.cpp`,
  `updateDeviceFromCoordinates()` hands OpenAL's `AL_ORIENTATION` the `downwardVector()` where the
  API wants a genuine UP vector. It was a Y-down compensation; the world is Y-up now, so it must read
  `upwardVector()`. **The vertical axis of the audio field is inverted as it stands.**
  ⚠️ Invisible to every visual check. **Verification:** play a positional sound ABOVE the listener and
  confirm which way it comes from — a one-line change is worthless without that check.

- [ ] **ATMOSPHERIC FOG — height falloff inverted.**
  `Graphics/Effects/Framebuffer/AtmosphericFog.cpp` computes `exp(k * (y - baseHeight))` with
  `Parameters::heightFalloff` defaulting to `+0.2F`. Under Y-down, `+Y` meant deeper, so density grew
  downward; under Y-up the same expression makes fog **denser with ALTITUDE**. Fix is to negate `k` at
  the use site **or** to flip the parameter's documented sign convention — that choice is an API
  decision, which is why it is not applied unilaterally.
  ⚠️ The **ray-reconstruction** half of the same shader is SOUND and needs no edit: it rides the
  signed `tanHalfFovY` contract and followed the flip on its own. Do not "fix" it too.
  **Verification:** a fog demo, camera below then above `baseHeight`.

- [ ] **MD5 ANIMATION — a FIFTH conversion site, missed by the migration.** (emeraude-base)
  `src/Animation/MD5AnimParser.hpp` still converts with `(md5.y, -md5.z, md5.x)`, a **REFLECTION**
  (det -1), in `md5ToEnginePosition()` **and** in `md5ToEngineQuaternion()`. The MD5 **mesh** path,
  `src/VertexFactory/FileFormatMDx.hpp`, was migrated to `(md5.y, md5.z, md5.x)`, a **ROTATION**
  (det +1). The two now disagree, so a clip parsed here does not live in the same frame as the mesh it
  animates — exactly the failure `FileFormatMDx.hpp` warns about in its own comment.
  ⚠️ The migration notes recorded FOUR sites for this conversion; this is a fifth, in a different
  module. It **is** reachable: projet-alpha's `animation-debug` demo calls it
  (`src/Builtin/AnimationDebug.cpp`).
  ⚠️ The two helpers move **together or not at all** — converting positions and leaving orientations
  on the old mirror is what renders a skinned MD5 upside down.
  **Verification:** visual, per format — `geometry-loader --demo-options 6` (MD5) with a clip applied.

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
- RENDERING SYSTEM: **the quality tier needs a DISTANCE driver.** `EnableHighQuality` is gone as
  a user setting (Aug 2026); the tier is now a rendering decision carried by
  `Saphir::Generator::Abstract`'s `HighQualityEnabled` flag, and `SceneRendering` enables it
  unconditionally — every program is generated at FULL quality today. What it gates:
  Fresnel-gated reflection, thin-surface transmission with Fresnel, and parallax occlusion
  mapping. Owner's intent: drive it from rendering DISTANCE (a distant surface takes the cheap
  branches). The flag is already part of the program cache key, so a distance switch produces
  its own program variants for free.
  (The former "PBR low-quality specular approximation" item is void: `lqSpecPower` lived in
  `generateFinalFragmentOutput()`, which was written for the Gouraud path and was deleted with
  it — it had no caller left.)
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
- RENDERING SYSTEM: **texture cache + BC7 compressor — converted to sub-services and the cache key
  FIXED (Aug 2026). Nothing left to do here; the one open consequence is folded into the pruning
  point of the shader-caches item below.**
  `Graphics::TextureCompressor` (ClassId `"TextureCompressorService"`) and `Graphics::TextureCache`
  (ClassId `"TextureCacheService"`) were "a grouping of statics" — the shape the owner does not want
  in the engine, after the earlier pass removing needless singleton logic. Both now derive from
  `EmEn::ServiceInterface`, are value members of `Graphics::Renderer`, are enrolled by
  `Renderer::initializeSubServices()` into `m_subServicesEnabled` and are terminated in reverse
  order with every other sub-service. The compressor is initialised BEFORE the cache, because the
  cache holds a reference to it. Access from outside: `renderer.textureCompressor()` and
  `renderer.textureCache()`, both returning a CONST reference.
  ALL MUTABLE STATIC STATE IS GONE. `TextureCompressor::s_initialized`: the one-time
  `bc7enc_compress_block_init()` moved into `onInitialize()`, so a caller can no longer reach a
  compression method before the encoder is ready, and the old static `initialize()` the caller had
  to remember is DELETED (forgetting it used to produce only a runtime error log).
  `TextureCache::s_cacheDirectory` and `s_initialized`: replaced by the `m_cacheDirectory` member
  and the base class `usable()` state. The pure private helpers `generateMip()` and
  `compressLevel()` moved to an anonymous namespace in `TextureCompressor.cpp`; what remains static
  in the headers is only `static constexpr` constants and `compressedSize()`, pure arithmetic on
  its arguments.
  The service now owns the whole BC7 path: `TextureCache::getOrCompress(resourceName, pixmap,
  maxMipLevels)` does the disk lookup, compresses through the `TextureCompressor` sub-service on a
  miss and stores the result — callers no longer orchestrate try/compress/store. Two call sites
  migrated: `Texture2D::createFromPixelData()`, and the DEFAULT-resource
  `CompressedImageResource::load()` overload, which compresses its generated fallback pixmap with
  `compressSingle` through `renderer.textureCompressor()` — the KTX2 payload path
  (`load(std::span< const std::byte >)` → `KTX2Decoder::decodeCompressed()`) is untouched and still
  reaches neither bc7enc nor the texture cache.
  ⚠️ **The cache key was BROKEN and is fixed.** It was `SHA256(resourceName | sourceFileSize |
  sourceModTime)`, but the caller passed the DECODED pixel byte count as "sourceFileSize" and
  `width * 1000000 + height` as "sourceModTime" — the key reduced to name + dimensions, so
  repainting a texture without changing its size served the stale BC7 blob forever, while the class
  documentation claimed file size and modification time. It is now FNV-1a (`Base::Hash::FNV1a`)
  over the DECODED PIXELS, folded with width, height and colorCount: correct by construction, and
  it needs no plumbing through `ResourceTrait`. File format `Version` went 1 → 2.
  ⚠️ **Upgrade consequence, for whoever reads a cache directory:** entries written by an earlier
  engine are ORPHANED — still on disk in `~/.cache/<app>/texture-cache/` (`.bc7cache` extension),
  never looked up again. `--clear-renderer-cache` (the owner's rename of `--clear-shader-cache`)
  removes them — 40 measured on this machine — because `TextureCache::onInitialize()` now honours
  that switch too, which is why it clears the three on-disk renderer caches. AUTOMATIC pruning
  stays open, tracked once in the shader-caches item below.
  ⚠️ **The thread-pool parameter was DEAD**: `compressLevel()` received a `Base::ThreadPool` and
  never used it. Compression is sequential per texture; the parallelism comes from the resource
  manager loading several textures concurrently on different workers. `compress()` and
  `compressSingle()` no longer take a thread pool — any document claiming compression is
  "parallelized across blocks using the engine ThreadPool" is FALSE and must be corrected.
  MEASURED (`material-debug`, all 10 options, RTX 3070 Ti, Release): cold cache = 231 mip-level
  compressions and 7 705 ms of BC7 compression, warm cache = 0 compressions, 0 ms. The texture
  cache is therefore worth ~7.7 s of load time — more than the `VkPipelineCache` (5 702 ms → 31 ms)
  and about twenty times the SPIR-V binary cache (393 ms → 10.3 ms). Zero compressions on the warm
  run also proves the content-addressed key is deterministic: every texture found its entry. Build
  -Werror clean, 1967/1967 emeraude-base unit tests pass.
  STILL TRUE, do not re-derive: `Texture2D` reaches BC7 by two distinct paths and which one runs
  depends on the SOURCE, not a setting — (a) from an `ImageResource` (PNG, JPEG, procedural), CPU
  BC7 encode at load time, cached on disk; (b) from a `CompressedImageResource` (KTX2 /
  `KHR_texture_basisu`), the mip chain arrives already block-compressed, is uploaded verbatim and
  touches neither bc7enc nor the texture cache. Format is `VK_FORMAT_BC7_SRGB_BLOCK` or
  `VK_FORMAT_BC7_UNORM_BLOCK`, chosen by the sRGB flag.
- SHADERS CODE GENERATION: **shader caches — audited Aug 2026, one item left.**
  The three stages are settled and measured; only the housekeeping is open.
  The SPIR-V binary cache is validated by an application header (magic, format version, source
  hash, shader stage, data size, FNV-1a content hash and a TOOLCHAIN IDENTITY hash = glslang
  version + SPIR-V generator version + client/target environment pair + engine version), all
  checked before any byte reaches `vkCreateShaderModule`, plus atomic write-then-rename — and it
  is now **ON by default** (`DefaultBinaryCacheEnabled`, Aug 2026). Measured on `material-debug`
  with all 10 options, 232 shader modules: 393 ms OFF → 10.3 ms warm (38×, 383 ms saved), and a
  cold run that WRITES the 232 blobs costs 391 ms, i.e. nothing — there is no first-launch
  penalty. The `VkPipelineCache` (owned by `Vulkan::Device`, persisted by `Graphics::Renderer`)
  takes the same demo's 294 pipelines from 5 702 ms to 31 ms with the driver's own disk cache
  disabled (182×, 7.4 MB blob). See `src/Saphir/AGENTS.md` and `src/Vulkan/AGENTS.md`
  § "VkPipelineCache".
  DONE (Aug 2026): the generated-GLSL DUMP no longer calls itself a cache. The setting is now
  `Core/Graphics/Shader/EnableSourceCodeDump` (was `EnableSourceCodeCache`, still OFF by default;
  its siblings `EnableBinaryCache` and `EnablePipelineCache` are untouched), and it writes to
  `~/.cache/<app>/generated-shaders/` (was `shader-sources/`), still one lazily created
  sub-directory per generator (SceneRendering/, ShadowCasting/, PostProcessing/,
  OverlayRendering/, GizmoRendering/, TBNSpaceRendering/). It never was a cache: nothing reads
  those files back (`AbstractShader::loadSourceCode()` has zero callers) and its key is a hash of
  the source it stores, so it structurally cannot be one — its only purpose is inspecting what the
  generators produced. It is still written BEFORE the binary-cache lookup, so a cache hit does not
  suppress it, and `readBinaryCache()` (former `readCache()`) no longer touches that directory at
  all. Verified at runtime on `material-debug` with all 10 options: 232 files land in
  `generated-shaders/`, no error logged; build is -Werror clean.
  ⚠️ **NO migration, and that is a deliberate owner decision**: Settings has no key-migration
  mechanism at all, so there is no alias, no fallback and no warning. An existing `settings.json`
  keeps the old key as dead JSON that is silently ignored, and anyone who had the dump enabled
  finds it OFF until they set the new key. Accepted because it is a debug facility defaulting to
  false.
  REMAINING:
  1. Add pruning: nothing cleans the cache directories except the `--clear-renderer-cache` option
     (which since Aug 2026 clears ALL THREE on-disk renderer caches: the SPIR-V binary cache, the
     `VkPipelineCache` and the texture cache). The engine is pre-release, so there is no deployed
     version to migrate — the gap is structural, not a backlog of user data. Two shapes of it, both
     of which pruning must cover: a directory that stops being written to (the dump was renamed
     `shader-sources/` → `generated-shaders/`), and entries whose FILENAME stops being produced
     because a content-addressed key scheme changed (the texture-cache key fix orphaned 40 entries
     here — orphaned, not invalidated: nothing ever opens them, so no header check can catch them).
     Both were cleared by passing the switch by hand, which is exactly what a mechanism should make
     unnecessary. One mechanism for every `~/.cache/<app>/` renderer directory (age / total size /
     stale format version), not one per cache.
  (Not an item: `SpvOptions::disableOptimizer` in `ShaderManager.cpp`. glslang is built here with
  `ENABLE_OPT=OFF` — `libSPIRV.a` contains zero SPIRV-Tools symbols — so the flag is SILENTLY
  IGNORED. Honouring it would mean adding SPIRV-Tools to the dependency cascade for no gain,
  desktop NVIDIA/AMD drivers re-optimising the SPIR-V they receive anyway.)
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
  starves the Wayland dialogue" family already suspected elsewhere.
  ⚠️ **ROOT-CAUSE ATTRIBUTION CORRECTED 2026-08-22 — the entry previously said "the engine attaches
  an acquire/release point without a buffer". It does not, and it cannot: THE ENGINE DOES NOT SPEAK
  EXPLICIT SYNC AT ALL.** Measured — of every object in the process's startup link map (engine,
  GLFW inside it, libdecor and its plugins, CEF), **not one references `wp_linux_drm_syncobj`**; the
  only binary that does is `libnvidia-glcore.so.610.57.04`, the NVIDIA Vulkan driver, dlopen'd at
  runtime. The acquire/release points therefore come from the **driver's WSI**, inside
  `vkQueuePresentKHR`. Auditing our present path for a sync point we never set would burn a session
  looking for absent code.
  The protocol rule being violated (`linux-drm-syncobj-v1`): a client must set the acquire and
  release points **if and only if a non-null buffer is attached in the SAME surface commit**. So the
  violation is an EXTRA `wl_surface_commit()` on the window surface, carrying the driver's pending
  sync-point state but no buffer — i.e. it comes from whoever else commits that surface. That is
  **GLFW**: its Wayland backend (3.4-108-g4263be2a here) calls `wl_surface_commit` at 11 sites and
  drives libdecor through 74 call sites (resize, fractional scale, opaque region, decoration
  updates), which is exactly why the failure is intermittent — one of those has to land between the
  driver setting its points and attaching its buffer.
  Known upstream family, same protocol, same shape, all GLFW/libdecor/explicit-sync:
  godotengine/godot#93669, kovidgoyal/kitty#7767, blender#135039 (needs a Blender-specific
  libdecor), wezterm#6699.
  Since 2026-08-22 both reporting sites (`Queue::present()`, `SwapChain::acquireNextImage()`) print
  that reading next to the error, through `vkResultDiagnosticHint()` — so a future run says on its
  own that the window died and the GPU did not.
  Next steps, in order: (1) capture the protocol trace on the owner's session (only a real
  compositor will do — X11/Xvfb has no libdecor and no explicit sync):
  `python3 tools/wayland-protocol-trace.py --capture -- ./projet-alpha --load-demo game-logic --disable-cef`
  keeps only the failing run, then `--analyse <log>` resolves the offending object to its
  `wl_surface`, prints the requests of the rejected commit and says whether it carried an attach —
  which names the committer outright. ⚠️ `WAYLAND_DEBUG` slows the client enough to hide a tight
  race: a clean sweep is a RESULT, not a failed attempt; (2) A/B the libdecor plugin —
  `LIBDECOR_PLUGIN_DIR=<dir with only libdecor-cairo.so>` — since the GTK3 plugin only started
  loading on 2026-08-22 (the symbol-interposition fix removed the `png_free` conflict that had been
  making libdecor refuse it), so the plugin in use CHANGED that day. ⚠️ The defect itself predates
  it — this entry measured it on 2026-08-04, with the cairo plugin — but nothing says the FREQUENCY
  is unchanged, and that A/B is what settles it. (3) Only then consider a GLFW-side fix/update; a
  vendored-submodule bug is fixed upstream, never worked around here.
- RENDERING SYSTEM: Hi-Z Occlusion (see the GPU-driven roadmap below — the SSR Hi-Z pyramid
  already exists, occlusion culling does not).
- RENDERING SYSTEM: GPU Frustum Culling — Move frustum culling to a compute shader for scalability with high instance counts.
- RENDERING SYSTEM: Indirect Draw / Draw Call Batching — Use vkCmdDrawIndexedIndirect to batch draws by pipeline/material, reducing per-draw CPU overhead.

## Material System Merge — StandardResource + PBRResource → ONE PBR material (DECIDED 2026-08-12)

**Owner decision.** The engine converges on a single lit material: Cook-Torrance PBR, taking the
*name and role* "Standard" (Godot-style: the standard material IS PBR). Existing materials are
CONVERTED — backward compatibility is not a goal. Grounded in a full audit (8-agent sweep,
2026-08-12; state of the art: UE4/5, Godot 4, Filament, glTF 2.0 all dropped Phong — the only
lit-cheap survivor, Unity Simple Lit, maps to Basic's tier, not Standard's).

**Approved decisions:**
- **D1 — Name/ClassId**: the merged class becomes `StandardResource` and **reuses ClassId
  `"MaterialStandardResource"`** (UID = FNV1a of that string). Payoff: the 19 mesh JSONs carrying
  that ClassId stay valid without edits; only 3 data files need fixing (see Lot 5).
- **D2 — Artistic `Amount` on Reflection/Refraction**: ported into the merged material as an
  optional override (Standard's artistic mix; PBR currently hard-codes 1.0).
- **D3 — Ambient component**: dropped (AO + IBL replace the concept in PBR).
- **D4 — Canonical legacy→PBR conversion at the parse boundary** (Khronos archived spec-gloss
  extension): `cdiff = diffuse × (1 − max(spec))`, F0 → `KHR_materials_specular` (Factor+Color),
  `roughness = 1 − glossiness`. Reminder: the JSON `Shininess` key IS an authored glossiness [0,1]
  (contract of `813ea2ea`). Unify PBR's existing fallback `1 − sqrt(s/128)`, which does not match.
- **D5 — Timing**: the glTF conformance bench runs BEFORE the merge (MaterialDebug's Phong
  comparison rows are the A/B control group and die with StandardResource).
- Feature blocks (clearcoat, sheen, transmission, iridescence, anisotropy, SSS…) stay **optional
  blocks inside the one material — never separate classes** (they compose; classes don't).
  PBRResource already has them all.
- **D6 — BasicResource is removed too (owner, 2026-08-12).** Final taxonomy:
  `Material::Interface` survives ONLY as the extension contract for future, genuinely different
  materials (different BRDF *structure* — cloth/hair/skin someday), and **`StandardResource` is
  the single concrete material in the engine**. "Quick cases" become configurations/presets of
  the one class: a color-only Standard binds zero texture samplers already (descriptor layouts
  are keyed per declared components, Interface.cpp:88-97).

**Blocking locks — no removal before all three are closed:**
1. **PBR cannot cut out**: zero `discard` in PBRResource.cpp — no `alphaMode:MASK`, no
   alpha-tested shadows (`requiresAlphaTestedShadows`). Standard's Opacity component
   (value+texture, configurable `AlphaThreshold`, StandardResource.cpp:1904-1909) is the model.
   Known degradation already documented at USDLoader.cpp:1539-1543 (cutouts fall back to blending).
   This is the one real piece of engineering in the merge.
2. **TerrainResource is hard-locked on Standard** (TerrainResource.cpp:124/195/215 — default,
   reject-anything-else, container).
3. **Two paths route through Standard's *container***: DefinitionResource.cpp:243 (JSON matType
   `"PBR"` is routed to the Standard container) and the remote console's named-material resolution
   (Manager.console.cpp:105/401). Reroute atomically; the merge makes both coherent.

**Lots:**
- [x] **Lot 0 — DONE (2026-08-12)**: tags `pre-material-merge-20260812` on both repos; measured
  baseline series (5 MaterialDebug views + stats,
  `~/.local/share/LNIsle/projet-alpha/captures/baseline-material-merge-20260812/`).
- [ ] **Conformance bench (D5 gate for Lot 4) — RAN, owner review PENDING.** Harness: the
  `loadGLTF` console command + a per-asset clean-instance loop over 19 Khronos
  glTF-Sample-Assets test models (sparse clone in `~/glTF-Sample-Assets/`, outside the repos).
  **19/19 loaded without failure**; gallery (ours vs Khronos reference, per feature) delivered:
  `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-20260812/galerie-banc-gltf.html`.
  Verified in detail: AlphaBlendModeTest **PASSES** (three distinct MASK cutoffs cut at their
  authored markers — the Lot 1 UBO threshold + cache-key fix working as designed);
  MetalRoughSpheres renders with correct axes. ⚠️ The bench IBL is the gltf-loader demo sky —
  tint differs from Khronos studio references by construction; judge behaviour, not colour.
  **The owner's gallery verdict is the Lot 4 gate.** Legacy-content visual drift at the flip is
  explicitly accepted (owner: industrial pipelines glTF/FBX/USD are the fidelity target).
- [x] **Lot 1 — Parity in PBRResource — DONE (2026-08-12).**
  DONE (documented in `src/Graphics/AGENTS.md` § "Alpha Test" and
  `docs/pipeline-caching-system.md`): the owner's 3-rule Opacity contract (value = global
  blend / map+`AlphaThreshold` = cutout / map = grayscale blend), UBO slots 50-51
  (Opacity/AlphaThreshold — former std140 padding, size unchanged), `enableAlphaTest(threshold)`,
  shadow trio reading the UBO threshold, RT `alphaCutoff` export, the program-cache key contract
  fix (material flag bits in `ProgramCacheKey` + SceneRendering/ShadowCasting generator keys),
  loaders (glTF `alphaMode MASK`, USD cutout + translucent value, FBX opacity rules 1/3),
  `gltf-loader` demo option 7 = IridescentDishWithOlives (MASK validation asset). Validated:
  clean -Werror build, 1967/1967 base unit tests, MaterialDebug bit-exact vs the Lot 0 baseline,
  goldLeaf cutout rendering confirmed. Also done (same day): value-only AutoIllumination
  overload (sets a WHITE emissive — PBR's default color is black, Standard's is white) and the
  D2 artistic Reflection/Refraction `Amount` override (UBO slots 52-53, block 304→320 B, UVW
  transforms shifted +4; neutral 1.0 = BRDF/Fresnel-controlled; JSON `Amount` on the
  texture-mode components; the env-IBL path keeps `IBLIntensity` as its knob).
  ⚠️ Shadow/RT cutout behaviour still needs a dedicated visual check (no shadow-casting MASK
  scene exists yet — conformance bench material). ⚠️ Legacy Standard JSONs conflate
  blending+discard@0.1 on opacity textures — map them to rule 3 at conversion (Lot 3),
  validate visually.
- [x] **Lot 2 — Decouple hard dependencies — DONE (2026-08-12).** Transitional strategy: the
  declared ClassId selects the container (Standard for legacy data, PBR for converted data) —
  zero visual change now, the Standard branches delete trivially at Lot 4. TerrainResource
  accepts both ClassIds (+ drive-by fix: `materialType.value()` was dereferenced on an empty
  optional in the error path); BasicGroundResource dispatches on both; DefinitionResource's
  matType `"PBR"` finally reaches the PBR container (was silently routed to Standard); the
  remote console resolves named materials Standard-first-then-PBR (both sites; documented in
  `Console/AGENTS.md`). MultiLayerMeshResource already dispatched on all three ClassIds;
  Toolkit's Basic default is Lot 7 scope; Resources/Manager containers and the loaders'
  MaterialMode dual paths die at Lot 4. Validated: clean build, terrain demo renders, console
  `setGround` named-material path exercised live. ⚠️ Pre-existing, unrelated, worth a look:
  the terrain adaptive grid spams `generateTriangleListIndicesForRT() returned empty indices`
  ~18k times/min with RT enabled (TriangleStrip geometry has no RT triangle-list path — dates
  from `44cb3a85`, 2026-03-10).
- [x] **Lot 3 — Canonical conversion (D4) at the parse boundary — DONE (2026-08-12).**
  PBRResource's legacy fallbacks now read the manifest `Shininess` as the authored GLOSSINESS
  [0,1] it is (813ea2ea contract): **roughness = 1 − glossiness** (Khronos), replacing the wrong
  exponent formula `1−sqrt(s/128)` (it clamped every authored glossiness to 1.0 → roughness
  ~0.91 regardless of the value). The luminance-invert-to-roughness heuristic on specular
  colors is gone. ⚠️ **Deliberate deviation from the D4 wording**: the Khronos "F0 = specular
  color" half is NOT applied — it presumes spec-gloss-authored F0 values (~0.04); legacy Phong
  colors are highlight intensities (bright greys) and would read near-mirror. F0 stays the
  0.04 dielectric default; the low roughness carries the "shiny" intent. One-line flip if the
  owner wants strict Khronos. ⚠️ These fallbacks have NO runtime user until Lot 4 flips the
  mesh JSONs to the PBR container (the 17 Diffuse-only files load as Standard today) —
  bench-validate then.
- [x] **Lot 4 + Lot 5 — Removal and rename — DONE (2026-08-12), ONE atomic commit.**
  ⚠️ Deviation from the approved plan (two commits): splitting them leaves an INCOHERENT repo
  state — between the two, the 19 mesh JSONs carrying `"MaterialStandardResource"` resolve to
  nothing and `TerrainResource` refuses to load. The swap (delete legacy + PBR takes the name
  and the ClassId) is therefore atomic; the diff is bigger, every commit is coherent.
  What landed: legacy `StandardResource.{hpp,cpp}` deleted; `PBRResource.{hpp,cpp}` `git mv`-ed
  to `StandardResource.{hpp,cpp}` (file history preserved), class renamed, **ClassId string
  `"MaterialStandardResource"` reused** so the 19 mesh JSONs stay valid untouched;
  `MaterialMode` enum + `LoaderOptions::materialMode` removed and the GLTF/FBX/USD dual paths
  collapsed to one; one lit-material container in `Resources/Manager`; `Material::Types` down to
  2 entries; redundant branches folded in Terrain / BasicGround / MultiLayerMesh /
  DefinitionResource / console (JSON `"Standard"` and `"PBR"` now synonyms).
  App side: HealthPack, Battery, Collision, DebugUtils, GeometryGenerator, MaterialDebug and the
  FBXLoader demo migrated off the deleted Phong API (Ambient dropped by design; shininess
  exponent → roughness via a documented perceptual table; white specular over a coloured albedo
  ⇒ dielectric, named metals ⇒ metalness 1 with the metal colour moved into the albedo).
  Data: 3 files fixed (`MetalBarrel.json` `"MaterialPBRResource"` → `"MaterialStandardResource"`,
  `demo.json` + `terrain_demo.json` invalid generic `"Material"` → the real ClassId).
  Validated: full cascade builds clean (-Werror, zero warning), 1967/1967 base unit tests,
  `material-debug` renders with no new runtime error. ⚠️ **Legacy content SHIFTS visually** —
  accepted by the owner ("mode industriel", only the glTF/FBX/USD pipelines are the fidelity
  target; legacy materials get fixed afterwards).
  ⚠️ **CMake must be RECONFIGURED after this kind of change**: `GLOB_RECURSE` caches the file
  list, and Ninja fails with `PBRResource.cpp missing and no known rule to make it` otherwise.

- [ ] **Lot 6 — Documentation (same day, per project rule)**: AGENTS.md network (Graphics, Scenes,
  Console — `Console/AGENTS.md:204` finally becomes exact), material JSON schema docs,
  projet-alpha `.claude/rules/` mirror, `generate_materials.py` header (optional: regenerate the
  3,918 dual-schema JSONs to single-schema — nothing requires the dual layout after the merge).
- [ ] **Lot 7 — Remove BasicResource — INVESTIGATED (2026-08-12), 2 prerequisites BUILT, the
  rest is a CHANTIER, not a cleanup.** The four-way investigation found the plan's estimate far
  too optimistic. Facts:
  - **DONE — vertex colours on StandardResource.** Only the vertex-shader half existed
    (`:2451-2464`) and was unreachable (nothing ever set `UseVertexColors` on a Standard
    material). Added `enableVertexColor()` and, per the owner's contract, a SINGLE folded albedo
    variable `SurfaceAlbedoFinal = albedo * svPrimaryVertexColor` declared once at the top of the
    fragment shader, with `albedoExpression()` routing the light generator, `fragmentColor()` and
    the alpha test through it. ⚠️ `BasicResource`'s `DynamicColorEnabled` gate is deliberately
    NOT ported: it existed only because Basic's default diffuse is Grey; Standard's default
    albedo is White, so multiplying unconditionally IS the contract. ⚠️ The light generator
    concatenates swizzles onto the name (`+ ".rgb"`, `+ ".a"`), so it must receive a NAME, never
    a compound expression — an unparenthesised one still compiles and silently swizzles the wrong
    operand.
  - **DONE — `emissionMultiplier()` on StandardResource.** It was implemented ONLY by
    BasicResource: deleting Basic would have deleted the engine's entire unlit-emission
    mechanism, and the skybox would render its raw [0,1] texel — near-black under photometric
    exposure, with ZERO logs. Ported, keyed on the AutoIllumination component.
  - **ALREADY MET — `isComplex()`** is feature-derived on StandardResource (`:1628-1631`), and a
    colour-only Standard already binds zero samplers (the owner's original reason for Basic is
    satisfied). Remaining nit: fold the ad-hoc normal-mapping term at `SceneRendering.cpp:212-219`
    into the predicate.
  - **DONE — the UNLIT flag (owner-approved).** `MaterialFlagBits::UnlitEnabled = 1U << 17` +
    `Interface::isUnlit()` + `StandardResource::enableUnlit()`. The decision point moved into
    `SceneRendering::isLightingRequested()`, which now gathers the three necessary conditions —
    scene light set enabled, instance asked for lighting, and **the material does not veto it**.
    Four call sites plus the light-generator setup route through it. glTF
    `KHR_materials_unlit` semantics: content carrying its own radiance is never re-lit, whatever
    the instance asked for. ⚠️ The unlit path writes `fragmentColor().rgb * emissionMultiplier()`,
    so an unlit material MUST carry an AutoIllumination component or it writes its raw [0,1]
    colour and reads black under photometric exposure. ⚠️ Deliberately NOT touched:
    `prepareUniformSets()` still keys the PerLight descriptor set on the raw flag — it seals the
    pipeline layout, and a declared-but-unused set is harmless whereas a missing one crashes.
  - **DONE — the two API gaps.** `setAlbedoComponentFromRenderTarget(TextureInterface, enableAlpha)`
    puts a raw GPU texture in the albedo slot (no resource dependency — the CALLER owns the
    lifetime and must outlive the material). ⚠️ It could NOT be an overload of
    `setAlbedoComponent`: `TextureResource::Texture2D` derives from BOTH
    `TextureResource::Abstract` and `Vulkan::TextureInterface`, so every existing call became
    ambiguous — hence the distinct name, matching `setReflectionComponentFromRenderTarget()`.
    `setAlbedoComponent(texture)` now also takes an `enableAlpha` flag (default false, behaviour
    unchanged) and propagates `PrimaryTextureCoordinatesUses3D` for cubemap albedos.
    Validated on screen: `offscreen-rendering`'s CCTV screen displays its render target.
    ⚠️ That block sat inside `if ( false )` and was DEAD — re-enabled, since it is the only
    consumer of this path and a disabled consumer lets the path rot unnoticed.
  - **BLAST RADIUS**: 103 engine + 39 app code sites, 15 mesh JSONs carrying
    `"MaterialBasicResource"`. Then the legacy lighting machinery dies: `m_usePBRMode` and its 14
    branches, the (n+2)/(8π) normalisation, the Gouraud per-vertex path, ~898 lines of
    `LightGenerator.PerFragment*.cpp`, plus ~48 doc lines.
  - ⚠️⚠️ **SILENT FAILURES to guard against** (the dangerous part): a surface migrated off Basic
    without `enableVertexColor()` loses its colours with NO log (`VertexBufferFormatManager`
    emits `declareJump(VertexColor)` and discards the attribute); `FastJSON::getValidatedStringValue`
    returns nullopt with no trace, so the 15 mesh JSONs would silently fall back to another
    material class; and **`Shininess` means two different things** — Basic reads it as a RAW
    Blinn-Phong exponent, Standard as a glossiness [0,1], so migrating the 3,918 material files
    visibly changes them. Sprite alpha also differs (Basic gives the texture alpha priority over
    the uniform opacity; Standard replaces the whole alpha).
  - ⚠️ MDI eligibility and bindless enablement are gated on `isLightingEnabled()` /
    `useEnvironmentCubemap()`: swapping the cheap material changes which draws take the MDI path
    and which sampler-binding model they use. Verify both before and after.
