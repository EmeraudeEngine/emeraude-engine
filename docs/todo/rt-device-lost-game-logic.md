---
id: rt-device-lost-game-logic
title: Ray tracing — intermittent DEVICE_LOST in the game-logic demo
status: open
priority: medium
scope: Vulkan/RayTracing
opened: 2026-07-26
tags: [ray-tracing, intermittent, measured]
---

# Ray tracing — intermittent DEVICE_LOST in the `game-logic` demo

## Why

Filed 2026-07-26 at **2/8 runs** with `RayTracing/Enabled` true: `device_fault` reported a
`READ_INVALID` at a low address plus an `INSTRUCTION_POINTER_FAULT`, with the last per-queue GPU
markers a MIX of `AS-build:end` and `transfer:image-layout-transition` — the signature of BLAS
builds racing uploads across the round-robined transfer queues.

**Re-tested 2026-08-04: DID NOT REPRODUCE, 0/8** (RTX 3070 Ti, Release from `develop` at
`3225123c`). Four plausible fixes landed since (`Device::waitTransferQueuesIdle()`, the
`DeferredDestructor`, the one-shot queue-family ownership fix `ebae3d4c`, the shared-UBO registry
race `e8d63525`).

⚠️ **0/8 is NOT proof of a fix.** At the original 25 % rate, eight clean runs happen by chance
about 10 % of the time. Left open deliberately.

## What remains

- [ ] **16 consecutive clean runs** would put that at ~1 % and justify deleting this file.

Repro (do NOT re-derive the protocol):

```
cd .claude-build-release/Release && for i in $(seq 1 8); do timeout 40 \
  ./projet-alpha --load-demo game-logic --disable-cef > /tmp/gl_$i.log 2>&1; \
  grep -c DEVICE_LOST /tmp/gl_$i.log; done
```

## ⚠️ Traps when reading those logs

- `device_fault` also matches the startup line `VK_EXT_device_fault detected and enabled` — grep
  the FAULT REPORT, not the extension name.
- `[Error][UIManagerService] No default page found !` appears in every demo, including the ones
  that never fail.
