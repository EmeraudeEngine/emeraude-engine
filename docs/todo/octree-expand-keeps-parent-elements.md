---
id: octree-expand-keeps-parent-elements
title: OctreeSector::expand() keeps a splitting sector's elements AND files them in the children — two contracts in one class
status: open
priority: unranked
scope: Scenes/OctreeSector (both the rendering and the physics octrees)
opened: 2026-09-25
tags: [octree, physics, rendering, memory]
---

# OctreeSector::expand() keeps a splitting sector's elements AND files them in the children — two contracts in one class

## Why

`OctreeSector::insertWithPrimitive()` documents and enforces **ONE ELEMENT, ONE SECTOR** (an element descends only
while a child contains it entirely — the fix of the 104 GB `8^depth` blow-up). But `expand()` redistributes a
splitting sector's elements with `subSector->insert(element)` into EVERY child and never clears `m_elements`, and
`merge()` relies on the parent keeping them ("all elements remain in this"). So an element filed before a split
lives in the parent and in the child(ren) it touches, one copy per level that split; an element filed after lives
in one sector. Measured on `terrain` (2026-09-25): 424 elements kept by the split root of the rendering octree.

The rendering gathers now de-duplicate with a per-gather stamp (`AbstractEntity::markCollectedByRenderingGather()`),
so rendering is safe. The PHYSICS octree (`OctreeSector< AbstractEntity, true >`, `accumulateStaticEntityCorrections()`
with its `inheritedCandidates`) has not been audited for the same double visit: a pair tested twice costs time,
and a correction accumulated twice would be a defect.

## What remains

1. Audit the physics broad phase for duplicate candidates (count unique vs visited on `terrain` / `collision`).
2. Decide the contract (owner): either `expand()` moves each element into the one child that fully contains it
   (and `merge()` pulls the children's elements back up), or the class documents "parent keeps all" and every
   walker de-duplicates. Measure memory and split/merge cost either way.
