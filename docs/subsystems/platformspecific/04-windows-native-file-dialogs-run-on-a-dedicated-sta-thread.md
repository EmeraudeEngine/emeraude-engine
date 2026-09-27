## Windows: native file dialogs run on a dedicated STA thread

`OpenFile.windows.cpp` and `SaveFile.windows.cpp` do **not** show the modern COM dialog
(`IFileOpenDialog` / `IFileSaveDialog`) on the engine's main thread. Their `execute()` is a thin
wrapper that delegates to the shared helper `runFileDialogOnDedicatedThread()`
(`Helpers.hpp` / `Helpers.windows.cpp`): it spawns a **dedicated STA thread** with an empty
message queue, runs the dialog body (`OpenFile::showDialog` / `SaveFile::showDialog`) there, and
waits for it. The helper owns the thread, COM init, the cross-thread owner handshake
(`AttachThreadInput`) and the message-pumping wait; the per-dialog `showDialog()` only does the COM
dialog work and receives the owner `HWND`. On failure to spawn the worker (OS resource exhaustion),
it falls back to running the dialog inline on the caller thread.

**Why (perf):** the dialog's modal message loop pumps **every** message of the thread it runs on.
On the engine main thread that means the heavy main-thread traffic (rendering, input, and — under
a CEF-based consumer, the browser traffic too), and the Windows shell reacts by re-resolving its navigation pane on every burst:
thousands of redundant known-folder / cloud-sync-root (`SyncRootManager`) registry lookups. On a
machine whose known folders are redirected to a cloud provider (OneDrive, the Windows default), this
delays the dialog **3–5 s**. A dedicated thread has an empty message queue, so the pane resolves once
and the dialog appears in **~140 ms** (measured ≈×25 speed-up). Apps with an idle UI thread (e.g.
Notepad++) never hit this — which is why the same dialog is instant elsewhere on the same machine.

> [!WARNING]
> Do **not** "simplify" by calling `Show()` directly on the calling thread — it silently
> reintroduces the 3–5 s freeze on any machine with cloud-redirected known folders (invisible on
> a plain dev box, hence hard to diagnose).

### Owner-based modality, centering & Z-order

The dialog is shown **owned by the real main window** (its `HWND` is passed to the dialog body and
fed to `IFileDialog::Show(owner)` and the legacy `OPENFILENAMEW.hwndOwner` / `BROWSEINFOW.hwndOwner`).
Win32 owner relationships are the OS-native mechanism for all three behaviors the dialog needs, so
the OS handles them for free:

- **Modality** — the OS disables the owner while the dialog is up.
- **Centering** — the shell positions the dialog on its owner, correct monitor, no post-show jump.
- **Z-order** — an owned window is always above its owner and is **raised with it on activation**, so
  the dialog resurfaces after an alt-tab / taskbar click instead of being buried behind the main
  window.

The owner is **cross-thread**: the dialog runs on the worker (STA) thread, the owner lives on the
main thread. A cross-thread owner deadlocks if the owner thread stops pumping messages (e.g. a bare
`join()`), so it is made safe here by two things the helper does:

1. **The caller pumps messages while it waits** (see § Responsiveness) — so the owner's implicit
   `WM_ENABLE` / activation sends, dispatched from the worker, are serviced instead of deadlocking.
2. **`AttachThreadInput(workerThreadId, ownerThreadId, TRUE)`** for the dialog's lifetime — merges the
   two threads' input state so activation and **focus return on close** behave as if same-thread.
   It is detached after the dialog returns. (Attaching a thread to itself fails harmlessly on the
   inline-fallback path, where owner and dialog are already on the same thread.)

The worker still replicates the owner's DPI awareness context (`SetThreadDpiAwarenessContext`) so the
dialog renders at the right scale. Everything is **gated on `parentToWindow`**: when it is `false`
the dialog is shown with a null owner — no modality, no centering — for callers that deliberately
want an ownerless dialog.

### Responsiveness — message-pumping wait

`execute()` is synchronous by contract (the consumer's IPC handler needs the result to send its
ZeroMQ reply), so the caller must wait for the worker — but a bare `join()` **hard-blocks** the main
thread, which breaks two things:

1. The dialog **owns the main window cross-thread**, so the OS dispatches `WM_ENABLE` / activation
   sends to the main thread from the worker; a non-pumping thread never services them, deadlocking
   the native modal handshake.
2. After ~5 s a non-pumping thread is marked **"Not Responding"** (DWM ghost: greyed snapshot,
   spinning cursor), even though the dialog is alive on the worker.

`runFileDialogOnDedicatedThread()` therefore replaces the bare `join()` with a **drain-then-wait**
loop on the worker's thread handle: each iteration first drains the caller thread's own queue
(`PeekMessage` / `Translate` / `Dispatch`), then blocks in `MsgWaitForMultipleObjectsEx` until the
worker terminates or new serviceable work arrives, then `join()`s (which returns immediately).
`WM_QUIT` is handled specially — it is re-posted (`PostQuitMessage`) and the dialog is waited out, so
the engine's main loop sees the quit only after the dialog closes.

> [!WARNING]
> The wake mask **excludes `QS_INPUT`** (it is `QS_PAINT | QS_TIMER | QS_POSTMESSAGE |
> QS_SENDMESSAGE`), and `MWMO_INPUTAVAILABLE` is **not** used (hence drain-*first*). This is not
> cosmetic: `AttachThreadInput` shares the worker's input queue with the main thread, so mouse-moves
> / keystrokes aimed at the **dialog** flag input as "available" on the main thread. With `QS_INPUT`
> in the mask (or with `MWMO_INPUTAVAILABLE`), the wait would return immediately and repeatedly while
> `PeekMessage` — which only retrieves messages for the main thread's *own* windows — removes nothing:
> a **100 % CPU busy-spin whenever the user moves the mouse over the dialog**. The main window is
> disabled by the modal and has no input to process anyway; it only needs paint / posted (CEF) / timer
> messages. Sent messages (the owner's `WM_ENABLE` / activation handshake) are serviced by the wait
> regardless of the mask.

This does **not** reintroduce the 3–5 s perf storm: that was the *dialog's* modal loop pumping heavy
traffic, and the dialog's loop runs on the worker. This pump only services the main thread's own
messages (paint, CEF, DWM responsiveness pings). The main window is disabled by the native modal (it
is the owner), so the pumped input is ignored — **responsive but non-interactive**, the correct
modal behavior.

### Reentrancy — a reentrant open is refused, not nested

The pump above dispatches messages, which re-enters the main window's `WndProc`. A dispatched
message can call back into `runFileDialogOnDedicatedThread()` to open a **second** native file dialog
on the same thread. This is **refused at the top of the function** (a `thread_local` "active" flag,
RAII-scoped over the whole operation): a reentrant call logs a warning and returns `false`, which the
caller treats as a cancellation.

Refusing — rather than nesting — is the only robust fix, because the damage is done by the nested
dialog *opening*, not by any nested pumping:

- A nested dialog owned by the same main window calls `EnableWindow(owner, FALSE)` on `Show()` and
  `EnableWindow(owner, TRUE)` on close. Win32 owner enable/disable is **not ref-counted**, so the
  inner dialog's close would **re-enable the main window while the outer dialog is still modal** —
  the app becomes interactive under an open modal. Preventing the nested `Show()` is what closes this
  hole; merely avoiding a nested pump would not.
- Two stacked native file choosers are not a meaningful UX anyway.

> [!WARNING]
> Do **not** "fix" a reentrant dialog request by opening it on another worker/owner or by pumping
> differently — a second dialog owned by the same window reopens the non-ref-counted owner-enable
> hole above. The reentrant request must be refused (or deferred by the caller until the first
> dialog closes).

---
