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

### What is NOT atomic: a listener's switch, a controller's device slot (owner decision 2026-11-15)

`KeyboardListenerInterface::enableKeyboardListening()` / `isListeningKeyboard()`, the pointer listener's pair, and a
controller's device slot (`GamepadController::attachDeviceID()` / `JoystickController::attachDeviceID()`, read by
`usable()` at every poll) are plain members: the input manager reads the listeners' switches on the main thread, where
they are written. An application whose LOGIC thread reads them too — the testbed's player polls `isListeningKeyboard()`,
`isListeningPointer()` and its gamepad every logic cycle — changes them inside
`Scenes::Manager::betweenLogicCycles()` (scenes doc 08): the write then lands between two cycles. ThreadSanitizer
reported the three of them (and the testbed's own flags) written by the main thread at an act change while the logic
thread read them; the owner chose that one ordering over per-member atomics (a group of switches changes together, and
the listener interfaces stay copyable). 0 report after, 3 runs.

### What a listener DOES: it records, the logic thread applies (owner decisions, 2026-11-17)

The sections above are about the STATE of a device and a listener's switch. A listener's CALLBACK
(`onKeyRelease()`, `onPointerMove()`, `onMouseWheel()`…) runs on the main thread, inside
`Input::Manager::waitSystemEvents()` — and what it does there must not touch what the logic thread owns. The scene's
node controllers did: `OrbitController` wrote its camera node at every pointer event (`Node::setPosition()` +
`lookAt()`, which push the camera's coordinates into the view's logic state), while the logic thread processed the
node and published the view. A node has no lock.

The contract (`Scenes::AbstractNodeController`, scenes doc 02 § Controllers): **the callback only records, under the
controller's own mutex; `update()`, called by the logic thread at every cycle, applies what was recorded to the node.**
One logic cycle of delay at most; the main thread never waits for a cycle (a wait per pointer event would stall the
event pump: a mouse sends dozens of moves per cycle). Any listener that drives a scene object follows it — the
alternative for a rare, discrete change is `Scenes::Manager::betweenLogicCycles()`.

Proof (Linux, engine 3186ee4e + the change):
- ThreadSanitizer (Debug, window-less, the software device; no demo, a glTF file opened in the model viewer through
  `Core.openFiles`, then two drags of 60 pointer moves and 20 wheel notches injected by the console): 31 reports
  before — `ViewMatrices2DUBO::updateViewCoordinates()` and `Node::lookAt()` on the main thread against the logic
  thread's `publishStateForRendering()` and `Node::getWorldCoordinates()` —, exit code 66; 0 report after, exit 0.
- The same behaviour (Release, RTX 3070 Ti, window-less, the validation layer loaded, 0 VUID): the camera node's
  position read after each stage — framed at the opening, after a drag of 40 moves, three notches in, five out, a move
  without a drag — is the SAME to nine digits with the binary before and the binary after.
- `NodeController` has no caller that gives it a node (engine, testbed, a downstream application): exercised by a
  temporary change that handed it a probe node in the model viewer. The keypad's steps arrive one by one (-6, 0, +6
  on X, then Y, then Z; the transform-space and mode keys move nothing), none dropped; 91 injected keys and a drag
  under ThreadSanitizer: 0 report; the node destroyed, the controller lets it go. ⚠️ Its gamepad page was NOT
  exercised (no gamepad can be injected): its 18 bindings were compared one by one with the code they replace.
- A control on ordinary scenes (the controllers are updated at every cycle, with or without a node): the render
  statistics of `animation-debug`, `default` and `geometry-loader` are the same with the binary before and after
  (22 instances / 61198 triangles in the view, 21 / 61186 in the shadows for the first), 0 VUID.

⚠️ These positions start from the model viewer's FALLBACK framing of that day (it never got its model's extents, a
defect repaired right after: scenes doc 12): they prove the controller, not the framing, and a `Box` opened now is
framed closer.

**Peers' round 17** (2026-11-17, engine 5b23b534), each with its own binary of before (engine 8332013e) and the new
one, the same scenario:
- Windows (RTX 3500 Ada, driver 596.58, MSVC 19.44 `/W4 /WX`): Release and Debug 0 error / 0 warning; the five
  positions identical digit for digit before and after, and equal to Linux to nine digits; exit 0, 0 VUID.
- macOS (Apple M6, MoltenVK 1.4.2, `-Wdocumentation -Werror`, `-Wthread-safety` silent): Release and Debug 0 error /
  0 warning; the five positions identical digit for digit before and after, equal to Linux to at least seven
  significant digits (the drag's X differs in the eighth); exit 0, 0 VUID.
- Both: `animation-debug` at 22 / 61198 | 21 / 61186, no step dropped, no contract fault. `Core.openFiles` answers
  in 5.4 s (Windows) and 5.8 s (macOS) with BOTH binaries: not this change.

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
