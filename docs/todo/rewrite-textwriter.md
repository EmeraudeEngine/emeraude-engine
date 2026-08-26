---
id: rewrite-textwriter
title: Rewrite TextWriter as the text component of an interactive composed surface
status: open
priority: unranked
scope: Overlay
opened: unknown
tags: [overlay, ui, text]
---

# Rewrite TextWriter as the text component of an interactive composed surface

## Why

**Owner intent (recorded 2026-08-26):** the overlay manager builds its screens out of
**composed surfaces**, and the goal is a surface that is **interactive** — not a static
blit. `TextWriter` is one of the components such a surface is made of: the one that writes
text into it. The rewrite exists to serve that role, which the current class was not designed
for.

## What remains

- [ ] Define what `TextWriter` owes an interactive composed surface (re-writable regions,
  invalidation of only what changed, hit-testing coordinates for the text it lays out, cursor
  and selection if the surface is editable) and rewrite it against that contract.

## References

- Sibling idea, same subsystem: `composedsurface-native-menu.md` — reworking `ComposedSurface`
  so it can express a native menu. The two share the interactive-surface goal; they are kept
  apart because either can be finished without the other.
