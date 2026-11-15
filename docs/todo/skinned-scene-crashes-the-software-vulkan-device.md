---
id: skinned-scene-crashes-the-software-vulkan-device
title: A skinned scene kills the software Vulkan device (llvmpipe) within a second — SIGSEGV in a rasterizer thread's shader code
status: open
priority: high
scope: Graphics/RenderableInstance (skinning), Scenes (acceleration structures), Saphir (generated shaders) — cause not located
opened: 2026-11-15
tags: [crash, defect, measured, llvmpipe]
---

# A skinned scene kills the software Vulkan device (llvmpipe) within a second — SIGSEGV in a rasterizer thread's shader code

## Why

The testbed's `animation-debug` (skinned characters) on the Mesa software device (`llvmpipe (LLVM 19.1.7, 256 bits)`,
Mesa's Vulkan driver `libvulkan_lvp.so`, forced with `Core/Video/VulkanDevice/ForceGPU` in a settings COPY) dies by
SIGSEGV in a `llvmpipe-N` thread — one of the driver's rasterizer workers, inside JIT-compiled shader code (no symbol),
a READ of an unmapped address. 4 launches of 4 on 2026-11-15 (Linux workstation):

| Build | When | What was read |
|---|---|---|
| Debug + ThreadSanitizer, `--disable-cef` | a few seconds after the scene became active (twice) | `SEGV on unknown address … caused by a READ`, the faulting frame in `libvulkan_lvp.so` |
| Release, validation layer loaded (proven), `--disable-cef` | 0.5 s after "The application successfully started." | return code -11, **0 VUID** before it |
| Debug under gdb, no layer | at the first frames | `Thread "llvmpipe-11" received signal SIGSEGV`, `#0 0x00007fff… in ?? ()` (JIT code), no engine frame on that thread |
| Debug + ThreadSanitizer, `--disable-cef`, the testbed's **`terrain`** (NO skinned object in that scene) | at the shutdown, while the scene was being removed — after 4 minutes of load and a run without a fault | `SEGV on unknown address … caused by a READ`, frame #0 without a symbol (JIT code), frame #1 in `libvulkan_lvp.so`; the process aborted there, no clean exit |

**The fifth row (2026-11-15, one run) is not a skinned scene**: the same signature — a read of an unmapped address from
the driver's generated code — at the moment the scene's GPU objects are released. So "skinned" in this item's title is
what the first four launches had in common, not an established cause: a read through an object that was just retired
(candidate (b) below, widened to any buffer or image a frame still in the driver's hands reads) fits the five.

The same device runs the static `default` scene without a fault (six TSan runs the same day, loads, unloads and a clean
shutdown). The hardware devices run `animation-debug` without a fault (the verification rounds of 2026-11-14, three
OS): a hardware GPU does not fault on a wild read, it returns whatever is there — if the read is ours, the picture is
wrong silently on every real device.

## What remains

- Reproduce the `terrain` case first in Release with the validation layer (load, wait, `Core.shutdown()`): a VUID at
  the release of the scene would name the object. Under TSan the scene takes four minutes to start.
- Locate the read. Bisect on llvmpipe by feature, one launch each (the crash is immediate): the ray-traced effects off
  (`LightingLane`, a launch decision — the `default` runs compiled the ray-traced effects' shader variants on this
  device, so it seems to answer the ray-query extensions: confirm it from the device's extension list), the shadow passes off, the
  skinned characters removed (a demo option, or another skinned scene), TAA / post-process off. Then read the shader
  that remains.
- Candidates, none proven: (a) ~~the skinning buffer read before it is built~~ — RULED OUT as the cause: the
  renderer created the skinning buffer and its descriptor sets on its first shadow pass while the logic thread tested
  and wrote them; that is a publication flag since 2026-11-15 (scenes doc 12), ThreadSanitizer no longer reports it, and
  the device still dies a second after the skinned scene is loaded (three runs of three that day); (b) a ray query through an acceleration structure whose
  dynamic (skinned) bottom level is not built yet or was retired; (c) an index into the bindless tables or a light /
  material buffer beyond what is bound; (d) a defect of the driver's own (its ray-query and JIT paths are young).
- When located: fix in the engine if the read is ours (the validation layer saw nothing: 0 VUID — GPU-assisted
  validation, which instruments the shaders' accesses, is the tool to try on the hardware device), or record the driver
  limit in `docs/caution-points.md` and close.

## ⚠️ Traps

- Nothing to learn from the hardware devices' silence: they do not fault.
- The software device is slow but starts fast (`default` in 6 s under TSan, the window reduced to 960 x 540 in the
  settings copy); `animation-debug` reaches its crash in under two seconds in Release.
- A core dump is not collected on the workstation (no `coredumpctl`): run under `gdb -batch -ex run -ex 'bt' --args …`.

## References

- Scenes doc 12 § *Plain state shared by the logic and rendering threads* (the skinning publication flag).
- `docs/caution-points.md` § *TSan on a machine whose display GPU runs the NVIDIA driver*.
