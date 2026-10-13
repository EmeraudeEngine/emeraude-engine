---
id: beam-and-path-extras
title: Beam and path extras — dashes, arrows, per-vertex colour, the beam's texture scroll, life and strike-again time
status: open
priority: medium
scope: Scenes/Component/Path, Scenes/Component/Beam, Graphics/Material/PathResource, Graphics/Material/BeamResource, Saphir (PathGLSL, BeamGLSL)
opened: 2026-10-13
tags: [graphics, lines, beams, paths, owner-request]
---

# Beam and path extras — dashes, arrows, per-vertex colour, the beam's texture scroll, life and strike-again time

## Why

The first passes of the Path (2026-09-29, graphics doc 35) and of the Beam (2026-09-28 → 30, graphics doc 33) left
out the styling features the owner's request and its references name. Split out of the former segment-rendering item
on 2026-10-13 (its record: graphics docs 33-36), where they were open since 2026-09-30 (no functional work since,
checked 2026-10-13).

## What remains

Paths (`Component::Path`, `Material::PathResource`, `Saphir::PathGLSL`):
- **Dashes**: the arc length is already in the point's `w` (the tessellated polyline), so a dash pattern is a
  fragment-side test on the interpolated arc length (Rougier 2013; A. Klein). Pattern in metres or pixels, like the
  width.
- **Arrows**: a head at the end (or along the path) — a cap shape, built in the ribbon or as extra vertices.
- **Per-vertex colour** (a gradient along the path): the point record carries position + arc length only; a colour
  needs room in the path SSBO or a 1D ramp.
- **A Bézier path from the console**: `SceneManager_Path_setPoints` takes Polyline / UniformBSpline / CatmullRom; a
  Bézier path (anchors + handles, `Math::BSpline`) is set from code only.

Beams (`Component::Beam`, `Material::BeamResource`) — Half-Life `env_beam` / `env_laser` parity:
- **A texture scrolled along the beam** (HL's texture scroll rate): today the look is analytic (profile, noise).
- **A life / fade-out**: a beam that dies after a time, its luminance faded.
- **"Strike again time"**: a RANDOM interval between arcs; today `setArcMotion(seed, restrikeRate, drift)` re-strikes
  at a fixed rate (`BeamResource.hpp:418`).
- **JSON declaration of a beam** in a scene definition (today: C++ and the console / MCP adapter only).

## ⚠️ Traps

- Everything per-frame travels through the instance-transforms SSBO (the stations, the points), never the material UBO:
  a material UBO has ONE frame region (graphics doc 25, Rule 1). The UBO holds the look only.
- A beam of TWO stations cannot wander (graphics doc 33 § Traps): a fade or texture scroll must not assume interior
  stations.
- One program set per beam (the renderable name is in the program cache key): a feature that adds a program variant
  multiplies it per beam.
- After a material change on a path, `markLookChanged()` (bounds + debug look).

## References

- Half-Life (GoldSrc) FGD, `env_beam` / `env_laser` (start / end entity, amplitude noise, brightness, width, texture
  scroll rate, life, strike again time).
- Rougier, "Shader-Based Antialiased, Dashed, Stroked Polylines", JCGT 2013; A. Klein, "Rendering thick lines with
  dashes"; three.js `LineSegments2` / `LineMaterial`.
- Unity `LineRenderer` / `TrailRenderer`, Unreal Niagara ribbon / beam emitter.
- engine `docs/subsystems/graphics/33-beams-lasers-and-electric-arcs.md`, `35-paths-polylines-and-curves.md`,
  `docs/subsystems/saphir/27-the-beam-ribbon-a-vertex-stage-that-builds-a-beam.md`.
