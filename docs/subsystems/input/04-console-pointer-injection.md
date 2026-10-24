## The console drives a POLLED mouse button and the wheel (2026-10-23)

### Why

A consumer reads a button two ways: by EVENT (`PointerListenerInterface::onButtonPress()`) or by POLLING the state
(`PointerController::isButtonPressed()`, `s_deviceState`). The console's `mouseClick` / `mousePress` /
`mouseRelease` only dispatched events (`injectMouseClickEvent()`), so a polled button never read pressed — and there
was no wheel injection at all. projet-alpha's player fires its weapon while the left button is POLLED held and cycles
its weapons only with the wheel: an AI could neither select nor swing the sword (found verifying the actor sounds,
owner: "on rajoute la fonctionnalité").

### The contract

- `PointerController::injectButtonState(button, held)` — the `KeyboardController::injectKeyState()` model: an
  `s_injectedState` array (one relaxed atomic per button, main thread) OR-ed into the hardware state at each per-frame
  copy (`readDeviceState()`), and written into `s_deviceState` at once (the copy may be disabled). Out-of-range button:
  ignored.
- `mousePress` holds the button for the polling code until `mouseRelease` (both still dispatch the event).
  ⚠️ `mouseClick` stays EVENT-ONLY: a press and a release in the same instant are never seen by a reader that polls
  once per logic cycle — use the pair, with the hold the consumer needs (one logic cycle is 16.7 ms).
- `Input::Manager::injectMouseWheelEvent(x, y, xOffset, yOffset, modifiers)` and the console
  `mouseWheel(x, y, yOffset, xOffset = 0, modifiers = 0)`: dispatched like the GLFW scroll callback
  (`dispatchMouseWheel()`: to the tracking listener, else to the pointer listeners). Non-finite values and bad
  modifiers are refused. `yOffset` positive = away from the user (scrolling up).

### Measured (projet-alpha `animation-debug`, Linux Release, test ports, 2026-10-23)

`mouseWheel(100, 100, 1)` twice: "Cycling weapon to 'RocketLauncher'" (the first selection), then 'Sword'. The player
placed 1.8 m from a paladin, `mousePress(100, 100, 0)`, 0.1 s, `mouseRelease(100, 100, 0)`: the paladin groaned
(`hurt1`, own sound) with 4 armour-hit one-shots; 0 `VUID-`.
