## 1b. Vision

### Production-Grade Real-Time Runtime

Emeraude Engine targets **production-grade real-time visual quality**. This is not
about the ecosystem (no editor, no blueprint system, no marketplace). The target is the
**runtime** — the core that runs on every end-user machine.

Scope: modern PBR renderer, ray-traced effects, virtual shadow maps, temporal upscaling,
full post-process stack, material system, audio engine, physics. If the developer
using emeraude-engine has to work harder on tooling — that is an acceptable trade-off.
The runtime output quality is the non-negotiable imperative.

### Positioning

Emeraude Engine offers developers a real path to production-quality real-time rendering with
**full source access, LGPLv3, zero royalties, zero contractual dependency.**

The trade-off is honest: less tooling, more code. The target audience is developers who
**want** to write C++ and understand their engine — not drag-and-drop users. Those who
accept this trade-off get a runtime that owes nothing to anyone.

### Open-Core Business Model

The engine follows an **open-core** model:
- **Runtime (emeraude-engine):** LGPLv3, free forever. The community's property.
- **Studio tooling (proprietary):** Scene editors, asset pipelines, productivity
  wrappers — paid products built on top of the free runtime.

The distinction is critical: **paid tools add convenience, never functionality.** A
developer who never pays has the exact same runtime capabilities. No feature gating,
no crippled free tier, no bait-and-switch. This model funds continued development of
the free runtime while keeping it permanently open.

### Vulkan-Only by Design

Emeraude Engine is **Vulkan-only**. This is a deliberate architectural decision, not a limitation.

UE5 maintains D3D11, D3D12, Vulkan, Metal — each backend is a compromise. They design
abstractions to the lowest common denominator. Emeraude Engine speaks directly to the GPU
through Vulkan, the open standard maintained by the **Khronos Group**. No abstraction layer,
no backend switching, no compromise.

This means:
- **Zero backend abstraction overhead** — the code talks directly to the GPU
- **Vulkan render passes, subpasses, layout transitions** are first-class citizens, not wrapped
- **Explicit synchronization** — full control over GPU scheduling
- **One target to optimize** — every microsecond gained benefits 100% of users
- **Khronos Group standard** — industrial-grade, cross-platform, vendor-neutral

Vulkan runs everywhere: Linux, Windows, Android, macOS/iOS via MoltenVK. The engine must be
implementable as a **Khronos Group showcase application**.

### AI-Driven Development

Emeraude Engine is developed **with AI and for AI**. The human is the **architect and director**.
The AI is the **implementor and analyst**.

This means:
- **The codebase must be AI-readable** — clear naming, consistent patterns, documented contracts
- **AI diagnostic tools are first-class** — RenderDoc integration, programmatic GPU analysis,
  automated visual regression testing are part of the engine, not external afterthoughts
- **Every rendering decision must be measurable** — frame capture, draw call counts, render pass
  structure, vertex throughput. No blind optimization, no guesswork.
- **The AI must be able to autonomously diagnose rendering issues** — capture a frame, analyze
  the GPU pipeline, identify bottlenecks, and propose solutions backed by data

This is a new model of engine development where the human defines the vision and architecture,
and the AI executes, measures, and iterates at industrial speed.

### AI Runtime Control — GOLD RULE

> [!CRITICAL]
> **The engine has a Remote Console (TCP port 7777).** Any AI working on this project
> **MUST** use it. This is not optional — it is the primary tool for understanding what
> the engine is doing at runtime.
>
> ⚠️⚠️ **THE PORT IS CLOSED BY DEFAULT (since 2026-08-27).** The listener starts only when the
> setting **`Core/Console/EnableRemoteListener` is `true`** in the application's `settings.json`,
> and it then binds to **`Core/Console/RemoteListenerAddress`** (default `127.0.0.1`, loopback
> only). A `Connection refused` on 7777 means the instance was launched without the key —
> **enable it and relaunch; do NOT keep retrying, polling or waiting on a running instance that
> never opened the port.** The engine logs `Remote console disabled (Core/Console/EnableRemoteListener = false)`
> at startup in that case. A human at the keyboard can open it live with **Shift+F10** (dialog asking
> the port, session only) — an AI cannot press that key, so it edits the setting and relaunches. Rationale: the console is an unauthenticated command channel
> (`Core.quit()`, settings, scenes, screenshots) and downstream applications are shipped to end users.
>
> **Cross-platform tool (required on Windows):** Use `tools/remote-console.py` — works on Windows, Linux, and macOS:
> ```bash
> python3 tools/remote-console.py "COMMAND"
> ```
> It prints the text of the answer and exits with status 1 when the command failed (`--json` prints
> the raw line). **Wire format (2026-09-27): every request gets exactly ONE response, one JSON object on
> one line** (`{"ok":…,"outputs":[{"severity","kind","message"}]}`), so `nc` still works but shows raw
> JSON. `describeCommands()` lists every command with its typed parameters, and
> `tools/console-conformance.py` checks the whole contract against a live instance.
> Details: [`docs/ai-runtime-control.md`](../ai-runtime-control.md) § Wire format.
>
> **MCP server (2026-09-27)** — the same typed commands as Model Context Protocol tools, over
> Streamable HTTP, closed by default: `Core/MCP/Enabled = true` (loopback `127.0.0.1:17778`), then
> `claude mcp add --transport http emeraude http://127.0.0.1:17778/mcp`. Both protocol eras
> (2026-07-28 and 2025-11-25), `Renderer_screenshot` returns an inline image, `list_changed` follows the
> active scene; `tools/mcp-conformance.py` checks it. Details: [`docs/ai-runtime-control.md`](../ai-runtime-control.md)
> § The MCP server, [`src/Console/AGENTS.md`](../../src/Console/AGENTS.md) § 7b.
>
> **When the user asks "what's on screen?"** → take a screenshot:
> ```bash
> python3 tools/remote-console.py "Core.RendererService.screenshot()"
> ```
> Then **read the PNG file** to see the rendering output. You have eyes. Use them.
>
> **When you need to verify a rendering change** → screenshot before and after — but ⚠️ **a raw
> `screenshot()` after a fixed `sleep` is NOT comparable between two runs.** A freshly loaded
> scene keeps moving for tens of seconds (exposure adaptation, TAA/SVGF accumulation, animation
> start-up), so two scripts whose delays differ compare two different moments and the gap reads
> exactly like a regression. Use the bench, which waits for the image to CONVERGE first:
> ```bash
> python3 tools/demo-capture-bench.py --exe ./projet-alpha --demo reflexion-debug \
>     --demo-options 0,6,0 --camera 0,1.6,4 --look-at 0,1,0 \
>     --crop 300,200,1700,1400 --out /tmp/after.png --control /tmp/after-control.png
> ```
> Measured on `reflexion-debug`: two runs agree to **0.229** mean luminance when matched by
> convergence, against **7.171** when one captures at t~4s and the other at t~8s.
> `--control` takes a second capture of the SAME run: its difference to `--out` is the noise
> floor of any pixel diff you draw from that scene. **Shoot it before reading a diff** — on a
> scene with an animated subject the floor can reach 59 % of pixels, and without it that noise
> reads as a broken render.
>
> **When you need to understand the scene** → query it:
> ```bash
> python3 tools/remote-console.py "Core.SceneManagerService.getSceneInfo()"
> ```
>
> **When you need to know what resources exist** → ask the engine:
> ```bash
> python3 tools/remote-console.py "Core.ResourcesManagerService.listResources(MeshResource)"
> ```
>
> **The AI can inject keyboard and mouse events** — interact with the running app like a user:
> ```bash
> # Inject Shift+F3 key press (key=292, modifiers=1=Shift)
> python3 tools/remote-console.py "Core.InputManagerService.keyPress(292, 1)"
>
> # Click at screen coordinates (x, y)
> python3 tools/remote-console.py "Core.InputManagerService.mouseClick(1920, 1000)"
>
> # Quit the application gracefully (Shift+Escape)
> python3 tools/remote-console.py "Core.InputManagerService.keyPress(256, 1)"
> ```
>
> **The AI interaction loop**: Screenshot → Analyze → Inject input → Screenshot → Verify.
> This gives full autonomy: see the app, click on things, verify results, iterate.
>
> **The AI can create 3D scenes autonomously** — via live commands or JSON:
> ```bash
> python3 tools/remote-console.py '{"Name":"Scene","Boundary":512.0,"Background":{"Type":"SkyBox","Resource":"Miramar"},...}'
> ```
>
> The complete reference is in [`docs/ai-runtime-control.md`](../ai-runtime-control.md).
> **Read it before any runtime work.**
