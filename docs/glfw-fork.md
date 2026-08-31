# The GLFW Fork — one patch, and how to move it forward

`dependencies/glfw` is **not** upstream GLFW: it is the fork
[`EmeraudeEngine/glfw`](https://github.com/EmeraudeEngine/glfw), checked out as a submodule of the
engine and pinned to a commit of its `em/customization` branch.

**Current pin:** GLFW **3.5.1** + the custom patch (`f4360311`, rebased 2026-08-31).
The previous pin was `4263be2a` = 3.4-108, an untagged `master` state.

What the pin change actually brings — 8 files, 28 insertions, of which only two touch code:

| Upstream commit | Effect |
|---|---|
| `b7ef2919` Wayland: Fix rounding of fractional scale sizes | `src/wl_window.c` — matters on a fractional-scale HiDPI Wayland session, which is the owner's workstation |
| `463cf736` Win32: Fix scancode for media keys | `src/win32_window.c` |

The rest is the version bump (3.5.0 → 3.5.1), the changelog, `.gitattributes` and `CONTRIBUTORS.md`.

## 1. What the fork adds — exactly one commit

`em/customization` carries **a single commit**, *"Add a way to get a copy of keyboard keys and mouse
buttons state"* (author LondNoir, 2025-01-12), touching two files:

| File | Addition |
|---|---|
| `include/GLFW/glfw3.h` | `#define GLFW_EM_CUSTOM_VERSION` + declarations of `glfwGetKeyboardState()` / `glfwGetMouseButtonState()` |
| `src/input.c` | Their implementation — a bulk copy of `_GLFWwindow::keys[]` / `mouseButtons[]` into a caller-owned byte array, consuming `_GLFW_STICK` exactly like `glfwGetKey()` does |

**Why:** upstream only offers per-key polling (`glfwGetKey()`), one library call per key. The engine
needs the *whole* keyboard and mouse state once per frame; the patch turns N calls into one memcpy-like
loop, sticky-key semantics preserved.

**How the engine consumes it:** through the `GLFW_EM_CUSTOM_VERSION` define, in
`src/Input/KeyboardController.cpp:114` and `src/Input/PointerController.cpp:105`. Both sites have an
`#else` fallback, so **an unpatched GLFW still compiles** — it silently falls back to the slow path.
A build that "works" is therefore *not* proof the patch survived: check the define, not the build.
`update-glfw.py` now enforces exactly that (§ 2).

## 2. Updating GLFW — `update-glfw.py`

```sh
cd <engine root>
python3 update-glfw.py [--onto REF] [--from {local,origin}] [--no-push]
```

Sequence: fetch `upstream` **and `origin`** (tags included) → resolve the target reference → reconcile
the local `em/customization` with origin's → fast-forward the local `master` mirror → rebase
`em/customization` onto the target → **verify the patch survived** → push `master` and
force-with-lease `em/customization`.

| Option | Effect |
|---|---|
| `--onto REF` | Reference the patch is rebased onto. **Default: the latest upstream tag**, i.e. a release. Pass `--onto upstream/master` for the bleeding edge, or any committish to pin something else. |
| `--from {local,origin}` | Only consulted when the local `em/customization` and `origin/em/customization` have **actually diverged**. Behind origin is fast-forwarded, ahead of origin is rebased as it is — neither asks anything. Without it, a real divergence is a hard error that moves no local branch. |
| `--no-push` | Everything local, nothing published. |

**The default target is deliberately not `upstream/master`.** Right after a release, upstream's tip is
the *next* version's development branch, so following it silently pins a `-dev` state (see § 3).

**It refuses to publish a lost patch.** After the rebase, the script checks that
`GLFW_EM_CUSTOM_VERSION` is still defined in `include/GLFW/glfw3.h` at the branch tip and prints the
patch-id of every commit above the target. If the marker is gone, it fails **before** the push:

```
Customisation verified: GLFW_EM_CUSTOM_VERSION present, 1 commit(s) above 3.5.1.
  f4360311  patch-id 6cf953c0cbc91fa356e2062e566af973c2a072f1  Add a way to get a copy of keyboard...
```

Other guarantees, each exercised on a disposable bench (two local remotes, no network):

- **A freshly initialised submodule works.** It sits on a detached HEAD with *no* local branch at all;
  `master` and `em/customization` are created from their remotes rather than assumed to exist.
- **`--onto` is validated before anything moves**, so a typo leaves the repository exactly as it was.
- **A diverged local `master` is reported, not forced.** The mirror branch is meant to be a strict
  image of upstream; the script prints the `git branch --force` command and stops.
- **The dirty-tree guard only counts tracked modifications** (`--untracked-files=no`), matching what
  git itself refuses. A stray untracked file does not block an update.
- **Any unforeseen git failure prints a message, not a Python traceback.**

## 3. Traps

- ⚠️ **`upstream/master` is not a release.** Right after 3.5.1, upstream's tip is `Start 3.6`: it bumps
  `project(GLFW VERSION 3.6.0)` and purges the 3.5 changelog. Rebasing on `master` lands you on
  **3.6.0-dev**. This is why the script defaults to the latest *tag*; `--onto upstream/master` is the
  opt-in.
- ⚠️ **There is no `3.5.0` tag, and `3.5.1` looks like a dev commit.** The annotated tag `3.5.1`
  ("Tag 3.5.1 release") sits on a commit *named* `Start 3.5.1`. That is upstream's own doing: a
  misconfigured tool briefly published an incorrect `3.5.0` tag, the rollback was manual, and the
  first release of the 3.5 series was renamed 3.5.1 to supersede it everywhere (upstream #2758, no
  code change beyond the patch number). **The pin is on a real release** — do not "fix" it because
  the commit subject reads like the opening of a development cycle.
- ⚠️ **The submodule pin is a separate act.** The script ends on the rebased `em/customization` tip;
  the gitlink in the engine (`dependencies/glfw`) still points at the old commit until it is committed
  in the engine repository.
- ⚠️ **A rebase rewrites the patch's SHA every single time**, even when nothing changed — re-running
  the script on an already-updated fork yields a new SHA for a bit-identical patch (measured). The only
  stable identity is the **patch-id**: `6cf953c0cbc91fa356e2062e566af973c2a072f1` for this patch
  (`git show <sha> | git patch-id --stable`). `4263be2a` and `f4360311` share it. **Compare patch-ids,
  never SHAs**, to prove the customisation survived.
- ⚠️ **A stale local branch is normal.** The submodule sits on a **detached HEAD**, so the local
  `em/customization` branch is whatever a previous run left there and is routinely behind (or diverged
  from) `origin/em/customization`, which is authoritative. `--from origin` discards the local tip (it
  is printed before being dropped, and stays reachable until git prunes it).
- ⚠️ The patch reaches into GLFW internals (`_GLFW_STICK`, `_GLFWwindow::keys[]`,
  `mouseButtons[]`, `src/internal.h`). It applied cleanly through 3.5.1 because those internals did
  not move, but **an upstream refactor of the input internals will conflict** — that is the expected
  failure mode of the rebase, and the resolution is to re-express the same bulk copy, never to drop
  the patch. Note that resolving a conflict *changes the patch-id*: record the new one here.

## 4. Build integration

`cmake/SetupGLFW.cmake` adds the submodule with `add_subdirectory(... EXCLUDE_FROM_ALL)`, forces
`GLFW_BUILD_EXAMPLES/TESTS/DOCS` and `GLFW_INSTALL` off, exposes the include directory as `SYSTEM`,
and defines `GLFW_INCLUDE_VULKAN` **publicly** so every TU pulling `<GLFW/glfw3.h>` sees the Vulkan
entry points.

Nothing in the engine's own CMake lists GLFW sources, so a GLFW file rename is handled by GLFW's own
CMakeLists — 3.5 renamed `src/cocoa_time.c/.h` → `src/macos_time.c/.h` and deleted
`src/xkb_unicode.h`, and the engine never noticed. **This particular pin change renames nothing**
(the 3.4-108 state already had 3.5's layout), but a pin change that touches GLFW's `CMakeLists.txt`
— this one does, for the version bump — makes CMake re-run configure on the next build. Let it, or
reconfigure explicitly after a pin change.
