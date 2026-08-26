---
id: shared-ubo-pooling-short-lived
title: Extend SharedUniformBuffer pooling to short-lived entities
status: open
priority: unranked
scope: Vulkan
opened: unknown
tags: [vulkan, memory, performance]
---

# Extend SharedUniformBuffer pooling to short-lived entities

## Why

Particles and projectiles allocate and free UBO/VBO ranges constantly; the pooling strategy that
already serves long-lived entities would cut that churn.

## ⚠️ Traps

- The shared-UBO registry has already produced a **non-deterministic missing-geometry** defect
  through a check-then-act race on a shared registry (fixed 2026-08-04, `e8d63525`). Any pooling
  extension touches that same registry.
- Point-light light passes share ONE descriptor set and are distinguished by the **dynamic
  offset** — any dedup must key on the offset too.

## Origin

Inherited from the historical root `TODO.md`.
