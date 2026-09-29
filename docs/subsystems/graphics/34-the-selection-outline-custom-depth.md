## The selection outline — a "custom depth" pass (Sep 2026)

Shows the selected entity: a thin line of constant PIXEL width around it, FULL where it is visible, DIMMED where
other geometry hides it. Owner decisions (2026-09-28): screen-space, hidden parts dimmed, thin pixel line;
architecture = Unreal's CustomDepth. Open work: engine `docs/todo/segment-rendering-beams-curves-outlines.md` § B.

### Using it

- `Scenes::Scene::setHighlightedEntity(entity)` / `setHighlightedEntity(nullptr)` — any thread, held weakly (a dead
  entity just stops being outlined). One entity at a time.
- The editor drives it: `Scenes::Editor::Manager::setSelection()` / `clearSelection()` (the concrete `Node` /
  `StaticEntity` shares itself through `shared_from_this()`).
- Console / MCP: `highlightEntity(<address>)`, `clearHighlight()`, `setHighlightStyle(r, g, b, width, hiddenOpacity)`
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

### ⚠️ Limits (first pass)

- Internal-target frames only (`Renderer::renderFrameWithInternal()`): the direct swap-chain path has no scene target.
- One entity. The composite runs FULLSCREEN whenever an entity is highlighted: `(2·width + 1)²` depth taps per pixel at
  most (81 at 4 px). Not measured yet; scissoring it to the entity's projected bounds is the obvious saving.
- The style setters are called from the logic/console thread and read by the render thread (plain floats, like the
  material look setters) — the same exposure as the rest of the engine's "look" setters.
