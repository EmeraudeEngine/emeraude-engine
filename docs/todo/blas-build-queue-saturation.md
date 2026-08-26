---
id: blas-build-queue-saturation
title: BLAS build queue saturation under heavy GPU load
status: open
priority: medium
scope: Vulkan/RayTracing
opened: unknown
tags: [ray-tracing, scalability, measured]
---

# BLAS build queue saturation under heavy GPU load

## Why

Under heavy synthetic GPU load (vkmark), the RT demo options can trip `Xid 109` at load time. It
is **BLAS-build queue saturation** (16 concurrent builds), a scalability limit — **not** a
lifecycle bug: zero validation errors precede it, and the lifecycle defects around it were fixed
and verified (0 fault / 10 runs post-fix, the whole 6-run matrix clean without synthetic load).

## What remains

- [ ] Bound the number of concurrent BLAS builds, or pace them, so a loaded GPU cannot be pushed
  into a fault by the load path.

## References

- projet-alpha `docs/caution-points.md` (Sponza DEVICE_LOST section) records the measurement and
  the distinction from the lifecycle bug.
- Related: `rt-device-lost-game-logic.md` (a different, intermittent signature).
