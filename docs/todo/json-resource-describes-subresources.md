---
id: json-resource-describes-subresources
title: A resource JSON must be able to DESCRIBE its sub-resources, not only point at them
status: open
priority: unranked
scope: Resources
opened: unknown
tags: [resources, json, scene]
---

# A resource JSON must be able to DESCRIBE its sub-resources, not only point at them

## Why

**Owner intent (recorded 2026-08-26):** any resource JSON should be able to **describe** a
sub-resource inline, instead of naming one that the resource manager must already know. Today a
manifest points into the stores; the dependency has to exist elsewhere, declared separately.

**The end state the idea aims at: ONE JSON file describing an ENTIRE scene** — every material,
geometry, texture and node of it, self-contained.

## What remains

- [ ] Let every resource type accept, wherever it accepts a resource NAME, a nested object that
  describes that resource directly (the "direct data description" the historical `TODO.md` line
  referred to).
- [ ] Keep both forms valid: naming a stored resource stays the way to SHARE one between several
  consumers; describing it inline is for what belongs to a single owner.
- [ ] Then verify the composition holds recursively, which is what makes the whole-scene-in-one-file
  case work.

## ⚠️ Traps

- **Identity and sharing**: an inline-described resource has no store name, so two identical
  inline descriptions are two resources unless they are deduplicated on content. Decide that
  deliberately — the engine already paid for a cache key that reduced to name + dimensions.
- `FastJSON::getValidatedStringValue` returns nullopt **with no trace**: a malformed nested
  description must not fall back silently to a default resource.
