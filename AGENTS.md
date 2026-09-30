# Emeraude Engine — AI router

C++20, **Vulkan-only** real-time 3D engine (LGPLv3), Linux / macOS / Windows. Built on the standalone
foundation library [emeraude-base](dependencies/emeraude-base/AGENTS.md) (`EmEn::Base`); tested through the
`projet-alpha` testbed. Target: production-grade real-time visual quality of the RUNTIME.

> ⚠️ **This file is a ROUTER (2026-09-27).** Everything it used to say was moved VERBATIM to `docs/agents/`
> and every subsystem `AGENTS.md` routes to `docs/subsystems/<area>/`, one file per topic. **Read only the
> file(s) your task needs.** New knowledge goes into the matching `docs/` file — never back into an
> `AGENTS.md`, which only grows by one table row when a docs file is added.

## Non-negotiable rules (detail in the linked file)

- **Co-development**: a lack in the foundation is fixed IN emeraude-base; the engine never works around it.
  → [`docs/agents/02-1a-foundation-emeraude-base.md`](docs/agents/02-1a-foundation-emeraude-base.md)
- **Y-UP right-handed** (`+X` right, `+Y` up, `-Z` forward); imports are the identity. Never call Vulkan
  directly (use `Graphics/`); VMA for GPU memory, RAII everywhere; per-frame GPU buffers are one per frame in
  flight; `…Color` material names are sRGB; normal maps follow the Khronos convention; `LC_NUMERIC` is `"C"`;
  a resource shared across scenes is owned once at device level.
  → [`docs/agents/05-3-core-axioms.md`](docs/agents/05-3-core-axioms.md)
- **Industrial quality, no workaround, escalate architectural choices to the owner, documentation in the same
  session and signalled to the user.** → [`docs/agents/12-8-code-generation-directives.md`](docs/agents/12-8-code-generation-directives.md)
- **LGPLv3**: never integrate incompatible code; cite every open-source contribution.
  → [`docs/agents/06-3b-licensing.md`](docs/agents/06-3b-licensing.md)
- **Open work** lives in `docs/todo/`, one file per idea, deleted when done.
  → [`docs/todo/README.md`](docs/todo/README.md)
- C++ conventions (tabs, braces, `m_` members, UPPERCASE acronyms, no exceptions):
  [`docs/cpp-conventions.md`](docs/cpp-conventions.md). Pitfalls by area: [`docs/caution-points.md`](docs/caution-points.md)
  (large: search it, do not read it whole).

## Where to read — by task

| Task touches | Router / file |
|---|---|
| Rendering, materials, post-process, lighting lanes, RT effects | [`src/Graphics/AGENTS.md`](src/Graphics/AGENTS.md) |
| Vulkan objects, memory, sync | [`src/Vulkan/AGENTS.md`](src/Vulkan/AGENTS.md) |
| Generated shaders (Saphir) | [`src/Saphir/AGENTS.md`](src/Saphir/AGENTS.md) |
| Scene graph, components, lights, console adapters | [`src/Scenes/AGENTS.md`](src/Scenes/AGENTS.md) |
| glTF / FBX / USD / WAD loaders | [`src/Scenes/Loaders/AGENTS.md`](src/Scenes/Loaders/AGENTS.md) |
| Editor, AV console (virtual devices) | [`src/Scenes/Editor/AGENTS.md`](src/Scenes/Editor/AGENTS.md), [`src/Scenes/AVConsole/AGENTS.md`](src/Scenes/AVConsole/AGENTS.md) |
| Physics / Audio / Input / Animations | [`src/Physics/AGENTS.md`](src/Physics/AGENTS.md), [`src/Audio/AGENTS.md`](src/Audio/AGENTS.md), [`src/Input/AGENTS.md`](src/Input/AGENTS.md), [`src/Animations/AGENTS.md`](src/Animations/AGENTS.md) |
| Resources (async loading, stores) | [`src/Resources/AGENTS.md`](src/Resources/AGENTS.md) |
| Console, MCP server, remote control | [`src/Console/AGENTS.md`](src/Console/AGENTS.md), operator view [`docs/ai-runtime-control.md`](docs/ai-runtime-control.md) |
| Networking (HTTPS, UDP, TCP, serial) | [`src/Net/AGENTS.md`](src/Net/AGENTS.md) |
| Overlay (ImGui), Tool | [`src/Overlay/AGENTS.md`](src/Overlay/AGENTS.md), [`src/Tool/AGENTS.md`](src/Tool/AGENTS.md) |
| OS-specific code | [`src/PlatformSpecific/AGENTS.md`](src/PlatformSpecific/AGENTS.md) |
| Core lifecycle, tracer, source tree | [`src/AGENTS.md`](src/AGENTS.md) |
| clang-tidy results and on-purpose findings (zero NEW finding per change) | [`docs/clang-tidy-ledger.md`](docs/clang-tidy-ledger.md) |
| Foundation (math, image/audio/mesh factories, I/O, unit tests) | [`dependencies/emeraude-base/AGENTS.md`](dependencies/emeraude-base/AGENTS.md) |
| Every other topic document (coordinate system, pipeline cache, exports, PCH…) | [`docs/agents/11-7-documentation-index.md`](docs/agents/11-7-documentation-index.md) |

## The former content of this file (`docs/agents/`)

| Section | File |
|---|---|
| Context (coordinates, platform, API) | [`01-1-context.md`](docs/agents/01-1-context.md) |
| Foundation: emeraude-base (build, PCH, exports, symbol hiding) | [`02-1a-foundation-emeraude-base.md`](docs/agents/02-1a-foundation-emeraude-base.md) |
| Vision | [`03-1b-vision.md`](docs/agents/03-1b-vision.md) |
| Architecture map | [`04-2-architecture-map.md`](docs/agents/04-2-architecture-map.md) |
| Core axioms | [`05-3-core-axioms.md`](docs/agents/05-3-core-axioms.md) |
| Licensing | [`06-3b-licensing.md`](docs/agents/06-3b-licensing.md) |
| Open work rule | [`07-3c-open-work-docs-todo-one-file-per-idea.md`](docs/agents/07-3c-open-work-docs-todo-one-file-per-idea.md) |
| Platform-specific recommendations | [`08-4-platform-specific-recommendations.md`](docs/agents/08-4-platform-specific-recommendations.md) |
| AI-friendly codebase | [`09-5-ai-friendly-codebase.md`](docs/agents/09-5-ai-friendly-codebase.md) |
| AI runtime control | [`10-6-ai-runtime-control.md`](docs/agents/10-6-ai-runtime-control.md) |
| Documentation index | [`11-7-documentation-index.md`](docs/agents/11-7-documentation-index.md) |
| Code generation directives | [`12-8-code-generation-directives.md`](docs/agents/12-8-code-generation-directives.md) |
| Link index | [`13-link-index.md`](docs/agents/13-link-index.md) |
