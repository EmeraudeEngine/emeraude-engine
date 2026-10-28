## 43. Light glints — a light's visible source as a point that never flickers (Oct 2026)

A lamp's lens, a few square decimetres seen at a grazing angle, is smaller than a pixel beyond some tens of metres. A
rasteriser draws a sub-pixel triangle when it covers a pixel centre and misses it otherwise: the lens is drawn on one
frame and gone on the next, and the glare multiplies the blink. `city` at night, the camera driving: every distant
head appeared and vanished together as the camera bobbed across the pixel rows (owner, 2026-10-28: "the distant halos
flicker, it is not realistic"). Without the temporal anti-aliasing (the owner's setting) nothing integrates it; with
it, the Karis-weighted resolve keeps the image stable but erases the distant heads.

**The answer is the stars'** (graphics doc 42 § "The stars"): the source is drawn as a normalised Gaussian footprint
of at least 0.7 px carrying the light it sends to the camera, wherever it falls inside its pixel — the technique of a
game's distant city lights (A. Courrèges, "GTA V — Graphics Study", 2015).

### The contract

| Party | Code | What it does |
|---|---|---|
| The light | `AbstractLightEmitter::setVisibleSourceArea(m²)` (0 = none, the default; refused if negative / non-finite) | declares the source; published with the light's frame state (`publishedVisibleSourceArea()`) |
| The filing | `LightSet::hasVisibleSource(readStateIndex)` → `PostProcessStack::syncSceneEffects(…, lightGlints, …)` → `fileLightGlintsEffect()` | a SCENE-DRIVEN effect, filed the first frame a point or spot light publishes a source; `Core/Graphics/PostProcessing/LightGlints/Enabled = false` declines it |
| The effect | `Graphics::Effects::Lighting::LightGlints`, `EffectSlot::LightGlints` (after `Stars`) | gathers the enabled lights' published state every frame (render thread, under the light set's lock), uploads them camera-relative, one instanced additive pass, a combine `em_Color += glints` |

### The photometry

- A **spot**'s source is a flat face looking along the spot's direction (a luminaire's lens): toward the camera it
  shows I × cos α (α between the spot's direction and the camera; I the light's published intensity, candelas, ×
  its unit-luminance colour) and an area A cos α; nothing from behind (a full-cutoff luminaire seen from above shows
  nothing). A **point** light's source is a sphere of area A (a bulb): I in every direction, its disc A / 4.
- E = I cos α / d² (lux) at the camera, spread by a unit Gaussian whose peak is E / (2π σ² Ω_px), Ω_px the pixel's solid
  angle (2 tan(fovX/2)/W)(2 tan(fovY/2)/H) cos³θ — Σ L Ω over the pixels is E, the stars' formula. × the pre-exposure,
  the brightest channel bounded at 60 000 (a half float).
- σ = clamp(0.4 × the source's projected side in pixels, 0.7, 24): a point far away, a glow of the lens's size up
  close (the geometry shows the rest).
- Drawn with the **unjittered** projection after the TAA (a jittered sub-pixel point is rejected or smeared by the
  resolve — the stars' reason), before the camera's optics (the glare spreads a glint like any light).
- **Occlusion**: the glint's centre and its four neighbours (one pixel away) are tested in the FRAGMENT stage (where the
  chain's barriers make the depth readable) against the scene's Euclidean distance (`emViewDistance()`, graphics doc
  41's view reconstruction); a surface nearer than d (1 − 0.5 %) − 0.25 m hides the tap, the visible share (0, 0.2 …
  1) scales the glint. The bias keeps the luminaire's own head and lens from hiding it; the sky (depth 1) hides nothing.
- What it does NOT do: the aerial perspective (the haze runs before the TAA) does not dim a glint; the light's draw
  distance (`setDrawDistance()`) does not stop it (a glint is cheap, it shows the lamps the scene no longer lights);
  cubemaps and render-to-textures do not see glints. At most `MaxGlints` (65 536) a frame, a warning beyond.

### Measured (2026-10-28, Linux, RTX 3070 Ti, 1920×1080, RT lane, TAA OFF — the owner's settings)

`city` side 32, night (time of day 0.95, clock stopped), the 361 lamps declaring their lens (0.162 m²; the lens no
longer emits), the car driven from the spawn, 32 consecutive frames (`temporalCapture(32)`), the pixels above 150 in the
band of the distant lamps:
- BEFORE (an emitting lens, 2000 nits): `4311 4711 … 334 77 69 108 1193 978 53 62 599 2002 2014 522 110 …` — whole
  groups of heads on and off; |second difference| / mean **0.57**.
- AFTER (the glints): `581 595 393 256 216 180 210 … 219 255 250 256` — the motion's own trend; **0.15**. Every distant
  lamp is present on every frame.
- 0 VUID with the validation layer; the effect files itself ("A light of the scene has a visible source: the light
  glints join the chain"). From an aerial view the heads stay dark: the lenses face down.
