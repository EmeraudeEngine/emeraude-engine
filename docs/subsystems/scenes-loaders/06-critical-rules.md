## Critical Rules

1. **Never add Scene dependencies** to this namespace — that's the whole point of the separation
2. **Lambda capture safety** — same rules as before: never capture `this`, pre-resolve shared_ptr
3. **Default resource on every error path** — never leave a nullptr slot
4. **No axis conversion here — and none in the consumer either.** The engine world is Y-up (`+X` right, `+Y` up, `-Z` forward), which is exactly what glTF, USD and FBX carry, so the import is the IDENTITY. The old 180° X rotation and the per-asset `swapX/swapY/swapZ` flip are **both deleted**; read *Axis flip* above before considering either.
5. **Never add a triangle winding swap** — glTF, FBX and USD keep the authored winding verbatim (with `computeTriangleNormal(false)`). The unconditional swap they used to carry was a mirror compensation; re-adding one renders every face inside-out. `WADLoader` is the documented exception, because its bake genuinely negates Z.
6. **Key every resource on the asset INDEX, through `buildResourceKey()`** — a glTF or FBX name is not unique and never was an identity. Keying on the name alone hands the first homonym's geometry AND material to every later one, silently; it cost three bench runs of mis-attribution. See *The resource key* above.
7. **If an asset looks mirrored, MEASURE before compensating** — the cause is not in the loaders. Protocol in [`docs/coordinate-system.md`](../../coordinate-system.md).
