---
id: normal-calculation-bypass-backfacing
title: Re-enable the normal-calculation bypass when a surface does not face a light
status: open
priority: unranked
scope: Saphir/LightGenerator
opened: unknown
tags: [shaders, performance]
---

# Re-enable the normal-calculation bypass when a surface does not face a light

## What remains

- [ ] Re-enable the early-out that skips normal-dependent work when the surface faces away from
  the light.

## ⚠️ Traps

- Two-sided lighting flips the normal view-side (`N = dot(N, V) < 0 ? -N : N`, NOT
  `gl_FrontFacing`). A naive "does not face the light" test must not fight that rule.

## Origin

Inherited from the historical root `TODO.md` ("Re-enable" — it existed once).
