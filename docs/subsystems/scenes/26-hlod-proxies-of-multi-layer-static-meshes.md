## HLOD proxies of multi-layer static meshes (Oct 2026)

A static mesh merged per region with one material per layer (a city's buildings: four facades, roofs, glass, three
window tints, awnings, signs, steel — seventeen layers) costs one draw per layer and per region. Seen from above, every
region is in view: projet-alpha's `city` at side 128 drew 6387 view batches from its aerial pose, the render thread
saturated at 23.4 ms a frame (Linux, 3070 Ti). The cost is the draw calls, not the triangles.

**The answer is the hierarchical level of detail** (Epic, "Hierarchical Level of Detail",
https://dev.epicgames.com/documentation/unreal-engine/hierarchical-level-of-detail-overview-in-unreal-engine ; Unity
HLODSystem, https://github.com/Unity-Technologies/HLODSystem): beyond a distance a cluster is drawn as ONE proxy instead
of its parts.

### The contract — `Scenes::Toolkit::generateHLODProxy(name, layered, layers, options)`

| Input | Meaning |
|---|---|
| `layered` | The detailed mesh's shape, one GROUP per layer, in the space of the entity at the cursor. |
| `layers` (`HLODLayer`, one per group) | `role`: `Omit` (left out), `Body` (merged, tinted per vertex by `colour`, LINEAR — e.g. the layer's texture `ImageResource::averageColor()` through `sRGBToLinear()`), `Glow` (merged on the caller's glow material). |
| `options` (`HLODOptions`) | `switchDistance` (the proxy is drawn from it; the CALLER draws the detailed mesh up to it with `setDrawDistanceRange()`), `glowMaterial` (null: the Glow layers are left out), `glowRasterization` (a depth bias when the glow lies on the body like a decal), `castShadows`. |

The proxy is an entity at the cursor with ONE renderable of at most two layers: the body (one engine material,
`+HLODVertexColour`: vertex colour as the albedo, matte, no texture) and the glow. It never collides (the detailed entity
keeps the collision). No decimation: the triangles are the layers' own.

⚠️ **The glow has ONE emission colour** (the glow material's): `StandardResource`'s emission comes from its uniform
`AutoIlluminationColor`, never from the vertex colour — the far windows of a city take one average tint.

### Measured (2026-10-29, Linux, 3070 Ti, 1920×1080, `city 128,1,12,0`, day, a quiet machine)

The buildings' 244 region entities with a proxy at 300 m (`City::BuildingProxyDistance`):
- aerial pose (0, 300, −1300), physics off: 6387 → **2810** view batches; render-thread CPU 23.4 → **17.8 ms** a frame
  (−24 %); **42.6 → 55.6 FPS**; shadow batches 3612 → 3904 — the detailed meshes AND the proxies cast: the shadow
  lists did not honour the draw distance range; fixed the same day (scenes doc 22): 1426;
- spawn pose: 3065 → 1856 view batches, 39.3 → 44.3 FPS (the render thread is not the limit there on this machine).
- Windows-PA (RTX 3500 Ada, 720p, CPU-bound): aerial day 26 → 37-38 FPS, spawn 33-34 → 39; macOS-PA (M6, 1440p): aerial
  12.3 → 19.3 FPS. No colour pop at the switch; by day the proxies' window quads (1-3 px at 300 m, no texture to blend
  into) alias into dark dashes (macOS-PA) — an open look question.
- 0 VUID, 0 `[Error]` (city 32, validation layer). Day: the far blocks read as their facades' average colour; night:
  their windows stay lit as warm points.
