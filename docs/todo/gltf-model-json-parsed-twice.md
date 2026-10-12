---
id: gltf-model-json-parsed-twice
title: GLTFLoader reads and parses the model's JSON twice per load
status: open
priority: unranked
scope: src/Scenes/Loaders/GLTFLoader.cpp (load(), loadImages(), readModelJSON(), locateBuffers())
opened: 2026-10-12
tags: [gltf, loaders, allocatus-reduxus]
---

# GLTFLoader reads and parses the model's JSON twice per load

## Why
Rule C step 3 (2026-10-12) added a trust-boundary check before fastgltf: `load()` reads the model's JSON
(`readModelJSON()`) to refuse a buffer or image uri that is not UTF-8 once percent-decoded. `loadImages()` then calls
`locateBuffers()`, which reads and parses the same JSON again. Load time only, but a `.gltf` carrying large `data:` URIs
pays the file read and the JSON parse twice.

## What remains
- Parse once in `load()` and hand the result to `loadImages()` / `locateBuffers()` (the types are in the .cpp's
  anonymous namespace: a private member or an extra parameter of a private function). Allocatus Reduxus: prove the
  removed read and allocation.
- The Windows peer's alternative: keep the parsed fastgltf asset across the two calls — an ownership change, an owner
  decision.

## References
- Engine `docs/caution-points.md` (rule C step 3, the fastgltf uri pre-check).
