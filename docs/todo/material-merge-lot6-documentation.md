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

**Lot 7 is now DONE too** (`a16195166` — `BasicResource` deleted, `m_usePBRMode` and the
`LightGenerator.PerVertex/PerFragment*` files gone, verified 2026-09-08), which makes this item the
**last** open piece of the material merge and enlarges its surface: the docs now describe a class
that no longer exists in any form.

### Measured stale surface (re-counted 2026-10-11)

**About 48 `BasicResource` mentions.** The `AGENTS.md` files are routers since 2026-09-27, so the content has moved to
`docs/subsystems/`. Ranked, so the pass can be ordered:

| File | Mentions |
|---|---|
| `docs/caution-points.md` | 17 |
| `docs/subsystems/scenes-loaders/03-implemented-loaders/04-wadloader.md` | 5 |
| `docs/subsystems/graphics/08-8-material-component-system.md` | 4 |
| `docs/subsystems/saphir/12-legacy-specular-energy-normalisation.md` (history, kept on purpose) | 3 |
| `docs/saphir-v2-proposal.md` | 3 |
| `docs/subsystems/vulkan/04-critical-layoutmanager-thread-safety.md`, `docs/subsystems/graphics/05-5-material-ubo-system.md`, `docs/ai-runtime-control.md` | 2 each |
| `docs/windows-export-api.md`, `docs/shadow-mapping.md`, `docs/reflection-pipeline.md`, `docs/pipeline-caching-system.md`, `docs/subsystems/saphir/10-ibl-ambient-pass.md`, `docs/subsystems/saphir/11-…`, `docs/subsystems/graphics/13-12-…/23-…` | 1 each |

⚠️ **Not every mention is a deletion.** Three kinds, and they must be told apart:

- **Obsolete** — describes Basic as a live tier ⇒ rewrite or drop.
- **Historical and load-bearing** — explains *why* a contract has its present shape (the whole
  `## Legacy (Blinn-Phong) Specular` section of `src/Saphir/AGENTS.md` documents a path whose three
  generator files no longer exist, yet the `(n+2)/(8π)` reasoning is why the surviving specular is
  normalised) ⇒ keep, mark clearly as history.
- **Still true of the on-disk format** — `"MaterialBasicResource"` remains an accepted JSON class-id
  synonym on purpose (`MultiLayerMeshResource.cpp:262`, deliberate, commented in place) ⇒ keep, and
  say it is a format synonym rather than a class.

## What remains

- [ ] The `docs/subsystems/` files listed above (Graphics, Scenes loaders, Saphir, Vulkan) and `docs/caution-points.md`,
  told apart by the three kinds above. The Console's named-material resolution (formerly `Console/AGENTS.md:204`,
  now in `docs/subsystems/console/`) finally becomes exact: it describes what used to be Standard-then-PBR.
- [ ] Material JSON schema documentation: a single lit ClassId; `"Standard"` and `"PBR"` are synonyms in
  `DefinitionResource`.
- [ ] Optional: regenerate the 3,918 dual-schema material JSONs to single-schema. Nothing requires the dual layout
  after the merge.

No longer open (2026-10-11): the projet-alpha `.claude/rules/` mirror has 0 mentions, and `generate_materials.py` no
longer exists in the tree.

## Already recorded, do not redo

Two durable contracts were lifted out of the lot 7 item before it was deleted (2026-09-08):

- **A surface variable handed to `LightGenerator` must be a NAME, never an expression** — the
  generator concatenates `.rgb` / `.a` onto it, so `a * b` becomes `a * b.rgb`, which compiles and
  silently swizzles the wrong operand. Now in `src/Saphir/AGENTS.md`.
- **`DynamicColorEnabled` was deliberately not ported**: the gate existed only because Basic's
  default diffuse was Grey; with `DefaultAlbedoColor{White}` the unconditional multiply IS the
  contract. Now in `src/Graphics/AGENTS.md`.

## References

- Taxonomy after the merge: `Material::Interface` survives ONLY as the extension contract for
  future, structurally different materials (a different BRDF *structure* — cloth/hair/skin
  someday). Feature blocks (clearcoat, sheen, transmission, iridescence, anisotropy, SSS) stay
  optional blocks INSIDE the one material, never separate classes.
