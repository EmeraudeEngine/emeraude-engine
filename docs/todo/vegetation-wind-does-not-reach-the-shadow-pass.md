---
id: vegetation-wind-does-not-reach-the-shadow-pass
title: Vegetation sways in the main pass, but its shadow stays still
status: open
priority: unranked
scope: Saphir/Generator/ShadowCasting, Graphics/RenderableInstance
tags: [vegetation, shadows, shaders]
opened: 2026-09-21
---

# Vegetation sways in the main pass, but its shadow stays still

## Why

The wind landed in the main pass on 2026-09-21 and is verified on screen: on the projet-alpha
`tree-generator` bench, **16.34 %** of the foliage pixels move between two captures two seconds
apart, while the ground under the trees moves **0.70 %** — trunk bases and noise. The shadow map
holds the undisplaced tree, so a swaying canopy casts a frozen shadow. It is visible as soon as
there is a sun.

The cause is structural, not an oversight: the wind state rides in the instance-transforms SSBO
header, and **the shadow pass never receives that descriptor set**. `PerSceneTransforms` is
enabled by `Saphir::Generator::SceneRendering` alone, and
`RenderableInstance::Abstract::castShadows()` takes no `sceneTransformsDS` parameter at all —
only `render()` does.

⚠️ Note that the two halves are currently CONSISTENT, and that is why nothing flickers:
`LightGenerator::ShadowMap` evaluates the shadow term at the **undisplaced** position, which is
what the undisplaced shadow map holds. Displacing one without the other reproduces the defect the
skinning path already hit — "every animated pose self-occluded, the whole body flickered down to
the ambient term" (`LightGenerator.ShadowMap.cpp:96`). **The two must move together.**

## What remains

1. Enable `SetType::PerSceneTransforms` in `Saphir::Generator::ShadowCasting` and declare the same
   SSBO block there (the header layout must stay in lockstep with
   `Scenes::SceneInstanceTransforms::Header`).
2. Pass the descriptor set down: `castShadows()` gains a `sceneTransformsDS` parameter, bound the
   way `render()` binds it. There is a single real call site,
   `Scenes/Scene.rendering.cpp:391`.
3. Call `VertexShader::enableVegetationWind()` in the shadow generator under the same condition as
   `SceneRendering` does.
4. **In the same commit**, switch `LightGenerator::ShadowMap` to sample at
   `vertexShader.vertexPositionExpression()` instead of its own skinning ternary, so the shadow
   term follows whatever displacement the shadow map was built with.

## Traps

- ⚠️⚠️ Step 1 changes the **sealed pipeline layout of every shadow caster**, not only of
  vegetation: a descriptor set is added to a layout shared by all of them. Verify the directional
  CSM path explicitly — `docs/caution-points.md` records directional shadows being dead for an
  unknown number of sessions while the captures looked plausible, and only the validation layers
  caught it.
- ⚠️ Validation layers ON, and read the FIRST VUID of a burst.
- ⚠️ The measurement that settles it is the one above: diff two captures and compare the moved
  pixel ratio of the foliage band against the shadow band. Eyeballing a swaying tree does not tell
  you whether its shadow follows.

## References

- `Scenes/SceneInstanceTransforms.hpp` — the header carrying the wind state.
- `Saphir/VertexShader.cpp` — `generateVegetationWindCode()`, `vertexPositionExpression()`.
- projet-alpha `src/Builtin/AGENTS.md` § 6b — the bench and how the motion was measured.
