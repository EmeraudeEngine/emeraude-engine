---
id: post-process-chain-run-allocates-vectors-at-every-flush
title: The post-process chain allocates four std::vector per frame — the overlay group, its denoise entries, their partition, the combine contributions
status: open
priority: unranked
scope: Graphics/PostProcessor (ChainRun: m_combineGroup, recordSharedDenoise(), takeDenoisePartition(), recordCombine())
opened: 2026-11-15
tags: [allocatus-reduxus, per-frame-allocation, post-processing]
---

# The post-process chain allocates four std::vector per frame — the overlay group, its denoise entries, their partition, the combine contributions

## Why

Seen while the chain executor was split into `PostProcessor::ChainRun` (2026-11-15; the owner kept the split free of any
other change): every execution of the chain, and every flush of an overlay group inside it, builds `std::vector`s on the
heap —

- `ChainRun::m_combineGroup` (`IndirectPostProcessEffect *`), one per run (two runs in a cut frame);
- `recordSharedDenoise()`: `denoiseGroup` (`DenoisePass::GroupEntry`), then one `partition` per working resolution
  (`takeDenoisePartition()`);
- `recordCombine()`: `contributions` (`CombinePass::GroupEntry`).

Each is bounded by the number of effect slots (`EffectSlotCount`): a group cannot hold more members than the chain has
occupants. Typical frames flush two groups (pipeline doc § 3b), so about eight heap allocations per frame on the
rendering thread.

## What remains

- On an optimization order only (plan Allocatus Reduxus): each vector becomes an `EmEn::Base::StaticVector` sized by
  `EffectSlotCount` when `N × sizeof(T)` stays under 4 KiB — check `sizeof(DenoisePass::GroupEntry)` and
  `sizeof(CombinePass::GroupEntry)` first —, else one member vector reserved once and cleared per flush.
- `DenoisePass::record()` and `CombinePass::record()` take `const std::vector< GroupEntry > &`: their signatures move to
  `std::span< const GroupEntry >` with it.
- Proof: the removed allocations counted (a heap profile of one frame before and after), the GPU timing zones and the
  pixels unchanged (pipeline doc § 3d, the method).

## References

- `src/Graphics/PostProcessor.cpp` — `PostProcessor::ChainRun`.
- `docs/post-processing-pipeline.md` § 3b, § 3c, § 3d.
