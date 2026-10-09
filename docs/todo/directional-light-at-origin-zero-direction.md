---
id: directional-light-at-origin-zero-direction
title: A directional light placed at the origin gets a ZERO direction in five places
status: open
priority: high
scope: src/Scenes/Component/DirectionalLight.cpp, src/Scenes/LightSet.cpp
opened: 2026-10-09
tags: [lighting, defect]
---

# A directional light placed at the origin gets a ZERO direction in five places

## Why
Found by the `Vector::normalize()` audit (2026-10-09, base item `vector-normalized-absolute-epsilon`, closed). A
directional light that does not use its direction vector takes `-position.normalized()`: at the origin that is the
zero vector (before and after the normalize fix). `DirectionalLight.cpp:100-105` guards it (`lengthSquared() < 1e-12F`
→ the frame's forward vector), but the other sites do not: `DirectionalLight.cpp` ~307, 320, 333, 346 and
`LightSet.cpp:733` — a zero light direction there (shadow matrices, the light UBO).

## What remains
- One helper used by every site (the guarded rule of line 105), and a test or a runtime check with a directional light
  at the origin (0 VUID, a lit scene).

## References
- `src/Scenes/Component/DirectionalLight.cpp`, `src/Scenes/LightSet.cpp`.
