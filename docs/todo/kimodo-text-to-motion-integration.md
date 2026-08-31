---
id: kimodo-text-to-motion-integration
title: Generate character animation from a text prompt (Kimodo, dev-time)
status: blocked
priority: unranked
scope: Animations
opened: 2026-08-31
blocked-by: [skeletal-animation-retargeting]
tags: [animation, ai, tooling]
---

# Generate character animation from a text prompt (Kimodo, dev-time)

## Why

[kimodo.cpp](https://github.com/localai-org/kimodo.cpp) turns an English sentence into a skeletal
animation, locally, in **18.4 s** on an RTX 3070 Ti. Evaluated end to end on 2026-08-30/31: the
motion reached the Paladin on screen through the engine's own skeletal path, zero validation
errors. The measurements are in [`../text-to-motion-kimodo.md`](../text-to-motion-kimodo.md).

**Owner direction (2026-08-30/31): integrate into the engine, mechanism still open** — "a CMake
option or something". The checkout stays an unversioned directory until that is designed; it was
briefly made a projet-alpha submodule and rolled back on purpose.

### ⚠️ The use case decides the shape, and it decides the retargeter's too

The owner's stated intent is **disposable animations for scenes** — motion worth generating and
throwing away, never curated. That is the argument that makes engine-side integration
*functionality* rather than authoring convenience: the value of a disposable clip lies precisely in
producing **no artifact**, and a file-based pipeline forces you to name, store and version exactly
what you decided not to keep.

It splits into two very different builds:

| | where it runs | shipped | what it buys |
|---|---|---|---|
| **A** | scene build, developer machine | nothing | write a sentence instead of hunting for a clip |
| **B** | runtime, from **pre-computed embeddings** | denoiser 1.13 GB + **16 KB per prompt** | a fixed prompt vocabulary, but a **different seed per spawn** — ten idling guards, ten genuinely different idles |

`kimodo_generate_embedding()` is what makes B possible: the 15 GB encoder runs at authoring time
only, and its output is 16 KB and reusable forever.

⚠️⚠️ **This constrains [`skeletal-animation-retargeting.md`](skeletal-animation-retargeting.md),
which is built first.** Under A the retargeter can be a tool. Under B the motion arrives during
play and the retargeting runs with it — callable off the main loop, allocation-disciplined. Writing
the tool then discovering the runtime need is a rewrite, not an adaptation.

**Settled 2026-08-31: the owner wants B**, so the retargeter is written to runtime constraints. B
is measured as viable: 1.6 s for 4 s of motion at 20 diffusion steps, cost strictly linear in the
step count, encoder entirely out of the runtime (a 16 KB embedding per prompt). Numbers and the two
caveats — unmeasured quality at 20 steps, and 83 % GPU contention with the renderer — are in
[`../text-to-motion-kimodo.md`](../text-to-motion-kimodo.md) § 3.

## Blocked on

The retargeter ([`skeletal-animation-retargeting.md`](skeletal-animation-retargeting.md)). Kimodo
emits a 30-joint SOMA skeleton; every character in this project is a 70-bone Mixamo rig. Without
retargeting there is nothing to play the motion on. The cost split is roughly **85 % engine,
15 % Kimodo**.

## What remains

- [ ] Optional CMake dependency on kimodo.cpp, defaulted **off**.
- [ ] **A CEF dev-mode authoring tool in projet-alpha** (owner request, 2026-08-31): prompt →
      motion → the character, driven from the web UI rather than a console command. ⚠️ Prerequisite
      on this workstation: `chrome-sandbox` is **not SUID**, so CEF has never run here — every
      session so far used `--disable-cef`. Fix with
      `sudo chown root:root chrome-sandbox && sudo chmod 4755 chrome-sandbox` in the build's
      `Release/` directory. The UI contract lives in projet-alpha, `docs/cef-integration.md` and
      `docs/ui-web-template.md` — named rather than linked, because this repository is clonable on
      its own and a cross-repository relative path would dangle.
- [ ] **Judge quality against step count** — generate one prompt at 100 / 50 / 20 steps and compare.
      Time is measured, quality is not, and it decides whether the cheap end of the dial is usable.
- [ ] Work around the dead `kimodo_runtime_options` (§ 6.3): device selection currently goes
      through the `KIMODO_BACKEND` environment variable only.
- [ ] Console command generating a clip and injecting it through the resource manager.
- [ ] Decide how the 15 GB text encoder is provisioned on a developer machine (it is **not** a
      submodule-sized dependency; the 1.13 GB motion denoiser is).
- [ ] Upstream the two build defects of § 6, or fork under the EmeraudeEngine org as was done for
      `glfw` — the local patch currently lives only in an unversioned checkout.

## ⚠️ Traps

- **Licences are not uniform across the weights.** SOMA and G1 are NVIDIA Open Model (commercial
  use allowed); the SMPL-X checkpoint is research-only and is deliberately absent from the
  published bundle. Only **SOMA** is both human-shaped and usable.
- **The port is young** — 23 commits when evaluated, and its CPU-only build path does not compile
  (two missing Vulkan guards, documented in the reference).
- **SOMA-30 has two bones per hand.** For a sword-and-shield character this decides whether the
  model serves combat or only secondary body motion. The 77-joint expansion is not implemented.
- A **skeleton-only** GLB carries no `skins`, so the engine routes its channels to
  `NodeAnimation` and the skeletal animator never sees them.

## References

- [`../text-to-motion-kimodo.md`](../text-to-motion-kimodo.md) — evaluation, licences, measurements
- [`../animation-retargeting.md`](../animation-retargeting.md) — the SOMA-30 reference
