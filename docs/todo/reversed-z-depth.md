---
id: reversed-z-depth
title: Reversed-Z depth (float depth reversed, optionally an infinite far plane) so a kilometre-scale view keeps its precision
status: open
priority: unranked
scope: Graphics (projection, depth test, every depth reader), Saphir, post-process effects, shadows
opened: 2026-10-27
tags: [depth, precision, projection, vulkan]
---

# Reversed-Z depth (float depth reversed, optionally an infinite far plane) so a kilometre-scale view keeps its precision

## Why

The depth test is conventional everywhere: `D32_SFLOAT`, `VK_COMPARE_OP_LESS_OR_EQUAL`, near 0 / far 1
(`ViewMatricesInterface.hpp`, `Camera::setNearestObjectDistance()`). A float's exponent then piles its precision near
the near plane, where a large scene needs it least. Reversing the range (near 1, far 0, `GREATER_OR_EQUAL`, clear to 0)
matches the float's distribution to the perspective's 1/z and gives an almost uniform relative precision at every
distance — and allows an infinite far plane (Reed, "Depth Precision Visualized", NVIDIA 2015; Upchurch & Desbrun,
"Tightening the Precision of Perspective Rendering", JGT 2012; Kemen, "Maximizing Depth Buffer Range and Precision",
2012).

Trigger (owner, 2026-10-27): projet-alpha's `water-world` now sees the sea to the horizon — view distance 25 km with
the default 0.1 m near plane, far/near = 250 000 (it was 4 km). No z-fighting was seen on a shore ~2 km away (the
temporal difference of the sand band equal at 25 km and 4 km, 0.23 vs 0.19/255), but the margin is the float's, not a
design. Owner decision: the demo takes the long view now, the engine gets reversed-Z as this separate item.

## What remains

- Inventory every depth producer and consumer: the projection matrices (perspective, orthographic, cascaded shadow
  maps, cubemap shadows, the RT camera rays), the depth compare ops and clear values of every pass, the pipelines'
  depth-bias signs, every shader that reconstructs a position or a view depth from the depth buffer (the post-process
  chain's view block, SSR, SSAO, SSGI, TAA, motion blur, DoF, fog, volumetrics, the grab pass), depth-based opacity of
  the water, the readbacks (picking, focus, metering), the debug views.
- Decide with the owner: reversed-Z with a finite far plane, or with an infinite far plane (then the view distance
  becomes a culling distance only); shadow maps reversed too or not (orthographic: precision is linear, reversing is
  optional).
- The proof: a depth-precision bench (two coplanar-ish surfaces at 1 m … 20 km, z-fighting counted per pixel) before /
  after, every demo at 0 `VUID-`, the three OS.

## ⚠️ Traps

- Every place that hard-codes `1.0` as "the sky / nothing drawn" (depth == 1) flips to `0.0`: grep for it, it is the
  classic regression of a reversed-Z port.
- Depth bias changes sign; slope-scaled bias must be re-tuned.
- `temporal-upscalers-fsr-xess-dlss` needs to know the convention (its § Depth).

## References

- N. Reed, "Depth Precision Visualized", NVIDIA developer blog, 2015.
- P. Upchurch, M. Desbrun, "Tightening the Precision of Perspective Rendering", Journal of Graphics Tools 16(1), 2012.
- engine `src/Graphics/ViewMatricesInterface.hpp` (`setNearestObjectDistance()`'s warning), projet-alpha
  `docs/subsystems/builtin/16-14-waterworld-scene.md` § the sea to the horizon.
