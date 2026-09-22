---
id: back-face-environment-reflection-normal
title: The back face of a double-sided material loses its F0 in the environment term
status: open
priority: unranked
scope: Graphics/Material/StandardResource (reflectionNormal), Saphir/LightGenerator (ambient/IBL NdotV)
opened: 2026-09-22
tags: [pbr, double-sided, ibl, gltf-conformance, cross-platform-bench]
---

# The back face of a double-sided material loses its F0 in the environment term

## Why

Found by the macOS peer session, then REPRODUCED on Linux (RTX 3070 Ti, same commit, same ScreenSpace lane),
so it is neither the platform nor the lane. On `NormalTangentTest_back`, against Khronos `back-side.png`, the
gold column's back has no gold: its plates read (108,126,156) from the back, against (147,141,93) from the
front. The blue column's back is dark slate, and the pink back is a near-perfect mirror (239,238,239).
`NormalTangentMirrorTest_back` shows the same.

Suspected cause, from reading the code and NOT measured: `reflectionNormal` (`StandardResource.cpp`, 12
declarations) is never turned toward the viewer. Only the direct-light path does it
(`LightGenerator.PBR.cpp:571/581`, `N = dot(N, V) < 0.0 ? -N : N`, the two-sided lighting of `5bec23db`).
On a back face, `NdotV = max(dot(reflectionNormal, -I), 0)` clamps to 0, the Fresnel pins at 1, and F0 is
lost. `reflect()` does not care about the sign, which is why the reflected image still looks plausible.

## What remains

- [ ] Measure it: a double-sided plane seen from both sides with one known environment, and compare the
      environment term per side.
- [ ] One two-sided rule for every normal the fragment uses (direct, ambient, reflection, clear coat).
- [ ] Re-check `TransmissionTest`: through the MASK spheres' cut-outs, the inside surface is an untinted,
      upside-down mirror of the sky (macOS). It is probably the same cause.

## ⚠️ Traps

- Bench two platforms only with the SAME viewer keys. The first Mac/Linux pair was shot on two different
  backgrounds (`GreenLandscape`, the default, against the owner's value), so a pixel diff of that pair
  compares skies, not renderers.
