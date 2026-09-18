---
id: gltf-resource-prefix-collides-between-asset-variants
title: The glTF resource prefix is keyed on the file STEM, so two variants of a model share one namespace
status: open
priority: unranked
scope: Scenes/Loaders/GLTFLoader, Resources/Manager
opened: 2026-09-18
blocked-by: []
tags: [gltf, loader, resources, measured]
---

# The glTF resource prefix is keyed on the file STEM, so two variants of a model share one namespace

## Why

`GLTFLoader::load()` builds every resource key under

```cpp
m_resourcePrefix = "glTF:" + filepath.stem().string() + "/";
```

The **stem** is the file name without its extension or directory, so

- `Models/SunglassesKhronos/glTF-Binary/SunglassesKhronos.glb`
- `Models/SunglassesKhronos/glTF-Draco/SunglassesKhronos.gltf`

produce the **same** prefix, `glTF:SunglassesKhronos/`, and therefore the same geometry, material,
texture and image keys — although they are different files with different geometry encodings and,
in this case, different texture encodings (PNG against WebP).

This is the defect class already recorded for meshes *inside* an asset — see `buildResourceKey()`
and `Scenes/Loaders/AGENTS.md` § "The resource key — AN ASSET NAME IS NOT AN IDENTITY" — reappearing
one level up, between assets.

## What it actually did (measured 2026-09-18)

It does not bite in the common case, because replacing a viewer scene unloads its resources first.
It bit when a manual `Core.openFiles()` of one variant preceded a conformance-bench run of the
other, inside one session: the bench's A/B for `SunglassesKhronos` reported **98.16 % of pixels
differing, mean 39.28/255**. Re-run from a session that had not hand-loaded the asset, the same
comparison is **6.35 % / 0.10** and reproduces exactly, twice.

So the symptom is a **wrong measurement**, not a crash or a visible artefact — the worst kind.

## What remains

Make the prefix identify the FILE, not its stem. Candidates, in rough order of preference:

1. The path relative to the asset root (`SunglassesKhronos/glTF-Draco/SunglassesKhronos`), which is
   stable across machines when the root is the store, and readable in a resource dump.
2. A hash of the absolute path, which is opaque but cannot collide.
3. The stem plus the parent directory name (`glTF-Draco/SunglassesKhronos`) — the cheapest, and
   enough for the corpus layout, but it still collides for two same-named files two directories
   apart.

⚠️ Whatever is chosen, the key appears in resource dumps and in the console, so keep it readable:
an opaque hash makes `Resources::Manager` listings unusable for diagnosis.

## ⚠️ Traps

- **Do not "fix" this by clearing the cache between loads.** The cache is what makes a second
  instance of the same asset cheap; the defect is the KEY, not the caching.
- The collision is invisible whenever scenes unload cleanly between loads, so a test that loads two
  variants through the viewer will pass. Reproduce it by loading variant A, then variant B, **without
  letting the first scene be torn down** — which is what `Core.openFiles()` followed by a bench run
  does.

## References

- `src/Scenes/Loaders/GLTFLoader.cpp` — `m_resourcePrefix`, and `buildResourceKey()` in
  `Scenes/Loaders/Interface.hpp` for the same lesson one level down.
- `tools/gltf-conformance-bench/README.md` § *The compressed-variant A/B* — the measurement that
  exposed it.
