---
id: specular-horizon-occlusion
title: Fade the environment reflection of normal-mapped texels whose reflection dips below the surface
status: open
priority: unranked
scope: Graphics/Material (declareEnvironmentFrame, IBL specular), Saphir (LightGenerator ambient pass), Graphics/Effects/Lighting (SSR, RTR composite)
opened: 2026-09-28
tags: [ibl, reflections, normal-mapping, specular-occlusion, state-of-the-art]
---

# Fade the environment reflection of normal-mapped texels whose reflection dips below the surface

## Why

Since `0c29b08a` (2026-09-28, docs/caution-points.md § Two-sided normals) a normal-mapped texel that leans away from
a grazing view keeps its normal instead of being flipped upside down. That is the correct shading normal, but its
reflection vector `reflect(I, N)` now points BELOW the geometric surface, and the environment term samples what lies
under the ground: on the glTF bench, the far rims of `NormalTangentTest`'s bumps (top-down view) reflect the green
ground of the sky map. Owner (2026-09-28): "Ok pour un item pour l'atténuation du reflet."

A real surface cannot reflect light arriving from below its own geometric plane: the relief it is made of occludes
it. Engines fade that term with a horizon factor built from the reflection vector and the GEOMETRIC normal.

## State of the art (check each against its source before implementing)

- J. Russell, "Horizon Occlusion for Normal Mapped Reflections", Marmoset blog, 2015 — the classic form:
  `horizon = min(1 + horizonFade · dot(R, Ngeometric), 1); specular *= horizon²`.
- J. Jimenez, X. Wu, A. Pesce, A. Jarabo, "Practical Real-Time Strategies for Accurate Indirect Occlusion",
  SIGGRAPH 2016 course (Activision) — specular occlusion from a visibility cone, of which the horizon test is the
  normal-map case.
- Google Filament's IBL evaluation applies a horizon fade of this form to the indirect specular (to confirm in its
  sources, `shading_lit` / light indirect).

## What remains

Owner decisions first (the engine has several reflection paths):

1. **Which terms**: the IBL prefiltered specular of the ambient pass (`declareEnvironmentFrame()`, the reflection
   and refraction consumers), the SSR / RTR composite (they read the G-buffer normal), the clear coat's layer — all of
   them, or the IBL only.
2. **The fade strength**: a fixed `horizonFade` (Russell's 1.3) or a material/setting knob.
3. **Where the geometric normal comes from**: the frame's third column is already there in every path that has a
   normal map (`WorldTBNMatrix[2]`, `transpose(ViewTBNMatrix)[2]`); the post-process composites would need it in
   the G-buffer or a reconstruction from depth.

Then measure: the glTF bench pair (before / after) on `NormalTangentTest`, `NormalTangentMirrorTest`, `BoomBox`,
`ClearCoatTest` — the far rims must stop reflecting the ground, the front views must not move.

## ⚠️ Traps

- The fade needs the GEOMETRIC normal: computed from the perturbed one it is identically 1 (a reflection never dips
  below the plane of the normal it was reflected about).
- Double-sided back faces: the geometric normal must be the one already turned toward the viewer, or the fade
  kills the reflection of every back face (the 2026-09-22 `NormalTangentTest_back` case).
- Fade the ENVIRONMENT (indirect) specular only: the direct lights' specular is already bounded by NdotL.

## References

- docs/caution-points.md § Two-sided normals (the 2026-09-28 fix and its bench measurements).
- Bench captures of 2026-09-28: before/after crops of `NormalTangentTest_top-down`, `BoomBox_front`,
  `ClearCoatTest_front`.
