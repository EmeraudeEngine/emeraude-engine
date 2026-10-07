---
id: console-input-injection-wheel-and-polled-buttons
title: The console cannot inject a mouse wheel, and injected buttons never reach the polled pointer state
status: open
priority: unranked
scope: engine Input (Manager console commands, PointerController)
opened: 2026-10-07
tags: [console, mcp, input, testing]
---

# The console cannot inject a mouse wheel, and injected buttons never reach the polled pointer state

## Why

Found by the macOS and Windows peers on 2026-10-07, both unable to script projet-alpha's sword test in
`animation-debug` (the paladin-death deadlock fix, engine `ed17e501`):

- a weapon is selected only by `Player::onMouseWheel()` → `cycleWeapon()`, and `InputManagerService` offers
  `keyDown` / `keyPress` / `keyUp` / `mouseClick` / `mouseMove` / `mousePress` / `mouseRelease` — no wheel;
- firing is POLLED: the player reads `PointerController::isButtonPressed(Button1Left)`, whose `s_deviceState` is
  refreshed from `glfwGetMouseButton()` (`src/Input/PointerController.cpp`), so `mousePress` / `mouseRelease` (event
  dispatch) never fire a weapon — checked live on macOS with the default rocket launcher: no effect.

So any test that needs a weapon, or any polled mouse button, needs a hand on the mouse. A tooling lack, not a defect:
the owner ranks it.

## What remains

- [ ] A `mouseWheel(x, y)` console / MCP command dispatching the wheel event (the same path as the GLFW callback).
- [ ] Injected button presses visible to the polled state: an injection overlay in `PointerController` (pressed by
      `mousePress`, cleared by `mouseRelease`), OR'ed with the device state — without letting it stick when the
      console client disconnects.
- [ ] `tools/mcp-conformance.py` after the console change; then the sword test scripted end to end.

## References

- `src/Input/Manager.cpp` (the `mouse*` commands), `src/Input/PointerController.cpp` (`s_deviceState`).
- projet-alpha `src/Actor/Player.cpp` (`onMouseWheel`, `cycleWeapon`, the polled fire).
