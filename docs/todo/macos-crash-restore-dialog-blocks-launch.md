---
id: macos-crash-restore-dialog-blocks-launch
title: After a crash, macOS blocks the next launch in glfwInit() on the "reopen windows?" dialog
status: open
priority: high
scope: PlatformManager (macOS), the application bundle
opened: 2026-10-11
tags: [macos, unattended-runs, defect]
---

# After a crash, macOS blocks the next launch in glfwInit() on the "reopen windows?" dialog

## Why

macOS-PA (M6, 2026-10-11): after two crashes, the next launches hung for 240 s. `sample`: main thread in
`PlatformManager::onInitialize → glfwInit → _glfwInitCocoa → [NSApplication run] → … NSPersistentUIRestorer
promptToIgnorePersistentStateWithCrashHistory → [NSAlert runModal]` — AppKit's state-restoration prompt waits for a
human. Any unattended run (sweeps, CI, a peer session) blocks.

## What remains

- Opt the application out of window state restoration (e.g. register `ApplePersistenceIgnoreState = YES` in the
  application's user defaults before `glfwInit()`, or the bundle / GLFW equivalent) — check which one AppKit honours
  before NSApplication starts.
- Re-test: a forced crash, then an unattended launch must not block.
