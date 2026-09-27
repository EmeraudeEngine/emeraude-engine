## 12. Post-Processing Effects

### SSR (Screen-Space Reflections)

> [!CAUTION]
> **The Hi-Z pyramid's mip 0 is a DOWNSAMPLE, not a copy — and it must be a MIN.** With pixel
> doubling on (`Core/Graphics/PostProcessing/Reflections/ScreenSpace/PixelDoubling`), the trace target is half-res
> while the scene depth stays full-res. `SSRHiZCopyComputeShader` used to do
> `texelFetch(srcDepth, p)` with `p` the DESTINATION texel, which copied the source's **top-left
> corner** 1:1 into the whole pyramid — `sourceMaxX/Y` only ever clamped, they never scaled. The
> march then compared its rays against depths belonging to entirely different pixels.
>
> Measured before the fix (RenderDoc, `reflexion-debug --demo-options 0,5,0`): mip 0 held the
> top-left quarter magnified 2×, **88 %** of its texels at the far plane, and the trace kept hits
> on **3.19 %** of the screen (confidence is channel **B** of the trace target, not alpha —
> `outHit = vec4(hitUV, confidence, 0.0)`). After: **41.7 %** at the far plane, matching the real
> frame, and **13.59 %** hit rate — 4.3× more.
>
> ⚠️ The reduction is a MIN because a MIN pyramid is conservative; averaging depths invents a
> surface halfway between two of them. Mip 0 is no exception. ⚠️ The defect was LATENT at full
> resolution (the engine default), where destination and source sizes coincide — it only appears
> once pixel doubling is enabled, which is why nothing caught it.

> [!CAUTION]
> **A camera-ward ray is CLIPPED to the near plane, never rejected.** The trace used to bail out
> on `reflDir.z < 0.0` ("rays toward the camera cannot be resolved against a single depth layer").
> That threw away every reflection of the geometry sitting **between the surface and the eye** —
> on a mirror sphere, the entire near floor. Only the *projection* of an endpoint that crossed
> behind the eye was ill-defined; the screen-space segment itself is perfectly marchable, so the
> ray length is now solved against the near plane instead (McGuire & Mara, *Efficient GPU
> Screen-Space Ray Tracing*, JCGT 3(4), 2014, § 3).
>
> Measured (RenderDoc, `reflexion-debug --demo-options 0,5,0`): the sphere's hits occupied
> `hitUV.y ∈ [0.111, 0.578]` with **67 %** piled into the single 0.5–0.6 bin and the lower **42 %**
> of the screen never reached once — a hard wall, not a fade. After the clip, stone in the
> sphere's lower half went **7.6 % → 13.6 %** (RTR ground truth 28.7 %), confirmed on screen.
>
> ⚠️ This makes `D.z < 0` REACHABLE in the traversal, where `tPlane`'s `1e18` branch used to be
> dead code. `1e18` is the CORRECT answer there: depth decreases along such a ray, so one already
> in front of a cell's nearest surface can never meet it, and the free-flight branch is right —
> it is not a missed refinement. ⚠️ This was **not** a Y-up residual: the rejection predates the
> flip. The owner's inversion hypothesis was reasonable and wrong, and only the hit-destination
> histogram separated the two.
>
> ⚠️ **The 0.0746 confidence ceiling that looked like a bug was the FLOOR, not the sphere.** A
> disc validated at one camera pose was reused on a capture taken at another and measured
> pavement while reporting "sphere". Select a surface by what the shader itself publishes — the
> normals attachment's packed `roughness + metalness * 2` — never by a remembered pixel disc. On
> the real sphere pixels the trace is healthy: 62.3 % hit rate, mean confidence 0.494, max 1.0.
>
> ⚠️ `reflexion-debug` is **not run-to-run deterministic**: two captures from the SAME binary
> differ on ~326 k pixels (max 194 LSB). A bit-identical control is inapplicable on this scene —
> a change can only be shown to sit BELOW that floor.

> [!NOTE]
> **The residual gap against RTR is STRUCTURAL — do not spend another session chasing it.** After
> the near-plane clip, every miss on `reflexion-debug`'s mirror sphere was attributed by a
> temporary miss-reason code in the trace target's alpha (the resolve reads only `.xy` and `.z`, so
> the instrumentation could not change a pixel — and the hit count confirmed it, 3,225 vs 3,226).
> Pose (0, 2, 9), sphere selected by the normals attachment's packed value 2.1:
>
> | share of the sphere | reason |
> |---|---|
> | 41.6 % | hit |
> | 43.3 % | the reflected ray LEAVES THE SCREEN |
> | 9.1 % | end of the ray reached with no hit — rays aimed at the sky, and since the skybox sits at the far plane a ray never gets behind it, so the cubemap fallback is the CORRECT answer |
> | 6.0 % | step budget exhausted |
>
> The dominant term is the structural limit of screen space, which is exactly what RTR does not
> suffer and what the whole 13.6 % vs 28.7 % gap measures. **The step budget is not recoverable
> either**: raising `maxSteps` 128 → 512 moved that term 6.0 % → 2.0 % yet left the hit rate at
> 41.6 % — the freed rays go on to leave the screen or reach the ray's end, never to hit. Four
> times the iterations, zero extra hits. `maxSteps{128}` stays.
>
> ⚠️ The first attempt at that comparison was INVALID and said the opposite (hits 41.6 % → 11.5 %,
> step-exhaustion 6 % → 28 %, which is arithmetically impossible when the budget grows). The
> sphere mask held 8,059 texels instead of 7,749: under `renderdoccmd` everything is slowed by
> orders of magnitude and a fixed sleep does NOT guarantee the camera has been placed. **Read the
> pose back (`Act.getPosition()`) into the capture log, and treat the mask's texel count as the
> control** — a differing count means the two captures are not comparable, whatever the numbers
> say. ⚠️ Never let a capture runner delete previous `.rdc` files either: it destroyed the
> baseline the probe had to be compared against.

> [!CAUTION]
> **`needsMaterialProperties` is a COMBINE-PASS codegen request, not "give me the texture", and
> two disjoint delivery paths exist.** A previous revision of this file claimed the `reflection`
> nibble was "written and read by NOBODY" — that was WRONG, and the mistake came from grepping the
> effects for `materialPropsTex`/`matProps` while the generated combine sampler is named
> **`emMaterialProps`**. Seven effects read it there: `SSR.cpp:1655` and `RTR.cpp:1537` decode it
> as `float(uint(texture(emMaterialProps, vUV).r * 255.0) >> 4u) / 15.0` into
> `ssrReflectivity`/`rtrReflectivity`, gate on `> 0.0`, and weight their mix by it —
> `mix(em_Color.rgb, data.rgb / confidence, confidence * intensity * reflectivity)`. SSAO, SSGI,
> RTAO, RTGI and ContactShadows read their own nibbles the same way.
>
> The two paths:
> - **Overlay effects** (`producesOverlay()`) set `CombineContribution::needsMaterialProperties`
>   and read `emMaterialProps` in their combine snippet. `CombinePass` emits the sampler, hashes
>   it into the pipeline variant key, and binds `context.materialProperties` — aborting the whole
>   combine group with a `TraceError` if it is null. This allocates nothing.
> - **Direct effects** (`AtmosphericFog`, `VeilingGlare`, `DepthOfField`) declare their own
>   `materialPropsTex` in `set = 0` at the last binding of `getInputLayout(N)`, and hand-write
>   `context.materialProperties` into their per-frame set inside `execute()`.
>
> Allocation is a THIRD, separate thing: the virtual `requiresMaterialProperties()`, OR-ed by
> `PostProcessStack` and turned into the `VK_FORMAT_R8G8B8A8_UNORM` MRT attachment by `Renderer`.
> `PostProcessor` skips any effect whose flag is set while the texture is null, so the pointer is
> guaranteed non-null inside `execute()`.

> [!CAUTION]
> **The post-process reflectivity is a UBO value behind a FLAG, never a GLSL literal.**
> `"Reflection": { "Type": "Value", "Data": x }` publishes a reflectivity for SSR/RTR without any
> cubemap or sampler. The scalar used to reach the shader as a baked literal
> (`declareSurfaceReflectivityMap("(" + std::to_string(amount) + ")")`), and **the program caches
> key on the descriptor layout and on material FLAG BITS — never on plain values**. Two materials
> differing only in that amount were one edit away from sharing a program built with the other
> one's literal. The scalar now lives in the UBO (sharing the `ReflectionAmount` slot, which this
> path never sets — the two branches of `parseReflectionComponent` are alternatives) and the
> routing in `MaterialFlagBits::PostProcessReflectivityEnabled`, which the caches DO key on.
>
> ⚠️ Same pass fixed the parse: it read `getValue(componentData, "Amount")` while
> `parseComponentBase` sets `componentData` to the **NUMBER itself** for a `Value` type, so the
> authored figure was silently replaced by the 0.5 fallback — the path had never honoured its own
> parameter. It now uses `parseValueComponent()`, the helper the Roughness component uses for the
> identical shape. **The correct JSON is `"Data": x`, not `"Amount": x`** — an earlier revision of
> this file documented the wrong key.
>
> ⚠️ **DIAGNOSED AND FIXED (2026-08-26) — it was a FLAG BIT COLLISION, and it was introduced by the
> commit above, not inherited.** `PostProcessReflectivityEnabled` was first written as `1U << 17`,
> the value `UnlitEnabled` already held. Nothing checks these values: a duplicate compiles silently,
> and `enableFlag(PostProcessReflectivityEnabled)` therefore also set `UnlitEnabled` — so declaring a
> post-process reflectivity on a material silently made it **UNLIT**.
>
> The symptom surfaced far from the cause. Authoring `"Reflection": { "Type": "Value", "Data": 0.0 }`
> on `Grounds/Pavement005` made that material's authored roughness of 0.8 read as the 0.5 default in
> the normals attachment — the 1,050,417-pixel floor group at packed 0.800 vanishing into the 0.500
> group — because the unlit codegen path does not declare roughness the same way. The material loaded
> without error and a JSON round-trip of the file was byte-identical, which is what made it look like
> a parsing mystery.
>
> ⚠️ **The mistake that produced it: "the enum" was read, but not to its closing brace.** The free-bit
> survey stopped at `AlphaTestEnabled = 1U << 16` and never reached `UnlitEnabled` further down. A
> `NEXT FREE BIT` marker now sits at the end of `MaterialFlagBits` — keep it current, and add new bits
> there.
>
> ⚠️ **And the earlier claim that the defect "predates this change" was FALSE.** It was asserted in a
> commit message without being tested: the `Value` path had never once been exercised against the
> ORIGINAL code, because the first attempt used the wrong JSON key and failed to load.
>
> Verified after moving the flag to `1U << 18`, same scene and pose: the floor's packed roughness is
> back to **0.800** (1,049,893 px) and its published reflectivity is exactly **0.0000** — which is
> also the first end-to-end validation that the `Value` path honours its authored parameter at all.

> [!CAUTION]
> **The material-properties A channel had TWO consumers and NO producer until Aug 2026.**
> `fogResponse` (high nibble) and `dofMask` (low nibble) were decoded faithfully by
> `AtmosphericFog` and `DepthOfField` — and the pack wrote a hardcoded literal `1.0` for the whole
> channel, so both nibbles were pinned at 15 forever and neither modulation did anything. A
> contract with two careful readers and no writer reads as working code in every review; the only
> way to catch it is to trace the value back to its producer.
>
> Both are now real material properties, following the pattern of the two live nibbles beside them
> (`aoResponse` from `aoIntensity`, `emissiveMask` from `autoIlluminationAmount`): continuous,
> UBO-backed, `clamp(x, 0, 1)`, authored by the root-level JSON keys `"FogResponse"` and
> `"DoFMask"` or by `setFogResponse()` / `setDoFMask()`. **Both default to 1.0**, so a manifest
> that says nothing keeps the previous behaviour exactly.
>
> ⚠️ They cost NO UBO growth: offsets 54-55 were the two STD140 padding floats before the first
> `vec4` UV transform, implicit on the GLSL side. Declaring them explicitly fills the hole without
> shifting a single later offset. There is no room left there — the next scalar needs a real
> layout change.
>
> Verified at runtime on `light-and-shadow-debug`: with the defaults the frame sits inside the
> scene's own run-to-run noise (646 k pixels / 33 LSB against a 634 k / 33 LSB floor from the same
> binary twice), and forcing the ground to `FogResponse = 0` drops it to a mean of **93.60** —
> **bit-identical to the same ground with the fog switched off entirely**.

> [!CAUTION]
> **`"Reflection": { "Type": "None" }` does NOT publish a zero reflectivity — the name is
> misleading, and the explicit opt-out is a DIFFERENT declaration.** With `None` the parse returns
> early, `m_useReflection` stays false, and `LightGenerator::materialPropertiesExpression()` falls
> through its priority ladder to `clamp(max(metalness, 1.0 - roughness), 0.0, 1.0)` — a
> **participation mask** for the traced reflections, deliberately not an energy weight (the code
> says why: a smooth DIELECTRIC — glass, metalness 0, roughness 0 — must participate, and the old
> `metalness * smoothness` product zeroed it so glass lost every traced reflection; the effects
> apply the real Fresnel and roughness fade themselves). Measured on `reflexion-debug`: the
> polished metal sphere publishes **1.0**, and `Grounds/Pavement005` — roughness 0.8,
> `Reflection: None` — publishes **0.2**, not 0.
>
> **To publish exactly zero, author `"Reflection": { "Type": "Value", "Amount": 0.0 }`.** No new
> format value is needed and none was added: `FillingType::Value` routes through
> `StandardResource::parseReflectionComponent()` to `m_postProcessReflectivityAmount`, which
> `setupLightGenerator()` hands to `declareSurfaceReflectivityMap()` — **priority 1**, the top of
> the ladder — as a GLSL literal. `ssrReflectivity`/`rtrReflectivity` then fail their `> 0.0` gate
> and the surface receives no traced reflection at all. The sentinel is `-1.0F` ("not declared"),
> so `0.0` is honoured rather than treated as absent. The other two ways to reach zero are the
> artistic-cubemap flag (`m_reflectionArtistic`) and a reflectivity map authored to zero.
>
> ⚠️ That literal is NOT keyed by the program caches, which hash descriptor layout + material FLAG
> BITS and never plain values. It is safe here only because the global key also hashes the
> **renderable's name** (`SceneRendering::computeProgramCacheKey()` point 3), so two materials with
> different `Amount`s on different renderables cannot collide. The residual hazard is narrow but
> real: swapping a material for a differently-valued one on the SAME renderable keeps the same key
> and reuses the program built with the OLD literal.

> [!NOTE]
> **The nibble quantization used to TRUNCATE — fixed Aug 2026, and it was worth a full step.** The
> pack was `uint(x * 15.0)`; it is now `uint(x * 15.0 + 0.5)` for all three live fields
> (reflectivity, aoResponse, emissiveMask; R's low nibble is hardcoded 0, G's low and B's high are
> hardcoded 15, A is a literal 1.0). Since 0.8 is not representable in binary,
> `1.0 - 0.8 = 0.19999998807907104` in float32, times 15 is `2.999999761581421`, and the bare
> `uint()` yielded **2** where the intent was 3 — the pavement published 0.1333 instead of 0.2, a
> 33 % under-report. Measured in the attachment with RenderDoc, at a verified pose (sphere mask
> bit-identical at 31,004 px): floor **0.1333 → 0.2000**, sphere **1.0 → 1.0** unchanged.
>
> ⚠️ Only EXACT values escaped the defect (metalness 1.0 gives exactly 15), which is why a mirror
> read a clean 1.0 and hid it for every other surface — a reference object that happens to sit on
> an exactly-representable value is the worst possible witness for a quantization bug.
> ⚠️ `+ 0.5` with truncation, not `round()`: GLSL leaves `round()`'s behaviour on a .5 tie
> implementation-defined. x is clamped to [0,1] upstream, so `x * 15.0 + 0.5` stays in [0.5, 15.5]
> and can never overflow the nibble.

> [!NOTE]
> **The resolve reads the trace target with `texelFetch`, not `texture`.** Channels R and G carry
> hit **coordinates**, which are not a filterable quantity. Trace and resolve are both half-res,
> so `vUV` lands on a texel centre and bilinear happened to return that texel untouched — the
> trap is latent, not active. Move the resolve to full-res with bilinear restored and every
> hit/miss boundary fabricates a UV halfway toward (0,0) carried by a non-zero confidence: the
> resolve samples the screen corner and believes it.

5-pass pipeline at half resolution (except composite at full-res):

1. **Trace**: Ray-marches in screen space using depth+normals, outputs hitUV + confidence
2. **Resolve**: Samples reflected color at hitUV; on SSR miss, falls back to environment cubemap
3. **Blur H**: Horizontal Gaussian blur on resolved colors
4. **Blur V**: Vertical Gaussian blur
5. **Composite**: Blends blurred SSR with scene color

**Cubemap Fallback** (UE4/UE5 standard approach):
When SSR ray finds no screen-space hit, the resolve pass reconstructs the reflection direction in view space, transforms to world space via inverse view matrix, and samples the environment cubemap. This eliminates black patches at screen edges.

Key design:
- Inverse view matrix (3×3 rotation) passed via push constants (3 × vec4 = 48 bytes; the whole
  `ResolvePushConstants` block is 92 bytes, under the 128-byte floor)
- `envFallbackIntensity` parameter controls fallback strength (0.0 = disabled, 0.3 = default) —
  it is the TRUST in a screen-space miss ("no information", not "sky"), multiplied by the same
  Fresnel weight and roughness fade a hit carries
- The fallback samples the ACTIVE scene's prefiltered environment (bindless reserved cube slot 2)
  and scales it by `FrameContext::skyLuminance` (push constant) — a normalized source becomes nits,
  exactly as the RTR miss path does with `ambientLight.w`
- Resolve descriptor set: 6 bindings (color, trace, depth, normals, pyramid, albedo — the effect
  declares `requiresAlbedo()`)

**The resolve output is the RTR contract — PREMULTIPLIED `(color · fresnelTint · confidence,
confidence)`, with the scalar Fresnel INSIDE the confidence (Sep 2026).** The primary-surface
Fresnel `F = F0 + (1-F0)·(1-NdotV)⁵`, `F0 = mix(vec3(0.04), albedo, metalness)`, is split like
RTR splits it: `fresnel = max(F.r, F.g, F.b)` multiplies the confidence (hit: the trace confidence;
miss: `envFallbackIntensity · roughnessFade`), and the NORMALIZED tint `F / fresnel` rides on the
color — a metal reflects in its own color, a dielectric at the physical 4 % head-on. The blur
filters that premultiplied pair; the combine divides the color by the filtered confidence and mixes
by the confidence.
> [!CAUTION]
> **The Fresnel must be in the WEIGHT, not only on the color — a previous revision of this file
> said the opposite, and it cost every participating dielectric 16 % of its shading.** From
> `529b46c4` (Aug 2026) to Sep 2026 the resolve wrote `vec4(reflColor × F, confidence)`: the color
> carried the Fresnel, the confidence did not, and the output was not premultiplied while the
> combine divides by the confidence as if it were. The mix then REMOVED `confidence × intensity ×
> reflectivity` of the pixel — ≈ 43 % on a roughness-0.5 dielectric (mask 8/15, intensity 0.8) —
> and ADDED only `F × intensity × reflectivity` ≈ 2 % of reflection. Measured on
> `global-illumination` (the default-material floor, same pose, same exposure): SSR on/off moved
> the lit floor **129.8 → 109.5 (−16 %)** while RTR moved it **149.2 → 151.8 (+2 %)**. The two
> lanes were SUBSTITUTING different fractions of the pixel for the same surface, so the
> screen-space lane read as "the floor mirrors the room" and the traced one as "it does not".
> The former justification ("any weight folded into the confidence is cancelled by the division")
> confuses the two roles: the division renormalizes the COLOR; the mix weight IS the confidence,
> and it is precisely where the lobe weight belongs. Where a hit has no Fresnel in its weight the
> mix substitutes far more of the pixel than the lobe reflects — a darkening, never a "look".
> ⚠️ The same commit made dielectrics participate at all (`max(metalness, 1-roughness)`), which is
> what exposed the defect: before it, a metalness-0 surface never reached the combine.

**Reflectivity mask (material-properties G-buffer, R high nibble) is PARTICIPATION, not energy
(Aug 2026):** materials without an explicit reflection component publish
`max(metalness, 1 - roughness)` (`LightGenerator::materialPropertiesExpression()`, priority 3) —
`max`, not the former product `metalness × (1-roughness)`, which zeroed out every smooth
DIELECTRIC (glass: metalness 0 → mask 0 → no traced reflection at all). The physical attenuation
belongs to the effects' per-pixel Fresnel + roughness fade, never to the mask.

**Code references:**
- `Effects/Lighting/SSR.hpp` — Parameters, ResolvePushConstants, setEnvironmentCubemap()
- `Effects/Lighting/SSR.cpp` — Shader source, descriptor layouts ("SSRResolveInput"), pipeline creation
- `PostProcessEffect.hpp` — Base interface
- `PostProcessor.hpp/cpp` — Chain management, push constants. `configure()` retires its previous grab pass + per-frame descriptor sets through `Renderer::deferredDestructor()` (frames-in-flight safety, no mid-frame `waitIdle`) — see `src/Vulkan/AGENTS.md`, "Deferred destruction contract". `recordBlit()` (and `GrabPass::recordBlit()`) follow the **batched barrier contract**: exactly two batched `pipelineBarrier()` calls around the back-to-back copies, never one barrier per transition — see [`docs/post-processing-pipeline.md`](../../../post-processing-pipeline.md) § 3 before touching either.
