---
id: imposter-bake-lit-rig-compiles-on-render-thread
title: The lit imposter bake rig compiles ~28 programs per variant synchronously on the render thread
status: open
priority: unranked
scope: Graphics/RenderTarget/ImposterBake, Scenes/Toolkit (bakeTreeImposter), Saphir program generation
opened: 2026-09-25
tags: [vegetation, imposter, stall, startup]
---

# The lit imposter bake rig compiles ~28 programs per variant synchronously on the render thread

## Why

By reading (2026-09-25, the terrain hang diagnosis): since the "ImposterCopies" rig is `Lighting::Lit`, each bake
variant generates an AmbientPass plus the light-pass variants per layer (bark, leaves) — ~28 pipeline and program
generations, against 2 before — synchronously on the render thread, inside the bake's `prepareRender`. `terrain`
bakes 20 variants (4 species × (3 + 2 old seeds)), one job per frame, 3 renders each. ⚠️ The slow start first
blamed on it (frames averaging ~1 s, no screenshot between 23 s and 44 s) was the scene's first frame reading the
never-written triple-buffer slot (engine `docs/caution-points.md` § *NEVER-WRITTEN slot*, fixed the same day): with
that fixed, screenshots complete from the first poll. The compile cost itself is UNMEASURED.

## What remains

- Measure the split: program generation vs GPU work in those frames (GPU profiler, timestamps around
  `prepareRender`).
- If compile-bound: warm the rig's programs off the render thread (the thread pool), or share the tree's own lit
  programs with the rig (same material, same layers), before its first render.
- Cheaper still (hang diagnosis, verified by reading): record ONLY the AmbientPass in a bake target. The light passes
  write nothing the atlas copies (the atlas takes colour/normal/albedo from the ambient pass), so the rig's CSM pass,
  its cloud-shadow reads and most of its program variants are pure cost there.
