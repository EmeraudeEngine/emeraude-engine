# Emeraude Engine - AI Context

## 1. Context

**Core Framework**: Modern C++20 Vulkan 3D Engine.
**Coordinates**: **Right-Handed Y-DOWN** (Consistent across Physics, Rendering, Audio).
**Platform**: Windows 11, Linux (Debian/Ubuntu), macOS.
**Graphics API**: **Vulkan-only** — no D3D11, no D3D12, no Metal, no OpenGL. Single backend, mastered in full.

## 1b. Vision

### Unreal Engine 5 Runtime Killer

Emeraude Engine targets **surpassing Unreal Engine 5 runtime visual quality**. This is not
about the ecosystem (no editor, no blueprint system, no marketplace). The target is the
**runtime** — the core that runs on every end-user machine.

UE5's runtime consists of: renderer (Nanite, Lumen, virtual shadow maps, TSR), post-process
stack, material system, audio engine, physics. That is our battleground. If the developer
using emeraude-engine has to work harder on tooling — that is an acceptable trade-off. The
runtime output quality is the non-negotiable imperative.

### Strategic Positioning

"UE5 Runtime Killer" is a **direction**, not a frontal war. The real goal: give indie
developers a **genuine choice**. Today, a dev who wants AAA-quality rendering is locked
into UE5's licensing (5% royalties, Epic controls the terms) or Unity's unpredictable
pricing. Emeraude Engine offers a third option: **professional-grade runtime, LGPLv3,
zero royalties, zero contractual dependency.**

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
> **Cross-platform tool (required on Windows):** Use `tools/remote-console.py` — works on Windows, Linux, and macOS:
> ```bash
> python3 tools/remote-console.py "COMMAND"
> ```
> On Linux/macOS, `nc` (netcat) also works:
> ```bash
> # Linux (use -q flag):
> echo "COMMAND" | nc -q 2 localhost 7777
>
> # macOS (use -w flag — macOS nc does not support -q):
> echo "COMMAND" | nc -w 2 localhost 7777
> ```
>
> **When the user asks "what's on screen?"** → take a screenshot:
> ```bash
> python3 tools/remote-console.py "Core.RendererService.screenshot()"
> ```
> Then **read the PNG file** to see the rendering output. You have eyes. Use them.
>
> **When you need to verify a rendering change** → screenshot before and after.
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
> The complete reference is in [`docs/ai-runtime-control.md`](docs/ai-runtime-control.md).
> **Read it before any runtime work.**

## 2. Architecture Map

| Category | System | Path | Context |
|---|---|---|---|
| **Framework** | Core / Tracer | [`src/AGENTS.md`](src/AGENTS.md) | App lifecycle, Logging. |
| | Libs (Math/Utils) | [`src/Libs/AGENTS.md`](src/Libs/AGENTS.md) | Foundational types. |
| | Libs/Animation | [`src/Libs/AGENTS.md`](src/Libs/AGENTS.md) | Skeletal data types (Joint, Skeleton, AnimationClip, Skin). |
| | Platform | [`src/PlatformSpecific/AGENTS.md`](src/PlatformSpecific/AGENTS.md) | OS implementations. |
| | Testing | [`src/Testing/AGENTS.md`](src/Testing/AGENTS.md) | Unit tests. |
| **Graphics** | **Graphics Layer** | [`src/Graphics/AGENTS.md`](src/Graphics/AGENTS.md) | **Start Here**. High-level. |
| | Vulkan Layer | [`src/Vulkan/AGENTS.md`](src/Vulkan/AGENTS.md) | Low-level abstraction. |
| | Saphir (Shader) | [`src/Saphir/AGENTS.md`](src/Saphir/AGENTS.md) | Shader generation. |
| **Sim** | **Physics** | [`src/Physics/AGENTS.md`](src/Physics/AGENTS.md) | Physics system. |
| | Audio | [`src/Audio/AGENTS.md`](src/Audio/AGENTS.md) | OpenAL spatial audio. |
| | Input | [`src/Input/AGENTS.md`](src/Input/AGENTS.md) | Keyboard/Mouse/Pad. |
| **Data** | Resources | [`src/Resources/AGENTS.md`](src/Resources/AGENTS.md) | Async loading. |
| | AssetLoaders | [`src/AssetLoaders/AGENTS.md`](src/AssetLoaders/AGENTS.md) | Composite format loaders (glTF, FBX stub). |
| | Scenes | [`src/Scenes/AGENTS.md`](src/Scenes/AGENTS.md) | Scene graph. |
| | Animations | [`src/Animations/AGENTS.md`](src/Animations/AGENTS.md) | *In Dev*. Data types in `Libs/Animation/`, runtime eval here. |
| **Tools/UI** | Overlay (ImGui) | [`src/Overlay/AGENTS.md`](src/Overlay/AGENTS.md) | UI & Debug. |
| | Console | [`src/Console/AGENTS.md`](src/Console/AGENTS.md) | Runtime control (TCP + in-game). |
| | AVConsole | [`src/Scenes/AVConsole/AGENTS.md`](src/Scenes/AVConsole/AGENTS.md) | Virtual devices. |
| | Scene Editor | [`src/Scenes/Editor/AGENTS.md`](src/Scenes/Editor/AGENTS.md) | Picking, gizmos, entity manipulation. |
| | Tool | [`src/Tool/AGENTS.md`](src/Tool/AGENTS.md) | Editor tools. |
| **Net** | Networking | [`src/Net/AGENTS.md`](src/Net/AGENTS.md) | HTTP/Download, UDP/SSDP, Serial, WiFi. |

## 3. Core Axioms

### Design
1.  **POLA:** Principle of Least Astonishment. APIs must be predictable.
2.  **Pit of Success:** Make the right way easier than the wrong way (e.g. Fail-safe resources).
3.  **No Gulf:** Avoid complexity in user-facing APIs.

### Constraints
1.  **Coordinates:** **Y-DOWN** is absolute law. Gravity is `+Y`.
2.  **Vulkan:** NEVER call Vulkan directly. Use `Graphics/` abstractions.
3.  **Memory:** Use **VMA** for GPU. Use **RAII** for CPU. No raw pointers.
4.  **Frame Sync:** Any GPU buffer (SSBO/UBO) updated per-frame **MUST** be double-buffered (one per frame-in-flight). See [`src/Scenes/AGENTS.md`](src/Scenes/AGENTS.md) → Frame Synchronization.
5.  **sRGB Convention:** Material variable names ending with `Color` trigger `VK_FORMAT_*_SRGB`. Data textures (normals, roughness) use `_UNORM`. Enforced in `Material::Component::Texture` constructors. See `Material/Component/Texture.hpp`.
6.  **BC7 Compression:** All `Texture2D` resources are BC7-compressed at load time when `textureCompressionBC` is available. Compressed data cached on disk (`TextureCache`). Format: `VK_FORMAT_BC7_SRGB_BLOCK` or `VK_FORMAT_BC7_UNORM_BLOCK` based on sRGB flag.
7.  **Image Format Awareness:** When adding new `VkFormat` variants, update `Image::pixelBytes()` AND `Image::colorCount()` — missing cases return 0, breaking multi-layer uploads silently.

## 3b. Licensing

**License:** LGPLv3 (GNU Lesser General Public License v3).

1. **No license violation:** Never integrate code that would violate LGPLv3 compatibility.
   Check the license of any external code before integration.
2. **Citation required:** All code contributions from open-source projects must be cited
   with author name, original license, and source URL in a comment at the point of use.
3. **AI-generated code:** When adapting algorithms from published papers or open-source
   implementations, cite the original source (paper DOI, repository URL, author name).

## 4. Platform-Specific Recommendations

### Linux/NVIDIA/X11 (GNOME, KDE)

**Issue:** VSync causes micro-stuttering due to double synchronization (driver + compositor).

**Solution:**
```
Core/Video/EnableVSync = false
Core/Video/EnableTripleBuffering = true (optional)
Core/Video/FrameRateLimit = 60  (or monitor refresh rate)
```

The compositor handles display sync; the app limits its frame rate to avoid wasting resources.

**Details:** See [`src/Vulkan/AGENTS.md`](src/Vulkan/AGENTS.md) (Present Mode Selection) and [`src/Graphics/AGENTS.md`](src/Graphics/AGENTS.md) (Frame Rate Limiter).

## 5. AI-Friendly Codebase

This engine is developed **with AI and for AI** — AI is not a helper, it is the primary implementor.

**Core principle:** When an AI identifies an unclear concept or confusing interface, it should **STRONGLY SUGGEST refactoring it**. An unclear interface that causes bugs once will cause bugs again.

**AI diagnostic integration:**
- **RenderDoc in-application API** — programmatic GPU frame capture (`EMERAUDE_ENABLE_RENDERDOC=ON`)
- **RenderDoc Python module** — autonomous .rdc analysis (draw calls, render passes, vertex throughput)
- **Remote Console screenshot** — `Core.RendererService.screenshot()` via TCP console for visual regression testing
- **RenderDoc CLI capture** — `renderdoccmd capture` for pipeline regression testing

See [`docs/cpp-conventions.md#ai-friendly-code-guidelines`](docs/cpp-conventions.md#ai-friendly-code-guidelines) for detailed guidelines.

## 6. AI Runtime Control

The engine exposes a **Remote Console** (TCP port 7777) that enables AI agents to control
a running application autonomously. This is the foundation for AI-driven 3D content creation.

**Capabilities:**
- **Resource discovery** — query available skyboxes, meshes, materials
- **Scene creation** — via live commands or JSON scene descriptions
- **Camera control** — position, orientation, visual verification via screenshots
- **Audio control** — full TrackMixer playback management
- **Settings/window** — runtime configuration changes

**Reference:** [`docs/ai-runtime-control.md`](docs/ai-runtime-control.md) — **THE complete guide** for
any AI operating the engine at runtime. Contains the full command reference, JSON scene format
specification, spatial orientation tutorial, and visual feedback loop methodology.

**Console system:** [`src/Console/AGENTS.md`](src/Console/AGENTS.md) — Architecture, command flow,
how to add new commands.

## 7. Documentation Index

-   **AI Runtime Control:** [`docs/ai-runtime-control.md`](docs/ai-runtime-control.md) (**AI operator guide** — resource discovery, scene creation, command reference).
-   **Philosophy:** [`docs/architecture-philosophy.md`](docs/architecture-philosophy.md) (Deep dive).
-   **Tracer:** [`docs/tracer-system.md`](docs/tracer-system.md) (Logging rules).
-   **Conventions:** [`docs/cpp-conventions.md`](docs/cpp-conventions.md) (Includes AI-friendly guidelines).
-   **Physics:** [`docs/physics-system.md`](docs/physics-system.md).
-   **Resources:** [`docs/resource-management.md`](docs/resource-management.md).
-   **Coordinates:** [`docs/coordinate-system.md`](docs/coordinate-system.md) (Y-DOWN convention — absolute law).
-   **Graphics Hub:** [`docs/graphics-system.md`](docs/graphics-system.md) (High-level rendering architecture).
-   **Scene Graph:** [`docs/scene-graph-architecture.md`](docs/scene-graph-architecture.md) (Entity-Component hierarchy).
-   **Shadow Mapping:** [`docs/shadow-mapping.md`](docs/shadow-mapping.md) (PCF, color projection, render pass types).
-   **Pipeline Caching:** [`docs/pipeline-caching-system.md`](docs/pipeline-caching-system.md) (Critical for render pass compatibility).
-   **Runtime Session:** [`docs/runtime-session.md`](docs/runtime-session.md) (Launch, connect, interact with a running instance).
-   **Toolkit:** [`docs/toolkit-system.md`](docs/toolkit-system.md) (Scene construction helper — the fast way to build scenes vs manual Scene API).

> [!CRITICAL]
> **Maintenance:** AI documentation is **MORE IMPORTANT than the code itself.** A code change
> without its corresponding documentation update is an **incomplete delivery**. After every
> modification (code, architecture, feature, bugfix, refactor), the AI **MUST**:
> 1. Update all affected `AGENTS.md` files (subsystem context, architecture maps, patterns).
> 2. Update affected `docs/` files (patterns, caution points, troubleshooting).
> 3. **Explicitly signal to the user** which documentation was updated and why.
> 4. If unsure which docs are affected, ask — never silently skip.
>
> Undocumented code is **technical debt that compounds**. The AI documentation network is
> the engine's institutional memory — without it, every future AI session starts blind.
> Run `/update-docs` when available, but **do not rely on it as a substitute for inline updates.**

## 8. Code Generation Directives

> [!CRITICAL]
> These rules are **NON-NEGOTIABLE** for any AI-generated code in this engine.

### Professional Standard
This engine targets **surpassing Unreal Engine 5 runtime visual quality** as its benchmark.
Every generated implementation must meet professional-grade quality. No shortcuts,
no "good enough" approximations, no deferred-quality patterns. The AI is the primary
implementor — the human architect directs, the AI executes and measures.

### Architectural Contract Compliance
All generated code must comply with established contracts:
- **POLA / Pit of Success / No Gulf** design axioms
- **Service interfaces** for engine subsystems
- **Observer/Observable** for loose coupling
- **RAII** for all resource lifetimes
- **Vulkan abstraction** (never direct Vulkan calls)

When a contract is missing or insufficient, **the contract must be created or
improved** as part of the implementation — not bypassed.

### No Workarounds — Industrial-Grade Integration Only
Emeraude Engine is held to an **industrial standard**. It must be implementable
as a Khronos Group showcase application. This means **zero tolerance for
hacks, workarounds, or ad-hoc solutions**.

When integrating with professional tools (RenderDoc, Vulkan Validation Layers,
profilers, etc.), **use their official integration path** — Vulkan layers,
documented APIs, proper manifests. Never resort to `LD_PRELOAD` injection,
manual symbol lookups, environment variable tricks, or any approach that a
Khronos engineer would reject in a code review.

When something doesn't work, **diagnose the real cause** (missing layer
manifest, wrong library path, incorrect API usage) instead of piling on
workarounds. Read the tool's documentation and understand its architecture
before writing a single line of integration code.

### Mandatory Decision Escalation
When facing architectural choices (multiple valid approaches, unclear trade-offs,
or potential contract changes), **ALWAYS escalate to the user** with:
1. Clear description of each option
2. Constraints and trade-offs per option
3. Recommendation with justification

Never make autonomous architectural decisions. The project owner decides.

### Documentation First — No Silent Changes

> [!CRITICAL]
> **AI documentation maintenance is MORE IMPORTANT than the code.**

Every code change **MUST** be accompanied by its documentation update **in the same
work session**. This is not optional, not "nice to have" — it is the **highest priority
deliverable**.

**The rule:**
- Before reporting a task as complete, verify that all affected `AGENTS.md` and `docs/`
  files reflect the change.
- **Explicitly tell the user** what documentation was updated: file paths, what changed, why.
- If a change affects the Architecture Map, Link Index, or cross-references — update them.
- If a new subsystem, pattern, or API is introduced — document it immediately.
- If an existing pattern changes — update every doc that references the old pattern.
- **Never assume** the user will remember to ask for doc updates. Signal proactively.

**Active enforcement — the AI MUST remind the user:**
If the user tries to move on to the next task without documentation being updated,
the AI **MUST push back** and insist. The argument: *"Without this doc update, the
next AI session will misunderstand this area of the project, make wrong assumptions,
and waste your time. Updating now takes 2 minutes. Reverse-engineering later costs hours."*
Do not be passive about this. Actively remind, actively propose doc updates, actively
flag when documentation is stale or missing after a change.

**Why this is non-negotiable:**
The `AGENTS.md` network is the **only persistent context** an AI has across sessions.
Code without documentation forces every future AI session to reverse-engineer intent,
architecture, and constraints from scratch. This wastes the user's time, introduces
regression risk, and degrades the quality of AI assistance over time. The documentation
IS the engine's memory.

## Link Index

All outbound references from this file, grouped by type.

### AGENTS.md — Subsystems

| System | Path |
|--------|------|
| Core / Tracer | [`src/AGENTS.md`](src/AGENTS.md) |
| Libs (Math/Utils) | [`src/Libs/AGENTS.md`](src/Libs/AGENTS.md) |
| Platform | [`src/PlatformSpecific/AGENTS.md`](src/PlatformSpecific/AGENTS.md) |
| Testing | [`src/Testing/AGENTS.md`](src/Testing/AGENTS.md) |
| Graphics Layer | [`src/Graphics/AGENTS.md`](src/Graphics/AGENTS.md) |
| Vulkan Layer | [`src/Vulkan/AGENTS.md`](src/Vulkan/AGENTS.md) |
| Saphir (Shader) | [`src/Saphir/AGENTS.md`](src/Saphir/AGENTS.md) |
| Physics | [`src/Physics/AGENTS.md`](src/Physics/AGENTS.md) |
| Audio | [`src/Audio/AGENTS.md`](src/Audio/AGENTS.md) |
| Input | [`src/Input/AGENTS.md`](src/Input/AGENTS.md) |
| Resources | [`src/Resources/AGENTS.md`](src/Resources/AGENTS.md) |
| AssetLoaders | [`src/AssetLoaders/AGENTS.md`](src/AssetLoaders/AGENTS.md) |
| Scenes | [`src/Scenes/AGENTS.md`](src/Scenes/AGENTS.md) |
| Animations | [`src/Animations/AGENTS.md`](src/Animations/AGENTS.md) |
| Overlay (ImGui) | [`src/Overlay/AGENTS.md`](src/Overlay/AGENTS.md) |
| Console | [`src/Console/AGENTS.md`](src/Console/AGENTS.md) |
| AVConsole | [`src/Scenes/AVConsole/AGENTS.md`](src/Scenes/AVConsole/AGENTS.md) |
| Scene Editor | [`src/Scenes/Editor/AGENTS.md`](src/Scenes/Editor/AGENTS.md) |
| Tool | [`src/Tool/AGENTS.md`](src/Tool/AGENTS.md) |
| Networking | [`src/Net/AGENTS.md`](src/Net/AGENTS.md) |

### Documentation

| Document | Path |
|----------|------|
| AI Runtime Control | [`docs/ai-runtime-control.md`](docs/ai-runtime-control.md) |
| Architecture Philosophy | [`docs/architecture-philosophy.md`](docs/architecture-philosophy.md) |
| Tracer System | [`docs/tracer-system.md`](docs/tracer-system.md) |
| C++ Conventions | [`docs/cpp-conventions.md`](docs/cpp-conventions.md) |
| Coordinate System | [`docs/coordinate-system.md`](docs/coordinate-system.md) |
| Graphics System | [`docs/graphics-system.md`](docs/graphics-system.md) |
| Scene Graph Architecture | [`docs/scene-graph-architecture.md`](docs/scene-graph-architecture.md) |
| Shadow Mapping | [`docs/shadow-mapping.md`](docs/shadow-mapping.md) |
| Physics System | [`docs/physics-system.md`](docs/physics-system.md) |
| Resource Management | [`docs/resource-management.md`](docs/resource-management.md) |
| Runtime Session | [`docs/runtime-session.md`](docs/runtime-session.md) |
| Pipeline Caching | [`docs/pipeline-caching-system.md`](docs/pipeline-caching-system.md) |
| Toolkit System | [`docs/toolkit-system.md`](docs/toolkit-system.md) |