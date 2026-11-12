## Swap-chain failure paths: a refused frame submission, a headless recreation, an image count beyond capacity, the retries of an on-demand application (Nov 2026)

### A refused frame submission

A refused `vkQueueSubmit()` of the frame's main batch is a legal outcome (`VK_ERROR_OUT_OF_HOST_MEMORY`,
`VK_ERROR_OUT_OF_DEVICE_MEMORY`, `VK_ERROR_DEVICE_LOST`, a validation layer rejecting the batch): the specification
leaves the primitives the batch names unaffected. `Renderer::renderFrame()` then calls `discardAcquiredImage()`: an
empty batch waits the frame's binary semaphores (the acquisition and the shadow-map / render-to-texture ones, each
once) and signals the in-flight fence, and the swap-chain is marked degraded — the acquired image is never presented,
only a recreation gives it back. The next frame is dropped and `retryDegradedSwapChain()` recreates the swap-chain
(at most `MaxSwapChainRecreationFailures` attempts, then the stop of doc 24).

### The defect (2026-10-29 / 2026-11-11): a window-less run died in the recreation

`recreateRenderingSubSystem()` queried the window SURFACE before recreating (`Surface::update()`, then its
`maxImageExtent`). A window-less run (`--window-less`) has no surface: its swap-chain is HEADLESS, engine-owned images
(`SwapChain::isHeadless()`). Any degradation of it — a refused submission, a resize — reached
`m_window.surface()->update()` on a null pointer: SIGSEGV at `Surface.cpp` (`this=0x0`), on the rendering thread.

- Seen in a downstream application run with `--window-less` (2026-10-29, Linux, NVIDIA, Xvfb), and on Linux Intel
  RPL-S with Synchronization Validation rejecting a submit (`SYNC-HAZARD-READ-AFTER-WRITE`, 3 runs of 3, before the
  upload ordering lot of doc 25): two engine items, one defect.
- **Owner decision 2026-11-11: a degraded headless swap-chain is recreated without a surface**, by the same protocol
  as a window's (the same attempts, the same give-up). `recreateRenderingSubSystem()` skips every surface query for a
  headless swap-chain; `SwapChain::recreate()` already rebuilt the engine-owned images from the published framebuffer
  size. A window-less resize takes the same path.

### Proof (`--inject-frame-submit-failure`, a build with `EMERAUDE_FAULT_INJECTION`)

`FaultInjection::FrameSubmitFailureArgument`: `--inject-runtime-failure-delay` seconds after the main loop started,
Core calls `Renderer::injectFrameSubmitFailure()`; the next frame's main submission is refused before it reaches the
queue (as the specification states for a failed submission) and traces
"FAULT INJECTION: the main submission of the frame #N is refused.". Inert in a normal build (`if constexpr`).

Measured 2026-11-11, Linux main, RTX 3070 Ti, projet-alpha Debug with the option (`.claude-build-debug-fault`), under
gdb, validation layer loaded:

| Run | Before the fix | After the fix |
|---|---|---|
| `basic-scenery --window-less` | SIGSEGV, `Surface::update (this=0x0)` ← `recreateRenderingSubSystem` ← `retryDegradedSwapChain` ← `renderFrame` | headless swap-chain recreated (1280x720), exit 0, 0 VUID |
| `sponza --demo-options 1 --window-less` | — | recreated; 77 FPS 8 s and 13 s after the incident (frames go on), exit 0, 0 VUID |
| `basic-scenery`, a real window (Wayland) | — | the swap-chain recreated on the surface, 19 FPS at 2880x1620 afterwards, exit 0, 0 VUID |

The only `[Error]` lines of these runs are the injection's own.

⚠️ `xvfb-run` does not isolate a GLFW application on a Wayland desktop: `WAYLAND_DISPLAY` stays set and GLFW opens
its window on the compositor. Unset it (or pass the X11 switch) to really run on Xvfb.

### Not settled

- What degraded the swap-chain in the downstream application's window-less run of 2026-10-29 (an overlay surface was
  being created with a 0.001 x 0.001 geometry just before) is unknown; the recovery is now proven whatever the cause.

### A driver that creates more images than the engine handles (owner decision 2026-11-11)

The engine requests `minImageCount` images (2 or 3, `SwapChain::selectImageCount()`), but the specification lets the
implementation create MORE; `vkGetSwapchainImagesKHR` returns the real count. The swap-chain's frames
(`StaticVector< Frame, 5 >`), the renderer's frame scopes (`MaxFramesInFlight`, 5) and its present semaphores (5) are
sized by that count: above 5 their `resize()` was a `std::abort()` (no exceptions), at startup or at the first
recreation. Found by reading (2026-10-25), never observed on the cascade's machines.

- **Refused, never an overflow**: `SwapChain::prepareFrameData()` refuses a count of 0 or above its capacity
  ("The driver created a swap-chain of N images; the engine handles 1 to 5 : the swap-chain is refused !"), and
  `Renderer::createRenderingSystem()` checks its own capacities again. At startup the renderer service fails (exit 1);
  at a recreation the attempts of doc 24 run, then the give-up.
- Running on such a driver (frames in flight = min(images, 5), per-image arrays with their own cap) is a larger change,
  not done.
- `retrieveSwapChainImages()` queries the images with a LOCAL count and refuses an answer different from the count
  the frames were sized by (2026-11-12): macOS-PA found that `--inject-swap-chain-image-count=4` on a real 3-image
  surface left 4 frames for 3 images (the second query wrote the driver's 3). ⚠️ So an injected count BELOW the capacity
  takes effect on a HEADLESS swap-chain only (window-less); on a real surface it is refused at the second query. A count
  ABOVE the capacity is refused first, everywhere. Measured 2026-11-12 (Linux, RTX 3070 Ti, Debug): `=4` on a real X11
  surface (Xvfb) → "The swap-chain gave 3 images where 4 were counted : the swap-chain is refused !", exit 1, 0 VUID;
  `=4` window-less → 4 frames in flight, exit 0.
- **Proof** (`--inject-swap-chain-image-count=N`, a build with `EMERAUDE_FAULT_INJECTION`: the driver's answer replaced
  at every creation, headless included). Measured 2026-11-11, Linux main, RTX 3070 Ti, `basic-scenery --window-less`,
  Debug, validation layer loaded: N = 6 → the refusal, the renderer service fails, exit 1, 0 VUID, no abort; N = 4 →
  4 frames in flight, 60 FPS, exit 0, 0 VUID.

### The recreation attempts of an application that renders on demand (owner decision 2026-11-11)

`retryDegradedSwapChain()` runs from `renderFrame()` only. In `RenderingMode::OnDemand` without an active scene the
rendering thread sleeps until a redraw is requested (`Core::renderingTask()`'s gate): a downstream application that
draws when one of its web surfaces repaints rendered no frame while nothing changed, and the attempts stopped at
"attempt 1/30" for minutes (Linux, Xvfb, `--inject-swap-chain-out-of-video-memory` then a resize, 2026-11-11, 0 % CPU)
— on a desktop, a window frozen until the user touched it.

- **Fix**: a degraded swap-chain that is not given up is one of the renderer's debts (`Renderer::owesSwapChainRecreation()`,
  in `owesFrames()`, beside an armed capture and a recording): the on-demand gate opens on it, the safety timeout of the
  gate (`OnDemandRenderingSafetyRefreshHz`, 60) picks it up, and the attempts run at their own pace
  (`SwapChainRecreationRetryInterval`, 100 ms) until the swap-chain is recreated or given up. The recreation stays on
  the rendering thread inside the scene lock; nothing is added on the main thread.
- **A/B** (2026-11-11, Linux main, RTX 3070 Ti, projet-alpha Debug with `EMERAUDE_FAULT_INJECTION`,
  `--window-less --rendering-mode=on-demand`, no scene loaded, on Xvfb, `--inject-swap-chain-out-of-video-memory`
  injected 42 s after the start, a temporary switch disabling the debt for the A run): **without** it, not one attempt
  in 60 s; **with** it, the 30 attempts and the give-up in 3 s, then the stop of doc 24 (exit 95). 0 VUID.
- ⚠️ The stop of doc 24 shows the engine's native dialog (zenity on Linux) even in a window-less run, and the main
  thread waits for it to be closed: an unattended window-less run hangs there. A test on a desktop must run on Xvfb
  with `WAYLAND_DISPLAY` unset, else the dialog lands on the user's screen.

### Peers (engine a9fe7200, 2026-11-12)

- macOS (macOS-PA, Apple M6, MoltenVK): Release 389/389, Debug 389/389, a fault-injection tree 689/689, 0 warning;
  `-Wdocumentation -Werror` clean; `basic-scenery` and `sponza 1` 0 VUID, 0 `[Error]`, exit 0. Injected refused
  submission: the swap-chain recreated on its surface, the frames going on, exit 0; image count 6 → the refusal, exit 1,
  no abort; image count 4 on a real surface → 3 frames in flight while the frames were sized for 4 (the finding that
  made `retrieveSwapChainImages()` refuse a changed count, § A driver that creates more images).
- Windows (Windows-PA, RTX 3500 Ada, window-less, lane RayTracing): MSVC 14.44 Release 387/387, Debug 387/387, a
  fault-injection tree 683/683, 0 warning under /W4 /WX; `basic-scenery` and `sponza 1` 0 VUID, 0 `[Error]`, exit 0.
  Injected refused submission: the headless swap-chain recreated 3 ms later, the frames going on, exit 0; image count 6
  → the refusal, exit 1; image count 4 (headless) → 4 frames in flight, exit 0.

- Engine c72d165c (the second-query guard): macOS-PA, `=4` on its real surface → "The swap-chain gave 3 images where 4
  were counted : the swap-chain is refused !", the startup refused, exit 1, no abort, 0 VUID; Windows-PA, the injected
  refused submission again recovered headless, exit 0, 0 VUID.

### Related

- `24-swap-chain-given-up-for-lack-of-video-memory.md` — the recreation attempts, the give-up and its exit codes.
- `25-uploads-ordered-before-their-readers.md` — the hazard that made the layer reject a submission.
- `docs/subsystems/source-tree/08-startup-fault-injection.md` — the fault-injection build.
