---
id: imposter-velocity-captured-before-billboard
title: The imposter's velocity is synthesized before its billboard exists (raw quad positions)
status: open
priority: unranked
scope: Saphir / Graphics::Material::StandardResource
opened: 2026-09-28
tags: [taa, velocity, imposter, saphir]
---

# The imposter's velocity is synthesized before its billboard exists (raw quad positions)

## Why

Found by reading while building the beam ribbon (2026-09-28), NOT measured. `SceneRendering` synthesizes the
velocity outputs (`AbstractVertexStage::synthesizeVelocityClipPositions()`) before it calls
`material->generateVertexShaderCode()`, and the synthesis writes `vertexPositionExpression()` into the GLSL as it
runs. `StandardResource` switches the imposter billboard on (`enableImposterBillboarding()`) inside
`generateVertexShaderCode()` — AFTER. The velocity of an imposter is therefore computed from the raw `[-1, 1]²`
quad attribute, not from `imposterPosition`.

## What remains

1. Confirm in the generated GLSL of an imposter program (`Core/Graphics/Shader/ShowSourceCode`, `forest`): the
   `svClipPositionCurrent` line should read the raw position attribute.
2. Measure the effect before changing anything (TAA debug view on a moving camera over the far trees).
3. The fix is ready-made: move `enableImposterBillboarding()` into `StandardResource::prepareVertexStage()`, the
   hook the beam ribbon added for exactly this (`Material::Interface::prepareVertexStage()`, called before the
   synthesis).

## References

- `docs/subsystems/saphir/27-the-beam-ribbon-a-vertex-stage-that-builds-a-beam.md` § ORDER.
