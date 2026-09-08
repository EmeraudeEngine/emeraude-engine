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

### Measured stale surface (2026-09-08)

**47 `BasicResource` mentions across 13 Markdown files.** Ranked, so the pass can be ordered:

| File | Mentions |
|---|---|
| `docs/caution-points.md` | 17 |
| `src/Graphics/AGENTS.md` | 7 |
| `src/Scenes/Loaders/AGENTS.md` | 5 |
| `src/Saphir/AGENTS.md` | 5 |
| `docs/saphir-v2-proposal.md` | 3 |
| `src/Vulkan/AGENTS.md`, `docs/ai-runtime-control.md` | 2 each |
| `AGENTS.md`, `docs/graphics-system.md`, `docs/pipeline-caching-system.md`, `docs/reflection-pipeline.md`, `docs/shadow-mapping.md`, `docs/windows-export-api.md` | 1 each |

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

- [ ] `AGENTS.md` network: Graphics, Scenes, Console (`Console/AGENTS.md:204` finally becomes
  exact — it describes the named-material resolution that used to be Standard-then-PBR).
- [ ] Material JSON schema documentation (single lit ClassId; `"Standard"` and `"PBR"` are
  synonyms in `DefinitionResource`).
- [ ] projet-alpha `.claude/rules/` mirror.
- [ ] `generate_materials.py` header.
- [ ] Optional: regenerate the 3,918 dual-schema material JSONs to single-schema — nothing
  requires the dual layout after the merge.

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
