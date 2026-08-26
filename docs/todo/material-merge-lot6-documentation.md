---
id: material-merge-lot6-documentation
title: Material merge — Lot 6, documentation of the single StandardResource
status: open
priority: high
scope: Graphics/Material
opened: 2026-08-12
tags: [documentation, material]
---

# Material merge — Lot 6, documentation of the single StandardResource

## Why

The merge landed (one concrete lit material: `Material::StandardResource`, Cook-Torrance PBR,
reusing the ClassId `"MaterialStandardResource"`; the legacy Blinn-Phong class deleted). The
documentation network still describes the old two-class world, and the project rule says the doc
update ships in the same session as the code — this one is late.

## What remains

- [ ] `AGENTS.md` network: Graphics, Scenes, Console (`Console/AGENTS.md:204` finally becomes
  exact — it describes the named-material resolution that used to be Standard-then-PBR).
- [ ] Material JSON schema documentation (single lit ClassId; `"Standard"` and `"PBR"` are
  synonyms in `DefinitionResource`).
- [ ] projet-alpha `.claude/rules/` mirror.
- [ ] `generate_materials.py` header.
- [ ] Optional: regenerate the 3,918 dual-schema material JSONs to single-schema — nothing
  requires the dual layout after the merge.

## References

- Taxonomy after the merge: `Material::Interface` survives ONLY as the extension contract for
  future, structurally different materials (a different BRDF *structure* — cloth/hair/skin
  someday). Feature blocks (clearcoat, sheen, transmission, iridescence, anisotropy, SSS) stay
  optional blocks INSIDE the one material, never separate classes.
