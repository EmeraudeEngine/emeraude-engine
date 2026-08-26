---
id: manual-glsl-sources
title: Prepare a way to use manual GLSL sources
status: open
priority: unranked
scope: Saphir
opened: unknown
tags: [shaders]
---

# Prepare a way to use manual GLSL sources

## What remains

- [ ] A path for hand-written GLSL alongside the generated programs.

## ⚠️ Traps

- The generated-GLSL dump (`Core/Graphics/Shader/EnableSourceCodeDump`, OFF by default) is **not**
  a cache and nothing reads it back — `AbstractShader::loadSourceCode()` has zero callers. A manual
  path must not be built on top of that directory.

## Origin

Inherited from the historical root `TODO.md`.
