## The selection outline — a "custom depth" pass (Sep 2026)

Shows the selected entity: a thin line of constant PIXEL width around it, FULL where it is visible, DIMMED where
other geometry hides it. Owner decisions (2026-09-28): screen-space, hidden parts dimmed, thin pixel line;
architecture = Unreal's CustomDepth. Open work: engine `docs/todo/segment-rendering-beams-curves-outlines.md` § B.

### Using it

- A SET of entities, one style (owner decision 2026-09-29): `Scenes::Scene::setHighlightedEntities(entities)` (one
  step), `addHighlightedEntity()`, `removeHighlightedEntity()`, `clearHighlightedEntities()`, `highlightedEntities()`;
  `setHighlightedEntity(entity)` replaces the set by one (nullptr clears). Any thread, held weakly (a dead entity just
  stops being outlined). Touching entities share ONE silhouette: the whole set is one depth.
- The editor drives it: its selection (Shift+click adds or removes) is published whole on every change
  (`docs/subsystems/scenes/25-editor-selection-states-group-transform-panel.md`).
- Console / MCP: `highlightEntity(<address>)` (replaces the set), `addHighlight(<address>)`,
  `removeHighlight(<address>)`, `clearHighlight()`, `getHighlights()` (JSON array of addresses, in insertion order),
  `setHighlightStyle(r, g, b, width, hiddenOpacity)`
  (colour as DISPLAYED, sRGB; width 1-8 px; hidden opacity 0 = visible parts only, 1 = x-ray). Defaults: orange
  (1, 0.6, 0.1), 2 px, 0.35. `Renderer::selectionOutline()` for code.

### How it works — two passes

1. **Custom depth** (`SelectionOutline::recordDepth()`, after the scene pass, outside any render pass): the
   highlighted entity alone is drawn into a `SelectionDepthTarget` (`D32_SFLOAT`, the scene size, created on first
   use, recreated when the extent changes, left in SHADER_READ_ONLY) by `Scene::renderSelectionDepth()`, with the
   **depth-only shadow-casting programs** (`castShadows()`, programs generated for this target on first use):
   skinning, alpha test, wind and heightfields come for free, the outline follows what the GPU drew. The target's
   view DELEGATES to the main camera (the scene target's scheme).
2. **Composite** (`SelectionOutline::recordComposite()`, in the final composite pass after the tone mapping and before
   the debug overlay, the gizmos and the UI — display colours, neither exposed nor blurred by the TAA): a fullscreen
   triangle, alpha-blended. A pixel INSIDE the selection draws nothing (an outline, not a tint); outside, it looks for
   the nearest selection pixel within the width (a disk): found, it is outline, soft over its last half pixel, full
   when the nearest VISIBLE one is in reach, else `hiddenOpacity`. Visible = the selection's linear depth is the
   scene's front-most (the post-process grab pass depth, 1 % + 1 cm tolerance: the scene depth is jittered, the
   selection's is not). Without a grab pass depth, everything is drawn full.

### ⚠️ Contracts it changed

- **The shadow-casting programs drop their depth BIAS and CLAMP for `RenderTargetType::SelectionDepth`**
  (`Saphir::Generator::ShadowCasting::onGraphicsPipelineConfiguration()`, the target type is in the program key).
  Biased, every visible part read "behind the scene" and was drawn dimmed.
- **The shadow-casting push paths never jitter**: `Unique::pushMatricesForShadowCasting()` pushes the UNJITTERED
  projection, `Multiple`'s pushes a ZERO jitter. Identical for every shadow map (their views never jitter); required
  here, where the view is the main camera's, or the outline trembles by half a pixel every frame.
- An emissive overlay (a beam, `Material::Interface::writesGeometryBuffer()` false) is skipped by the custom depth: it
  is no surface, and these programs cannot build its ribbon.

### Cost and the scissor (2026-09-29)

GPU zones `SelectionDepth` and `SelectionOutline` (inside `FinalComposite`), read with
`Core.RendererService.getGPUTimings()` (`Core/Graphics/GPUProfiler/Enabled`). The disk search costs ~width² taps per
pixel it covers. RTX 3070 Ti, 2880x1620, `geometry-generator`:

| Width | Full screen (before) | Scissored, figurine half (~1/4 of the screen) |
|---|---|---|
| 1 px | 0.096 ms | — |
| 2 px (default) | 0.195 ms | 0.072 ms |
| 4 px | 0.56 ms | 0.06 ms (object high on screen) |
| 8 px | 1.81 ms | 0.66 ms |

`SelectionDepth` stays ~0.04-0.1 ms (one entity drawn depth-only). Owner decision: the scissor only, no cheaper search
— a full-screen selection still pays the full-screen price, and wide lines stay quadratic.

**How the scissor is found** (owner decision: the scene publishes the box):
- `Scene::publishStateForRendering()` (logic thread) copies each highlighted entity's `getWorldRenderBoundingBox()`
  into the logic slot, TAGGED with its entity (`m_publishedHighlights`). The render thread reads them through
  `Scene::highlightedWorldBoundingBoxes()` for the frame's read slot: the same pose the scene pass draws, no race.
- `SelectionOutline::updateScreenArea()` projects each box with the unjittered main camera (`projectedArea()`) and
  keeps the UNION. The 12 edges are clipped
  against `w = 1e-4` (Blinn & Newell, "Clipping using homogeneous coordinates", SIGGRAPH 1978): a box straddling the
  eye still gives the exact screen area, a box entirely behind the eye gives an EMPTY one (the pass is skipped — 0 ms
  when looking away).
- `recordComposite()` grows the area by `ceil(width) + 1` px and scissors the fullscreen triangle to it.

⚠️ Traps:
- **Whole screen when the box is unknown**: the slot still tags the PREVIOUS selection (1-2 frames after a change), or
  the logic is PAUSED (no publication since the change: `Core.cpp` skips `publishStateForRendering()`). Always
  correct, only slower.
- **The render box must cover what the GPU draws.** A vertex that leaves `m_renderBoundingBox` (skinning beyond the
  bind-pose bounds, strong wind) is outlined only inside the scissor — the same box the octree culls with, so such an
  entity would already pop. Grow the component's bounds, never the margin.
- The camera INSIDE the box legitimately covers the whole screen (0.61 ms at 4 px).

### ⚠️ Limits (first pass)

- Internal-target frames only (`Renderer::renderFrameWithInternal()`): the direct swap-chain path has no scene target.
- The scissor of a set is the UNION of the entities' projected boxes (`Scene::highlightedWorldBoundingBoxes()`, one box
  per live entity published per slot): two entities at opposite corners scissor nearly the whole screen. Measured:
  both `geometry-generator` halves, 2 px, 0.119 ms.
- The style setters are called from the logic/console thread and read by the render thread (plain floats, like the
  material look setters) — the same exposure as the rest of the engine's "look" setters.
