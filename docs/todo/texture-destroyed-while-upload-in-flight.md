---
id: texture-destroyed-while-upload-in-flight
title: A texture released right after its upload destroys its VkImage while the transfer still uses it
status: open
priority: unranked
scope: Vulkan / Graphics resources (texture upload, destruction)
opened: 2026-09-30
tags: [vulkan, lifetime, textures, validation]
---

# A texture released right after its upload destroys its VkImage while the transfer still uses it

## Why

Found on Linux (RTX 3070 Ti) while proving the triad's WAD hardening (section 3, 2026-09-30): a WAD whose BSP has a
cycle is refused AFTER its wall / flat textures were created, uploaded and BC7-compressed. The loader's references
die with the refusal, the textures are released at once, and:

- 4 × `VUID-vkDestroyImage-image-01000`: "`vkDestroyImage()` can't be called on VkImage … that is currently in use
  by VkCommandBuffer …" — the upload (or compression) command buffer has not completed;
- then 4 × `VUID-vkDestroyDevice-device-05137` at shutdown: objects left alive.

Two more variants, same session, timing-dependent (the identical runs before showed none):

- the same refused WAD, another run: 2 × `VUID-vkCmdCopyBufferToImage-dstImage-parameter` and 4 ×
  `VUID-VkImageMemoryBarrier-image-parameter` — an upload RECORDED after its image was destroyed (use after
  destruction, worse than the first variant);
- at SHUTDOWN after 20 heavy glTF loads in a row through the console (Sponza, ABeautifulGame, FlightHelmet…):
  4 × `VUID-vkDestroyBuffer-buffer-00922` ("currently in use by VkCommandBuffer"), 2 × `VUID-vkFreeMemory-memory-00677`
  and 6 × `VUID-vkDestroyDevice-device-05137`, right after the resource containers' "N resource(s) unloaded".

Nothing to do with WADs: ANY load that fails after creating textures (glTF, FBX, USD, a store resource whose
dependency fails) takes the same path. The frames-in-flight rule (graphics doc 25) has no counterpart for the
transfer / compute work of an upload.

## Repro

1. Copy `/usr/share/games/doom/doom1.wad`, and in the copy set the right child of the ROOT BSP node of E1M1 to the
   root itself (NODES lump of E1M1: node `count - 1`, the uint16 at byte 24 = `count - 1`).
2. Launch projet-alpha WITHOUT a demo, validation layers ON, then `Core.openFiles("<copy>")`: "Corrupted WAD … the
   BSP node #235 is reached twice (a cycle)" then the VUIDs above.

(The generator used: the triad session's scratch `hostile/` script — rebuild it from the step above.)

## What remains

- Where a texture's image / buffers are destroyed, wait for (or defer past) the upload / compression submission that
  still references them: a per-resource fence or timeline value, or a deferred-destruction queue drained when the
  transfer completes. An architecture choice for the owner.
- Then check the 4 leftover objects at vkDestroyDevice disappear with it.

## References

- Engine `docs/subsystems/graphics/25-16-frame-synchronization-double-buffering-and-the-logic-trip.md` (Rule 1).
- Vulkan spec, VUID-vkDestroyImage-image-01000.
