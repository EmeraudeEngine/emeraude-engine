---
id: crash-fatal-in-resourcesmanagerservice-container-does-ba06b7
title: "Crash reports: Fatal in ResourcesManagerService — Container does not exist !"
status: open
priority: unranked
scope: src/Resources/Manager.cpp
opened: 2026-10-21
tags: [crash, crash-report-auto]
---

# Crash reports: Fatal in ResourcesManagerService — Container does not exist !

> Dropped by the downstream application's crash-report tool: a cause never seen before in the production crash reports.
> **Triage**: confirm it is a defect, give the file a better name, move it to the repository that owns the defect
> (an engine defect goes to `vendors/emeraude-engine/docs/todo/`, …) and rank it. The block between the
> `crash-reports` markers is rewritten at every run, wherever the item went (it is found by its signature line);
> write outside it. Deleting the item closes the cause: a new report of it is then flagged as a regression.

## Why

<!-- crash-reports:begin -->
| | |
|---|---|
| Machines | 1 — per version: 8.0.2 ×1 |
| Reports | 1 (first 2026-10-16, last 2026-10-16) |
| Versions | 8.0.2 ×1 |
| Platforms | macos ×1 |
| OS | Apple macOS 28.0 ×1 |
| GPU (selected) | unknown ×1 |
| ANGLE backend | metal ×1 |
| Died at milestone | SessionOpened ×1 |
| Single-instance lock | not recorded ×1 |

- **Cause (normalized):** `ResourcesManagerService: Container does not exist !`
- **Where:** `vendors/emeraude-engine/src/Resources/Manager.cpp` — `*EmEn::Resources::Manager::getContainerInternal`
- **Signature:** `ba06b7960ca58502`

Last error before the death (else the last trace), by number of reports:

- ×1 `died at SessionOpened`

Traces of the main journal (normalized, latest report; around the Fatal trace when there is one):

```
[RendererService][Success] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeSubServices`) TextureCacheService service up!
[IndexedVertexResource][Warning] (vendors/emeraude-engine/src/Graphics/Geometry/IndexedVertexResource.cpp `EmEn::Graphics::Geometry::IndexedVertexResource::createOnHardware`) The buffers are already in video memory ! Use processLogics() instead.
[RendererService][Success] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeSubServices`) PostProcessorService service up!
[RendererService][Warning] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeSubServices`) GraphicsExternalInputService service failed or disabled at startup!
[RendererService][Warning] (vendors/emeraude-engine/src/Graphics/Renderer.cpp `EmEn::Graphics::Renderer::initializeSubServices`) GraphicsRecorderService service failed or disabled at startup!
[OverflowCensus][Success] (vendors/emeraude-engine/src/Graphics/OverflowCensus.cpp `EmEn::Graphics::OverflowCensus::create`) Overflow census ready: <n> frame slots, <n> channels of <n> bytes each, disarmed.
[ResourcesManagerService][Fatal] (vendors/emeraude-engine/src/Resources/Manager.cpp `*EmEn::Resources::Manager::getContainerInternal`) Container does not exist !
```

Archives:

- 1 archive(s), in the downstream application's crash-report mirror.
<!-- crash-reports:end -->

## Triage (2026-10-21)

- **Owner**: the engine.
- **Machines**: ≈1 (reports grouped by CPU + RAM + OS fingerprint on 2026-10-21; several reports per machine
  when every launch fails).
- **Reading**: macOS, at startup: a resource container does not exist — a missing data file in the bundle?
- **Next step**: Read the journal for the container name; if it is a packaging omission the fix belongs to the downstream application (its resource packaging).
- **Plan**: `CrashReport Solucius`, wave W6 — a plan of the downstream application.

## 8.0.3 status (checked 2026-10-21)

**Possibly affected.** Engine `78ce1936` (1.0.1) registers a container that was missing (volumetric images for `Texture3D`); not verified to be this one.

Checked against the changes between 8.0.2 and 8.0.3 in the four repositories (the application 1.1.1 → 1.1.2, engine 1.0.0 → 1.0.1, its kernel 1.3.1 → 1.3.2, its frontend `47cbe7cd9` → `c3ab7b390`); nothing relevant after 8.0.3. Proof only comes from the 8.0.3 reports (plan § 5).

## What remains

- Read the journals of the archives counted above (in the downstream application's crash-report mirror).
- Find the defect, reproduce it, fix it where it lives.

## References

- The downstream application's crash-report documentation — what an archive holds, the flight recorder milestones.
- The downstream application's crash-report tool documentation — how this item was produced, how the cause is chosen.
