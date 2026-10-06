# Deferred punctual lights — one resolve instead of one forward pass per light (Oct 2026)

`Graphics::DeferredLightResolve` (`src/Graphics/DeferredLightResolve.{hpp,cpp}`) shades the **unshadowed,
unprojected point and spot lights** of the eligible materials in ONE fullscreen pass from the G-buffer. The forward
path draws every lit batch once per light that touches it (`Scene::renderLightedSelection()`), and on Sponza with
its 22 lamps that cost 36 of the 45 ms of `ScenePass` — ≈ 87 % of it GEOMETRY re-submission, not fill (three OS
measured, engine `docs/todo/mrt-single-pass-deferred.md` § Measured cost). Owner decision 2026-10-04, design
decisions 1-7 in the same item.

## The contract — three parties, one frame

| Party | Code | What it does |
|---|---|---|
| The light predicate | `Scenes::LightSet::isDeferredPunctualLight()` | enabled, no colour projection, no ACTIVE shadow (global switch AND casting flag AND shadow descriptor set). A shadowed light stays forward for every instance. |
| The snapshot | `DeferredLightResolve::prepare()` (Renderer, once per frame, before the scene pass) | applies the predicate ONCE, uploads the lights in VIEW space from their **published block** of the frame's render state (`AbstractLightEmitter::publishedBlock()`, the very bytes the forward UBO receives), sorts the pointers. |
| The forward skip | `Scene::renderOpaque(…, deferredLights)` → `renderLightedSelection()` | skips the point/spot passes of the snapshot's lights, for the batches whose material answers `Material::Interface::deferredLightingEligible()`. OPAQUE lists only: a translucent list is drawn after the resolve, it keeps every pass. |
| The material bit | `Saphir::LightGenerator::declareDeferredLightingCondition()` | the ambient program writes bit 0 of the material-properties R LOW nibble under the GPU half of the material's condition; the resolve shades only those pixels. |

⚠️ The CPU half and the GPU half of a material's eligibility MUST agree every frame: disagreeing counts a lamp twice
or not at all. `StandardResource` keeps them identical by construction:

- the STRUCTURE (lit, opaque, no clear coat / sheen / subsurface / anisotropy / iridescence / transmission /
  refraction / KHR specular map / shore foam — some switched on by a value at program generation) is evaluated in
  `setupLightGenerator()` and memorised in `m_deferredLightingCompiled`: the CPU reads what the compiled program
  writes, never what the parameters say later;
- the PARAMETERS (IOR 1.5, neutral `specularFactor` and `specularColorFactor`: F0 = 0.04 exactly) are tested by the
  shader on the uniform block and by the CPU on the same `m_materialProperties` floats.

A material class that does not override `deferredLightingEligible()` (the default: false) keeps every forward pass —
a new material type is safe by default.

## The frame

```
ScenePass (CLEAR pass)  : opaque — ambient, sun, the NON-deferred lights' passes
  end render pass
  barrier               : normals / material properties / albedo → SHADER_READ_ONLY, depth → DEPTH_STENCIL_READ_ONLY
  DeferredLights        : own render pass (scene colour, LOAD, additive ONE/ONE, RGB only), fullscreen triangle
  barrier               : back to attachment layouts + a global memory barrier (colour, velocity, reactive)
  RESUME pass (LOAD)    : translucent, TBN debug
```

- The split happens only when the snapshot is not empty: a scene without an eligible light records the single pass
  exactly as before, and never compiles the resolve pipeline.
- **The RESUME pass is NOT the post-process LOAD variant.** `SceneRenderTarget::buildScenePass(renderer, true)` is the
  CLEAR pass with LOAD operations and attachment-layout initial layouts, otherwise IDENTICAL — no subpass dependency.
  Vulkan render-pass compatibility ignores only load/store operations and layouts; the post-process variant carries
  two dependencies, and drawing the translucent objects' pipelines (sealed against the CLEAR pass) in it raised
  `VUID-vkCmdDrawIndexed-renderPass-02684` ("dependencyCount is incompatible … 2 != 0", 20 per run, 2026-10-04).
  Its synchronisation is therefore the resolve's own barriers.
- `prepare()` makes every resource the frame needs BEFORE the scene skips a single pass; `record()` cannot fail
  afterwards (a call-order defect returns false, logged nowhere because it cannot happen in the renderer's order).

## The frame's selection — invisible, resolved, forward (2026-10-06)

`prepare()` sorts every eligible light, from its published block, into ONE of three sets (owner decision 2026-10-06,
asked by the `labyrinth` demo: 548 ceiling lamps).

| Set | Rule | Drawn by |
|---|---|---|
| INVISIBLE | radius > 0 and its sphere misses the main camera's frustum (`Frustum::isSeeing(Sphere)`, the frame's state) | nobody: `invisibleLights()`, skipped by `Scene::renderOpaque()` for EVERY opaque batch of that target, whatever its material |
| RESOLVED | the `MaxLights` (1024 since the tiled culling; 128 before) visible ones closest to the camera, by `max(0, distance - radius)`; an unbounded light (radius 0) ranks by its distance | the resolve, tile-culled (§ Tiled culling) |
| FORWARD | the visible ones beyond 1024 | the forward passes, as before (same BRDF) |

- The INVISIBLE skip is exact: the G-buffer holds only what the frustum sees, so a light whose reach misses the
  frustum lights no visible pixel. Light-volume culling of deferred shading (S. Hargreaves, "Deferred Shading",
  GDC 2004; O. Shishkovtsov, GPU Gems 2 ch. 9, 2005). An unbounded light is never invisible.
- It applies to the opaque lists of the MAIN scene target only — the one `prepare()` was given. Cubemaps, render to
  textures and the translucent lists keep every forward pass. It is passed even on a frame with nothing to resolve
  (`Renderer`: `prepare()` ran, answered false, the invisible set is still this target's).
- The 128 used to be the first 128 of the light SET's order — a `std::set` of `shared_ptr`, i.e. ADDRESS order,
  wherever the lamps were. Ties are broken by the walk order, and the kept ones are put back in that order: a scene
  under 128 visible lights fills the buffer exactly as before.
- The RT light SSBO (`LightSet::updateVideoMemory()`, `MaxRTLights` 128) ranks the same way around the main camera
  (`Scene::updateVideoMemory()` passes `mainRenderTarget()->viewMatrices().position()`), directional lights first,
  WITHOUT a frustum test (rays leave the screen). A stopgap: light sampling is the real answer (item
  `rt-many-lights-sampling`). No main target = no ranking, the sets' order.
- `Core.RendererService.getDeferredLightStatistics()`: eligible / outside the frustum / resolved / left to the forward
  passes, for the last prepared frame (atomics: render thread writes, console reads).

⚠️ **A frustum is not an occlusion test.** In a maze, from the `labyrinth` spawn looking north, 498 of 548 lamps are
INSIDE the frustum behind the walls: 128 resolved, 370 left to the forward passes. Measured (RTX 3070 Ti, 2880×1620,
RT lane, validation ON, same pose, 2026-10-06): `ScenePass` 31.5 ms forward-only → 18.0 ms with the resolve
(`DeferredLights` 1.6 ms); 0 VUID. The remaining forward cost was the occluded lamps — gone with the tiled culling
the same day (§ Tiled culling: 0 forward, `ScenePass` 1.25 ms).

Exactness of the INVISIBLE skip (same instance, same pose, the 55 flickering lamps switched off, scene effects
bypassed; 493 eligible, 46 invisible, 128 resolved, 319 forward): deferred/deferred captures bit-identical; deferred vs
forward-only (which draws all 493) 1.41 % of the pixels > 2 levels, 0.045 % > 32, frame energy ratio 1.00005, forward
BRIGHTER in 99.9 % of the > 32-level pixels — the forward depth-bias leak documented above, not a dropped light (a
wrongly skipped lamp would DARKEN the deferred frame over a whole pool). `lighten-marbles` at launch: 33 eligible, 23
invisible, 10 resolved, 0 forward, 0 VUID.

macOS (macOS-PA, Apple M2, MoltenVK, 2560×1440, validation ON, 2026-10-06, engine 5a8fe394): same selection at the
`labyrinth` spawn (548 / 50 / 128 / 370); `ScenePass` 108.7 ms forward-only → 81.3 ms (−25 %, Linux −43 %),
`DeferredLights` 10.3 ms, frame 149.8 ms; `lighten-marbles` 0 forward; 0 VUID, 0 UNASSIGNED. ⚠️ Its profiler lists
`DeferredLights` under `FinalComposite` (item `gpu-profiler-deferred-lights-scope-misparented`).

Windows (Windows-PA, 1280×720, validation ON, 2026-10-06): same selection on both GPUs; `ScenePass` RTX 3060 Laptop
21.9 → 14.2 ms (−35 %), AMD iGPU 69.1 → 48.8 ms (−30 %); `lighten-marbles` never sends a light forward; 0 VUID,
0 MSVC warning.

## Tiled culling — the occluded lamps (2026-10-06)

Owner decisions 2026-10-06 (item `deferred-resolve-occluded-lights-go-forward`, closed): tiled culling, `MaxLights`
128 → 1024, one BIT per light per tile (no per-tile cap, no overflow path). References: J. Andersson, "DirectX 11
Rendering in Battlefield 3", GDC 2011; A. Lauritzen, "Deferred Rendering for Current and Future Rendering Pipelines",
SIGGRAPH 2010.

- `record()` dispatches `TileCullComputeShader` before the resolve: one 16 × 16 workgroup per tile (`TileSize`). It
  reduces the tile's view-space z range over the pixels carrying the deferred-lighting bit (atomic min / max on
  order-preserving integers), builds the tile's four side planes from the frame's JITTERED inverse projection (each
  plane through two depths of a tile edge — perspective or orthographic — oriented by the tile centre), and sets a
  light's bit when its sphere meets the planes and the z range (an unbounded light: every tile). A tile without an
  eligible pixel keeps an empty mask. Masks: `LightWordCount` (32) words per tile, one device-local buffer per frame
  in flight, sized with the scene target (2.35 MB at 2880×1620); a global memory barrier hands them to the fragment.
- The resolve walks its tile's bits with `findLSB`, in light order — the same terms, in the same order, as the
  per-pixel loop minus lights that add exactly nothing. A lamp behind a wall lies beyond the tile's depth range.
- The descriptor set (bindings 1, 3, 4, 5) and the 96-byte push constants serve both stages; the cull pipeline is
  target-independent, the masks are recreated with the target (deferred destructor).
- A/B: `Core.RendererService.setDeferredLightTileCulling(0|1)` — off, every tile holds every resolved light (the
  per-pixel loop of before). **The two frames are BIT-IDENTICAL** (0 differing pixels): `labyrinth` at 2880×1620 and at
  2885×1616 (partial tiles), Sponza's launch pose — the exactness proof of the culling.

Measured (RTX 3070 Ti, validation ON, 2026-10-06), `labyrinth` spawn, 2880×1620: 548 eligible, 50 invisible, **498
resolved, 0 forward** (370 forward before); `ScenePass` **18.0 → 1.25 ms**, `DeferredLights` 0.66-0.74 ms (6.2 ms for
the same 447 lights with the culling off), frame ≈ 35 → 18.5 ms; with the 55 flickering lamps off and the scene effects
bypassed, deferred vs forward-only: 1.41 % > 2 levels, 0.045 % > 32, energy 1.00005 — the same figures as before the
tiles (the forward depth-bias leak). Sponza: 21 resolved, `DeferredLights` 0.77-1.3 ms, culling on/off bit-identical.
`lighten-marbles`: 127 eligible, 65 invisible, 62 resolved, `DeferredLights` 0.17 ms. Three window sizes, 0 VUID; MCP
conformance 1881/0, console 4864/0.

macOS (macOS-PA, Apple M2, 2560×1440, validation ON, engine 15501821, 2026-10-06): PASS, 0 VUID / UNASSIGNED / [Error];
`labyrinth` 498 resolved, 0 forward; `ScenePass` 81.3 → **5.2 ms**, `DeferredLights` 10.3 → 3.9 ms, frame 149.8 →
74.4 ms; culling on/off **bit-identical** at 2560×1440 and at 2566×1434 (partial tiles on both axes; the Mac refuses a
1923×1077 window); `DeferredLights` 5 ms on vs 39.4 ms off. Sponza: on/off bit-identical apart from a fixed cluster of
6-9 pixels that flips between two captures at the SAME setting too (the M2's pre-existing Sponza run noise);
`DeferredLights` 4.4 ms on vs 6.2 off. `lighten-marbles`: 0 forward.

Windows (Windows-PA, RTX 3060 Laptop + AMD Radeon iGPU, 1280×720, validation ON, 2026-10-06): PASS on both, 0 MSVC
warning, 0 VUID on 8 launches. `labyrinth` 498 resolved, 0 forward; `ScenePass` NVIDIA 14.2 → **0.64 ms**, AMD 48.8 →
**3.56 ms**; culling on/off **bit-identical** on both GPUs at 1280×720 and 1907×1027 (partial tiles on both axes; the
laptop refuses 1923×1077); `DeferredLights` on vs off at 1907×1027: NVIDIA 0.70-0.85 vs 7.79 ms, AMD 2.4 vs 18.1 ms.
Sponza on/off bit-identical (two outliers traced to the scene still settling / still loading, not to the culling).
**Accepted on the three OS** (2026-10-06).

## The shading — the forward pass, term for term

Read off a generated `RenderableInstancePointLightPassFragmentShader` (`Core/Graphics/Shader/EnableSourceCodeDump`):
GGX, Smith-Schlick with k = (r + 1)² / 8, Fresnel-Schlick, kD = (1 − F)(1 − m), the shared windowed inverse square
(`Effects/Shared/LightFalloffGLSL.hpp`), the spot cone with the forward pass's epsilon guard. G-buffer inputs:

| Input | Source |
|---|---|
| Position | depth × the inverse of the frame's JITTERED projection (`projectionMatrix(readStateIndex)`), view space |
| N | `normals.rgb` — ALREADY oriented toward the viewer by the ambient pass, from the geometric side |
| Roughness | `normals.a` minus the metal flag (`a ≥ 1.5 → a − 2`): the SAA-widened value (owner decision) |
| Base colour | `albedo.rgb` (RGBA8 sRGB, decoded on read) |
| Metalness | `1 − albedo.a` (the diffuse weight; an eligible surface does not transmit) |
| F0 | `mix(0.04, albedo, m)` |

⚠️ **Never re-orient N in the resolve** from the normal itself (`dot(N, V) < 0 ? -N : N`). The first version did, and
lit the mortar grooves of Sponza's walls (deferred brighter in 88 % of the big-difference pixels of the wall,
2026-10-04): a normal-map texel leaning away from a grazing view is legitimate on a front face — the forward pass
decides the side from the GEOMETRIC normal (`LightGenerator.PBR.cpp`, `docs/caution-points.md` § Two-sided normals).

## Measured (RTX 3070 Ti, 2880×1620, Sponza launch pose, 2026-10-04)

| | forward | deferred |
|---|---|---|
| `ScenePass`, every scene effect bypassed | 45.5 ms | 13.8 ms (of which `DeferredLights` 0.64 ms) |
| Full frame, RT lane, everything on | 111 ms (9 FPS) | 80 ms (13 FPS) |
| `VUID-` with `VK_LAYER_KHRONOS_validation` | 0 | 0 |

Fidelity, NIGHT (`--demo-options 1,60`: the 22 lamps alone light 95 % of the pixels; pinned f/2.8 1/30 ISO 3200;
same instance, `Core.RendererService.setDeferredPunctualLights(0|1)`; two deferred captures bit-identical):

| Configuration | pixels > 2 levels | > 32 levels | lamp energy deferred / forward |
|---|---|---|---|
| As shipped (SAA roughness) | 7.9 % | 0.50 % | 0.985 |
| G-buffer roughness raw (experiment) | 5.0 % | 0.47 % | 0.987 |
| Raw roughness + forward slope bias 0 (experiment) | 0.2 % | 0.02 % | 0.9997 |

The remainder is NOT the resolve's: with the SAA and the forward's depth-bias leak taken out, the two paths agree to
0.03 % of the lamp energy. The leak: the forward light passes test `LEQUAL` with a slope-scaled bias of −1, so a
hidden layer within the bias of the front one ADDS its lighting — dense foliage (Sponza's ivy, its cypress) reads
brighter in forward than in deferred, which shades only the visible surface (engine item
`forward-light-pass-depth-bias-leaks-hidden-layers`).

Apple M2 (MoltenVK, 2560×1440, macOS-PA, 2026-10-04): `ScenePass` 268 → 100 ms, frame 346 → 178 ms, 0 VUID; the
split's store/load ≈ 0.2 ms on that tile-based GPU; `DeferredLights` 4.1 ms; night comparison as on Linux (deferred
brighter in only 1.4 % of the > 32-level pixels). `lighten-marbles` (127 point lights): `ScenePass` 332 → 15 ms.
Once, in one of six deferred/deferred pairs, 2669 px of speckles on stone near the right lantern (max 34 levels) —
not reproduced in the five other pairs; forward/forward and deferred/deferred otherwise differ by the same few
hundred ivy pixels on that GPU. Watch for it.

Windows (Windows-PA, 2026-10-04, 1280×720, validation on, MSVC 0 warning): RTX 3060 Laptop `ScenePass` 101 → 30 ms,
frame 130 → 59 ms (8 → 16 FPS, RT lane); AMD Radeon iGPU `ScenePass` 251 → 75 ms (effects bypassed), its frame
still ~710 ms because RTGI's trace alone is 541 ms there. 0 VUID on both, every switch, the night runs, `lighten-marbles`
and `simple-room`. Night fidelity, both GPUs within 0.04 points of each other: deferred/deferred bit-identical,
deferred vs forward 11.7 % of the pixels > 2 levels, 1.2 % > 32, lamp energy 0.953 (linear), forward brighter in
99.9 % of the > 32-level pixels — on the foliage and as thin lines along silhouettes (the depth-bias leak); the gap
is larger at 720p than at 2880×1620 (a pixel covers more foliage layers and edges, and the SAA widens more).

What was left of the forward lamp passes on Sponza (Linux 4.0 ms of 13.4) was the cypress's `LeafSpring` leaves, a
glTF `BLEND` material: translucent, forward by design. Their alpha is a leaf SILHOUETTE, not a translucency (owner,
2026-10-04): the demo now loads them as a cutout (`LoaderOptions::cutoutMaterialNames`) — opaque, deferred-lit.
`ScenePass` 13.8 → 10.2 ms (effects bypassed), frame 80 → 73.5 ms (14 FPS), 0 VUID; the cypress reads denser and
sharper (the BLEND let the background through, sorted per object), mean luminance unchanged (106.96 → 106.90). On
the M2 (macOS-PA): `ScenePass` 100 → 75.6 ms, the forward remainder 29.4 → 1.6 ms, deferred/deferred captures now
bit-identical (the per-run noise in the tree came from the translucent cypress); the cutout adds 3.4 ms to the base
pass there (a `discard` defeats the tile GPU's hidden-surface removal) and SSGI/SSR +6 ms (they now see the tree).
The lantern speckle stays a watch item: it was on STONE, not in the tree. Windows (Windows-PA): deferred
`ScenePass` 31.4 → 23.3 ms on the RTX 3060 Laptop (−26 %), ~75.5 → 52.5 ms on the AMD iGPU (−30 %) at 1280×720,
0 VUID on both; the cypress's foliage coverage rises in every height band (top band 10 → 28 %), none loses any.

⚠️ In DAYLIGHT the 22 lamps add almost nothing a camera sees (0.01 % of the pixels gain more than 4 levels with the
sun off and the sky's ambient on): compare at night, or the comparison proves nothing.

## A/B and diagnosis

- `Core/Graphics/DeferredPunctualLights/Enabled` (launch, default true), `Core.RendererService.setDeferredPunctualLights(0|1)` (live).
- GPU profiler: the `DeferredLights` scope inside `ScenePass` (`Core/Graphics/GPUProfiler/Enabled`).
- `Core.RendererService.getDeferredLightStatistics()`: what the last frame did with the eligible lights (§ The frame's
  selection).
- `Core.RendererService.setDeferredLightTileCulling(0|1)`: the tile culling's exactness A/B (§ Tiled culling).

## Not yet

Line lights (LTC) and the sun stay forward (the sun is the next step toward ONE geometry pass). Tiled light culling
is DONE (§ Tiled culling). See the item `mrt-single-pass-deferred`.
