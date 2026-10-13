---
id: detect-missing-lean-exports-on-linux
title: Catch a missing EMEN_LEAN_API / EMEN_API annotation on Linux, before the Windows link does
status: open
priority: unranked
scope: CMakeLists.txt (export options), src/emeraude_export.hpp
opened: 2026-10-11
tags: [build, windows, exports, tooling]
---

# Catch a missing EMEN_LEAN_API / EMEN_API annotation on Linux, before the Windows link does

## Why

The engine is a shared library; on Windows only the annotated symbols are exported (export-all hit the PE limit of
65535 exports, 2026-08-05). `EMERAUDE_USE_FULL_EXPORTS` exports `EMEN_API` + `EMEN_LEAN_API`, `EMERAUDE_USE_LEAN_EXPORTS`
only `EMEN_LEAN_API`. On Linux both options are off and the macros expand to nothing; on macOS they always give
default visibility since `2e2685549` (one typeinfo across the two images, `src/emeraude_export.hpp:38-67`). Either
way the engine itself is not built with hidden visibility, so every symbol stays visible and a missing annotation only
shows on the Windows peer, one repository further (2026-10-11: a new free
function `Vulkan::probePhysicalDevices()` annotated `EMEN_API` was unresolved at a lean-exports downstream link).

## What remains

- A Linux check build: the engine with `-fvisibility=hidden -fvisibility-inlines-hidden` and the lean (then full)
  exports enabled — `EMEN_API_IMPL` already expands to `__attribute__((visibility("default")))` off Windows — so a
  consumer's link fails on Linux exactly like `LNK2019` on Windows. Decide whether it is a CMake option, a CI job or a
  documented recipe.
- Check what else hidden visibility changes (RTTI / `dynamic_cast` across the boundary, typeinfo of exceptions-free
  code, template instantiations) before making it a default.

## References

- `rtti-key-function-audit` (the `-Wweak-vtables` follow-up of the macOS typeinfo fix): the RTTI-across-the-boundary
  half of the check above.
- `docs/caution-points.md` § *macOS: typeid, std::any and dynamic_cast across the engine boundary — EMEN_API is
  default visibility on macOS (2026-10-12)*.
