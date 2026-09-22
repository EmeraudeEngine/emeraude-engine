---
id: instanced-vegetation-casts-no-shadow
title: Instanced vegetation casts no shadow — the shadow program declares a set the instance cannot bind
status: open
priority: unranked
scope: Graphics
tags: [vegetation, shadows, instancing, measured]
opened: 2026-09-22
---

# Instanced vegetation casts no shadow — the shadow program declares a set the instance cannot bind

## Why

Measured on the projet-alpha `forest` demo (240 trees in 16 groves, `Component::MultipleVisuals`),
2026-09-22. Every tree draw into the shadow map is **skipped**, 36 distinct instances reporting:

    Descriptor set contract violation: the sealed pipeline layout declares the
    'PerSceneTransforms' set, but the renderable instance cannot provide it.
    The draw call is skipped.
      Renderable  : Aspen0
      RenderTarget: ShadowMap (2048x2048)

The main view is unaffected — the trees render, they simply cast nothing.

⚠️ Do NOT confuse this with the vertex-buffer offset defect found in the same run
(`docs/caution-points.md` § *A sub-geometry range indexes the INDEX buffer*). That one made the
trees invisible everywhere and is FIXED; this one survives it and is a separate, narrower gap.

## What remains

The engine's own diagnostic states the shape of the fix and it is worth repeating:

> the generation-time condition (`Saphir::Generator`) and the binding-time condition of that set
> have diverged. **Fix the pair, never the binding alone.**

- Generation: `Saphir::Generator::ShadowCasting` enables `SetType::PerSceneTransforms` per
  renderable, for a renderable carrying `hasVegetationWind()` — the shadow pass needs the wind
  state so the shadow follows the sway.
- Binding: `RenderableInstance::Abstract::castShadows()` receives `sceneTransformsDS` and skips the
  draw when it is `nullptr`.

So either the shadow path must carry the descriptor set for an instanced renderable too, or the
generator must not ask for it in a case where it cannot be supplied — and that second branch costs
the feature (a swaying tree with a rigid shadow), so it is not obviously the cheap one.

⚠️ Worth checking first, before designing anything: `SceneRendering::useInstanceTransformsSet()`
refuses the set for an instanced program *unless* there is a velocity attachment, on the grounds
that instanced programs carry their model matrices in a VBO and only need the header. The shadow
generator has no equivalent rule. The two generators disagreeing about the same set is the most
likely seat of the divergence.

## References

- `src/Saphir/Generator/ShadowCasting.cpp`, `src/Saphir/Generator/SceneRendering.cpp:170`
  (`useInstanceTransformsSet()`).
- `src/Graphics/RenderableInstance/Abstract.cpp:1070` (the skip) and `:1008`
  (`traceMissingDescriptorSet()`).
- The vegetation wind itself: `src/Graphics/AGENTS.md` § 15c.
