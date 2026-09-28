---
id: sprite-disappears-at-some-angles
title: Graphics — a sprite disappears from some viewing angles
status: open
priority: unranked
scope: Graphics
opened: 2026-09-28
tags: [graphics, sprites]
---

# Graphics — a sprite disappears from some viewing angles

## Why

Found by the owner on 2026-09-28: **a sprite vanishes from a certain angle, and comes back when one
walks around it**. Seen on two demos with the same sprite (`fire001`, animated, additive, unlit,
depth write off, `CenterAtBottom`):

- `citadel`: the keep's door torch showed no flame in a window-less capture (camera at
  (-24, 1.6, -36.5) looking at (-27, 2.3, -41), the flame's base at (-27.6, 2.3, -39.17)), while
  the gate torches and the braziers, seen from other angles, showed theirs;
- `game-logic`: the `Fire` actor (the same `fire001`) — the owner reports the same behaviour.

Not the sprite's animation: every one of `fire001`'s 60 frames is well filled (mean gray 106-145 /
1000 over the frame; no empty frame).

## Leads (to check, none measured yet)

- **Back-face culling of the billboard quad**: if the frame built by the billboard (the spherical
  `CartesianFrame::getSpriteModelMatrix()`, or the upright `getUprightSpriteModelMatrix()` added the
  same day, or the GPU `getBillBoardModelMatrix()` / `getUprightBillBoardModelMatrix()`) comes out
  with its normal AWAY from the camera for some directions, a culled quad disappears. The spherical
  path goes through `computeYAxis()`, which has special cases around the vertical.
- **Culling by extent**: the sprite's render box is the flat quad in object space; the billboard
  rotation is not in it (`Scene.rendering.cpp` notes that volumes built at Z = 0 ignore the vertex
  billboard). Seen edge-on from the side, a flat box may fall out of the frustum test.
- **Sorting / blending** of the translucent pass, depth-tested against the geometry behind.

## What remains

- [ ] Reproduce: walk around a `fire001` sprite (`citadel` keep torch, `game-logic` Fire) and note the
  angles where it vanishes.
- [ ] Find the cause among the leads (or another) and fix it in the engine.