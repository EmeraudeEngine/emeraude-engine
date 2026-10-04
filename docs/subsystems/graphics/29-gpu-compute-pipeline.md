## GPU Compute Pipeline (Graphics/Compute/)

### Infrastructure

The engine supports Vulkan compute shaders via:
- `Vulkan::ComputePipeline` — Compute pipeline with `setShaderModule()` for shader stage init
- `Vulkan::CommandBuffer::dispatch()` — Compute shader dispatch (vkCmdDispatch wrapper)
- `Saphir::ShaderManager::getShaderModuleFromSourceCode()` — Runtime GLSL→SPIRV compilation

### Graphics/Compute/XRayAnalyzer

GPU-accelerated volumetric cross-section scanner using Vulkan compute shaders:
- **GLSL compute shader**: 16×16 workgroups, Möller-Trumbore ray-triangle intersection
- **Spatial grid SSBO**: 128² cells with triangle index lookup (avoids brute-force 150K tests per pixel)
- **Bit-packed output**: 32 pixels per uint32 (32× memory reduction vs raw pixels)
- **Device-local output SSBO** + **host-cached staging buffer** for fast PCIe readback
- **Push constants**: Per-slice depth, resolution, grid parameters
- **Pipeline barriers**: compute→transfer→host for correct synchronization
- **Performance**: 46ms/slice at 16K×16K on RTX 3070 Ti (5.5 Gpixels/sec)

Architecture:
```
Triangles SSBO (binding 0) ─┐
Grid Cells SSBO (binding 2) ─┼─→ Compute Shader ─→ Output SSBO ─→ vkCmdCopyBuffer ─→ Staging Buffer ─→ memcpy ─→ CPU
Grid Indices SSBO (binding 3)┘                    (device-local)                      (host-cached)
```

Code references:
- `Graphics/Compute/XRayAnalyzer.hpp` — Public API (addShape, setViewpoint, prepare, scan, scanAll)
- `Graphics/Compute/XRayAnalyzer.cpp` — Vulkan pipeline setup, GLSL source, dispatch loop
- `Vulkan/ComputePipeline.hpp:setShaderModule()` — Shader stage initialization
- `Vulkan/CommandBuffer.hpp:dispatch()` — vkCmdDispatch wrapper
- `Vulkan/Buffer.hpp:setHostReadable()` — HOST_CACHED_BIT for fast CPU reads

### Graphics/Compute/IBLBaker + Graphics/IBLTexture (IBL lot 1, Jul 2026)

The image-based-lighting GPU bricks. `IBLTexture` (a `Vulkan::TextureInterface`) is an
**engine-baked, GPU-only texture**: no CPU pixel data, image created with
`STORAGE_BIT | SAMPLED_BIT`, always `RGBA16F` — the only 16F layout with **mandatory**
`STORAGE_IMAGE` support (`R16G16_SFLOAT` storage is an optional Vulkan feature; never rely
on it cross-platform). Three roles drive dimensions and sampler:

| Role | Image | Sampler (cache name) |
|------|-------|----------------------|
| `BRDFLut` | 2D 128², 1 mip | `IBLBrdfLut` — clamp-to-edge both axes (NdotV × roughness) |
| `IrradianceCubemap` | cube 32², 1 mip | `IBLIrradiance` — bilinear, single mip |
| `PrefilteredCubemap` | cube 128², 6 mips (128→4) | `IBLPrefiltered` — trilinear, `maxLod = VK_LOD_CLAMP_NONE` |

`IBLTexture::storageView(mip)` exposes per-mip **storage views** for compute `imageStore`
(cube roles → `2D_ARRAY` views, 6 layers = faces; LUT → plain 2D).

`Compute::IBLBaker` bakes the content. `generateBRDFLut()` (lot 1): split-sum LUT
(Karis 2013), 1024 Hammersley samples, Smith GGX with the **IBL k remap (`k = a²/2`)** —
never the analytic-light Disney remap. Reconstruction in shaders:
`specular = prefiltered * (F0 * lut.x + lut.y)`; the two channels also feed the
Fdez-Agüera multi-scatter compensation (lot 3) with no extra resource.

**In-texture celestial body mask (Sep 2026).** `bakeEnvironment()` takes a fourth argument,
`IBLBaker::StarMask` (direction toward the body in cubemap = world space, cone half-angle in
radians, `enabled()` when the half-angle is positive; a default-constructed mask keeps everything).
The push-constant block grew a `vec4 starMask` (offset 16) that `ProbeConvolver` pushes zeroed —
zero half-angle = no mask — so the borrowed prefilter pipeline is unaffected. The GLSL `maskStar(L,
mip)` in the common block redirects any sample inside the cone to the rim, the cone widened by the
footprint of the source mip read (`(π/2)·2^mip / sourceSize`); all three sample sites go through it
(mirror copy, prefilter loop, irradiance loop). `Scene::environmentStarMask()` builds it from the
background's brightest `InTexture` star, and the mask is part of the bake identity. Since 2026-10-05
`maskStar()` delegates to the shared `emMaskStar()` (`Graphics/Effects/Shared/StarMaskGLSL.hpp`), the
one mask RTGI and the irradiance probe volume apply to their raw-cubemap sky term too.

`bakeEnvironment(source, irradiance, prefiltered, starMask)` (lot 2): per-environment assets in ONE
blocking submission, re-baked at every sky change. Both passes use **filtered importance
sampling** (Křivánek & Colbert, GPU Gems 3 ch. 20): each sample reads the SOURCE mip whose
texel solid angle matches the sample solid angle — this is why environment cubemaps carry
their full mip chain, and why 64-512 samples/texel suffice. Details:
- Prefiltered: GGX importance sampling, N=V=R, cosθ weighting, roughness = mip/(mips−1),
  **mip 0 = direct copy** (roughness-0 shortcut), samples = 64 + 32·mip.
- Irradiance: cosine importance sampling, 512 samples, **+1 FIS mip bias** (without it the
  near-normal samples read the detailed source mips and a sun disc prints the fixed
  Hammersley sequence as a star-shaped artefact), stores **E/π** — ambient shading is then
  `albedo * texture(irradiance, N) * environmentLuminance`, matching the scalar path on a
  uniform sky.
- The baker works entirely in **cubemap space** (identity face mapping, `faceDirection()`);
  the world-to-cubemap Y negation stays a CONSUMER contract.
- Measured on the RTX 3070 Ti: ~1 ms uncontended (submit+wait, 1024² source); up to a few
  ms when the graphics queue is draining a frame — one-shot per sky change. Upgrade path
  for per-frame dynamic skies: fence-polled async submit (documented in the code).
- Debug: compile with `EMERAUDE_DEBUG_IBL_FACES` to dump every baked face as tonemapped
  PNGs to `/tmp/ibl-*.png`.

**Trigger & publication (Scenes side):** `Scene::updateEnvironmentIBL()` polls in
`processLogics` (same pattern as the background photometry poll): when the mutex-protected
`BindlessTextureSet::environmentCubemap()` identity changes (and is not the engine default),
it bakes into a **ping-pong pair** of scene-owned `IBLTexture` (frames in flight keep
sampling the published pair untouched), then publishes via
`BindlessTextureSet::setIrradianceCubemap()/setPrefilteredCubemap()` — mirrored to the
reserved slots by `syncTextureSet()` (UPDATE_AFTER_BIND hot-swap), parked on the default
cubemap by `clearTextureSet()` on scene switch.

**Baking runs on the GRAPHICS queue** (one-shot, `waitIdle` at boot): the images are later
sampled by fragment shaders on that same queue — using the compute queue would demand a
queue-family ownership transfer on an EXCLUSIVE image. Barrier sequence:
`UNDEFINED → GENERAL` (compute write) → dispatch → `GENERAL → SHADER_READ_ONLY_OPTIMAL`
(fragment read), via `Vulkan::Sync::ImageMemoryBarrier`.

Wiring: `Renderer::createDefaultResources()` creates + bakes the LUT once and publishes it
with `updateTexture2D(BindlessTextureManager::BRDFLutSlot, …)`; `Renderer::brdfLUT()`
exposes it for effects binding it through their own descriptor sets.

> [!CRITICAL]
> **Engine cubemap sampling convention (Y-UP, settled Aug 2026):** a world direction `D`
> samples any environment cubemap **RAW — `D` itself, no component negation.** Every
> sampling site obeys it: the skybox (`Material/Helpers.cpp` `checkPrimaryTextureCoordinates`),
> the material reflections (`StandardResource`), `LightGenerator`, SSR, RTGI, RTR, and the
> IBL generation (`IBLBaker`).
>
> ⚠️ **This REPLACES the Jul 2026 rule `vec3(D.x, -D.y, D.z)`**, which existed only because
> the world was Y-down (UP = -Y) while cubemaps are stored Y-up. The Y-up flip removed the
> reason; five of the six negations went with the flip and the sixth — the skybox display —
> followed once measured in the `coordinates-debug` scene. **A negation re-introduced anywhere
> now swaps the +Y/-Y faces and mirrors the four side faces vertically**; measured symptom:
> the magenta `Y-` face of `AxisDebug` at the zenith while the compass sphere there is green.
>
> **The hardware face convention is untouched and NOT negotiable**: the `(dx, dy, dz)` tables
> in `CubemapResource` / `CubemapMovieResource` and the frozen `IBLBaker.cpp:211-216` table
> are the standard Vulkan cube-face mapping. Consequence to know: that convention is
> LEFT-handed, so in this right-handed world **each face image displays mirrored horizontally**
> relative to the stored pixels. The equirectangular loader bakes that in (its
> `u = atan2(dz,dx)/2π + 0.5` output reads non-mirrored on screen); packed / per-face assets
> must therefore be authored the same way — as the standard skybox sets are.

### Environment cubemap mip chains (IBL lot 1, Jul 2026)

`TextureCubemap::createTexture()` now **always builds the full mip chain**
(`Image::getMIPLevels`), ignoring the global `Core/Graphics/Texture/MipMappingLevels`
setting (default 1 — which used to silently disable every cubemap mip in the engine), and
the shared `"Cubemap"` sampler uses `maxLod = VK_LOD_CLAMP_NONE`. Rationale: the IBL
prefiltering (filtered importance sampling) reads the source chain by solid-angle ratio,
and roughness-driven `textureLod()` (PBR transmission) needs real mip content. Memory cost:
+33% on a handful of cubemaps. The upload blit chain (`ImageTransferOperation::finalizeForGPU`,
per-layer × per-mip `vkCmdBlitImage`) is exercised for cubemaps since this change — LDR and
HDR (RGBA16F) validated visually (water-world BlueSky reference frame).
