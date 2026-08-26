---
id: reflection-automatic-component-missing
title: Reflection: { "Type": "Automatic" } does not create the component
status: open
priority: medium
scope: Graphics/Material
opened: unknown
tags: [material, log-lies]
---

# Reflection: { "Type": "Automatic" } does not create the component

## Why

Observed Jul 2026, **still present Aug 2026**.
`setReflectionComponentFromEnvironmentCubemap()` raises `m_isUsingEnvironmentCubemap` and then
calls `setReflectionAmount()`, which warns `The material 'X' has no reflection component !`
because no `ComponentType::Reflection` is ever emplaced.

⚠️ As it stands **the log accuses materials that declared reflection correctly** — that already
sent one session chasing a false lead.

## What remains

- [ ] Decide which end is wrong: either the bindless path should stop going through
  `setReflectionAmount()`, or the warning itself is wrong.

## Note

Verified in both the (then) `StandardResource` and `PBRResource` before the material merge; re-check
against the single merged class.
