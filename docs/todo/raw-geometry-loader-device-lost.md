---
id: raw-geometry-loader-device-lost
title: raw-geometry-loader loses the device on its first frames (GPU read fault, 0 VUID)
status: open
priority: unranked
scope: Graphics (attribution open), projet-alpha demo raw-geometry-loader
opened: 2026-10-05
tags: [device-lost, crash, measured]
---

# raw-geometry-loader loses the device on its first frames

## Why

Found 2026-10-05 while sweeping the ethereal-player demos (RTX 3070 Ti, Linux, Release, engine `a7409def`):
`./projet-alpha --window-less --load-demo raw-geometry-loader` aborts (exit 134) right after the scene loads,
every time (3/3, also when switched to from `physics-debug` in a running instance). It predates the ethereal-player
change of that day: the build with the committed `Act.cpp` / `Player.*` crashes the same way.

```
[Error][VulkanFence] Unable to wait the fence : VK_ERROR_DEVICE_LOST !
[Error][VulkanDevice] DEVICE LOST (Fence::wait) — GPU diagnostics follow:
	[device_fault]
	  - addr=… type=VK_DEVICE_FAULT_ADDRESS_TYPE_INSTRUCTION_POINTER_FAULT_EXT precision=0x10
	  - addr=0x0 type=VK_DEVICE_FAULT_ADDRESS_TYPE_READ_INVALID_EXT precision=0x1000
	[checkpoints] last GPU markers reached per queue: transfer:image-layout-transition (every queue)
```

With the validation layers ON (`Core/Video/VulkanInstance/EnableDebug`): **0 VUID**, same fault. An invalid READ
from a shader (instruction pointer fault), not an API misuse the layers see: a buffer device address or a bindless
index the demo's raw geometry leaves dangling is the first suspect.

## Peers (2026-10-05)

- macOS M2 (MoltenVK, ScreenSpace lane only: no RT, no acceleration structure): NO device loss, 3/3 launches with
  validation, 0 VUID, clean shutdown. The first discriminator to run on Linux is therefore
  `LightingLane = ScreenSpace` at launch: if the loss goes away, the faulting read is in an RT pass or a BLAS/TLAS
  address.

- Windows (2026-10-05, engine `735d9a83`): lost on BOTH GPUs. NVIDIA: `VK_ERROR_DEVICE_LOST` on a fence wait within
  40 s, exit 0xc0000409, 0 VUID before. AMD: lost at the `Queue::submit` of the IBL BAKE of the background (`DarkSky`),
  `VK_EXT_device_fault`: `addr=0x0 type=READ_INVALID` — a NULL-address read; 26 VUIDs, all after the loss.
- NOT the in-texture star mask of the IBL bake / RT lanes (`a7409def`, which touches `IBLBaker`): on Linux, the build
  with that commit's code reversed still loses the device, 2/2 (2026-10-05). On Linux the loss also comes before any
  "Environment IBL baked" line.
- Lead: a null address read on the RT-capable devices only (the M2 has no RT and no loss) — a buffer device address
  of an empty or not-yet-uploaded geometry (a raw geometry without indices?) in a BLAS / TLAS or an RT material table.

## What remains

- Attribute: the GPU profiler / checkpoints of the frame, then switch the post-process lane (`None`, `ScreenSpace`)
  and the RT lane (LightingLane at launch) to see which pass reads the address; the demo's options
  (`--demo-options`) to find the geometry that triggers it.
- Placement: the item sits here because a fault without a VUID points at the engine; move it to projet-alpha if the
  demo turns out to build something invalid.

## References

- `src/Builtin/RawGeometryLoader.cpp` (projet-alpha).
- Items `post-device-loss-robustness`, `renderer-fail-fast-on-device-loss` (what happens AFTER the loss).
