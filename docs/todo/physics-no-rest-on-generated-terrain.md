---
id: physics-no-rest-on-generated-terrain
title: Physics — bodies never come to rest on generated terrain
status: open
priority: medium
scope: Physics
opened: 2026-08-25
tags: [physics, measured]
---

# Physics — bodies never come to rest on generated terrain

> [!IMPORTANT]
> **Owner decision: a SEPARATE subject from the Y-up migration** — no axis sign is involved, it
> was only surfaced while measuring the Y-up physics gate.

## Why

On `balls-of-steel` (diamond-square ground) a tracked ball descended steadily for 50 s
(Y −19.5 → −33.2, ~3 units per 10 s) without converging.

⚠️ **NOT free fall** — `|v|` stayed between 0.2 and 1.8 instead of accelerating toward 9.81 m/s²,
and contact kept re-arming, so the motion IS surface-constrained.

Flat ground DOES rest correctly (`physics-debug`: a 1 m sphere settles at exactly Y = 1.000000,
velocity 0).

## What remains

- [ ] Decide whether a permanent roll is CORRECT on that terrain (slope everywhere, no flat cell)
  or whether friction is too low. That question is open and must be answered before any fix.
