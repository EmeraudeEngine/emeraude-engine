## 14. Pipeline Efficiency Objectives

> [!CRITICAL]
> **Baseline established via RenderDoc programmatic analysis** (LightAndShadowDebug, 6 objects, 3 lights).
> Every pipeline modification must be measured against this baseline using `/renderdoc-capture`.

### Current State (Baseline)

| Metric | Value | Assessment |
|--------|-------|------------|
| Draw calls per frame | 86 | Acceptable for 6 objects |
| Render passes per frame | 48 | **High** — 42 post-process + 3 shadow + 2 geometry + 1 overlay |
| Draws per object (geometry) | 4× | **Redundant** — multi-subpass G-buffer |
| Post-process passes | 42 | **Excessive** — 9 effects producing 42 passes |
| Compute dispatches | 0 | **Missing** — all effects use fragment shaders |
| Heaviest mesh | Ground plane, 2M indices × 7 renders = 14M/frame | **Disproportionate** |

### Optimization Roadmap (Priority Order)

| Priority | Objective | Current | Target | Impact |
|----------|-----------|---------|--------|--------|
| **P1** | MRT single-pass deferred | 4 draws/object | 1 draw/object | -75% geometry draws |
| **P2** | Fuse chainable post-process passes | 42 passes | ~15-20 passes | Fewer render pass transitions |
| **P3** | Compute shaders for blur/SSAO | Fragment-only | Compute + Fragment | Shared memory, no RP transitions |
| **P4** | Mesh LOD / tessellation | 2M indices flat ground | Adaptive | Scalable scene complexity |
| **P5** | GPU-driven culling | CPU-side | Compute dispatch | Scalable to large scenes |

> P2 has an **owner-approved phased plan** (2026-08) with per-effect pass counts, merge
> targets and execution order — see
> [`docs/post-processing-pipeline.md`](../../post-processing-pipeline.md) § 5.
> Phases D, B, C, A and E are DONE: batched grab-pass barriers + `offscreenComposite`
> swap-chain pass (D); the direct `Effects::Resolve::*` folded into the final shader (B);
> ToneMapping applies the bloom itself (C); the nine overlay effects apply through the
> shared generated `CombinePass` (A) and the seven separable-blur effects run their
> blurs through the shared MRT `DenoisePass` (E) — their own apply/composite AND blur
> passes are GONE. Before touching ANY overlay effect's apply or blur math, read
> `docs/post-processing-pipeline.md` § 3b/§ 3c: the math now lives in the effect's
> `combineContribution()` / `denoiseContribution()` GLSL snippets.

### UE5 Comparison (Same Scene)

| Aspect | emeraude-engine | UE5 equivalent |
|--------|----------------|----------------|
| G-buffer | Multi-subpass, 4 draws/object | Single-pass MRT, 1 draw/object |
| Post-process | 42 separate render passes | Fused passes + compute shaders |
| Blur (VeilingGlare, SSAO, DoF) | Fragment shader per pass | Compute shader with shared memory |
| Culling | CPU-side | GPU-driven (compute) |
| Mesh detail | Fixed resolution | Nanite (virtualized geometry) |

### Measurement Protocol

Every pipeline modification **must** follow this protocol:
1. **Before**: Run `/renderdoc-capture` on the test scene, record metrics
2. **Implement**: Make the change
3. **After**: Run `/renderdoc-capture` again, compare metrics
4. **Verify**: Visual output must be identical or improved (read the thumbnail)
5. **Report**: Delta in draw calls, render passes, vertex throughput

No blind optimization. No guesswork. Data drives every decision.
