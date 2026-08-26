---
id: merge-font-pixelfactory-fontresource
title: Merge Font from PixelFactory and FontResource
status: open
priority: unranked
scope: Resources
opened: unknown
tags: [resources, text]
---

# Merge Font from PixelFactory and FontResource

## What remains

- [ ] Merge the two font representations: `PixelFactory`'s `Font` (now in **emeraude-base**,
  `EmEn::Base::PixelFactory`) and the engine's `FontResource`.

## ⚠️ Traps

- This one crosses the repository boundary (base ← engine). The foundation must not learn about
  engine resources; the merge therefore has to move the engine's side down, not the reverse.

## Origin

Inherited from the historical root `TODO.md`, written before `EmEn::Libs` was extracted into
emeraude-base.
