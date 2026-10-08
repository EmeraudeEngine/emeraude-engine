---
id: ambient-pass-dead-branches
title: Ambient pass generator lost its glass and Standard+IBL legs (three dead branches, deleted 2026-10-08)
status: open
priority: high
scope: src/Saphir/LightGenerator.cpp
opened: 2026-10-08
tags: [ave-robustus-ii, saphir, lighting, defect]
---

# Ambient pass generator lost its glass and Standard+IBL legs (three dead branches, deleted 2026-10-08)

## Why
Found by GCC `-Wduplicated-cond` when the paranoid warning set was turned on (2026-10-08). In the ambient-pass
`if / else if` chain of `Saphir/LightGenerator.cpp`:

| Line | Condition | Already taken by |
|---|---|---|
| 1185 | `m_useReflection` | — (PBR low-quality fallback) |
| 1241 | `m_useRefraction` | — (PBR low-quality refraction) |
| 1261 | `m_useReflection && m_useRefraction` | 1185 → **never reached** |
| 1273 | `m_useReflection` | 1185 → **never reached** |
| 1294 | `m_useRefraction` | 1241 → **never reached** |

So the glass ambient blend (Fresnel mix of reflection and refraction), and the reflective / refractive ambient legs with
IBL that follow, are never emitted: every reflective or refractive material takes the 1185 / 1241 branches, whatever its
lighting model. The chain was likely meant to be split by a material-model condition (PBR vs Standard) — commits
`7bbb3cd7` ("the chain order is a structure") and `cdcf68e7` touched it last.

## State (owner decision, 2026-10-08)
The three dead branches were DELETED so the paranoid `-Werror` build is green: the rendering is byte-for-byte unchanged
(they never ran). Their code is in engine git history at the parent of the warning-pass commit — `git log -S
"m_useReflection && m_useRefraction" -- src/Saphir/LightGenerator.cpp` finds it.

## What remains
- Read the history of the chain, decide with the owner which material reaches which branch.
- Fix the structure so every branch is reachable (or delete the ones that are truly obsolete).
- Visual proof: a glass material and a Standard reflective material with IBL, before / after, pixel-measured; 0 VUID.

## References
- projet-alpha `docs/plans/ave-robustus-ii.md` § 6.1.
