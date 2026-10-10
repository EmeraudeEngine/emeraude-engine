---
id: macos-terrain-open-subprocess-error
title: macOS terrain logs "Failed to run a subprocess : Interrupted system call" (an unexplained `open` run)
status: open
priority: high
scope: PlatformSpecific/Desktop/Commands.mac.mm, whatever calls it during terrain
opened: 2026-10-11
tags: [macos, defect, investigation]
---

# macOS terrain logs "Failed to run a subprocess : Interrupted system call" (an unexplained `open` run)

## Why

macOS-PA (Mac mini M6, Release, validation ON, 2026-10-10), `--load-demo terrain`, while the imposter atlases bake:
`[Error][Commands] Failed to run a subprocess : Interrupted system call`. It comes from
`Commands.mac.mm` (`runDesktopApplication` / `runDefaultDesktopApplication`, `reproc::run` of `open …`, logged when
the exit code is not 0 with whatever `errorCode` holds — EINTR may not be the real cause). No caller of those
functions was found in the engine or projet-alpha sources: who spawns `open` during `terrain` is unknown. The scene
loads, 0 VUID.

## What remains

- Find the caller (breakpoint on the two functions), say why `open` is run at all during a demo load.
- Log the real failure (exit code AND error code separately).
