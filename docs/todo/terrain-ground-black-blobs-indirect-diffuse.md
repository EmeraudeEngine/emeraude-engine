---
id: terrain-ground-black-blobs-indirect-diffuse
title: Hard-edged near-black blobs on the terrain ground, produced by the IndirectDiffuse concept
status: open
priority: unranked
scope: Graphics/Effects/Lighting (SSGI, RTGI), Graphics/GIDenoiser, Saphir (heightmapped ground material)
opened: 2026-09-25
tags: [indirect-diffuse, nan, denoiser, terrain]
---

# Hard-edged near-black blobs on the terrain ground, produced by the IndirectDiffuse concept

## Why

Reported by the macOS peer (screen-space lane, M2) and the Windows peer (Auto → traced lane, RTX 3060 Laptop) on
the bare terrain (`terrain --demo-options 100000,25,0,0,1`, 2026-09-25): scattered PURE-BLACK blobs, a few px to
~30 px, hard-edged, on the near ground (`Materials/Grounds/Mud001`: albedo + normal + HEIGHT texture). Reproduced on
Linux (screen-space lane): in a 1440 × 324 px crop of the near ground (mean 109/255), 163 px below 8/255 in 47 blobs,
the blobs at ~6/255.

Bisect by concept, same pose, one launch each (`PostProcess.disable(...)`):

| disabled | dark px | blobs |
|---|---|---|
| nothing | 163 | 47 |
| ContactShadows | 85 | 33 |
| AmbientOcclusion | 76 | 37 |
| **IndirectDiffuse** | **1** | **1** |
| all three | 0 | 0 |

The albedo is not the cause: `Mud001-color_a.png` has no alpha, its darkest texel is 14/255, 0.01 % below 20. A
blob sits in direct sun, so a merely ZERO indirect term would leave the sunlit albedo: near black means the whole
pixel is killed — the signature of a non-finite value reaching the tone mapper, spread into blocks by a filter.
`Core/Graphics/PostProcessing/DebugNonFinite = true` shows nothing, but it instruments the SSR buffers only.

⚠️ Regression status UNKNOWN: `GIDenoiser` (shared by SSGI and RTGI) was made camera-relative on 2026-09-25
(`27cb4df9`), the same day the peers saw the blobs; nobody had looked at this ground before.

## What remains

1. Instrument SSGI / RTGI / GIDenoiser outputs with the `DebugNonFinite` colouring, and name the buffer.
2. Check a pre-`27cb4df9` build at the same pose (regression or not).
3. Suspects: the GTAO sky-visibility term (bent normal on a heightmapped, normal-mapped ground), the albedo
   demodulation (a division), the à-trous filter spreading a NaN into blocks.

## References

- The bisect captures: session of 2026-09-25 (scratchpad `bare*.png`, pose = the terrain spawn).
