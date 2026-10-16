## Listener registrations are tokens; GLFW callbacks reach their object through the window (Ave Robustus II P4, Oct 2026)

Owner decisions 2026-10-16 (plan Ave Robustus II, phase P4): one generic registration token in emeraude-base, a
deferred removal during a dispatch, and a callback context as the window user pointer.

### Registering a listener

`Input::Manager::addKeyboardListener(listener)` and `addPointerListener(listener, priority)` return a
`Base::ScopedRegistration` (emeraude-base `src/ScopedRegistration.hpp`): the listener is served until the token ends
(destroyed, `reset()`, or replaced). There is no public remove any more, and no `removeAll*()`.

```cpp
class MyController final : public Input::KeyboardListenerInterface { … };

// A member of the owner, declared AFTER the listener it registers (destroyed first):
Base::ScopedRegistration m_keyboardRegistration;

m_keyboardRegistration = inputManager.addKeyboardListener(&m_controller);
m_keyboardRegistration.reset(); // stop listening now (optional: the owner's end does it)
```

- An empty token comes back when the listener is null or already registered (traced).
- The input manager outlives every token (Core declares it before the services and scenes that listen).
- The holders today: Core (`m_coreKeyboardRegistration`), `Overlay::Manager` (`enable()`, released by `enable(false)`
  AND by `onTerminate()` — it never unregistered before), `Scenes::Editor::Manager` (constructor), `Scene` (its node
  and orbit controllers, `enable()` / `disable()`), projet-alpha's `Act` (the player, `registerPlayerControl()` /
  `unregisterPlayerControl()`; the act's end also unregisters it now).

### A dispatch never walks a list that changes

The keyboard and pointer lists are walked on the main thread by the GLFW callbacks and the console injections. Every
walk goes through `Manager::dispatch()` under a `DispatchScope`:
- a listener removed DURING a dispatch leaves a null slot, skipped, and is never called again, not even later in the
  same dispatch; the outermost dispatch compacts the list when it ends;
- a listener added during a dispatch waits until it ends (newest-first order kept, priority invariant kept);
- a listener removed by its own handler cannot be asked about propagation any more: an event it processed stops there;
- removing a pointer listener also clears the priority slot and the move-events tracking it held.

⚠️ Fixed through it (2026-10-16): F4 (unload the act) and Shift+Escape (stop) run inside Core's `onKeyRelease()`,
DURING the key dispatch, and removed the player and the scene's node / orbit controllers from the list being walked
(a range-for over a shrinking vector: undefined behaviour, survived by luck). Proven with a test-only trace: each of
the two keys now compacts 2 keyboard and 2 pointer slots after the dispatch. Also fixed: `addKeyboardListener()`
looked for duplicates with `binary_search` on a list that is never sorted (the pointer side had been fixed before).

### GLFW callbacks: no static instance

GLFW gives each window ONE user pointer. The `Window` owns it: a `Window::CallbackContext {window, inputManager}`
member, set as the user pointer at creation (`Window::callbackContext(handle)`).
- The window callbacks read `context->window` (`Window::fromHandle()`).
- The input manager attaches itself while its callbacks are linked (`Window::attachInputManager()` returns a token,
  held in `m_windowAttachment`); every input callback reads `context->inputManager` and returns at once when it is null
  (`Input::Manager::fromHandle()`). `Input::Manager::s_instance` is gone; the injections are member functions.
- Two GLFW callbacks carry no window, so they keep a static receiver, set and cleared ONLY by a token, null-checked,
  one owner at a time (a second one is refused, traced): `Window::s_monitorCallbackReceiver`
  (`m_monitorCallbackRegistration`, the monitor hot-plug) and `Input::Manager::s_joystickCallbackReceiver`
  (`m_joystickCallbackRegistration`). The token also uninstalls the callback.
- macOS: the pinch gesture monitor captures the manager; it is removed by `unlinkWindowCallbacks()` and by the
  destructor, and a second install replaces the first.

### Verified (Linux, 2026-10-16)

- GCC Release and clang `-fsyntax-only -Wdocumentation -Wthread-safety` on the touched TUs: 0 warning; clang-tidy
  21.1.6 on the changed lines: 0 (2 fixed: a forwarding reference never forwarded, designated initializers).
- projet-alpha `default`: F4 twice with reloads, then Shift+Escape — the player registered / unregistered each time,
  exit 0, 0 VUID; the deferred compaction proven by the trace above.
- The startup fault-injection sweep (clang ASan): 33 services, 0 ASan error, 0 leak, 0 validation message.
