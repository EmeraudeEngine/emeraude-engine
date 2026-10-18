## Implemented Loaders

### USDLoader (OpenUSD via tinyusdz — geometry, materials, sky, INSTANCING, Aug 2026)

Reads `.usd` / `.usda` / `.usdc` / `.usdz`. Its reference asset, Intel's Jungle Ruins, is the
engine's **GOLD GOAL** (owner: *"le Saint Graal"*) — see
[`../../docs/scene-loaders-usd.md`](../../../scene-loaders-usd.md) § 8. It now translates
meshes, materials, dome lights and **point instancers**. **`capabilities()` still returns
`None`** — a known lag between the mask and what is delivered, to be corrected. Full design:
[`../../docs/scene-loaders-usd.md`](../../../scene-loaders-usd.md).

**Two halves (2026-10-18, owner decision: analysis / materialization):** the `USD::` analysis (`USDParser.hpp/.cpp`)
composes a root layer (`USD::composeStage()`: sublayers, then every arc to a fixed point) and translates the stage
(`USD::analyse()`: inventory report, dome lights with their RAW image path, PointInstancers, Tydra's render scene, the
material factors and texture asset identifiers, the mesh placements, the punctual lights and their nodes) into plain
data; `USD::forEachMesh()` then builds the baked shapes ONE AT A TIME for a consumer, so a large stage never holds all
its shapes at once (JungleRuins' memory peak unchanged). `USDZArchive` (`USDZArchive.hpp/.cpp`) left the loader's TU and
gained `openMemory()` (the fuzzer's bytes). `USDLoader` keeps the file system: the root layer read (a file, or the
archive through its resolver), the texture resolution (archive, next to the stage, case-insensitive), the dome image
path, the deduplication, the levels of detail and every resource. The split exists so all of it can be fuzzed:
`src/Fuzzing/fuzz_usd.cpp` reads a USDA / USDC layer or a USDZ from memory, hermetically (a USDZ resolves against its
own table, a plain layer's resolver refuses every asset). Verified identical (2026-10-18): a dump of everything
`load()` delivers — every shape hashed bit for bit, the mesh / geometry / material names, each material's textures and
factors, the nodes (frame, mesh, light), the lights, the instance sets (every frame hashed) — byte-identical before
and after, and the sorted [USDLoader] traces identical, on WorldLobby.usdz with composition (942 meshes, 141
materials, 30 lights), JungleRuins option 1 (84 meshes, 87 materials) and option 2 (Anthurium: 6 meshes, 6 instance
sets of 138 instances, levels of detail), and NewSponza_Curtains_USD_YUp.usda through `Core.openFiles()` (the same Tydra
refusal); 0 VUID — then once more with the patched lightusd below.

**Limits (2026-10-18, found by `fuzz_usd`, owner decision "patch + engine bound"):** the first campaign's six OOMs were
ALL in lightusd, fixed in ext-deps-generator's `patches/tinyusdz.patch` (see `libraries/tinyusdz.yaml`):
`StreamReader::read()` accepted a SHORT read as success (non-zero byte count), so a truncated USDC array went on as
zeros and a 1 KB file declared 67 M paths (4.8 GB); and an internal reference to an ANCESTOR (only the direct
self-reference was refused) expanded exponentially (4 KB of USDA → 16.7 GB in 21 s). Engine side, a root layer's parse
is budgeted at max(`USD::MinimumMemoryLimit` 256 MiB, `USD::MemoryLimitPerFileByte` 16 × the file) through
`USDLoadOptions::max_memory_limit_in_mb` (`USD::setLoadOptions()`; lightusd's own default is 512 MiB) — a 15 KB USDC
asked for 400 MB. The composition options' `max_memory_limit_mb` is advisory: `composition.cc` never reads it.

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
