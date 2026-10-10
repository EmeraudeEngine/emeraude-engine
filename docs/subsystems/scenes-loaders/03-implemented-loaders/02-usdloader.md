## Implemented Loaders

### USDLoader (OpenUSD via tinyusdz — geometry, materials, sky, INSTANCING, Aug 2026)

Reads `.usd` / `.usda` / `.usdc` / `.usdz`. Its reference asset, Intel's Jungle Ruins, is the
engine's **GOLD GOAL** (owner: *"le Saint Graal"*) — see
[`../../docs/scene-loaders-usd.md`](../../../scene-loaders-usd.md) § 8. It now translates
meshes, materials, dome lights and **point instancers**. **`capabilities()` still returns
`None`** — a known lag between the mask and what is delivered, to be corrected. Full design:
[`../../docs/scene-loaders-usd.md`](../../../scene-loaders-usd.md).

> [!CAUTION]
> **`lightusd::LoadUSDFromFile()` composes NOTHING.** It reads the root layer, parses the
> `subLayers` metadata, and returns success — a 19-sublayer stage comes back holding 2 prims.
> Composition is an explicit, separate pipeline and the loader uses it:
>
> ```
> LoadLayerFromFile → CompositeSublayers → LayerToStage
> ```
>
> **`CompositeAllArcs()` is deliberately NOT called.** It resolves references, payloads,
> inherits and variants eagerly; measured on Intel Jungle Ruins that is **24 minutes and 15 GB
> resident with no convergence**. Sublayers alone take seconds at ~3 GB and yield the whole
> non-instanced scene. Prototype references are resolved on demand, per element.
> ⇒ **`inherits` and `variants` are not applied on this path.**

> [!WARNING]
> **An unresolved reference fails SILENTLY and takes its whole prim with it.** tinyusdz stores
> each sublayer's working directory as the raw relative path written in the file, never joined
> with the root, so a reference made from inside a sublayer is looked up relative to the
> *process* working directory. Symptom measured before the fix: 84 meshes loaded, **0
> PointInstancer prototypes, no error raised**. The loader defends against it by (a) composing
> from an absolute path and (b) seeding the resolver with **every** directory of the stage tree.
>
> This is why the loader's first functional output is an **inventory**: byte-scanning a USDC
> crate cannot tell you what a stage contains (crate files compress their token table), and a
> plausible-looking scene with missing prototypes is indistinguishable from a correct one
> without counting prims.

> [!CAUTION]
> **ALWAYS PRINT tinyusdz's `warn` STRING.** Every composition entry point returns one, and it is
> where the library says what it silently gave up on. `LayerToStage` was discarding **every
> `PointInstancer` of the asset with its entire subtree** — the prim type was missing from its
> reconstruction table — and it said so, every single time, in a string the loader threw away.
> That cost a full session of diagnosis. All four steps now trace it.

#### Point instancers — read from the PRIMS, never from Tydra

`PointInstancer` appears **zero times** in the whole of `src/tydra/`: Tydra will never report
instances, and `RenderScene::instances` is about something else. `collectInstancers()` walks the
prims directly. Tydra does *descend* into a prototype and convert its meshes, exposing
`RenderMesh::abs_path` — which is what ties an instancer to the renderable it draws, with no
patch to Tydra.

Three traps, all paid on this asset (details in `docs/scene-loaders-usd.md` § 4.6):

1. **A transform is conjugated, not permuted.** `C·T·C⁻¹` where `C` is the +90°-about-X axis
   change. Closed form: translation through `C`, orientation quaternion conjugated by `C`'s
   quaternion, scale with **Y and Z swapped**. Permuting the position alone yields a forest in the
   right places, every plant rotated wrong.
2. **`/_class_` subtrees are skipped.** USD classes are abstract templates; Tydra converts them
   anyway. 12 meshes out for 6 real prototypes on one element — drawing them stacks a copy of
   every species at the origin.
3. **A prototype gets NO node**, only a resource. A node draws it one extra time, alone.

⚠️ A prototype root is assumed to sit at the identity (Tydra exposes absolute matrices only).
The assumption is **checked and logged**, never trusted silently.

`patches/tinyusdz.patch` in ext-deps-generator carries four fixes: the missing install rules
(upstream installs only its optional C API); an RAII guard restoring the asset resolver's state
after recursion — without it, every sibling sublayer after the first resolves against the wrong
directory, and upstream left a `TODO` asking for exactly that; a header include path only valid
inside the source tree; and the **missing PrimSpec→Prim table rows** for `GeomPointInstancer` and
five UsdLux types, all implemented upstream yet never listed, whose absence deletes the prim and
everything under it.
