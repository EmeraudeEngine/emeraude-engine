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

