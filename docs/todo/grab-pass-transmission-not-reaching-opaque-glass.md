---
id: grab-pass-transmission-not-reaching-opaque-glass
title: A transmissive material classed OPAQUE never sees the scene behind it — the grab pass is read before it is filled
status: open
priority: high
scope: Scenes/Scene.rendering + Graphics/Renderable
opened: 2026-08-29
tags: [measured, transmission, grab-pass, render-lists]
---

# Grab-pass transmission does not reach `VolumeAbsorptionProbe`'s balls

## The measurement, which is unambiguous

The probe's three balls are `transmissionFactor: 1`, `ior: 1.5`, `roughness: 0` — fully
transmissive glass. A white opaque backdrop was added behind them (2026-08-29) precisely so that
absorption would have something bright to absorb. It reads **(201, 200, 196)** in the capture.

The ball centres, where the Fresnel term is ~0.04 and so **96 % of the pixel should be transmitted
light**, read:

| | before the backdrop (dark trees behind) | after the backdrop (201 behind) |
|---|---|---|
| ball 1 centre | (28.55, 26.33, 13.53) | (28.2, 26.6, 13.5) |
| ball 2 centre | (28.48, 25.82, 13.59) | (27.8, 25.6, 12.6) |
| ball 3 centre | (29.09, 25.90, 13.71) | (28.7, 25.7, 12.9) |

**Changing what is behind the balls does not change the balls.** They are also darker than the trees
they used to stand in front of. What remains is `reflectedColor * fresnelDielectric` — the cubemap
reflection at 4 % — so `SurfaceTransmissionColor` is contributing ~nothing.

⚠️ Ball 1 declares **no** `KHR_materials_volume`, so its `thicknessFactor` is the spec's 0, the
refraction ray has zero length and the sample lands on the fragment's **own** screen position. Even
that reads black. The offset is not the explanation.

The generated shader is correct and was checked: it samples bindless 2D slot 4 (the grab pass) at
`gpRefractedUV`, and composes `reflected * F + transmitted * transmissionFactor * (1 - F)`.

## The lead — not yet proven

`Scene::sortRenderable()` (`Scenes/Scene.rendering.cpp`, ~line 1428) classifies as:

```cpp
if ( isOpaque )              { … Opaque / OpaqueLighted … }
else if ( needsGrabPass )    { … TranslucentGB / TranslucentGBLighted … }
else                         { … Translucent … }
```

**`isOpaque` wins over `needsGrabPass`.** A material that is opaque in the alpha sense (these balls
declare no `alphaMode`, so glTF OPAQUE, and their base colour alpha is 1) but requires the grab pass
would then be drawn in `renderOpaque` — *before* `GrabPass::recordBlit()` — sampling a grab pass
that does not contain the scene it stands in front of, and landing **inside** the grab pass itself.
That fits every number above.

⚠️ **It does not yet explain why `IridescentDishWithOlives.glb` works**, and that asset's `glassDish`
and `glassCover` also declare no `alphaMode`. Its cover demonstrably reads the grab pass — adding the
volume thickness map moved 169 522 pixels of it. So either `isOpaque()` already accounts for
transmission somewhere and the difference lies elsewhere, or the two assets differ in a way not yet
identified. **Find that difference before changing the classification**: a one-line reorder there
moves every transmissive material between render lists.

## What remains

- [ ] Establish what `Renderable::isOpaque(layerIndex)` returns for each of the two assets, and why
  they differ. Instrument it rather than reading it.
- [ ] Then decide the ordering rule. `needsGrabPass` plausibly has to be tested FIRST, since a
  grab-pass material must be drawn after the blit whatever its alpha mode.
- [ ] Re-derive `VolumeAbsorptionProbe`'s numeric criterion once the balls actually transmit — the
  backdrop and the thickness/distance pair are already in place and waiting.

## ⚠️ Traps

- ⚠️⚠️ **Editing a bench asset while the engine is running gives you the OLD asset, silently.** The
  probe was regenerated with a different thickness and the recapture came back **byte-identical**
  (max diff 0) because the resource manager served the cached material. Restart the engine after
  touching a `.glb`, and check the diff against the previous capture is non-zero before drawing any
  conclusion from "nothing changed".
- ⚠️ A ball rendering a fisheye of the environment is easy to read as refraction. Here it is the
  4 % **reflection**, and the transmission is simply absent. Measure a patch against what is behind
  before naming the effect.

## References

- `Scenes/Scene.rendering.cpp` (the classification), `Graphics/Renderer.cpp` (~1954, the blit gate
  and `renderTranslucentGB` after it), `tools/gltf-conformance-bench/make-volume-probe.py`.
