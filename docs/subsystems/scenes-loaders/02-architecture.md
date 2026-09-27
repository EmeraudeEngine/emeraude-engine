## Architecture

### Design Philosophy

Scenes::Loaders sits between `Base/` (raw data) and `Scenes/` (scene graph). Each loader:
1. Parses a composite file format (glTF, FBX, USDZ...)
2. Creates engine resources in containers (images, textures, materials, geometry, meshes, skeletons, clips)
3. Attaches skeletal data to renderables automatically via `SkeletalDataTrait`
4. Produces a format-agnostic `SceneData` describing the node hierarchy — no Scene/Node/Entity types

### Layer Rules

- **CAN depend on:** `Resources/`, `Graphics/`, `Animations/`, `Base/`
- **CANNOT depend on:** `Scenes/`, `Physics/`, `Audio/`, `Input/`
- **No Scene types:** `SceneData` uses `NodeDescriptor` (pure data), never `Node`, `StaticEntity`, or `Component::Visual`

### Capability declaration (added 2026-08-08)

`LoaderCapabilityBits` (`Interface.hpp`): `Geometry`, `Skinning`, `Animations`, `Lights`,
`Cameras`.

> [!WARNING]
> The mask describes **the loader**, not the file format. FBX carries lights and cameras; our
> FBX loader does not read them, so it must not advertise them. Current state:
>
> | Loader | Capabilities |
> |--------|--------------|
> | `GLTFLoader` | `Geometry \| Skinning \| Animations \| Lights \| Cameras` |
> | `FBXLoader` | `Geometry \| Skinning \| Animations` |
> | `WADLoader` | `Geometry` (a Doom level's lighting is BAKED, by design) |
>
> **Why it exists:** probing the produced `SceneData` for an empty light table cannot tell
> *"this loader ignores lights"* from *"this asset declares none"* — and both happen. Ask the
> capabilities before concluding anything about a scene's lighting.

> [!CAUTION]
> **The mask's grain is the DOMAIN, never the feature — do not read it as a completeness claim.**
> `GLTFLoader` returns every bit, and that is accurate at the grain the mask has, yet the loader
> still reads only `TRIANGLES` primitives, a single UV set, no vertex colours, no authored
> tangents, no morph targets, and never populates `instanceSets`. `Geometry` means *"this loader
> produces geometry"*, not *"this loader reads all of the format's geometry"*. An audit of what
> each loader actually consumes is the only answer to *"is this format fully supported?"* — the
> honest current answer for both glTF and FBX is **no**, and the gaps are listed per loader below.

### Common Interface

All loaders implement `Interface` (`Interface.hpp`):

```cpp
class Interface {
    void setOptions(LoaderOptions options) noexcept;

    virtual bool load(const std::filesystem::path & filepath, SceneData & output) noexcept = 0;
    virtual bool supportsExtension(std::string_view extension) const noexcept = 0;

    /* Mask of LoaderCapabilityBits. Pure virtual: a loader cannot inherit a lie. */
    virtual uint32_t capabilities() const noexcept = 0;

    /* Default: returns false. Loaders override to opt in. */
    virtual bool loadAnimationClipsOnly(
        const std::filesystem::path & filepath,
        const Animations::SkeletonResource & targetSkeleton,
        std::vector<std::shared_ptr<Animations::AnimationClipResource>> & output) noexcept;
};
```

`LoaderOptions` controls resource loading behavior:
- `excludedNodeNames` — skip specific nodes and their subtrees
- `onMeshLoaded` — `std::function<void(MeshDescriptor &)>` callback invoked once per mesh, right after the renderable, geometry and materials have been registered in their containers and pushed to `output.meshes` (before nodes are wired). Lets the caller patch the descriptor in place — typical use cases: enable IBL reflection on the produced materials (`StandardResource::setReflectionComponentFromEnvironmentCubemap()`), swap a renderable, override geometry. Symmetric across `FBXLoader` and `GLTFLoader` (5 invocation sites total: 4 in FBXLoader including the fallback paths, 1 in GLTFLoader).
- `skipSkinning` — skip bone weights, skins, and animations
- `forceDoubleSided` — `bool`, default `false`. Forces **every** material part of the loaded model to `RasterizationOptions{ CullingMode::None }`, OR-ed with the per-material asset flag (see *Double-sided materials* below). Symmetric across `FBXLoader` (applied per material part, default-material parts included) and `GLTFLoader` (per primitive). Use it when the asset's format/exporter cannot carry a double-sided flag the loader can read — the canonical case is **Mixamo FBX rigs**, which ship plain `FbxSurfacePhong` materials: ufbx only surfaces `double_sided` for glTF-style materials, so these models always import single-sided and thin shells (inner armour, cloth) render with see-through holes. Blunt instrument (whole model); for per-mesh selectivity use the `onMeshLoaded` hook instead. Two-sided lighting (back-face normal flip) is already engine-side, so forced back-faces are correctly lit. Validated on the Paladin (`src/Actor/Paladin.cpp`).
- `environmentReflectionIntensity` — `float`, **default `0.0F` — OFF, opt-in per asset (owner decision, Aug 2026)**. When > 0, every material the loader produces gets `setReflectionComponentFromEnvironmentCubemap(intensity)` — IBL specular from the scene's environment cubemap AND the promoted reflectivity in the material-properties G-buffer (SSR/RTR input). ⚠️ The environment cubemap is **UNOCCLUDED** and scaled to the sky's **absolute luminance**: enabling this on an INTERIOR makes every smooth dielectric and metal mirror the outdoor sky at full photometric brightness even in shade (measured on Sponza: glass roughness 0 and metal doors metalness 0.88 glowing green in a dark corridor). Enable it only where materials genuinely see the environment (object showcases: DamagedHelmet passes `1.0F`); occluding reflections (local probes / specular occlusion) is a separate pending work item.
- `uniformScale` — `float`, default `1.0F`. Uniform scale applied at load time, **coherently across the full skinned-mesh pipeline**: vertex positions (in `loadMeshes`), joint local TRS translations + inverse bind matrix translation columns (in `loadSkins`), and animation translation keyframes (in `sampleAnimStack`, covers both `load()` embedded clips AND `loadAnimationClipsOnly()` external clips). Rotations and scales of joint TRS plus per-vertex influence weights are never touched. Linear (rotation + uniform 1×1 scale) parts of the inverse bind matrix are unaffected by uniform scaling around origin, so only the translation column needs scaling there. **Critical**: the same factor must be passed to BOTH the rig load (`load()`) AND every subsequent `loadAnimationClipsOnly()` against that rig — otherwise animation translation keyframes describe positions in a different unit than the scaled bind pose, joints snap to wrong positions on every keyframe, and the rig visually collapses on the first animated frame. Also propagates to the renderable's bounding box, so collision shapes derived from the bbox reflect the scaled size automatically. Use cases: enlarging a Mixamo humanoid that ships at 1.7 m to 1.9 m for a knight silhouette (validated end-to-end on the Paladin); shrinking oversized Maya/Blender assets without re-export.

  > [!WARNING]
  > **`GLTFLoader` ignored this option ENTIRELY until Aug 2026** — zero occurrences in the whole
  > file, while this very paragraph described its semantics. A caller passing `uniformScale = 0.01F`
  > to a glTF got a model at scale 1 and no warning: a documented option that is a silent no-op is
  > worse than an absent one, because the caller has no reason to check. It is now applied at the
  > same four sites as the FBX path — vertex positions, joint local TRS translations, the inverse
  > bind matrix translation columns, and the translation keyframes (**including their CUBICSPLINE
  > in/out tangents**, which are lengths per second and scale exactly like the values they
  > interpolate). A scale channel is a RATIO and is never touched.
- `stripRootMotion` — `bool`, default `false`. When set, `loadAnimationClipsOnly()` zeroes the **horizontal (X, Z) components** of every translation keyframe on every root joint of the produced clips. Rotation + scale of the root and *all* channels of every other joint stay intact. The vertical (Y) component is preserved on purpose: it carries both the bind-pose hip-height offset (~0.85 m on a Mixamo humanoid — wiping it would sink the model halfway into the ground) and the natural up/down bounce of walking, jumping or crouching. Idiomatic "convert per-action FBX into in-place clip" pass at load time. Required for any FBX (Mixamo, Maya/Blender per-action) where the root bone carries forward locomotion AND the actor's displacement is also driven by gameplay code (physics force, navmesh) — without this, the two motions stack and the model snaps backward at every clip loop. Has no effect on `load()` (full-pipeline import) — only on `loadAnimationClipsOnly()`. **`GLTFLoader` does not implement `loadAnimationClipsOnly()`** (glTF carries its clips inside the asset, so the split-animation workflow has no glTF equivalent), which makes the option inapplicable there: since Aug 2026 the glTF path **logs a warning** instead of ignoring it, so a caller porting FBX code over cannot believe the root motion was stripped. **Future work — Option C (root-motion mode):** instead of stripping, extract the root delta per frame and feed it back to the actor as actual displacement (foot-planting, no foot-sliding, animation-driven speed). Would replace the actor-side `addForce` for animation-driven characters; tracked as a TODO for the locomotion subsystem.

**Note:** `flattenHierarchy` is NOT in `LoaderOptions` — it only affects scene building and belongs in `Scenes::SceneDataConsumer`.

### The resource PREFIX — A FILE NAME IS NOT AN IDENTITY EITHER (fixed 2026-09-18)

The same lesson as the section below, one level up. `GLTFLoader::load()` builds every key under a
per-asset prefix, and that prefix was the file **stem**:

```cpp
m_resourcePrefix = "glTF:" + filepath.stem().string() + "/";                       // until 2026-09-18
m_resourcePrefix = "glTF:" + path.replace_extension().lexically_normal().generic_string() + "/";
```

So `SunglassesKhronos/glTF-Binary/SunglassesKhronos.glb` and
`SunglassesKhronos/glTF-Draco/SunglassesKhronos.gltf` — different geometry encodings, different
texture encodings — shared one namespace, `glTF:SunglassesKhronos/`, and the second load served the
first's cached resources.

> [!CAUTION]
> **The symptom was a WRONG MEASUREMENT, never a crash.** The conformance bench read **98.16 % of
> pixels differing, mean 39.28/255** for that model when a manual `Core.openFiles()` of one variant
> preceded the run in the same session; from a clean session the same comparison is **6.35 % /
> 0.10**. After the fix the contaminated scenario reproduces **6.3451 %**, bit-identical to the clean
> one. A defect that only moves a number is the worst kind — nothing looks broken.
>
> It stayed hidden because replacing a viewer scene unloads its resources first, so the common case
> never collides. Reproduce it by loading variant A and then variant B **without letting the first
> scene be torn down**.

**The key is derived from the path AS GIVEN**, deliberately: `lexically_normal()` folds `.`/`..`
away and `generic_string()` forces forward slashes, so one file yields one key on every platform —
but it is *not* canonicalised to an absolute path, because a caller must be able to **predict the
key without loading anything**. projet-alpha's `Fox` does exactly that, to skip a reload it has
already performed (`src/Actor/Fox.cpp`, `FoxMeshResourceName`).

> [!WARNING]
> **The price of that choice: two SPELLINGS of one file are two keys.** Passing
> `data/data-stores/glTF/Fox.glb` in one place and an absolute path to the same file in another
> loads its resources twice. Pass an asset's path consistently. Canonicalising instead would have
> made every key machine-dependent and unwritable by hand.

### The resource key — AN ASSET NAME IS NOT AN IDENTITY (fixed 2026-08-28)

> [!CAUTION]
> **The identity of a mesh, a material, a texture or an image is its INDEX in the asset, never
> its name.** Neither glTF nor FBX imposes any uniqueness on names, and a resource container keyed
> on the name alone returns the **first homonym to every later caller, silently**: the second mesh
> named `Sphere` receives the first one's geometry **and its material**. Nothing is logged, no
> error path is taken, and the result looks exactly like an un-wired material feature.

Every loader therefore builds its keys through
`buildResourceKey(prefix, category, assetName, assetIndex)` (`Interface.hpp`), which yields
`{prefix}{Category}/{name}-{index}` — and the bare `{index}` when the asset declares no name, so
unnamed items keep the key they always had. `USDLoader` already used that convention
(`/mesh/<prim_name>-<index>`); `GLTFLoader` and `FBXLoader` were aligned onto it.

**What the collapse cost, measured on the Khronos conformance assets (2026-08-28).** It had been
mis-attributed for three bench runs to un-wired material extensions:

| asset | duplicate names | what rendered |
|---|---|---|
| `ClearCoatTest` | `ClearCoatSampleMesh` **×18**, materials 0…17 | all eighteen cells wore material 0's red, and the `Base layer` / `Coated` / `Coating Only` columns were **literally the same renderable** |
| `MetalRoughSpheresNoTextures` | `Sphere` ×98 | ninety-eight spheres, one material |
| `SpecularTest` | `OneSample` ×20, `FiveSamples` ×3 | thirty-five spheres collapsed to five |
| `TransmissionTest` | `Sphere` ×12, plus **three genuinely different** materials all named `BlueTransWithMask` | twelve identical spheres |
| `TransmissionRoughnessTest` | `RoughnessSamples` ×6, **images** `RoughnessGrid` ×2 | the image layer aliases too |

⚠️ `AnisotropyStrengthTest` and both iridescence models were **spared** — their meshes and
materials are *unnamed*, so the index fallback already saved them. That is why their failures are
genuinely un-wired extensions and must not be re-attributed to this defect.

⚠️ **The colour space is part of the key, the addressing is not.** The `sRGB` flag is baked into a
texture resource at creation and comes from the **usage**, not from the asset: the same image
legitimately serves as an sRGB albedo for one material and as a linear roughness map for another.
So one asset texture yields up to **two** engine resources, `…-srgb` and `…-data`, and the
loaders' `m_textures` cache carries **two slots per texture index** for exactly that reason
(it used to carry one, letting whichever usage resolved first impose its colour space on the
other — latent, no bench asset exercises it). The wrap modes, on the other hand, belong to the
asset texture itself: with the index in the key, each asset texture owns its own resource, so the
old `-<U><V>` suffix became redundant and was removed.

### Axis flip — `swapX` / `swapY` / `swapZ` (added Aug 2026, **DELETED Aug 2026**)

> [!CAUTION]
> **This mechanism no longer exists. Do not reintroduce it.** `AxisFlip.hpp` is deleted,
> `LoaderOptions::swapX/swapY/swapZ` are deleted, and `Interface::axisFlip()` is deleted.

**Why it existed, and why it is gone.** The engine's world→screen mapping used to be
**orientation-reversing**, so every chiral detail of every imported asset landed mirrored on screen
(carved text reading backwards, a left hand becoming a right one). The per-asset flip cancelled that
mirror one asset at a time. That was a **workaround for a root cause elsewhere**: the world
convention mixed Vulkan's Y-down NDC with OpenGL's -Z-forward eye space, making the eye→NDC map
`diag(+, +, −)` — a reflection.

The root cause was **fixed** in Aug 2026 by flipping Y in the projection itself
(`Matrix::perspectiveProjection()` `[Col1Row1] = -a`) and moving the world to **Y-UP**. With the
mirror gone from the pipeline, every local compensation it had accumulated became a defect in its own
right, and all of them were deleted together — see
[`docs/coordinate-system.md`](../../coordinate-system.md) § *What the mirror had left in the
tree*.

**What the import does today: NOTHING.** glTF, USD and FBX are all right-handed Y-up, `-Z` forward —
the engine's own convention. The import is the **IDENTITY**: no rotation, no mirror, no per-asset
flag, no winding swap.

⚠️ **If an asset looks mirrored, the cause is NOT here.** Do not add a flip, a negative scale or a
winding swap to compensate. Measure first (`Core.SceneManagerService.toggleCompass()`, two camera
poses — the protocol is in `docs/coordinate-system.md`), then fix the actual cause.

⚠️ **The unconditional winding swap of the loaders is gone too.** It was justified by the claim that
*"the 180° X rotation inverts the winding"* — **false**: a rotation has determinant +1 and NEVER
inverts a winding. It was compensating the mirror. glTF, FBX and USD now keep the authored winding
verbatim, with their `computeTriangleNormal(false)` partners.

**`WADLoader` is the one loader that KEEPS an unconditional winding swap** (`swapWinding = true`,
`WADLoader.cpp`), and that is correct: Doom geometry is baked with **Z negated** on positions AND
normals, so its bake is determinant +1 and the swap is a genuine property of the bake, not a mirror
compensation. Do not "harmonise" it with the other three.

### Double-sided materials (honored since Jun 2026)

Both loaders read the standard per-material **double-sided** flag and translate it to a per-layer
`RasterizationOptions{ CullingMode::None }` passed into `MeshResource::load(geometry, materialList, rasterizationOptions)` / `SimpleMeshResource::load(geometry, material, rasterizationOptions)`:
- **glTF** — `fastgltf::Material::doubleSided` (glTF 2.0 standard), read in `GLTFLoader::loadMeshes` per primitive.
- **FBX** — `ufbx_material.features.double_sided.enabled`, read in `FBXLoader` per material part.

> [!WARNING]
> **The FBX asset-driven path only fires for glTF-style materials.** ufbx maps
> `UFBX_MATERIAL_FEATURE_DOUBLE_SIDED` solely from the `main|DoubleSided` property in
> `ufbxi_gltf_material_features` — the `UFBX_SHADER_FBX_PHONG`/`FBX_LAMBERT` presets do **not**
> include it. So a standard FBX (Maya/Mixamo `FbxSurfacePhong`) **never** reports
> `double_sided.enabled == true`, no matter how it was authored: the FBX double-sided intent lives
> in the *Model node* `Culling` property (default `"CullingOff"` on every node — not a reliable
> per-material signal, and not exposed by ufbx's public API anyway). This is a format/exporter
> limitation, **not** an engine bug — the loader honours everything ufbx surfaces. For these assets
> use `LoaderOptions::forceDoubleSided` (above). Diagnosed Jun 2026 on the Paladin (Mixamo Phong).

Without this, back-faces were culled and thin double-sided surfaces (Sponza curtains, foliage,
cloth, inner armour shells) rendered front-face-only with see-through holes. The models were correct
(Sponza curtains declare `"doubleSided":true`); the loaders simply ignored the flag.

> [!WARNING]
> **The multi-material `MeshResource::load(geometry, materialList, rasterizationOptions)` overload
> previously IGNORED its `rasterizationOptions` argument** (every layer got defaults). It now applies
> `rasterizationOptions[i]` per layer (defaults when the vector is shorter). Keep it that way.

> [!NOTE]
> **Two-sided lighting (the other half) is implemented — view-based (`dot(N,V)`).** Geometry
> double-sidedness alone renders back-faces, but they need their shading normal flipped or they
> are lit with an inward-pointing normal. The lighting fragment shaders orient the normal toward
> the viewer: `N = dot(N, V) < 0.0 ? -N : N` — in `LightGenerator.PBR.cpp` (both the normal-mapped
> and geometric N) and `LightGenerator.PerFragment.cpp` (a shared `twoSidedN`/`twoSidedV` used by
> diffuse + specular). The legacy normal-mapped Phong path
> (`LightGenerator.PerFragment.NormalMap.cpp`) applies the same correction to its "facing away →
> discard" test; its tangent-space back-face shading stays approximate.
>
> **Why `dot(N,V)` and not `gl_FrontFacing`:** `gl_FrontFacing` keys off the triangle *winding*,
> so a surface whose visible face is wound "backwards" (e.g. a Perlin ground rasterized
> back-facing) gets its correct normal wrongly flipped → its lighting collapses to zero. The
> view-based test keys off the actual geometry (normal vs eye), so any surface facing the camera
> is lit correctly regardless of winding. (This replaced an earlier `gl_FrontFacing` version that
> broke point-light illumination on the ground.)

**Default behaviour preserved:** if `onMeshLoaded` is not set, the loader behaves exactly like before (no callback, every existing call site unaffected); it is invoked under `if ( m_options.onMeshLoaded )` (a default-constructed `std::function` evaluates to `false`). The former `materialMode` option is GONE (material merge, Aug 2026): there is a single lit material, so every loader populates the one `StandardResource` container.

### The lit material — ONE container (material merge, Aug 2026)

Every loader populates `Graphics::Material::StandardResource`, which **is** the Cook-Torrance
metallic-roughness material (the former `PBRResource`, renamed onto the surviving ClassId
`"MaterialStandardResource"`). The legacy Blinn-Phong `StandardResource` was deleted, and with it
the cross-material alias setters the loaders' configuration lambda used to rely on: there is no
second lit material to stay generic against any more, so a loader may call the PBR API directly
(`setAlbedoComponent`, `setRoughnessComponent`, `setMetalnessComponent`, `setNormalComponent`,
`setOpacityComponent`, `enableAlphaTest(threshold)`, `setAutoIlluminationComponent`,
`setReflectionComponentFromEnvironmentCubemap`…).

> [!WARNING]
> **There is no Ambient material component any more** — dropped by design, AO + IBL replace it. A
> loader translating a source format's "ambient colour" has nothing to map it onto; drop it rather
> than folding it into the albedo, which would double-count the lighting.

### Emissive is a LUMINANCE IN NITS, settled by the spec — anchor no constant

**Owner decision (2026-07-26), do not re-litigate.** glTF 2.0 `Specification.adoc` line 2118: the
product of the emissive texture and the emissive factor is in **cd/m² (nits)**, and
`KHR_materials_emissive_strength` is a unitless multiplier that explicitly "does not alter the
physical units". So:

```
emissiveFactor × emissiveTexture × emissiveStrength  IS  a luminance in nits
```

Follow it to the letter. **Invent no convention and anchor no constant.**

⚠️ **An asset authored "artistically"** — emissive in [0,1], no extension — is worth ~1 nit and
therefore renders **black** under photometric exposure. **Owner: we follow the spec, NO patch.** If
an emissive goes black, the fix is in the asset, not in the importer. (Measured before the decision:
1 asset file uses emissive, 0 use the extension, 4 `setEmissiveStrength` call sites outside the
loaders.)

⚠️⚠️ **Do not compensate an exporter bug.** Khronos glTF-Blender-IO **#1766**: Blender's watt-based
emission needs `× 683 / (2π)` to come out in conformant nits. That is the *exporter's* job.
Compensating engine-side would **double-correct** every properly exported asset — and the symptom
that tempts you into it (a dark emissive from Blender) is indistinguishable from the legitimate
artistic-authoring case above.

### Roughness/metalness — source CHANNEL and factor SEMANTICS per format (fixed Aug 2026)

The material scalar-component contract (see `Graphics/AGENTS.md` § Scalar components): the
texture component reads ONE color channel (default **Red**), and the scalar **MULTIPLIES** the
texel. Each loader owns the translation from its format's semantics:

| Format | Packing | What the loader passes |
|--------|---------|------------------------|
| **glTF** | ONE packed texture — roughness = **G**, metalness = **B** | the texture twice, with `Channel::Green` / `Channel::Blue`, and the glTF factors (spec: `factor × texel`) |
| **FBX** | separate grayscale maps (Red) | the texture with the **neutral factor** (default) — in FBX a connected texture **REPLACES** the scalar; passing the authored scalar would wrongly scale the map (metalness scalar 0 = FBX default = map erased) |
| **USD** | separate maps (Red) | the texture alone (neutral default factor) |

### KHR_texture_transform + transmission-through-the-scene (added Aug 2026)

- **The glTF SAMPLER is read** (Aug 2026): `asset.samplers[texture.samplerIndex].wrapS / wrapT`
  reach the texture resource through `TextureResource::setWrapModes()` before `load()`, and end up
  in the `VkSampler`'s address modes. glTF's own default, when a texture declares no sampler, is
  repeat. ⚠️ Before this, `asset.samplers` was read **only for animations** and every texture got
  repeat addressing: an asset asking for `CLAMP_TO_EDGE` had its border TILED instead, silently
  (measured on the Khronos `TextureTransformTest`). ⚠️ The addressing is baked into the sampler at
  creation, so it is part of the **resource identity**: two glTF textures may share an image AND a
  name while declaring different samplers, and the container returns the EXISTING resource for a
  known name. The loader used to append a `-<U><V>` code to the texture resource name for that
  reason; since 2026-08-28 the **glTF texture index** is in the key, so each asset texture owns
  its own resource and the suffix was **removed as redundant** (see *The resource key — an asset
  name is not an identity*). Same reasoning applies one level down to the sampler cache key
  ([`Graphics/AGENTS.md`](../../../src/Graphics/AGENTS.md) § "The identifier IS the sampler cache key").
  ⚠️ Still NOT read from the sampler: `magFilter` / `minFilter`, which stay driven by the global
  `Core/Graphics/Texture/*Filtering` settings — a known gap, and the reason an asset that disables
  mipmapping on purpose does not get it.
- **`KHR_texture_transform`** (per-texture-info UV scale/offset) is read by `GLTFLoader` and
  lands on the material through `setComponentUVWTransform(componentType, scale, offset, rotation)` —
  one glTF metallic-roughness texture info feeds BOTH the Roughness and Metalness components.
  The transform travels as a **material UBO vec4 (scale.xy, offset.zw)**, identity neutral,
  applied UNCONDITIONALLY at the sampling sites (shader program cache contract — values through
  the UBO, never GLSL literals). ⚠️ Ignoring the extension does not fail: the texture renders
  STRETCHED over the whole UV range (measured on CarConcept: tire treads, brake discs, paint
  flake maps with scales up to [200,400]). ⚠️ NOT supported, logged and ignored: the extension's
  `rotation` and its `texCoord` override (multi-UV gap). ⚠️ RT hit shading does NOT apply the
  transforms yet (raster-only) — known parity gap.
- **`KHR_materials_transmission` goes through the GRAB PASS** (`setTransmissionComponentFromGrabPass`):
  the extension's semantics is seeing THROUGH the surface — a car window shows the interior.
  The cubemap variant refracts the sky only (measured on CarConcept: the glass hid the cabin).
  The codegen falls back to the cubemap when the grab pass is unavailable (low quality/no
  bindless). ⚠️ A sun-facing pane still reads WHITE under a bright sky: the Fresnel reflection
  of a several-thousand-nit sky dominates a tens-of-nits interior — photometry, not a bug
  (the Khronos viewer's neutral studio is dim, hence its always-visible interiors).

> [!WARNING]
> **The failure mode is silent flattening, not an error.** Before the fix all loaders let the
> component read the default RED channel of the packed glTF texture — empty on most assets
> (measured ~0 everywhere on DamagedHelmet while G/B carried the actual maps): roughness 0 +
> metalness 0 uniform over every surface, i.e. a mirror-perfect dielectric with ZERO surface
> disparity. It reads like a lighting/IBL bug; it is a material-identity bug. Validate a
> loader's PBR path by looking for **per-surface disparity** (scratches in reflections, matte
> vs glossy zones), not just "textures are on".

`loadAnimationClipsOnly()` covers the **split-animation workflow** (Mixamo per-action exports, Maya/Blender per-action FBX). The asset file is opened, every `anim_stack` is sampled against the bones of `targetSkeleton` resolved **by joint name**, and the produced clips are appended to `output`. Joints with no matching node are silently dropped (kept at bind pose). See FBXLoader section for the concrete implementation.

### SceneData — Common Intermediate Format

`SceneData` (`SceneData.hpp`) is the format-agnostic output:

| Field | Type | Description |
|-------|------|-------------|
| `meshes` | `vector<MeshDescriptor>` | Loaded renderables + geometry + materials |
| `skeletons` | `vector<shared_ptr<SkeletonResource>>` | Skeletal data |
| `animationClips` | `vector<shared_ptr<AnimationClipResource>>` | Animation clips |
| `lights` | `vector<LightDescriptor>` | Punctual lights, in **photometric units** (see below) |
| `cameras` | `vector<CameraDescriptor>` | Authored camera viewpoints — **data only** |
| `instanceSets` | `vector<InstanceSetDescriptor>` | One renderable drawn N times — see below |
| `nodes` | `vector<NodeDescriptor>` | Format-agnostic hierarchy (name, localFrame, meshIndex, lightIndex, cameraIndex, childIndices) |
| `rootNodeIndices` | `vector<size_t>` | Root node indices |
| `skinJointNodeIndices` | `unordered_set<size_t>` | Joint nodes to skip in scene building |

#### Lights — the unit contract (added 2026-08-08)

> [!IMPORTANT]
> `LightDescriptor::intensity` carries the unit **the engine itself uses**, which is also what
> glTF `KHR_lights_punctual` specifies, so the glTF path applies **no unit conversion**. ⚠️⚠️ **But
> the colour does NOT mean the same (2026-09-25)**: KHR's intensity is the light's WHITE-equivalent
> and its colour multiplies it (a grey light is dimmer — `PointLightIntensityTest`), while the
> engine's light colour is a unit-luminance chromaticity. `SceneDataConsumer::attachLight()` folds
> `color.luminance()` into the intensity (the culling radius keeps deriving from the white-equivalent
> intensity), so an asset renders exactly as in the Khronos viewer:
>
> | Type | Unit | Engine setter |
> |------|------|---------------|
> | `Directional` | **lux** (illuminance) | `DirectionalLight::setIlluminance()` |
> | `Point` | **candela** (luminous intensity) | `PointLight::setIntensity()` |
> | `Spot` | **candela** | `SpotLight::setIntensity()` |
>
> A loader whose source format uses another unit MUST convert **once, here**. A descriptor whose
> unit depended on the producing format would defeat the point of a format-agnostic contract.
>
> Cone angles are stored in **DEGREES** (glTF authors radians — `GLTFLoader` converts).
> `range` is a **culling bound, never a dimmer**: the falloff is carried by the inverse square
> (`Graphics/Effects/Shared/LightFalloffGLSL.hpp`; it was lost from 2026-08-12 to 2026-09-25, when the range WAS
> the dimmer).
> `0.0F` means the asset declared none, and the engine default is left alone.
>
> **The node carries the AIM**: a directional or spot light shines along its node's local -Z
> (`KHR_lights_punctual`, UsdLux). The frame in `NodeDescriptor::localFrame` already holds that
> orientation; `SceneDataConsumer` reads its forward vector (`useDirectionVector(true)`) — it used
> to aim every directional light from its position toward the origin (2026-09-13).
>
> ⚠️⚠️ **Exporters ship lights WITHOUT energy.** Intel's Sponza 2022 glTF (3ds Max → Babylon.js
> exporter) declares 24 punctual lights, all with `intensity: 0` — the positions, colours and the
> sun's orientation are right, the photometry is gone, and neither the USD (no light at all) nor the
> FBX (`Null` nodes) of the same package carries it. The consumer refuses a light with no energy
> (one warning per light); the values belong IN the asset: `tools/gltf-lights.py FILE` lists them,
> `--set SUN=100000 --set 'lamp_light_*=100'` writes them (GLB binary chunk streamed, JSON rewritten).

#### Instance sets — redundancy is a HINT, not a draw order (added 2026-08-09)

`InstanceSetDescriptor` = `{ name, instances (vector<CartesianFrame<float>>), meshIndex }`.

It states the INTENT — *the same renderable, N times, here* — never the encoding. USD carries it
as a `PointInstancer`, glTF as `EXT_mesh_gpu_instancing`, FBX as duplicated nodes a loader may
fold. **Not a USD tax**: one consumer path serves every format.

> [!IMPORTANT]
> **How the instances reach the GPU is the CONSUMER's call**, because only the scene knows its own
> culling machinery. `Scenes::SceneDataConsumer` splits each set into spatial cells via
> `buildInstanceClusters()` (cell size through `setInstanceCellSize()`, default 32 units), so the
> rendering octree culls whole cells with no new culling path. A loader deciding that would be
> re-implementing the renderer.

> [!WARNING]
> **The mesh a set points at must NOT appear in `nodes`.** A prototype exists to be instanced;
> giving it a node draws one stray copy at the asset's origin — the classic instancing bug, and it
> looks like a random object floating in the scene rather than a loader mistake.

> [!WARNING]
> **Frames live in the same space as the meshes of the same `SceneData`.** A loader baking its own
> axis conversion into vertices MUST bake the very same one into these frames — and for a
> TRANSFORM that bake is a **conjugation** `C·T·C⁻¹`, never a permutation of the position. Get it
> wrong and every instance sits in the right place, rotated wrong.

> [!CAUTION]
> An asset can hold **no drawable node at all** and still be complete — every `PI_*.usd` element
> of Jungle Ruins is pure instances. `SceneDataConsumer::build()` returned early on an empty node
> table and would have dropped them while reporting success. It now checks both collections.

#### Cameras are data, never instantiated

An authored camera is a viewpoint, not a game camera (owner decision, 2026-08-08). The consumer
never turns one into a `Component::Camera` on its own — the caller decides. `yFieldOfView` is in
**degrees**; feed it to `Camera::setFieldOfView()`, which derives the focal length through the
camera's own sensor height, so the framing stays a lens.

Helper methods:
- `isSingleMesh()` — true if exactly one node has a mesh (skeleton joints don't count)
- `singleMeshNodeIndex()` — index of the single mesh-bearing node

**`MeshDescriptor::lightingEnabled`** (`bool`, default `true`, booleans last in the struct layout) declares whether the consumer must put this mesh on the **LIT path**. Default `true` means glTF and FBX behaviour is unchanged — a mesh coming from a lit format expects the light set, the ambient pass and the environment IBL. A loader that bakes its own lighting into the vertex colors on unlit materials — `WADLoader` is the reference case — sets it to `false`. `Scenes::SceneDataConsumer` previously called `visual.getRenderableInstance()->enableLighting()` **unconditionally at five sites**; all five now honor the descriptor's flag through the new `Graphics::RenderableInstance::Abstract::setLightingState(bool)`, the symmetric form of `enableLighting()`. It is implemented with `enableFlag`/`disableFlag` because `Base::FlagTrait< uint32_t >` offers no `setFlag(flag, state)` — and adding one to emeraude-base is barred by the *"Ave robustus!"* feature freeze.

### Two Consumption Paths

```
SceneData
    ├── Scenes::SceneDataConsumer  → Scene hierarchy (Nodes / StaticEntities)
    └── SimpleMeshResource::load(path) / MeshResource::load(path)  → Single mesh resource
```
