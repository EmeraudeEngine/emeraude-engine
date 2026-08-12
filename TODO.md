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
  - **OPEN — missing API**: no raw `Vulkan::TextureInterface` albedo setter (render-target-as-albedo,
    `src/Builtin/OffscreenRendering.cpp:120-123` depends on it), and `setAlbedoComponent(texture)`
    does not propagate `PrimaryTextureCoordinatesUses3D` (cubemap albedo).
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
