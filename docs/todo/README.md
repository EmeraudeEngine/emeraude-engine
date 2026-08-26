# docs/todo/ — one file, one idea

This directory replaces the former root `TODO.md`. It is the engine's open-work list.

## The rule (project-wide, all repositories)

1. **One file = one idea to do.** A concept that can be finished on its own gets its own file.
   Never a list of unrelated items in one file, never one idea spread over several files.
2. **Done = file deleted.** There is no `[x]`, no "DONE" section, no `done/` archive here.
   A file exists only while the work is open.
3. **The knowledge does NOT live here.** Measurements, traps and owner decisions that must
   survive the work belong in `docs/caution-points.md`, the `docs/` topic files and the
   `AGENTS.md` network — write them there *before* deleting the item file.
4. **Every file starts with the YAML front-matter below**, so an agent can sort the list
   without reading every body.
5. The file name IS the `id`: lowercase kebab-case, descriptive, stable.

## Front-matter contract

```yaml
---
id: volumetric-light-single-scattering   # == file name without .md
title: Volumetric light — real single-scattering pass
status: open          # open | in-progress | blocked | parked
priority: high        # high | medium | low | unranked
scope: Graphics/PostProcessing            # subsystem, or repo-relative path
opened: 2026-08-26    # YYYY-MM-DD, or "unknown" for items inherited from the old TODO.md
blocked-by: [csm-rework]                  # optional, list of ids
tags: [vulkan, shaders]                   # optional
---
```

- `status: parked` — deliberately not being worked on (an owner decision), kept so nobody
  re-opens the investigation by accident. It must say *why* in the body.
- `status: blocked` — waiting on another item (`blocked-by`) or on an owner decision.
- `priority: unranked` — the owner never ranked it. Do not invent a priority.

## Body layout

`# <title>` then free Markdown. Recommended sections: **Why**, **What remains**,
**⚠️ Traps**, **References**. Keep the item actionable; long-form knowledge goes to `docs/`.

## Conventions

- English only, like every other file in this repository.
- Reference code as `path/to/File.cpp:123`, and cross-reference other items by `id`.
- Adding an item is not a substitute for fixing an engine defect in the engine
  (see `AGENTS.md` § Co-Development Rule).
