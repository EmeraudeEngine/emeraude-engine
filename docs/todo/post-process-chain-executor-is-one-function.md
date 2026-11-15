---
id: post-process-chain-executor-is-one-function
title: PostProcessor::executeIndirectPostProcessEffects() is one 464-line function (cognitive complexity 123 against a threshold of 25)
status: open
priority: unranked
scope: Graphics/PostProcessor (executeIndirectPostProcessEffects() and the six stack-allocated helper classes it documents)
opened: 2026-11-15
tags: [readability, clang-tidy, baseline, refactor]
---

# PostProcessor::executeIndirectPostProcessEffects() is one 464-line function (cognitive complexity 123 against a threshold of 25)

## Why

clang-tidy 21.1.6, `readability-function-cognitive-complexity`: 123 / 25 on
`PostProcessor::executeIndirectPostProcessEffects()`. The finding is a BASELINE (the gate reports the changed lines
only): it surfaced on 2026-11-15 when the function received the frame's view as a parameter and its signature line
changed. Every other baseline exposed that way was brought clean in the lot that touched it (ledger § D11 and the rows
of 2026-11-01 to 2026-11-15); this one was not: the function is the chain executor — the frame's delta time, the view
reconstruction block, the frame context, then the loop over the effects with its combine groups, its shared denoise
batches and the two halves of a cut frame — and splitting it is a change of its own, with every post-process path to
verify again.

## What remains

- An owner decision on the cut. A reading that follows the function's own stages: (1) the frame clock (delta time,
  closed once per frame), (2) the view reconstruction block (from the `FrameView`), (3) the `FrameContext`, (4) the
  capabilities and the occupant gate, (5) the chain loop, itself in three parts — an effect outside a group, the
  combine group and its deferred flush, the shared denoise batch.
- The helpers take the context by reference; the six stack-allocated classes documented "lives for the duration of the
  chain" keep their lifetime (a helper must not outlive the call).
- Proof: the pixel A/B of scenes doc 12 § *View Matrix State Index — Critical Trap* (six scenes, several captures per
  launch on the static ones), the two halves of a cut frame (`cutFrame`, a scene with grab-pass objects and a
  pre-translucency effect), 0 VUID; clang-tidy on the changed lines: 0.

## ⚠️ Traps

- A static scene's single capture depends on the phase of the temporal cycle (scenes doc 12, the proof of 2026-11-15):
  compare several captures per launch, never two launches alone.
- The executor runs twice in a cut frame (`ChainPhase::PreTranslucency`, then `PostTranslucency`): the frame clock
  closes once, the view block is written twice with the same values.

## References

- `src/Graphics/PostProcessor.cpp` — `executeIndirectPostProcessEffects()`.
- `docs/static-analysis-ledger.md` — the row "The frame's view" (2026-11-15).
