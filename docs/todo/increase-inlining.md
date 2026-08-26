---
id: increase-inlining
title: Increase inlining where it pays
status: in-progress
priority: unranked
scope: cascade-wide
opened: unknown
tags: [performance, cpp]
---

# Increase inlining where it pays

## What remains

- [ ] Continue moving small, hot accessors into the headers, and measure. The project rule is
  RUNTIME > READABILITY > COMPILE TIME, so this is legitimate work — but it is only worth doing
  where a profile says so.

## Origin

Inherited from the historical root `TODO.md` (marked WIP, no date, no record of what was already
covered).
