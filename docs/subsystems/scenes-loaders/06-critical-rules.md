## Critical Rules

1. **Never add Scene dependencies** to this namespace — that's the whole point of the separation
2. **Lambda capture safety** — same rules as before: never capture `this`, pre-resolve shared_ptr
3. **Default resource on every error path** — never leave a nullptr slot
4. **No axis conversion here — and none in the consumer either.** The engine world is Y-up (`+X` right, `+Y` up, `-Z` forward), which is exactly what glTF, USD and FBX carry, so the import is the IDENTITY. The old 180° X rotation and the per-asset `swapX/swapY/swapZ` flip are **both deleted**; read *Axis flip* above before considering either.
5. **Never add a triangle winding swap** — glTF, FBX and USD keep the authored winding verbatim (with `computeTriangleNormal(false)`). The unconditional swap they used to carry was a mirror compensation; re-adding one renders every face inside-out. `WADLoader` is the documented exception, because its bake genuinely negates Z.
6. **Key every resource on the asset INDEX, through `buildResourceKey()`** — a glTF or FBX name is not unique and never was an identity. Keying on the name alone hands the first homonym's geometry AND material to every later one, silently; it cost three bench runs of mis-attribution. See *The resource key* above.
7. **If an asset looks mirrored, MEASURE before compensating** — the cause is not in the loaders. Protocol in [`docs/coordinate-system.md`](../../coordinate-system.md).
8. **A loaded file is HOSTILE input (triad, 2026-09-30).** A dropped file reaches the loaders (`Core.openFiles()`):
   - **glTF**: `fastgltf::validate()` right after the parse (the parse does NOT range-check any index — material,
     accessor, buffer view, image, node, skin…) and `isStrictNodeForest()` (at most one parent, no cycle — validate
     does not check the hierarchy): an invalid asset is REFUSED. Checked: 314 glTF-Sample-Assets + 11 store assets,
     none refused; 349 assets strict forests.
   - **glTF extents (triad 12, 2026-10-01)**: fastgltf (0.9.0) bounds-checks NOTHING it reads — its `span` and
     `iterateAccessor()` have no bounds check, the accessor type is an `assert` only — and `validate()` checks neither
     that a buffer view fits in its buffer nor that an accessor fits in its view. Worse, `validate()` itself indexes the
     sparse buffer views, the channel sampler and the sampler accessors without a range check. So, in that order:
     `hasValidatableIndices()` (those indices) BEFORE `validate()`; then `hasConsistentExtents()`: every buffer view in
     its buffer (a meshopt view: its source in its buffer, count × byteStride == byteLength), every accessor in its
     view (sparse included), every attribute of a primitive with the POSITION count (the vertex array is sized from
     POSITION: a longer NORMAL WROTE past it), a SCALAR index accessor, the skin joints in range, a MAT4 FLOAT
     inverse bind matrix accessor of at least as many elements as joints (only the first jointCount are read: more is
     LEGAL and overflowed the vector), a VEC3 translation / scale and a VEC4 rotation output. After the meshes, a
     JOINTS_0 value past its skin's joint count is refused (the vertex shader reads `bones[JOINTS_0]` unchecked). A
     meshopt view that fails to decode serves zeros of its declared size (it served an EMPTY span, read out of
     bounds) and the file is refused at the end. Any failure refuses the whole file, with the reason.
   - **glTF skins in any joint order**: glTF allows it, the `SkeletalAnimator` forward pass needs parents first.
     `parentFirstJointOrder()` builds the skeleton parents first (the identity when the skin already is, so no asset
     of the corpus changes), the `Skin` maps the glTF joint index (JOINTS_0) to it, and `loadAnimations()` uses the
     same mapping for its channel targets. The animator refuses a skeleton that fails `Skeleton::isValid()` and a skin
     that does not match its skeleton.
   - Proof (triad 12): a standalone fastgltf census of the 323 parseable glTF-Sample-Assets and the 11 store assets
     (192 skins): 0 out-of-order skin, 0 inverse-bind-matrix count or type issue, 0 view or accessor out of bounds,
     0 JOINTS_0 out of its skin; 13 hand-crafted files through `Core.openFiles()`: the valid ones load (an out-of-order
     skin, more matrices than joints), each hostile one refused with its reason, 0 VUID.
   - **WAD**: STRUCTURAL corruption is refused ("Corrupted WAD"): a lump outside the file, a TEXTURE1/2 table, entry or
     patch list larger than its lump, a BSP node reached twice (a cycle). An out-of-range CROSS-REFERENCE (sector,
     sidedef, patch) skips the element. A reserve() is bounded by what the lump can hold.
   - **Hierarchies are walked ITERATIVELY** (explicit stack, children pushed in reverse to keep the order): the glTF
     nodes, the six USD prim / node walks, and `SceneDataConsumer` (which also skips a node reached twice, for every
     loader). A 200 000-node glTF chain used to overflow the stack in `SceneDataConsumer::processNodeAsStatic()`.
   - Proof: hand-crafted hostile files (index out of range, `children` cycle, a 200 000-node chain; WADs with a lump
     outside the file, an oversized TEXTURE1 table, a BSP cycle) → clean refusals / a clean load, no crash. Fuzzing:
     engine item `engine-fuzz-harness-for-loaders`.
9. **An FBX hierarchy is wired in FILE order** — never by iterating the `unordered_map` keyed by `ufbx_node *`: the
   order of the roots and children (and which duplicate name the scene keeps verbatim) changed from run to run.
10. **A multi-material glTF mesh's descriptor keeps ALL its materials** — the MultiLayerMeshResource lambda copies
    `materialList`; it used to MOVE it, leaving the SceneData descriptor with the default material only.

