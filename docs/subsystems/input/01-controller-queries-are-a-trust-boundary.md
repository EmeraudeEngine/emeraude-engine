## Controller queries are a trust boundary (triad, 2026-09-30)

`KeyboardController`, `PointerController`, `JoystickController` and `GamepadController` are PUBLIC engine API: game
code passes them any integer / enum. Every query is range-checked ALWAYS, Release included (plan Ave Robustus, the
two-level ruling: a check costs one comparison here):

- an out-of-range key / button / axis / hat answers the neutral value — "not pressed", "released", 0, `Center` — and
  never reads or writes out of the device-state arrays (`KeyUnknown`, -1, included);
- `isConnected()` accepts `0 ≤ id < DeviceCount` (it accepted `DeviceCount`, one past the array);
- no throwing `.at()` (std::terminate under `-fno-exceptions`): bounded `operator[]` after the check.

Fixed on 2026-09-30 (never seen at runtime: no device on the three test machines — proven by code):

- `JoystickController::axeValue()` and `GamepadController::axeValue()` read the axis only when the device was NOT
  usable: a connected joystick / gamepad always answered 0, and with no device the joystick threw (`.at(-1)`,
  std::terminate) and the gamepad read `[-1]`.
- `GamepadController::isButtonReleased()` answered `== GLFW_PRESS`, exactly like `isButtonPressed()`.
- joystick buttons / hats were bound-checked in Debug only, with `>` instead of `>=`.
- projet-alpha `Player::getJoystickMoveInput()` mapped only the POSITIVE half of each axis (`else if ( value > 0 )`
  three times): Left, Upward and Forward were dead.

The console / MCP injection (`keyPress`, `mouseClick`, `mousePress`, `mouseRelease`) validates its arguments before
reaching the controllers (keys 32-348, buttons 0-7, modifiers 0-63).
