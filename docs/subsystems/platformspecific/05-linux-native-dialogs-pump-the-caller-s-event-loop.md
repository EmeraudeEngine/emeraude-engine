## Linux: native dialogs pump the caller's event loop

The Linux dialogs spawn `zenity` / `kdialog` and read their stdout. Doing that with the plain
`executeCommand()` parks the **calling thread** in `read()` for the entire life of the dialog, and
that thread is the engine main thread: the consumer's IPC handler runs inside
`onCoreMainLoopCycle()`, so nothing drains the window event queue while the user browses for a file.

Under Wayland this is fatal to the illusion: `xdg_wm_base` pings the client, GLFW answers with
`pong` from its event dispatch, and a client that stops answering is declared unresponsive. The user
gets a compositor prompt offering to **force-quit a perfectly healthy application** — measured on
GNOME/Mutter, main thread parked in `fgets` at `Helpers.linux.cpp` from `Message::execute`. X11
freezes identically, it is simply quieter about it.

The five `*.linux.cpp` dialogs therefore call **`executeCommandPumpingEvents()`**, which runs the
child on a worker thread and calls the supplied pump every 16 ms on the caller's thread. They pass
`[&window] { window.pumpEvents(); }` — which is why their `Window &` parameter, ignored for years,
is now used. Verified with gdb while a dialog was held open for 14 s (well past the ~5 s threshold):
the main thread sits in `wait_for(16ms)` inside `executeCommandPumpingEvents` and the `read()` is on
a worker thread, where the Windows path has been putting it all along.

**This is the exact counterpart of the Windows dedicated-STA-thread wait above** — same problem
(a synchronous `execute()` contract that must not hard-block the main thread), same shape (dialog
off-thread, pumped wait on the caller). Windows needs it for the modal owner handshake and the DWM
ghost; Linux needs it for the compositor ping. macOS needs nothing: `NSOpenPanel`'s modal session
pumps the run loop itself.

> [!WARNING]
> Do **not** route a dialog back through the plain `executeCommand()` "because it is simpler". It
> compiles, it works on the developer's X11 box, and it hands Wayland users a force-quit prompt on
> every file open.

### Reentrancy — Linux nests, it does not refuse

`pumpEvents()` delivers window callbacks, so a callback can open a second dialog while the first is
still up (`Shift+F10` does exactly this). `executeCommandPumpingEvents()` detects it with a
`thread_local` flag and makes the **inner** call fall back to the plain blocking form.

This differs deliberately from the Windows path, which *refuses* a reentrant open. The Windows
reason does not exist here: refusal is mandated there by Win32's non-ref-counted
`EnableWindow(owner)`, whose inner close would re-enable the main window under the outer modal. A
zenity dialog is a separate client that is not even parented to our surface, so a nested one is
merely a second window and refusing it would silently swallow a user action.

The honest limit: while the **inner** dialog is up, the main thread is blocked again and the
compositor can complain — the outer pump is below it on the stack, not running. Nesting the pumps
instead would fix that at the price of unbounded recursion; for a path this rare the trade is not
worth it. If nested dialogs ever become a real scenario, the fix is to defer the second request
until the first closes, not to nest the pumps.

---
