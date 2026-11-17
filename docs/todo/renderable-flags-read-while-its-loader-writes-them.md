---
id: renderable-flags-read-while-its-loader-writes-them
title: The resource flag word is written during their load by the geometries and the materials - audit who reads it before the load ends
status: open
priority: high
scope: Graphics/Geometry, Graphics/Material (their enableFlag() calls), Resources/ResourceTrait (the word)
opened: 2026-11-16
tags: [tsan, concurrency, audit]
---

# The resource flag word is written during their load by the geometries and the materials

## Why

The renderable's side is done (2026-11-17, three OS: scenes doc 12, the row "What a renderable IS and what its load
declares"): its bits are constructor-only. The word itself belongs to `Resources::ResourceTrait`, and the other
families still write it while they load - the geometries (`EnableTangentSpace`, `EnableNormal`,
`EnablePrimaryTextureCoordinates`, `EnableVertexColor`), the materials (`TextureEnabled`,
`UsePrimaryTextureCoordinates`, `UnlitEnabled`, `BlendingEnabled`, `IsCreated`…). ⚠️ No race is MEASURED there: this
is the audit the renderable's defect asks for, kept `high` because the mechanism is the one ThreadSanitizer reported.

## What remains

- List who reads those bits and on which thread; find a reader that runs before the load ends (a component or an
  instance built on a resource still loading, a shader generation).
- Measure under ThreadSanitizer the scenes that build on resources still loading (several runs each: a loading race
  shows when the threads meet).
- Apply the renderable's rule where a reader exists: the bits that say what the type is go to the constructor, what
  a load declares becomes a member written before the publication.

## ⚠️ Traps

- `sprite` draws its 100 000 sprites at random places: never a pixel A/B between two launches.
- `Resources::Container` sets `DirectLoading` (bit 31) right after it creates a resource, before anyone else holds
  it: not a race.

## References

- Scenes doc 12; `src/Graphics/Renderable/Abstract.hpp` (`RenderableFlagBits`, the rule in its comment).
