---
id: temporal-upscalers-fsr-xess-dlss
title: Temporal upscalers — AMD FSR, Intel XeSS and NVIDIA DLSS as upscaling options
status: open
priority: unranked
scope: Graphics/Renderer, Graphics/PostProcessStack, Graphics/Effects/Resolve
opened: 2026-09-28
tags: [upscaling, fsr, xess, dlss, taa, temporal, render-resolution, owner-request]
---

# Temporal upscalers — AMD FSR, Intel XeSS and NVIDIA DLSS as upscaling options

## Why

Owner, 2026-09-28: integrate **AMD FSR**, **Intel XeSS** and **NVIDIA DLSS** as upscaling options. The
scene would render at a reduced internal resolution, and a temporal upscaler would rebuild the output
resolution from the jittered frames, the depth and the motion vectors. That buys GPU time (the owner's
screen is 2880×1620), and each of the three also runs at a 1:1 ratio as an anti-aliasing mode (FSR "Native
AA", DLAA, XeSS native).

Owner decision the same day: **the in-house TAA is improved first** (`taa-trails-under-motion`). This item
does not replace it: the TAA stays the default and the fallback on every machine. UE5's TSR was discussed
too; its code is under the Unreal EULA (ideas only, never code) and it is not part of this item.

## What remains

1. **State of the art and licences first** (`AGENTS.md`, research before any non-trivial addition). Verify
   and cite, for each SDK: its current version, its Vulkan backend, Linux / Windows / macOS (MoltenVK)
   support, which GPUs it runs on, its licence, and its redistribution terms (can the binaries ship next to
   an LGPLv3 engine and a proprietary application?). What is believed, to be confirmed:
   - **FSR 2 / FSR 3 upscaler** (FidelityFX SDK): MIT source, compute shaders, any vendor, Vulkan backend.
     The only one that could run on macOS through MoltenVK. FSR 4 (ML) targets recent AMD GPUs only;
     its availability and licence are unknown here.
   - **XeSS** (Intel SDK): closed binary. It runs on Intel GPUs (XMX) and, through a DP4a path, on other
     vendors. Vulkan support to be checked for the current version.
   - **DLSS** (NVIDIA DLSS SDK / NGX, or Streamline): closed binary, RTX GPUs only, Vulkan and Linux
     supported. Nothing on macOS.
2. **Owner decisions to take, with options once the research is done:**
   - Integrate each SDK directly, or go through an abstraction (NVIDIA Streamline wraps DLSS and can host
     others; AMD's FidelityFX API is another). Or have an engine-side interface with one backend per SDK.
   - Where the upscaler lives. It could be an occupant of the `TemporalAA` slot (it replaces the TAA
     resolve, and `PostProcess.select(TemporalAA, <effect>)` already switches occupants), or a stage of its
     own, placed after the TAA slot.
   - Closed SDKs as optional build components (a CMake option per SDK, the binary detected at run time),
     so a machine without them loses nothing.
   - Out of scope unless the owner says otherwise: frame generation (FSR 3 FG, DLSS FG) and DLSS Ray
     Reconstruction (it replaces the RT denoisers).
3. **Engine prerequisite: split the render resolution from the output resolution.** Today the scene,
   the G-buffer, the post-process chain and the swap chain share one size. Every effect then has to know
   which of the two it runs at: SSR / RTGI / RTAO (already half-res of the RENDER size), bloom and glare,
   DoF and motion blur (before or after the upscale: each SDK documents its expected order), tone mapping
   (the SDKs want HDR input, with an exposure value — see `scene-colour-pre-exposure`), and the UI / CEF
   overlay (composited at OUTPUT resolution, after the upscale).
4. **Inputs each SDK expects**, to map onto what the engine produces:
   - **Jitter**: today an 8-phase Halton (2,3). The SDKs want a phase count that grows with the upscale
     ratio (FSR 2: `8 × ratio²`), and the offset reported per frame in their convention.
   - **Motion vectors**: the engine's are jitter-free, in NDC delta. Check the SDK's expected space (UV or
     pixels), its sign, whether the jitter is included, and its dilation expectations.
   - **Depth**: its convention (reversed-Z or not, infinite far) and its format.
   - **Reactive / transparency masks** for what has no correct velocity. ⚠️ Known gaps: particles report
     zero motion, and translucent surfaces overwrite the velocity and depth behind them
     (`taa-trails-under-motion`, What remains point 3). An upscaler ghosts on them exactly like the TAA.
   - **Negative texture LOD bias** at a reduced render resolution (`log2(render / output)`, the SDKs give
     their value). The samplers set no `mipLodBias` today.
   - **Exposure / pre-exposure** input.
   - **History reset** on a camera cut.
5. **Runtime surface**: settings keys (`Core/Graphics/…/Upscaler`, quality mode, sharpness), console / MCP
   commands for a live switch, and an A/B protocol against the TAA with
   `projet-alpha/docs/temporal-stability-measurement.md` (parked-camera shimmer, camera-step trails,
   the `animation-debug` sword).

## ⚠️ Traps

- **A wrong velocity ghosts under any upscaler.** The Paladin's sword comb of 2026-09-26 was the skinned
  pose history advancing per logic tick (fixed 2026-09-28, `docs/subsystems/scenes/13-instance-transforms.md`),
  not the resolve. Check the moving thing reports a correct previous position before blaming an SDK.
- **The instanced motion history still advances per logic update** (same document). An upscaler would show
  the same defect as soon as a demo enables `EnableInstanceMotionHistory`.
- **Licences**: UE TSR code, never. FSR is MIT: cite it at the point of use, as `TAA.cpp` already does. For
  XeSS and DLSS, read their licence and redistribution terms before shipping anything, and keep them out of
  the default build if they do not allow it.
- Cross-platform rule: nothing may make a feature exist on one OS only when it can exist on all three. The
  TAA and FSR (if it works under MoltenVK) are the portable baseline; XeSS and DLSS are optional.

## References

- `src/Graphics/Effects/Resolve/TAA.cpp` (the resolve an upscaler would replace; FSR 2 citations in it).
- `src/Graphics/EffectSlot.hpp`, `src/Graphics/PostProcessStack.hpp` (the slot and jitter polling).
- `src/Graphics/Renderer.hpp` (Halton (2,3) jitter applied to the main view).
- Items: `taa-trails-under-motion`, `scene-colour-pre-exposure`.
- External, to verify and cite at step 1: AMD FidelityFX SDK (FSR 2 / 3) documentation, the Intel XeSS
  SDK developer guide, the NVIDIA DLSS programming guide and Streamline documentation.
