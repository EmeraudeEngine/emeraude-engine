## 5. AI-Friendly Codebase

This engine is developed **with AI and for AI** — AI is not a helper, it is the primary implementor.

**Core principle:** When an AI identifies an unclear concept or confusing interface, it should **STRONGLY SUGGEST refactoring it**. An unclear interface that causes bugs once will cause bugs again.

**AI diagnostic integration:**
- **RenderDoc in-application API** — programmatic GPU frame capture (`EMERAUDE_ENABLE_RENDERDOC=ON`). ⚠️ RenderDoc is **not a submodule** (Sep 2026): `cmake/SetupRenderDoc.cmake` fetches the sources (`FetchContent`, pinned in `cmake/RenderDocPin.cmake`) **only** when the option is On, and reuses an existing `dependencies/renderdoc` checkout instead of downloading again
- **RenderDoc Python module** — autonomous .rdc analysis (draw calls, render passes, vertex throughput). `cmake -P cmake/BuildRenderDocPython.cmake` clones the pinned revision on demand (the full tree is needed, the header alone cannot build it) and produces `dependencies/renderdoc/build/lib/renderdoc.so`
- **Remote Console screenshot** — `Core.RendererService.screenshot()` via TCP console for visual regression testing
- **RenderDoc CLI capture** — `renderdoccmd capture` for pipeline regression testing
- **glTF light inventory** — `tools/gltf-lights.py FILE.glb` lists the `KHR_lights_punctual` lights (type, intensity, colour, carrying node) and `--set NAME=VALUE` writes an intensity into the asset (GLB streamed): exporters ship lights at 0 (Intel Sponza, all 24), and the asset — not a demo — is where the value belongs
- **Sky manifest measurement** — `tools/sky-manifest.py sky.hdr --name X --sun-illuminance 100000` prints the `Cubemaps` and `Backgrounds` manifests of an equirectangular HDR from its texels (sun direction and energy profile, sky-only and total illuminance, `Luminance` anchored on the declared sun — or on the sky, `--sky-luminance`, when the sun is veiled): every number of a sky manifest is measured, none is guessed. `--locate NAME --store DIR --preview out.png` finds where the body of ANY store sky (packed cube or equirect, LDR or HDR) is painted and compares it with its manifest — LOOK at the preview
- **Wayland protocol trace** — `tools/wayland-protocol-trace.py` (Linux/Wayland): `--capture` reruns a command under `WAYLAND_DEBUG` until a compositor protocol error reproduces and keeps only the failing log; `--analyse` names the offending object, resolves the `wl_surface` it wraps, and prints the requests of the rejected commit with a reading. Use it whenever a window dies on its own or `VK_ERROR_SURFACE_LOST_KHR` shows up — the culprit is a request, not a GPU state

See [`docs/cpp-conventions.md#ai-friendly-code-guidelines`](../cpp-conventions.md#ai-friendly-code-guidelines) for detailed guidelines.
