---
id: texture3d-loads-the-default-volume-for-a-2d-image-name
title: A Texture3D named after a 2D image reports Loaded with the default volume, through an empty volumetric name
status: open
priority: high
scope: engine Resources/Manager (Texture3D store), Graphics/TextureResource/Texture3D, Resources/ResourceTrait
opened: 2026-10-19
tags: [defect, resources, silent-fallback]
---

# A Texture3D named after a 2D image reports Loaded with the default volume, through an empty volumetric name

## Why

Seen on Linux and Windows while verifying the VolumetricImages container (engine 78ce1936): the Texture3D container is
registered on the `Images` store, so its 4248 "available" resources are the 2D images. `loadResource(Texture3DResource,
"ID/cyberdemon-specular")` goes through `Texture3D::load(filepath)`, which derives the volume name with
`getResourceNameFromFilepath(filepath, "VolumetricImages")`: the path is not under `VolumetricImages`, the name comes
back EMPTY, the volumetric container warns "The resource named '' doesn't exist !", returns its Default volume, and the
Texture3D reports Loaded. A silent substitution: the caller asked for an image and got a generated gradient volume.

## What remains

- Owner decision on the intended behaviour: refuse a Texture3D whose file is not a volumetric source (explicit error,
  Failed), and/or give Texture3D its own `VolumetricImages` store so it no longer lists the 2D images.
- `getResourceNameFromFilepath()` returns an empty name for a path outside the store: a caller should refuse it
  instead of looking the empty name up (check every call site).
- A runtime check of the chosen behaviour (validation layer, 0 VUID).

## ⚠️ Traps

- Volumetric files are not read yet (`VolumetricImageResource::load(filepath)` refuses): until they are, the only
  volume is the generated default one.

## References

- `src/Resources/Manager.cpp` (the Texture3D registration on "Images"), `src/Graphics/TextureResource/Texture3D.cpp`,
  `src/Resources/ResourceTrait.cpp` (`getResourceNameFromFilepath()`), `docs/caution-points.md` § Resources.
