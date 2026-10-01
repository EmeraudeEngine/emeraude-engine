## Implemented Loaders

### GLTFLoader

Loads glTF 2.0 / GLB files. Uses `fastgltf` library (vendored, static).

**6-Phase Pipeline:**

| Phase | Method | Output |
|-------|--------|--------|
| 1 | `loadImages()` | `ImageResource` **or** `CompressedImageResource` (KTX2) |
| 2 | `loadMaterials()` | `Material::StandardResource` + `Texture2D` on-demand |
| 3 | `loadMeshes()` | `IndexedVertexResource` + `SimpleMeshResource`/`MeshResource` |
| 4 | `loadSkins()` | `SkeletonResource` + `Skin` |
| 5 | `loadAnimations()` | `AnimationClipResource` |
| 6 | `buildNodeDescriptors()` | `NodeDescriptor` hierarchy in `SceneData` |

Phases 4-5 skipped when `skipSkinning = true`.

**Resource naming:** `glTF:{stem}/{Category}/{name}-{index}` (e.g., `glTF:Fox/Mesh/fox1-0`; an
unnamed item keeps the bare `glTF:{stem}/{Category}/{index}`). Textures add `-srgb` / `-data`.
⚠️ The trailing index is **load-bearing**, not decoration — see *The resource key* above.

#### Skins: a joint's world matrix includes its NON-JOINT ancestors (fixed 2026-10-01)

> [!CAUTION]
> glTF skins a vertex with `Σ w · (J_global · IBM) · v`. `J_global` includes EVERY ancestor of the joint, joints or
> not, and the transform of the node that holds the skinned mesh is ignored. The engine places the mesh at that node
> (the scene graph). Until 2026-10-01 the skeleton composed only the joint chain, from a root with `NoParent`, so a
> transformed non-joint ancestor was lost. BrainStem has node 0 (+90° about X) → node 21 → node 2 (−90° about X, a
> channel targets it) → the root joint, and its mesh node is a child of node 0. It stood upright at rest (no skinning
> applies) and fell flat as soon as its clip played. Measured against the spec: off by 1.0 at rest and by 3.3 mid-clip;
> 0.0 since.

`skeletonLayout()` (owner ruling: "ancestors as joints"):

- **The common ancestor.** It is the lowest node above the first mesh node using the skin that every joint reaches
  walking up (`NoParent` when the scene root is above them all).
- **Carried nodes.** The non-joint nodes between the joints and that ancestor (exclusive) become skeleton joints that
  skin no vertex. Their inverse bind matrix is the identity.
- **Order.** The joints and carried nodes are sorted parents first by depth. A skin with nothing to carry (every corpus
  asset but BrainStem, Fox and RiggedFigure) keeps exactly the order of `parentFirstJointOrder()`.
- **Channels.** `loadAnimations()` maps a carried node's channels to the skeletal clip. It also keeps them in the node
  clip, for any plain content under the same node: the skinned mesh is never under a carried node.
- **The root transform.** `Skin::setRootTransform()` holds `(mesh node world)⁻¹ · (common ancestor world)`, at rest,
  per mesh. `SkeletalAnimator::computeWorldMatrices()` premultiplies it into every root joint, which brings the
  skeleton into the mesh node's space. An animated node between the mesh node and the common ancestor is not followed
  (a warning names it). A mesh used by nodes with different transforms keeps the first node's (a warning).
- **Proof.** Fox and RiggedFigure carry one node each, yet their matrices are unchanged (old and new both equal the
  spec, 0.0): their mesh hangs under that same node, and the root transform cancels it. CesiumMan and SimpleSkin carry
  nothing.

#### CUBICSPLINE — the output accessor has a STRIDE OF THREE (fixed Aug 2026)

> [!CAUTION]
> A glTF `CUBICSPLINE` sampler packs **three values per keyframe** in its output accessor —
> in-tangent, value, out-tangent — where `STEP` and `LINEAR` pack one. The loader used to read
> that accessor **flat** and index it against the timestamps, so the leading tangents became
> keyframe values and every cubic clip played wrong. The interpolation mode was mapped correctly,
> which is exactly what made it invisible: nothing was missing, everything was shifted.
>
> The fix is the `valuesPerKeyFrame` stride in `loadAnimations()`, plus the in/out tangents now
> being stored in the keyframes. **Test any change here against a CUBICSPLINE asset** — a LINEAR
> one cannot distinguish the two code paths.

**The mode is now delivered end to end.** `EmEn::Base::Animation` already carried both halves —
`VectorKeyFrame`/`QuaternionKeyFrame` in/out tangent storage and `Math::cubicSplineInterpolation()`
(GLTF Hermite basis, tangents scaled by the segment duration inside the evaluator), with unit tests
whose own comment noted the mode was *"declared but undeliverable"*. What was missing was the
consumer: `Animations::SkeletalAnimator` only ever branched on `Step` and fell through to
lerp/slerp. It now has a `CubicSpline` branch in **both** `sampleVectorChannel()` and
`sampleQuaternionChannel()`.

> [!WARNING]
> **A cubic rotation is evaluated COMPONENT-WISE, so its result is not unit length** and is
> normalized before it reaches the joint matrix. Skipping that normalization scales the joint's
> whole subtree — a rig that stretches on the segments between keyframes and snaps back on them.

#### Compressed glTF — the three extensions come as ONE package (Aug 2026)

A "compressed glTF" produced by **glTF-Transform** or **gltfpack** does not use one extension,
it uses three, and it lists all three in `extensionsRequired`:

| Extension | What it changes | Where it is handled |
|---|---|---|
| `KHR_texture_basisu` | images become **KTX2** containers (Basis UASTC or ETC1S) | `loadImages()` → `Graphics::KTX2Decoder` → `CompressedImageResource` |
| `EXT_meshopt_compression` | buffer views hold **meshopt-encoded blocks** | `MeshoptBufferCache` (in `GLTFLoader.cpp`), via a fastgltf `BufferDataAdapter` |
| `KHR_mesh_quantization` | attributes become **normalised integers** | nothing to do — see below |

> [!CAUTION]
> **There is no partial support to fall back on.** fastgltf validates `extensionsRequired`
> against the mask given to its `Parser`, and rejects the **whole file** with
> `Error::MissingExtensions` if a single one is missing. Before Aug 2026 none of the three were
> declared, so `Sponza.ktx2.glb` did not load *at all* — not "loaded untextured", not "loaded with
> broken geometry": zero nodes, one error line. If you add a compressed asset and the loader
> refuses it, check the extension mask in `load()` first.

**`KHR_mesh_quantization` needs no code.** fastgltf dequantises normalised integers on read
(`getAccessorComponentAt` honours `accessor.normalized`), and the compensating scale is carried by
the **node transforms** that `extractFrameFromNode()` already reads — glTF-Transform emits a
per-node uniform scale (8.133 on Sponza's `arch_stones_01`) that turns the `[-1,1]` quantised box
back into world units. The extension is declared on the parser purely so the file is accepted.

**`EXT_meshopt_compression` — fastgltf parses it but deliberately does not decode it.**
`BufferView::meshoptCompression` carries the metadata; running the meshoptimizer codec is the
loader's job. `MeshoptBufferCache` does it **lazily, and caches**: a compressed asset interleaves
several attributes into one view, so a dozen accessors read the same block and decoding per
accessor would redo the same work over and over. All ten `iterateAccessor` call sites take the
`MeshoptBufferAdapter` as their fourth argument.

> [!WARNING]
> **A missed call site does not error — it reads the encoded bytes as if they were vertices.**
> The default adapter is silently substituted when the argument is omitted, and the result is
> garbage geometry, not a diagnostic. If you add an accessor read to this loader, pass the adapter.

> [!NOTE]
> The cache is the load's memory high-water mark (~300 MiB decoded on Sponza, from ~99 MiB
> encoded). `load()` releases it right after the geometry is built and logs how much it dropped.

**KTX2 stays block-compressed from disk to VkImage.** `KTX2Decoder` transcodes UASTC/ETC1S to
**BC7** and the mip chain is uploaded verbatim — no decode to pixels, no `TextureCompressor` pass,
no `TextureCache` round-trip. The transcode runs on the **thread pool** (the container creation
function is enqueued), so the asset's images transcode in parallel. On a device without
`textureCompressionBC` the loader transcodes to RGBA8 into a plain `ImageResource` instead: correct,
but it forfeits the entire benefit — this is a safety net, not a supported target.

> [!CAUTION]
> **`KHR_texture_basisu` hangs the image off `Texture::basisuImageIndex`, and such a texture has
> NO plain `imageIndex` at all.** A loader reading only `imageIndex` does not degrade gracefully on
> a KTX2 asset: *every* material comes out untextured, with no error. `resolveTexture()` falls back
> from one to the other.

#### KHR_draco_mesh_compression — the OTHER compression family (added 2026-09-18)

Draco is the second, **independent** compression family: an asset uses it *instead of*
`EXT_meshopt_compression`, not alongside it. It is what Sketchfab, older Blender exports and most
`gltf-pipeline` output carry. `KHR_texture_basisu` may still accompany it (`CarConcept` does both).

| | `EXT_meshopt_compression` | `KHR_draco_mesh_compression` |
|---|---|---|
| Granularity | **buffer view** | **primitive** (attributes **and** indices, one bitstream) |
| Accessor | keeps its `bufferView` | **has NO `bufferView` at all** |
| Interception | a fastgltf `BufferDataAdapter` — transparent | **explicit, at every read site** |
| Decoder | `MeshoptBufferCache` | `DracoPrimitiveCache` |
| Lossy | no | **yes** (quantisation) |

> [!CAUTION]
> **THE MESHOPT DESIGN DOES NOT TRANSPOSE, and reusing it is a dead end.** fastgltf only calls a
> `BufferDataAdapter` once it has resolved a `bufferView`. A Draco accessor has none, so the
> adapter is *never consulted* — the interception has to happen at each read site instead. That is
> what `readDracoAwareAttribute()` / `readDracoAwareIndices()` (in `GLTFLoader.cpp`) exist for, and
> all nine read sites of `loadMeshes()` go through them.

> [!CAUTION]
> **A MISSED READ SITE PRODUCES SILENT ZEROS, NOT AN ERROR — this is the trap of this extension.**
> glTF § 5.1.1 says an accessor with no `bufferView` "MUST be initialized with zeros; extensions
> MAY override". `fastgltf::iterateAccessor` implements exactly that (`tools.hpp:746`): it emits
> `count` zero-initialised elements, no error, no log. Since **every** accessor of a Draco
> primitive is bufferView-less, a forgotten site yields a mesh collapsed onto the origin, silently.
> Declaring the extension on the `Parser` is what turns "the file is refused" into "the file is
> read", so the failure mode moves from loud to silent the moment support is added.
> **Hence the hard guard**: a bufferView-less accessor that Draco did not claim fails the mesh with
> an error naming the attribute. Never soften it into a warning.

**fastgltf parses, does not decode** — same division of labour as meshopt.
`Primitive::dracoCompression` carries the buffer view and the attribute map; `DracoPrimitiveCache`
runs the codec, **lazily and cached per buffer view** (one bitstream holds every attribute *and*
the indices of its primitive, so the nine read sites must not decode it nine times).

> [!WARNING]
> **`Attribute::accessorIndex` does NOT hold an accessor index inside `dracoCompression`.**
> fastgltf reuses its generic `Attribute` struct to carry the extension's map, whose values are
> Draco **attribute unique ids** (`GetAttributeByUniqueId`). The field name lies in this context,
> and indexing `asset.accessors` with it reads an unrelated accessor. The ids are also **per
> primitive** — `MorphPrimitivesTest` gives `POSITION:1/NORMAL:0` on one primitive and
> `POSITION:0/NORMAL:1` on the next.

> [!WARNING]
> **A Draco attribute is a value table plus a point → value mapping**, so `mapped_index()` is not
> decoration: reading the table in point order scrambles every attribute with fewer values than
> points (a hard edge splits UVs but not positions, and the two tables then differ in length).

**A primitive may be MIXED.** The extension covers the primitive's own attributes and its indices —
nothing else. Anything else it declares stays an ordinary accessor and takes the plain branch;
morph targets are the case that exists in the corpus (`MorphPrimitivesTest` has bufferView-bearing
target accessors next to bufferView-less attribute accessors). This loader reads no morph target
anyway (see *Known gaps*), so today that branch only matters for the guard.

**Three consistency checks**, because each fails silently otherwise: decoded point count must equal
`accessor.count` (the vertex array is pre-allocated from the accessor), face count × 3 must equal
the index count, and a disagreement on `normalized` between the bitstream and the glTF accessor is
reported — the accessor is the authority in glTF, Draco carries its own flag, and a mismatch would
scale an attribute by 255. **No corpus asset exercises that last one** (every Draco-compressed
attribute there is plain `f32`, or `u16` for `JOINTS_0` and the indices), so it is unverified.

> [!NOTE]
> **Draco is LOSSY** — do not expect bit-equality against the uncompressed variant, except on
> geometry whose quantised positions land exactly — `Box` usually comes out at 0 of 2 073 600
> pixels, but an intermittent single-LSB residual on ~0.44 % of them (max **1**/255) appears from
> run to run, so one LSB is the CAPTURE's floor and never a codec signal.
> Measured 2026-09-18 at the same viewer framing: `Avocado` 10.3 % of pixels differ, mean
> **0.18/255**; `CesiumMan` 3.3 %, mean **0.11/255** — a sparse scatter along silhouettes and
> contours, background strictly identical. That shape (tiny mean, high local maxima, edges only) is
> the quantisation signature; a **region** or a **block** of difference is not, and means a defect.

**Verified 2026-09-18** over the whole corpus, Vulkan validation layers ON: **16 of 17** Draco
variants decode, 0 error, 0 VUID — including `BrainStem` (59 bitstreams), `CarConcept`
(109, **KTX2 + Draco**), `VirtualCity` (167), and the skinned set (`CesiumMan`, `RiggedFigure`,
`RiggedSimple`, `BrainStem`) whose `JOINTS_0`/`WEIGHTS_0` go through the decoder.
`SunglassesKhronos` is the seventeenth and fails **for an unrelated reason**: it requires
`EXT_texture_webp`, which is not in the parser mask (item
`docs/todo/gltf-ext-texture-webp-not-in-parser-mask.md`).

**Those sixteen are now BENCH ROWS, not a one-off sweep.** `tools/gltf-conformance-bench` captures
each of them twice — plain and Draco, at an **identical framing** — and reports the per-pixel delta
under `dracoDelta` (`DRACO_MODELS` in `bench.py`, `--no-draco` to skip). Full run 2026-09-18: 64
captures, 0 error, 0 entity mismatch, worst mean delta **0.76/255** (`VirtualCity`). ⚠️ Two rows are
**not** geometry-codec numbers and the bench now says so: `CarConcept` (PNG against KTX2) and
`SunglassesKhronos` (PNG against WebP) ship their Draco variant with a different TEXTURE encoding,
so their delta carries two codecs — the `CarConcept` figure first published that day was never a
Draco measurement. ⚠️ The A/B
reuses the **plain** variant's bounds on purpose — quantisation dilates the bounding sphere by a
constant **+0.0977 %**, so a recomputed framing measures the camera move instead of the codec. Read
[`tools/gltf-conformance-bench/README.md`](../../../../tools/gltf-conformance-bench/README.md)
§ *The compressed-variant A/B* before judging any of those numbers.

> [!NOTE]
> `GLTFLoader` logs `Decoded N Draco-compressed primitive bitstream(s)` on a successful load. It is
> a **positive** trace on purpose: decoded geometry is indistinguishable from plain geometry once
> built, so that line is the only cheap evidence the path ran rather than being skipped.

**Build**: `cmake/SetupDraco.cmake` in **emeraude-base** (owner of external dependencies), pinned to
Draco **1.5.7**, decoder only. ⚠️ Draco installs **no CMake config package** — only
`lib/pkgconfig/draco.pc` — so `find_package(draco CONFIG)` cannot work and the archive is referenced
directly, as for FastGLTF. Symbol hiding needs no entry: the sweep globs `*.a`.

#### EXT_texture_webp — an IMAGE format, owned by the foundation (added 2026-09-18)

Images are WebP containers hung off `Texture::webpImageIndex`, exactly as `KHR_texture_basisu`
hangs its own. Decoded by **`EmEn::Base::PixelFactory::FileFormatWebP`, in emeraude-base** — not
here: a WebP is plain pixels, so it is an image format of the foundation, and decoding it inside
this loader would have left `.webp` unreadable everywhere else (dropped files, data stores,
material components).

> [!CAUTION]
> **WebP is NOT KTX2, and copying the KTX2 path would be the mistake.** A KTX2 container stays
> block-compressed from disk to `VkImage` and never becomes pixels. WebP has **no GPU format**: it
> decodes to RGB/RGBA into a `Pixmap` and then takes the ordinary consumer path (CPU BC7 +
> `TextureCache`), like PNG and JPEG.

> [!CAUTION]
> **`KHR_texture_basisu` and `EXT_texture_webp` are ALTERNATIVES on the same texture, and a texture
> using either has NO plain `imageIndex` at all.** `resolveTexture()` therefore tries all three
> legs. Stopping at the first one a reader happens to know is the whole trap: every material of the
> other kind comes out silently untextured, with no error — which is why the parser mask and the
> decoder had to ship together, never one without the other.

**Verified 2026-09-18**: the owner's `SheenWoodLeatherSofa.glb` (`KHR_texture_transform` +
`EXT_texture_webp`) loads fully textured, and `SunglassesKhronos/glTF-Draco` — **WebP and Draco in
one asset**, 8 bitstreams — renders with its transmissive lenses. emeraude-base unit suite
2049/2049.

#### Known gaps (glTF 2.0)

Not a wish list — these are silent today, so a diagnosis that assumes them present starts wrong:
`TRIANGLES` is the only primitive mode read; no `TEXCOORD_1+` (no multi-UV), no `JOINTS_1/WEIGHTS_1`
(4 influences max); no morph targets; of the glTF sampler only `wrapS`/`wrapT` are read — the
filters are not, nor is the per-`TextureInfo` `texCoord` index; all of `KHR_texture_transform` is applied
(offset, scale **and rotation**) on **every** map since 2026-09-14, except its `texCoord` override, which is
the multi-UV gap;
every extension in the parser mask is now read;
⚠️ when one is nevertheless missing the loader **names** it: `reportMissingExtensions()` re-parses
with every extension fastgltf knows — the only way to recover `extensionsRequired` from a file
fastgltf refused whole — and logs the difference against its own mask, because fastgltf's own
message says only that *something* is missing;
**transmission is the last one reading only its
scalar factor, never its texture** (clearcoat's three maps and sheen's two are read since
2026-09-14, see below); animation channels targeting a node that is
not a joint of `skins[0]` go to a separate node clip (`Component::NodeAnimation`, since triad 12), and
a skin other than `skins[0]` gets no joint channel; `instanceSets` is never populated (`EXT_mesh_gpu_instancing` not enabled).

**`KHR_materials_sheen`'s TWO MAPS are read since 2026-09-14.** Sixth occurrence of "GPU ready,
loader mute" — the `Sheen` ComponentType already sampled a **vec4** and the generator already read
`.rgb` from it, and even the `SurfaceSheenRoughness` variable name was already provisioned.
- ⚠️ The roughness is the **ALPHA** channel and gets a ComponentType of its own
  (`SheenRoughness`), exactly as the two specular maps and the iridescence thickness did. Not a
  channel of the colour component: the two maps may be different images, and they do NOT share a
  colour space — the colour is **sRGB**, the roughness is linear **DATA**. `Component::Texture`
  decides that from the variable name's `Color` suffix, so `SurfaceSheenRoughness` must never be
  renamed to end in `Color`.
- ⚠️ An asset may point BOTH `sheenColorTexture` and `sheenRoughnessTexture` at the same image —
  `SheenCloth` does (index 3 for both). That works because the texture cache keeps **two
  colour-space slots per texture index**; each component gets the one its usage requires.
- ⚠️ **`SheenCloth` tiles its 256×256 maps THIRTY times in U and V** through
  `KHR_texture_transform`, and that tiling IS the weave. Served since the UV transform table became
  INDEXED (2026-09-14) — before it, no sheen component type had a slot and the sheen read at 1/30 of
  the authored frequency, smearing a low-frequency violet wash over the fabric. Measured: the
  low-frequency hue dispersion across the cloth falls from **8.51° to 2.57°** (÷3.3) at an unchanged
  mean hue (214.5° → 213.4°) — the spurious modulation goes, the fabric's colour identity stays.

**`KHR_materials_specular`'s colour factor is carried as a `Math::Vector< 3, float >` since
2026-09-14, NOT a `PixelFactory::Color`.** The extension allows `specularColorFactor` **above 1**
so the material's IOR cannot cap the specular response, and `Color`'s constructor clamps every
component to [0, 1] — right for a colour, wrong for a multiplier. `SpecularTest`'s seventh row went
from flat (9.33 / 9.35 / 9.05 / 8.87) to a monotone ramp (**10.22 / 26.42 / 47.13 / 70.95**) with
the six other rows identical to the hundredth. See `docs/caution-points.md` § *A `PixelFactory::Color`
CLAMPS to [0,1]*.

**`KHR_materials_clearcoat`'s THREE MAPS are read since 2026-09-14** — the factor map, the
roughness map and the coat normal map. Like every extension before it in this file, the GPU side was
already complete: the three `ComponentType`s (`ClearCoat`, `ClearCoatRoughness`, `ClearCoatNormal`),
their samplers, their three texture setters and their fragment-generation blocks all existed, and
only the loader's read was missing. **Fifth occurrence of "GPU ready, loader mute" in this
workstream** — read the whole path asset → loader → UBO → codegen before estimating any of these.
- ⚠️⚠️ **The roughness map is the GREEN channel**, not the red one every other scalar map in this
  engine uses; the extension says so. Its generation block hard-coded `.r` and now reads the
  component's own `sourceChannelSwizzle()`, with the loader passing `Channel::Green`. Reading red
  would work on a single-purpose texture and silently produce the wrong roughness on the packed
  factor+roughness image glTF encourages.
- ⚠️ Each map **MULTIPLIES** its scalar, which is why the factors are still read when a map is
  present, and why both generation blocks now fold in `MaterialUB(ClearCoatFactor)` /
  `MaterialUB(ClearCoatRoughness)`. The two texture setters gained the scalar as a parameter,
  defaulting to **1.0** so every caller predating the glTF wiring is bit-exact.
- ⚠️ A `clearcoatFactor` of 0 means **no coat at all**, map or no map — the gate stays on the factor.
- ⚠️ All three are DATA, never sRGB. Their `KHR_texture_transform` is applied since the transform
  table became indexed (2026-09-14); no conformance asset declares one, so nothing in the bench
  moved.
- Measured on `ClearCoatTest`: the `Roughness variations` row's `Coated` column changes on
  **23.78 %** of its pixels (max delta 184) and now carries the coating's stripes, `Partial coating`
  3.35 %, while the **`Simple coating` row — the one declaring no texture — is bit-exact (0.00 %,
  delta 0)**, which is the control this change needed.

**`KHR_materials_iridescence` is READ IN FULL and `KHR_materials_anisotropy` is READ since
2026-08-28.** Both were estimated as "a BRDF to write" and both were pure wiring — an estimate made
by reading only the loader, which is wrong in the expensive direction whenever the shading layer is
ahead of it.
- **Iridescence** was passing ONE of the four values `setIridescenceComponent()` accepts and letting
  the IOR and both film thicknesses default — while the IOR and the thickness are the two axes both
  test models SWEEP. `evalIridescence()`, a complete thin-film function, had been in
  `LightGenerator.PBR.cpp` all along. ⚠️ Thicknesses are NANOMETRES on both sides: no conversion.
  **BOTH maps are read since 2026-08-29**: the per-pixel FACTOR map (`ComponentType::Iridescence`,
  R channel) and the THICKNESS map (`ComponentType::IridescenceThickness`, **G** channel), which got
  its own component exactly as the two specular maps did. ⚠️ Reading `.r` on the thickness map would
  work on a single-purpose texture and silently produce the wrong film on a packed one — the factor
  is often the R channel of that very image. ⚠️ It is DATA, never sRGB. ⚠️ The two maps are
  INDEPENDENT: a material may declare either, both or neither, so the thickness setter is called
  after the factor one and has the last word on the two thickness slots (both setters write them).
- ⚠️⚠️ **A shader defect found while wiring it**: the film thickness read
  `mix(min, max, 0.5)` — the MIDPOINT. The extension says the thickness comes from the thickness
  texture's G channel and that **without that texture it is the MAXIMUM**. A midpoint is a different
  colour at every angle and can never match a reference.
- ⚠️⚠️ **And a second one, worse, found the same way**: the ambient pass used `mix(min, max, 0.5)`
  while the light passes used `mix(min, max, 1.0)` — **one surface carried two different films
  depending on which pass shaded it**. Both now call `LightGenerator::iridescenceThicknessExpression()`,
  a single accessor, so they cannot drift apart again. The generated shaders are the check: one
  distinct expression across every scene shader, and no hardcoded weight left anywhere.
- **Anisotropy** appeared in the loader exactly once, in the parser mask. The shader already had an
  anisotropic GGX distribution, an anisotropic Smith-GGX visibility term and the tangent-frame
  construction with rotation. ⚠️⚠️ **UNITS DIFFER AND SILENTLY: glTF's `anisotropyRotation` is in
  RADIANS, the engine's is in TURNS** (the shader computes `rotation * 2π`) and
  `setAnisotropyRotation()` **clamps to [0,1]** — so passing radians through would be wrong by 2π AND
  flattened for anything above 1 rad, a plausible-looking wrong direction. Divided by 2π here.
  The `anisotropyTexture` (RG = direction, B = strength) is supported, the shader reading exactly
  those semantics.

Measured by a partitioned pixel diff: the three models that declare these extensions changed by
**6.6 % to 9.6 % of pixels with deltas up to 239**, while `MetalRoughSpheres` came out at **exactly
0** and `WaterBottle` at delta 1. Visually both are correct in kind — the anisotropic highlight
stretches from a round blob at strength 0 into thin bands at 1, and the iridescent spheres gain a
golden rim with a violet body and magenta/cyan fringes where they were plain blue-grey metal.
⚠️ **What is NOT established is a numeric conformance criterion for either test**, and four attempts
each came out confounded: a principal-axis aspect ratio cannot measure a curved arc; a percentile
threshold cannot survive the brightness change anisotropy itself causes; a saturation average over a
crop that contains lawn measures the lawn. **The 2026-08-27 anisotropy figures in the bench item came
from the first of those metrics and must not be trusted either.** These two tests need a measurement
method, not more rendering work.
⚠️ The bench's view plan for the two iridescence grids gained a **three-quarter** view: they are 3-D
grids and a dead-on `front` collapses them into overlapping rows, which is how their failure went
unattributed for three runs.

**`KHR_materials_volume` is READ since 2026-08-28** — the absorption inside a transmissive surface,
which is what turns clear glass into coloured or thick glass. The shader already implemented the
extension's Beer-Lambert formula verbatim (`LightGenerator.PBR.cpp`:
`exp(log(attenuationColor) / attenuationDistance * thickness)`); only the read was missing.

> [!CAUTION]
> **THE SPEC'S DEFAULTS ARE NOT THE ENGINE'S, and that is the real finding of this lot.** glTF says
> `thicknessFactor` **0** — 0 means THIN-WALLED, no volume — and `attenuationDistance` **+INFINITY**,
> no absorption. `StandardResource` defaults them to **1.0** and **1.0 m**. Those engine defaults are
> harmless *today* only because `attenuationColor` also defaults to white: `log(1)` is 0, so the
> product is zero and the absorption is the identity whatever the distance. **The moment a colour is
> set without a distance, the engine invents an absorption over 1 m that the asset never asked for.**
> The glTF loader therefore states the SPEC defaults and applies all three unconditionally; the
> engine-side defaults are left alone because they belong to the JSON material format, but they
> diverge, and anything reading `DefaultAttenuationDistance` should know it is not glTF's.

**Its `thicknessTexture` is READ since 2026-08-29** (`ComponentType::VolumeThickness`, **G**
channel, DATA never sRGB). It **multiplies** `thicknessFactor` rather than replacing it, and the
result reaches BOTH consumers through `StandardResource::volumeThicknessExpression()`: Beer's law,
and the LENGTH of the refraction ray whose exit point the screen-space refraction projects. That
second consumer is why it now matters visibly — a moulded glass refracts unevenly, thick at the
base and thin at the rim.
⚠️ `SubsurfaceThickness` is a DIFFERENT quantity and is not reused for it.
⚠️ Depth-based opacity OVERRIDES the whole expression with the measured water column from the depth
grab; the map has no say there, by design.
⚠️⚠️ The component's generation must stay **above** the transmission block: both emit at
`Location::Top`, where the order is the EMISSION order, and generating it after produced
`'SurfaceVolumeThickness' : undeclared identifier` **at runtime** — the C++ compiles either way.

⚠️ Order matters: `setTransmissionComponentFromGrabPass()` also writes those three slots from its own
defaults, so the volume must be applied **after** it or it is overwritten. The thickness MAP goes
after the factor for the same reason.
⚠️ **Infinity travels through on purpose.** `setAttenuationDistance()` clamps with
`max(0.0001, value)`, which leaves `+inf` intact, and the shader's `log(colour) / inf` is 0 — no
absorption. `attenuationColor` is clamped away from zero in the shader so `log()` never returns
`-inf`, so `-inf / inf` (a NaN) cannot arise.
⚠️ **NOT verifiable on any asset in the collection**, and that was measured before writing the code:
across **60 glTF files** (the vendored Khronos set plus projet-alpha's), 15 materials declare
`KHR_materials_volume`, **2** declare `attenuationColor` and **ZERO** declare `attenuationDistance` —
so per spec every one of them has no absorption, before the wiring and after it. Verified instead on a
purpose-built probe (three transmissive spheres identical but for their declared volume, generated
into the scratchpad, not a repo asset): body/rim green-over-red ratios **1.001 / 1.005** with no
volume, **1.001 / 1.004** with a colour but NO distance — the spec's `+inf` rule, and a genuinely
counter-intuitive one — and **1.008 / 1.174** with both, peaking at **147/255** of green excess at the
rim where the optical path is longest. Zero VUID.
⚠️ The thickness MAP (G channel) is logged and ignored: there is no `ComponentType` for a volume
thickness (`SubsurfaceThickness` means something else), so it needs a new one exactly as the two
specular maps did. One asset uses it — `IridescentDishWithOlives`' `glassCover`.

**`COLOR_0` is READ since 2026-08-28**, and it MULTIPLIES the base colour (the glTF contract).
⚠️⚠️ **The material carries a `…-vc` VARIANT, and that is not a workaround.** `UseVertexColors`
changes the material's shader contract — the codegen declares a vertex INPUT ATTRIBUTE for it —
while the attribute's presence is decided by the GEOMETRY. glTF declares `COLOR_0` per **primitive**
and shares materials across primitives, so the disagreement is the NORMAL case: measured on
`Sponza.ktx2.glb`, 67 of 448 primitives declare it and **17 of the 22 colour-using materials are also
used by colourless primitives**. Setting the flag on the shared resource would make those
primitives' shaders read an attribute their geometry does not provide. So the loader creates
`<name>-<index>-vc` for exactly the materials a `COLOR_0` primitive uses, and picks it **per
primitive**. An asset without `COLOR_0` pays nothing.
⚠️ The attribute itself is **per MESH** (the shape and its VBO are), pre-filled **white** — the
neutral multiplier — so a mesh with mixed primitives is coherent: the colourless ones take the plain
material and simply ignore the attribute. An unused vertex input attribute is legal; a shader input
with no attribute is not, and that asymmetry is what makes the split safe.
⚠️ `ShapeTriangle`'s vertex-COLOUR indexes are a separate list from its vertex indexes (a
face-varying attribute, as OBJ and FBX need) and default to `{0,0,0}` — leaving them alone paints
every triangle with colour 0. glTF shares POSITION's indexing, so the two sets are simply equal.
⚠️ The accessor may be **VEC3 or VEC4** and normalised ubyte/ushort as well as float (Sponza: VEC4
ubyte-normalised; `VertexColorTest`: VEC4 float). fastgltf converts a normalised accessor to [0,1]
floats, so only the component COUNT needs dispatching — reading a VEC3 through a vec4 iterator takes
its alpha from whatever follows in the buffer.
Measured: `VertexColorTest`'s three test tiles lose the complement-coloured X that is the README's
own failure signature — Red 39.8 % red + **58.1 % cyan** → **97.4 % red, zero cyan**; Green 37.6 % +
**59.9 % magenta** → **97.4 % green, zero magenta**; Blue **60.5 % yellow** + 39.5 % → **100 % blue,
zero yellow** — while the reference tiles stay 100 % pure. Sponza loads its 136 meshes with **zero
VUID**, which is the real proof the variant split is right: a material/geometry mismatch would fail
the vertex input state on those 17 shared materials.
⚠️ **The architecture is still upside down** and it is recorded as such:
[`docs/todo/vertex-attribute-presence-belongs-to-geometry.md`](../../../todo/vertex-attribute-presence-belongs-to-geometry.md).
The next optional attribute (`TEXCOORD_1+`, a second joint set) will hit exactly this wall.

**Authored `TANGENT` is READ since 2026-08-28** (it was ignored and always recomputed, and the
bitangent handedness did not exist). glTF's `TANGENT` is a **vec4** whose W is the bitangent
handedness (±1): the bitangent is `cross(normal, tangent) * w`, and that sign is the ONLY thing
distinguishing a mirrored UV island. `ShapeVertex` gained a handedness member in emeraude-base
(neutral +1, so every other loader and every generated shape is a **bit-exact no-op**), and the
loader now skips its own tangent computation when the asset authored them — recomputing over
authored data discards the mirroring, since the engine's derivation produces only the tangent and
leaves `biNormal()` assuming +1.
⚠️ **It is ALL-OR-NOTHING per mesh**, deliberately: the computation runs over the whole shape and
would overwrite the authored tangents of the primitives that did supply them. A mesh where only
some primitives carry `TANGENT` recomputes every one and logs it — keeping half authored and half
computed makes the two disagree at the seam.
⚠️⚠️ **`sizeof(ShapeVertex)` went 80 → 84 and `FileFormatNative` writes vertices as a RAW BLOB**,
so the native format version was bumped to 2 with no v1 read path (the format had no users; owner
decision 2026-08-28). A size change with an unchanged version is silent corruption.
Measured on `NormalTangentMirrorTest`, highlight angle per column over five roughnesses: the two
mirrored columns go from a circular spread of **107.7°** and **102.9°** — deviating −115.3° and
−110.8° from the `Geometry` reference — to **1.8°** and **1.3°**, sitting at −7.8° and −12.1°, the
same family as the already-passing `Normal` column (−6.3°). The `Geometry` and `Normal` columns
come out **identical to the decimal**, which is the control: they carry no mirrored UVs and must
not move. `NormalTangentTest` (no `TANGENT` in the asset) is **bit-identical on all three views**.
The three other assets that author tangents changed and did not regress: `BoomBox` (delta 97) reads
visibly crisper — speaker grilles, button rims, label text — because the normal map finally matches
the frame it was authored against; `WaterBottle` (delta 30-54) and `SheenCloth` (delta 3-4) differ
only in micro-detail.

⚠️⚠️ **The tangent frame can be right and the TEXTURE wrong: a normal map carries no metadata about
its convention.** glTF mandates +Y = UP in the image (the "OpenGL" sign; the engine decodes exactly
that, whatever the API's UV origin — Vulkan's is top-left and it changes nothing here). Tools of the
Direct3D family (3ds Max, Unreal) bake +Y DOWN, and an exporter that does not convert ships a
spec-violating asset that renders with every relief lit from below. Intel's Sponza 2022 (Babylon.js
exporter) was one: all 27 normal maps of `Sponza.ktx2.glb` were re-encoded with the green channel
inverted (2026-09-13). **Detector:** `normal-map-debug` in projet-alpha (the texture on a white quad
under an omni of known side; option 4 is a Khronos-convention control), or the curl test
`corr(∂R/∂v, ∂G/∂u)` on the PNG — negative for Khronos, positive for D3D. **Never** compensate in the
loader or the shader: the fix is the texture.

**`KHR_materials_specular` + `KHR_materials_ior` — FACTORS WIRED 2026-08-28, textures still not.**
⚠️⚠️ Their **GPU side was already complete and spec-exact**, and had been for an unknown number of
sessions: `LightGenerator.PBR.cpp:589-590` computes
`dielectricF0 = ((ior-1)/(ior+1))²` then
`F0 = mix(min(dielectricF0 · specularColor · specularFactor, 1), albedo, metalness)`, the material
UBO carries all three slots, and `StandardResource` declares them to the `LightGenerator` for every
material. **Only the loader's read was missing**, so every asset silently got the identity.
Measured on `SpecularTest` (whose 35 spheres declare `baseColorFactor [0,0,0,1]`, `metallicFactor 0`,
`roughnessFactor 0`, leaving the extension as the only thing that can light them): its four factor
rows go from flat (axis spread ≤ 0.17 / 255) to monotone (2.36 to 3.35), the sphere declaring
`specularFactor 0` now renders **exactly 0** — the spec's F0 = 0 — and the three *texture* rows stay
**bit-identical**, which is the built-in control. On `TransmissionRoughnessTest` the IOR axis goes
from **1.46** (noise) to **4.24 / 255 and monotone** in the right direction (diamond darkest, air
brightest: more F0 means more reflection and less of the bright backdrop transmitted).
**Both TEXTURES wired too (same day).** `ComponentType::SpecularColor` was added — glTF declares
**two** maps for that one extension and the material had one slot. `specularTexture` goes to
`ComponentType::Specular` reading its **A channel** (the only scalar map in this material that is
not red — the extension says so) and `specularColorTexture` to `ComponentType::SpecularColor`
reading RGB. Each half independently falls back to its UBO scalar when its map is absent, on the
established `componentIt != cend() ? variableName() : MaterialUB(...)` pattern.
⚠️ **The sRGB split is carried by the VARIABLE NAME**, and it happens to be exactly right:
`Component::Texture` enables sRGB when the variable name ends with `Color`, so `SurfaceSpecularColor`
is decoded (glTF declares `specularColorTexture` sRGB-encoded) while `SurfaceSpecularFactor` stays
linear (the A-channel factor map must). Do not rename either without re-reading that rule.
Measured: the three *texture* rows of `SpecularTest` go from flat (0.10 / 0.10 / 0.25) to monotone
ramps (3.12 / 3.60 / 2.41), and the four *factor* rows come out **bit-identical** — the control. The
two paths agree to within **0.33 / 255** with a ratio of 1.05‥1.11 (8-bit quantisation and texture
filtering across the sphere), which is what the test exists to check: each texture row must
reproduce the factor row above it.
⚠️ **`SpecularTest` still LOOKS black, and that is no longer a loader defect.** Its spheres declare
`baseColorFactor [0,0,0,1]`, `metallicFactor 0`, `roughnessFactor 0`: a mirror-smooth dielectric
whose F0 never exceeds 0.04, in a viewer whose lower hemisphere is dark forest. Khronos shoots it on
a bright studio environment that fills the sphere. Judge it on the RATIOS — same caveat as
`EmissiveStrengthTest`, inverted.
⚠️ Both specular maps get their **`KHR_texture_transform`** since 2026-09-14 — they already called
`transformedTexCoords()` and were refused by the six-entry switch, not by the call site. No
conformance asset declares one on them.

> [!NOTE]
> Two of those gaps are **live on the compressed Sponza**, which is now the reference asset:
> `Sponza.ktx2.glb` declares `TEXCOORD_1` on 371 of its 448 primitives and `COLOR_0` on 67. Both
> are read past in silence. "The second UV set is missing" is a known gap, not a KTX2 regression.
