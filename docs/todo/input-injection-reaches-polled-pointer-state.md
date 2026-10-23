---
id: input-injection-reaches-polled-pointer-state
title: Console input injection cannot press a polled mouse button nor turn the wheel
status: open
priority: unranked
scope: src/Input/Manager.cpp (injectMouseClickEvent(), the console commands), src/Input/PointerController.*
opened: 2026-10-23
tags: [input, console, mcp, ai-runtime-control]
---

# Console input injection cannot press a polled mouse button nor turn the wheel

## Why

Found 2026-10-23 while verifying projet-alpha's sword sounds from the console: the player's weapons fire on
`PointerController::isButtonPressed(Button1Left)`, a POLLED state (`s_deviceState`, filled by the GLFW callback),
while `mouseClick` / `mousePress` only dispatch events to the listeners (`injectMouseClickEvent()`) — an injected
press never makes `isButtonPressed()` true. And there is no wheel injection at all, the only way to cycle the
player's weapons (`Player::onMouseWheel()` → `cycleWeapon()`). An AI cannot select nor swing the sword, so the
paladin's reaction to it was verified only through the paladins' own swings hitting each other.

Unclear whether this is a defect (an injected press that the polled state ignores reads as a lie) or a missing
feature: the owner ranks it.

## What remains

- `mousePress` / `mouseRelease` also set the polled button state (and `mouseClick` a press visible for at least one
  logic cycle), the GLFW-thread / logic-thread protocol of `s_deviceState` kept (atomics, engine
  `docs/subsystems/input/03-device-state-across-threads.md`).
- A typed `mouseWheel(xOffset, yOffset)` command dispatching through `dispatchMouseWheel()`.
