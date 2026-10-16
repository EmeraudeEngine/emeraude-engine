## The device state is read across threads: one atomic per input (Ave Robustus II P5, Oct 2026)

Owner decision 2026-10-16 (plan Ave Robustus II, phase P5, lot 4): one atomic per key, button, axis and hat, relaxed.

### Who writes, who reads

The controllers' state lives in static arrays, shared by every controller instance of a kind:
- `KeyboardController::s_deviceState` / `s_injectedState` (349 keys), `PointerController::s_deviceState` (8 buttons),
  `GamepadController::s_devicesState` (a `GamepadState` per device: 15 buttons, 6 axes — `GLFWgamepadstate`'s layout,
  checked by a `static_assert`), `JoystickController::s_devicesState` (a `JoystickState` per device: 6 axes, 16
  buttons, 4 hats).
- WRITTEN on the main thread: the GLFW key callback (`changeKeyState()`), the per-frame `readDeviceState()` of each
  controller, the console injections (`keyDown` / `keyUp` → `injectKeyState()`), `clearDeviceState()` at a
  disconnection.
- READ on the LOGIC thread too: projet-alpha's `Player::readDirectInput()` polls `isKeyPressed()`, the gamepad and the
  joystick every logic cycle (`Act::processLogics()`).

Before 2026-10-16 the arrays were plain `char` / `float` / `bool` / `GLFWgamepadstate`: a data race on every input.
Proven with ThreadSanitizer on the OLD `KeyboardController` (the stress below built against it in isolation): a race
between `changeKeyState()` on one thread and `isAnyKeyPressed()` on the other, reported at once.

### The contract now

- Every element is a `std::atomic` (`uint8_t`, `float`, `bool`, `JoystickHatDirection`), loaded and stored with
  `std::memory_order_relaxed`: each input is coherent on its own. There is NO snapshot of the whole device — a reader
  may see W pressed and A not yet in the same frame, exactly as before (the owner chose per-input atomics over a
  published double buffer).
- `readDeviceState()` reads the hardware in one go into a LOCAL buffer (the custom GLFW's `glfwGetKeyboardState()` /
  `glfwGetMouseButtonState()`, `glfwGetGamepadState()`, the joystick's pointer + count), then publishes it element by
  element. ⚠️ `GLFW_EM_CUSTOM_VERSION` IS defined by the bundled GLFW (`dependencies/glfw/include/GLFW/glfw3.h`): the
  bulk functions are the live path, the per-key `glfwGetKey()` loop is the fallback for a stock GLFW.
- The injected keys stay OR-ed into the hardware state at each copy (a console `keyDown` survives the per-frame read).
- `JoystickState` lost its `EMEN_API` (it is only the private static's element type; atomics make it non-copyable).

### Reading it from the console (2026-10-16)

`Core.InputManagerService.gamepadState(slot)` and `joystickState(slot)` (read-only, also MCP tools) report a connected
device's buttons / axes / hats through `getRawState()`; an unknown slot is refused with the connected list.
⚠️ The per-frame copy of the gamepad and joystick states is OFF by default (`enableCopyGamepadsState()` /
`enableCopyJoysticksState()`, nothing in the cascade turns them on): the probe then reads the device once itself (the
same read and atomic publish, on the main thread) and says so ("Per-frame copy disabled: read from the device now.").
Measured 2026-10-16, Linux, a Logitech F710 at slot 0: at rest the triggers read -1 and the sticks ±0.004; while the
owner pressed the controls, a 1 Hz poll caught button 12 (D-pad right), both triggers (axis 5 = 1, axis 4 = 0.18) and
the right stick (axis 2 = 0.2). Windows, a "Retro Controller": hot unplug / replug traced at slot 0.

### ⚠️ MSVC: no `inline static` atomic arrays in an exported class

`GamepadController::s_devicesState` was first an `inline static` member: MSVC gives an inline static of an `EMEN_API`
class a dynamic initializer in every importing translation unit (`std::atomic` arrays are not constant-initializable
for it), removes it as unreferenced and warns C4505 — under `/WX`, 27 projet-alpha TUs failed (found by the Windows
peer; `constinit` is refused, C2127). Every such state is defined out of line in the controller's `.cpp`
(`JoystickController.hpp` already carried that note).

### Proof

- `tools/stress/InputStateStress.cpp` (`EmeraudeInputStateStress`, built with `EMERAUDE_ENABLE_TSAN`): the main thread
  presses and injects the player's keys while a second thread reads them like the logic thread, 5 s; run with
  `TSAN_OPTIONS=halt_on_error=1`. 2026-10-16, Linux, GCC TSan: 0 report on 6 runs (18-28 M write passes each); the
  same file against the old `KeyboardController`: a data race at once.
- projet-alpha `animation-debug`: `keyDown(65)` 2 s (A, strafe left) moved the view (the static red cube's centroid
  1482.6 → 1672.6 px), and after `keyUp(65)` the view held still (1672.6 two seconds later): the logic thread sees the
  injected key, then its release. 0 VUID.
- Found on the way: the gamepad's `getRawState()` stopped one short of the buttons and of the axes (an index loop to
  `..._LAST` exclusive); the keyboard's printed released keys as "Button #". Both walk the whole state now.
