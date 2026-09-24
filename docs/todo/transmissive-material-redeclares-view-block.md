---
id: transmissive-material-redeclares-view-block
title: A transmissive material re-declares the View uniform block and logs a warning per program
status: open
priority: unranked
scope: Saphir, Graphics/Material
opened: 2026-09-24
tags: [shaders, log-noise]
---

# A transmissive material re-declares the View uniform block and logs a warning per program

## Why

Every lit program of a material with screen-space transmission logs
`[Warning][Shader] An uniform block declaration named 'View' already exists !` — 14 lines on
`forest` for one `Parametrics/Diamond` mesh (2026-09-24). The generated code is correct:
`AbstractShader::declare(const UniformBlock &)` sees the duplicate, keeps the first declaration and
returns `true`. But a warning that fires on correct code trains everyone to skip warnings, and it
buries a real duplicate (two DIFFERENT blocks under one name) in the same noise.

**Source:** `StandardResource.cpp` calls `generator.declareViewUniformBlock(fragmentShader)` for the
grab-pass refraction offset (~line 3546) and for the depth-based opacity (~line 3649), while
`SceneRendering.cpp` (~line 709) has already declared that block in the same fragment shader for the
lighting.

## What remains

Choose where the de-duplication belongs (owner decision, two valid places):

- **In Saphir**: `AbstractShader::declare()` returns `true` SILENTLY when the existing block is
  IDENTICAL (same set, binding, members) and warns only on a real conflict. It fixes every current
  and future caller; it costs a structural comparison of the two declarations.
- **In the material**: ask the shader whether the block exists before declaring it (a query that
  does not exist today), leaving the warning as the strict guard it is.

## References

- Seen in the `forest` run logs of 2026-09-24, together with the grab-pass teardown defect fixed the
  same day (`docs/caution-points.md` § *the grab pass was recreated IN PLACE*).