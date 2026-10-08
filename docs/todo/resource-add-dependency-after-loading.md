---
id: resource-add-dependency-after-loading
title: ResourceTrait::addDependency() logs a refusal but adds the dependency anyway
status: open
priority: high
scope: src/Resources/ResourceTrait.cpp
opened: 2026-10-08
tags: [ave-robustus-ii, resources, defect]
---

# ResourceTrait::addDependency() logs a refusal but adds the dependency anyway

## Why
In `addDependency()`, the `Loading` and `Loaded` cases log "No more dependency can be added !" and then `break` — the
dependency is added anyway. Either the message or the flow is wrong; a dependency added to a Loaded resource never
triggers `onDependenciesLoaded()` again.

## What remains
- Owner: refuse (return false) or accept (and define what happens to a Loaded resource); test.

## References
- Found by the Ave Robustus II warning pass (2026-10-08, projet-alpha `docs/plans/ave-robustus-ii.md`); not raised by a warning, so left for its own fix.
