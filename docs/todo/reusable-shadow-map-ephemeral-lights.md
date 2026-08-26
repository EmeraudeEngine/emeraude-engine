---
id: reusable-shadow-map-ephemeral-lights
title: Reusable shadow map for ephemeral lights
status: open
priority: unranked
scope: Graphics/ShadowMap
opened: unknown
tags: [shadow-map, memory]
---

# Reusable shadow map for ephemeral lights

## Why

A short-lived light (a muzzle flash, a spell, a projectile) allocating its own shadow map is a
waste: they never coexist in large numbers.

## What remains

- [ ] A pool of reusable shadow maps handed to ephemeral lights for their lifetime.

## References

- Related in spirit to the `SharedUniformBuffer` pooling item for short-lived entities.

## Origin

Inherited from the historical root `TODO.md`.
