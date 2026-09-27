## Implemented Loaders

### WADLoader (level MATERIALIZER — Jul 2026, sky + photometric anchor Aug 2026, masked middle textures Aug 2026)

Loads a classic Doom-engine WAD (IWAD/PWAD) and materializes **one map** as static textured
geometry. Deliberately NOT a game loader: no things, no sprites, no mechanics — walls,
floors, ceilings, original textures, sector light levels baked as vertex colors.

**API:** `setMapIndex(n)` (1-based, WAD directory order — works across both naming schemes
ExMy and MAPxx) or `setMapName("E1M1")`. After `load()`: `loadedMapName()`,
`playerStartPosition()` / `playerStartDirection()` — already in ENGINE WORLD space (Y-up,
post-consumer), at the sector floor (eye height up to the caller). Scale: 32 map units per
meter (`MapUnitsPerMeter`), multiplied by `LoaderOptions::uniformScale`.

**Pipeline:**
1. Whole-file read → directory (name/offset/size per lump), map marker lookup (`ExMy`/`MAPxx`
   followed by `THINGS`). Every 8-char name is produced by `readName8()`, which **UPPERCASES** —
   see *readName8 uppercasing* below.
2. Map lumps: `VERTEXES`, `LINEDEFS`, `SIDEDEFS`, `SECTORS`, `SEGS`, `SSECTORS`, `NODES`, `THINGS`.
   The LINEDEF **flags** word (16-bit, record offset 4) is parsed, with named constants
   `LinedefTwoSided` (0x0004), `LinedefDontPegTop` (0x0008), `LinedefDontPegBottom` (0x0010) —
   only the two pegging flags concern a materializer, the rest are collision/sound/automap.
3. **Walls** from linedefs: one-sided → full quad; two-sided → lower/upper step quads seen from
   each side (sky-hack: no upper quad between two `F_SKY1` ceilings — see *Sky sectors* below),
   **plus the two-sided MIDDLE texture** (grates, fences, fake walls — see *Masked two-sided
   middle textures* below). UVs in texel space from sidedef offsets.
4. **Floors/ceilings**: per-subsector convex polygons reconstructed EXACTLY by
   **Sutherland-Hodgman clipping of the level bbox through the BSP node planes** down to each
   leaf, then by the leaf's segs (their right side faces the subsector — Doom convention).
   ⚠️ Neither fanning the seg vertices nor angular-sorting them works: subsector corners
   created by two partition lines carry no seg vertex → holes. The BSP clipping is the only
   correct source. A sector whose ceiling flat is `F_SKY1` emits **NO ceiling at all** — see
   *Sky sectors* below.
5. **Textures**: `PLAYPAL` palette → flats (raw 64×64) and composite wall textures
   (`TEXTURE1/2` + `PNAMES` + picture-format patch blitting, transparent texels alpha 0). The
   per-texel **coverage canvas** written while composing is also reduced to a single per-texture
   verdict cached in `textureHasHoles` — the basis of the masked classification below.
   No sky texture is resolved: the WAD's own `SKY*` lumps are never materialized.
6. One multi-material mesh: `VertexFactory::Shape` groups (one per **bucket**, triangle
   offset/count, winding swap i1↔i2 like GLTF/FBX) → `IndexedVertexResource`
   (`EnableNormal|EnablePrimaryTextureCoordinates|EnableVertexColor`) →
   `MultiLayerMeshResource` with per-layer `CullingMode::None` (double-sided on purpose).
7. Materials: unlit `BasicResource` per bucket, `setTextureResource()` + `enableVertexColor()`,
   `enableAlphaTest()` on the masked ones, plus the absolute-luminance anchor — see
   *Photometric anchor* below.

**Sky sectors — the engine skybox shows through (Aug 2026):**

A sector whose ceiling flat is `F_SKY1` emits **nothing**: no sky plane, no sky texture, no
stencil, no portal. The scene **background** is what fills the opening.

Why the hole is not a hole (verified in `Scenes::Scene::registerSceneVisualComponents()`): the
background renderable gets `setUseInfinityView(true)` + `disableDepthTest(true)` +
`disableDepthWrite(true)`, is inserted into the **Opaque** render list with distance `0.0F` (the
`isSpecial` / `isUsingInfinityView()` branch) and is drained first. Its geometry is a 512 m cuboid
with flipped winding drawn on the translation-free view, so it fills every pixel the level does not
cover; level geometry drawn afterwards with depth test **and** depth write ON simply overwrites it.
This is the modern equivalent of vanilla's sky visplane. With **no** background installed the
pixels are opaque black — never garbage.

The dead **episode sky-texture selection** (`SKY<e>` / `RSKY1-3` picked from the map name) was
**DELETED**.

The pre-existing **upper-wall suppression was already correct** and is untouched: `skyOnBothSides`
in `WADLoader.cpp` suppresses **BOTH** upper quads when both sectors' ceilings are the sky flat.
That is exactly vanilla Doom's sky hack — `R_StoreWallRange` (`r_segs.c`) sets
`worldtop = worldhigh`, so the `worldhigh < worldtop` guard is false and the upper texture is never
assigned. Sky adjacent to a **non-sky** ceiling still emits its upper wall — also vanilla.

Demo side (`projet-alpha`, `src/Builtin/DoomLoader.cpp`): the constructor calls
`enableBasicBackground({}, true)` — empty name = random pick from the sky store, `true` = the
generic KeyPad3 sky cycle. `enableBasicLighting()` is **deliberately NOT** called: installing a sky
must not put the level on the lit path (see *Unlit on purpose* below).

**Masked two-sided middle textures — grates, fences, fake walls (Aug 2026):**

Doom's two-sided linedefs can carry a **middle** texture in addition to the upper/lower steps. It is
not a wall: it hangs in the *opening* between the two sectors and whatever of the opening it does not
cover stays see-through. This is now materialized, as an **alpha-tested CUTOUT**.

*Engine side — new material contract* (canonical home:
[`src/Graphics/AGENTS.md`](../../../../AGENTS.md#alpha-test--the-binary-cutout-contract-aug-2026) § 5,
"Alpha Test — the Binary Cutout Contract" — keep the two in sync, and prefer editing that one):

- New flag **`MaterialFlagBits::AlphaTestEnabled = 1U << 16`** (`Graphics/Material/Interface.hpp`) —
  a binary **CUTOUT**: the fragment shader discards below a cutoff and the material **STAYS
  OPAQUE** (opaque render list, depth write kept, no back-to-front sorting, state-sorted batching
  preserved). `BasicResource::enableAlphaTest()` sets it.
- The discard now fires on **that flag, INDEPENDENTLY of the blending mode**. Gating it on blending
  was exactly what used to force a cutout out of the opaque list.
- ⚠️ **`isOpaque()` is deliberately UNCHANGED and must stay so** — an alpha-tested material **IS**
  opaque. Returning `false` there would push it into the distance-sorted **translucent** list AND
  enable colour blending (`GraphicsPipeline::configureColorBlendState` keys on that predicate),
  defeating the flag entirely.
- `isAlphaTest()` returns `true` for the flag, so the **RT pipeline** alpha-tests at hit time.
- `requiresAlphaTestedShadows()` now **also** returns `true` for the flag: a cutout must cast a
  **CUTOUT shadow**, not a solid rectangle. (It previously required `BlendingMode::Normal`, so a
  cutout would have shadowed solid.)

> [!WARNING]
> **The cutoff is FIXED at 0.5 and NOT configurable, on purpose.** Two reasons:
> 1. The **shader program cache keys on the material's DESCRIPTOR LAYOUT hash**, not on its flags or
>    values — a per-material cutoff literal baked into the generated GLSL could serve one material's
>    program to another sharing the same layout.
> 2. All **twelve floats** of `BasicResource`'s material-properties buffer are already claimed, so a
>    uniform-borne cutoff would require growing the block.
>
> 0.5 is the right value for a **coverage** mask, and it now agrees across all three paths: colour
> discard, shadow discard, and `GPURTMaterialData::alphaCutoff` (which already defaulted to 0.5).
> **A configurable cutoff needs the cache key fixed first.**

*Loader side (`WADLoader.cpp`):*

- **Compound bucket key.** The geometry bucket key became `SurfaceKey{name, SurfaceClass}` with
  `SurfaceClass::Opaque | Masked`. Required because the same texture **NAME** can be consumed both
  ways: measured in `doom.wad`, **WOOD1, GSTONE1, MARBLE2, SP_ROCK1, MARBFACE and FIREMAG3** are each
  used BOTH as an ordinary wall AND as a two-sided middle texture. Keying on the name alone would
  force one material for both — a grate turning its solid twin see-through, or the reverse.
- ⚠️ **The class is also part of the MATERIAL RESOURCE NAME** (`.../Material/{NAME}` vs
  `.../Material/{NAME}/Masked`): the resource container is keyed by name, so without it the two
  variants would collide and whichever loaded **first** would impose its mode on the other.
- ⚠️ **Index-alignment invariant.** `materialOrder`, the sub-geometry groups, `materialList` and
  `rasterizationOptions` stay index-aligned **BECAUSE they are all produced by one ordered iteration
  of the bucket map**: a sub-geometry index **IS** a layer index **IS** a material index. Keep it
  that way.
- **`emitQuad` / `emitWall` split.** `emitQuad` was extracted from `emitWall`: it takes the **V range
  EXPLICITLY** plus a `SurfaceClass`. `emitWall` delegates to it and keeps the **pegged-to-top** rule
  for ordinary walls (which DO tile vertically).
- **`emitMiddle` implements vanilla's rules** (`linuxdoom-1.10` `R_RenderMaskedSegRange`): the image
  is drawn **EXACTLY ONCE**, **NEVER tiled vertically**, anchored in **WORLD** space then **CLIPPED**
  to the opening — whatever of the opening it does not cover stays see-through.

  ```
  opening          = [max(floors), min(ceilings)]
  anchor (texel 0) = min(ceilings) + yOffset             (default, pegged to the top)
                   = max(floors) + texHeight + yOffset   (with ML_DONTPEGBOTTOM, 0x0010)
  V(z)             = (anchor - z) / texHeight            (V grows downward, 1 texel per map unit)
  ```

  Clipping the quad to **both** the one-texture-height span **and** the opening keeps V inside
  `[0,1]` **BY CONSTRUCTION**, so the image is **CLIPPED, never squashed**. A **POSITIVE** `yOffset`
  slides the visible image **UP**.
  ⚠️ **Reproducing this with `emitWall()` would stretch or tile it** and misalign a large share of
  the faces.
- **Masked classification is AUTOMATIC**, from the **coverage canvas already computed while
  composing**: a texture is `Masked` only when its **COMPOSED image leaves texels uncovered**. That
  is Doom's own notion of masked — transparency is the **ABSENCE of a patch post**, never a colour
  key (palette index 0 is an ordinary opaque colour). Cached per texture in `textureHasHoles`.
  Measured: **6 of the 20** textures used as middle textures in `doom.wad` are fully opaque "fake
  wall" decoration and correctly stay `Opaque`, **paying nothing** for the cutout.
- **Duplicate-side rule.** The LEFT side's middle quad is **skipped when it would exactly duplicate
  the right one** (same texture AND same offsets): the material is already double-sided
  (`CullingMode::None`), so two coincident quads would only **z-fight**. **Differing offsets DO make
  two genuinely different images** and both are emitted — as in vanilla, where each sidedef is drawn
  from its own viewing side.
- **Vanilla glitches deliberately NOT reproduced:** *Medusa* (a multi-patch texture used as a middle
  texture) and *tutti-frutti* (a texture shorter than 128 tiling with garbage).
- Middle textures do **not** block movement in vanilla unless `ML_BLOCKING` is also set. This
  materializer has **no collision at all**, so the point is moot — stated for completeness.

*Validation (runtime, measured):*

- Validated on **E4M3** (map index **30** in `doom.wad`, **28 MIDGRATE faces**) — chosen because
  **E1M1 has only 13 middle faces and ALL of them are fully-opaque `BRNBIG*` decoration**, so E1M1
  does not exercise the cutout at all.
- **MIDGRATE** is 128×128 with **7480 uncovered texels (45.7%)**, correctly classified `Masked`.
- Confirmed visually: the grate's gaps are **genuinely see-through** (the wall and floor behind are
  visible), the image is **anchored correctly** in the doorway with **no stretching**, and there is
  **no z-fighting**.
- E4M3 was scanned for hole-bearing textures **by slot**: MIDGRATE in the middle slot is the ONLY
  one — no opaque wall on that map uses a hole-bearing texture, so nothing there is misclassified.

> [!WARNING]
> ⚠️ **STILL A KNOWN GAP — do not claim otherwise.** A hole-bearing texture used on an **ORDINARY
> wall** (upper, lower or one-sided) is classified `Opaque`, so its uncovered texels render **BLACK**
> (the palette index 0 colour, alpha ignored). Vanilla showed the *tutti-frutti* glitch there
> instead. **No map validated so far exercises it.**

**`readName8` uppercasing (bonus fix, Aug 2026):**

`readName8()` now **UPPERCASES** the 8-char names it produces. It is the **single site where every
WAD name is born**, so the lump directory, `PNAMES`, the sidedef texture names and the sector flat
names all become comparable.

Why it matters: WAD names are uppercase **by convention**, but the convention is **NOT enforced** —
`doom.wad`'s **TEKWALL4** references its patch through a **LOWERCASE** `PNAMES` entry (`w94_1`) while
the lump is `W94_1`. The case-sensitive lookup **silently failed**, the composed texture kept its
**cleared canvas**, and the wall rendered **BLACK** — **15 faces in E1M1 alone**.

**ASCII only, deliberately**: a WAD name is ASCII by format, and `std::toupper` would drag a
**locale** into a hot parsing loop.

**Photometric anchor — `FullBrightLuminance` + the demo's fixed exposure, ONE JOINT CALIBRATION (Aug 2026):**

Public constant `WADLoader::FullBrightLuminance = 2000.0F` (nits, cd/m²). Every map material
(`Material::BasicResource`, one per bucket) also does:

```cpp
materialResource.setAutoIlluminationAmount(1.0F);
materialResource.setEmissiveStrength(FullBrightLuminance);
```

The emitted quantity is therefore **texel × vertexColor × 2000 nits**. The surfaces **EMIT, they are
not LIT** — the sector light level is already baked into the vertex colors, so re-lighting would
double-count it. Exactly the same reasoning `SkyBoxResource` uses for a sky.

Rationale: a Doom map carries **no photometry**. Its sector light levels are 0-255 ordinals authored
for a CRT, not luminances. Emitted raw into a photometric pipeline they landed at **0.038 mean
output**, so the map needs an ABSOLUTE anchor.

**Why 2000 nits and not 250:** a fully-lit Doom surface is treated as a **SUNLIT surface**
(~2000-5000 nits in the real world) rather than as **white on an SDR monitor** (~250 nits), for
exactly one reason — the map has to **share the frame with a sky**. 250 nits read well on its own,
but it sat **5 stops under a daylight sky**, so no single exposure could hold both.

> [!WARNING]
> ⚠️⚠️ **`FullBrightLuminance` AND THE DEMO'S EXPOSURE TRIAD ARE ONE JOINT CALIBRATION.** Moving
> either alone breaks the other. The anchor lives in `WADLoader.hpp`, the triad in
> `projet-alpha`'s `DoomLoader::onEnabled()` — both carry the rule in their own comments. Change
> them together, or not at all.

The triad, **derived** (engine photometry, `Graphics/Photometry.hpp`):
`display = L / (MeterCalibration · 2^EV100)` with `MeterCalibration = 1.2`, and
`EV100 = log2(N²/t · 100/S)`.

| Term | Value |
|------|-------|
| Aperture / shutter / sensitivity | f/8 · 1/125 s · **ISO 125** |
| EV100 | `log2(6400)` = **12.64** |
| Clipping point | **7680 nits** |
| Fully-lit surface (2000 nits) | **0.26** display-linear |
| Clear sky at 8000 nits | **1.04** — just at the top |
| Skies that still clip | **2 of 28**: JeGray (25500) and AutumnField (31800) |

**The owner's principle, now confirmed:** ONE fixed configuration suffices for EVERY Doom map,
because all Doom maps are authored on the same scale (sector light 0-255, same meaning everywhere).
Nothing about a map justifies a per-map exposure. **The variable was never the map — it was the
sky**, which is not part of Doom's calibration and spans 1 to 31800 nits across the engine's store.

> [!WARNING]
> ⚠️⚠️ **DIAGNOSTIC LESSON — the failure mode of the superseded 250 nits + ISO 1000 pairing.**
> That pairing clipped at **960 nits**, so **16 of the 28 skies went PURE WHITE** and their glare
> flooded the frame on any **outdoor** map. The MAP ITSELF was correctly and **identically** exposed
> the whole time — which is precisely what made it confusing: it read as *"the fixed lighting failed
> on some maps"* when the map was never the variable. **INDOOR maps (E4M3) hid it completely**, so an
> indoor validation target proves **nothing** about this. Generalisable: when a fixed-exposure scene
> looks wrong on *some* content only, first check what ELSE shares the frame and its absolute range,
> before touching the thing that changed appearance.

> [!WARNING]
> **`setAutoIlluminationAmount` is the MASK and is clamped to `[0,1]`.** The luminance belongs to
> `setEmissiveStrength`. Passing the nits through the amount **silently clamps them to 1**.

Measured on the **superseded 250 nits + ISO 1000** revision, identical viewpoint, sky = Forrest
(3000 nits), before → after the anchor. Kept because it demonstrates the **anchor mechanism** (an
absolute anchor is what lifts the map out of near-black); the numbers do **not** describe the
current calibration:

| Metric | Before | After |
|--------|--------|-------|
| Geometry mean | 0.03843 | **0.39782** (×10.4) |
| Geometry p95 | 0.05490 | **0.56350** |
| Frame clipped to pure white | 5.34% | **0.06%** |
| Crushed pixels (≤ 0.02) | 0.00% | 0.00% |

An earlier revision instead **capped the random sky pick at 300 nits**. Abandoned: it treated the
symptom, and the luminance criterion also picked photometrically-valid but thematically absurd
backgrounds (abstract chrome interiors over E1M1) because **luminance cannot express "looks like a
sky"**. No cap now — the whole sky store is fair game. The cap survives as the optional third
parameter `maxLuminance` of `projet-alpha`'s
`AbstractDemo::enableBasicBackground(materialName = {}, cyclable = false, maxLuminance = 0.0F)`
(zero = no cap; bounds ONLY the random pick — the KeyPad3 cycle stays generic over every sky on
purpose, it is the affordance for inspecting the whole store).
⚠️ **No demo uses `maxLuminance` any more** — it defaults to a no-op; do not assume it is
load-bearing.

**Unlit on purpose — `MeshDescriptor::lightingEnabled = false`:**

The loader sets `meshDescriptor.lightingEnabled = false` on its level mesh (field documented under
*SceneData* above). The level is self-illuminating by construction — baked vertex colors on unlit
`BasicResource` — and the lit path's ambient/IBL term is scaled by the background luminance, so
putting the level on it would multiply every surface by the sky brightness and destroy the baked
look.

> [!NOTE]
> **Honest status: this fixes a LATENT defect only.** `Scene`'s test is
> `m_lightSet.isEnabled() && instance->isLightingEnabled()`, and the light set is enabled only by
> `Scene::applyBackgroundLighting()` (or `DefinitionResource` / the console) — which the
> `doom-loader` demo never calls. The flag is therefore currently **INERT** for that demo. It would
> bite the moment any demo enabled the light set with a WAD level loaded. Kept and documented as
> latent by owner decision — do **not** present it as something a measurement demonstrated.

**Critical lessons (engine-wide):**
- ⚠️ **Resource lambdas run on the resource-manager loading threads** — capture every local
  buffer BY VALUE (`[pixels = std::move(rgba)]`), never by reference. By-ref captures caused
  intermittent load failures AND a segfault (use-after-free of the stack).
- ⚠️ **The sky does NOT light this level — MEASURED.** With the **camera exposure fixed** (the
  `doom-loader` demo pins `setAutoExposure(false)` + f/8 · 1/125 s · **ISO 125**, the current triad —
  see *Photometric anchor* above; this measurement was taken on the superseded ISO 1000 revision),
  the framing held
  fixed and NO sky pixels in frame, the level's pixels were **bit-identical across the entire sky
  store** (1 nit to 31800 nits). Any future claim that "the sky brightens the map" **must be
  re-measured with the camera pointing DOWN and the exposure pinned**.
- ⚠️ **An auto-exposing camera invalidates that measurement**, and this was also measured: with
  auto-exposure ON the metering tracks the sky and the map's own rendered brightness swings by
  **5.7×** across the store (mean 0.86 under a 1-nit sky, 0.15 with 13 % clipping under a
  31800-nit one) — a **sensor** effect, no light reaching the geometry.
- ⚠️ **`UP = +Y` in this engine** (since the Aug 2026 Y-up flip — it read `UP = -Y` until then): a
  `lookAt` target with a **HIGHER** Y looks **UP**. Getting
  that backwards produced a bogus *"the sky lights the level ×41"* conclusion in the session that
  introduced the skybox. That ×41 figure still appears in two code comments —
  `SceneData.hpp` (`MeshDescriptor::lightingEnabled`) and `WADLoader.cpp` (the
  `meshDescriptor.lightingEnabled = false` block) — the *mechanism* they describe is real, the
  **number is not measured**. Treat it as unverified, and reword those comments when the file is
  next touched. (`RenderableInstance::Abstract::setLightingState()` describes the same mechanism
  **without** the number — that one is fine.)
- **Perceptual color chain for unlit retro content**: textures are loaded with
  `enableSRGB(false)` and sector light stays raw — the direct swap-chain path does not
  re-encode, so keeping everything perceptual (texel × light) reproduces the original
  renderer's look. Decoding to linear darkened the whole map.

**V1 limitations (future work):** no collision (the demo flies through); fine texture alignment
still approximate on ordinary walls. **Two-sided middle textures are NO LONGER skipped** — they are
materialized as alpha-tested cutouts (see *Masked two-sided middle textures* above), and the
**pegging flags are now parsed** (`ML_DONTPEGTOP` / `ML_DONTPEGBOTTOM`), `ML_DONTPEGBOTTOM` being
honored for the middle-texture anchor. What remains on that front is the
**hole-bearing-texture-on-an-opaque-wall gap**: such a texture on an upper, lower or one-sided wall
is classified `Opaque` and its uncovered texels render black.
The sky is no longer a limitation either: sky sectors emit no ceiling and the scene background shows
through (see *Sky sectors* above).

**Resource naming:** `WAD:{stem}/Texture/{NAME}` and `WAD:{stem}/Material/{NAME}` (shared per WAD,
the masked variant suffixed `/Masked` — see the compound key above), `WAD:{stem}/{MAP}/...`
(per map: geometry, mesh).
