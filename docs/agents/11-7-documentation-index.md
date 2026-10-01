## 7. Documentation Index

-   **AI Runtime Control:** [`docs/ai-runtime-control.md`](../ai-runtime-control.md) (**AI operator guide** — resource discovery, scene creation, command reference).
-   **Open work:** [`docs/todo/`](../todo) (one file per idea to do; the rule is in [`docs/todo/README.md`](../todo/README.md) — done means the file is DELETED).
-   **Philosophy:** [`docs/architecture-philosophy.md`](../architecture-philosophy.md) (Deep dive).
-   **Tracer:** [`docs/tracer-system.md`](../tracer-system.md) (Logging rules).
-   **Conventions:** [`docs/cpp-conventions.md`](../cpp-conventions.md) (Includes AI-friendly guidelines).
-   **Physics:** [`docs/physics-system.md`](../physics-system.md); the overhaul under way (2026-10-01): [`docs/physics-overhaul.md`](../physics-overhaul.md).
-   **Resources:** [`docs/resource-management.md`](../resource-management.md).
-   **Coordinates:** [`docs/coordinate-system.md`](../coordinate-system.md) (Y-UP convention — absolute law).
-   **Graphics Hub:** [`docs/graphics-system.md`](../graphics-system.md) (High-level rendering architecture).
-   **Scene Graph:** [`docs/scene-graph-architecture.md`](../scene-graph-architecture.md) (Entity-Component hierarchy).
-   **Multi-Scene Resource Ownership:** [`docs/multi-scene-resource-ownership.md`](../multi-scene-resource-ownership.md) (**Code-generation doctrine** — who owns what across scene load/switch/delete; read before writing resource code).
-   **Shadow Mapping:** [`docs/shadow-mapping.md`](../shadow-mapping.md) (PCF, color projection, render pass types).
-   **Reflection Pipeline:** [`docs/reflection-pipeline.md`](../reflection-pipeline.md) (**the seven reflection paths and how they arbitrate** — reflectivity nibble, normals-buffer alpha packing, `mix()` composite, skinned-geometry BLAS refit. Read before touching SSR/RTR/IBL, any `Reflection` material component, or anything skinned that must appear in ray-traced effects).
-   **Post-Processing Pipeline:** [`docs/post-processing-pipeline.md`](../post-processing-pipeline.md) (frame structure, grab-pass batched-barrier contract, the three swap-chain render passes, the pass-merging roadmap — and § 4b, the **MEASURED per-pass GPU cost**, which is what settles where the frame time actually goes: ~70 % of it is one ray trace, and pass merging is finished as a lever. Read before touching `PostProcessor`, `GrabPass`, any `Effects/{Lighting,Atmosphere,Resolve,Camera,Style}/*` effect or the swap-chain passes, and before proposing any optimization — the numbers are there).
-   **Pipeline Caching:** [`docs/pipeline-caching-system.md`](../pipeline-caching-system.md) (Critical for render pass compatibility).
-   **Animation Retargeting:** [`docs/animation-retargeting.md`](../animation-retargeting.md) (playing a clip authored for ANOTHER skeleton — the mathematics, a measured worked example, and the naming traps that break legs in silence. **The engine cannot do this today.** Read before touching any cross-skeleton animation or mocap import).
-   **Text-to-Motion (Kimodo):** [`docs/text-to-motion-kimodo.md`](../text-to-motion-kimodo.md) (evaluation of an AI motion source: licences, measured cost, and what it cannot do).
-   **Runtime Session:** [`docs/runtime-session.md`](../runtime-session.md) (Launch, connect, interact with a running instance).
-   **Toolkit:** [`docs/toolkit-system.md`](../toolkit-system.md) (Scene construction helper — the fast way to build scenes vs manual Scene API).
-   **Scene Loaders & OpenUSD:** [`docs/scene-loaders-usd.md`](../scene-loaders-usd.md) (**design; USD not yet implemented** — the `SceneData` extension to lights/cameras/instancers, tinyusdz, and the **absorption rule**: nothing USD survives `load()`, a missing capability is added to `Scenes`. Also defines the Intel Jungle Ruins scene as the engine's **GOLD GOAL** — the owner's *Saint Graal*, the scene whose completion says the runtime has arrived, and the benchmark it is measured against until then. Read before touching any loader or `SceneData`).
-   **Windows Export API:** [`docs/windows-export-api.md`](../windows-export-api.md) (`EMEN_LEAN_API` / `EMEN_API` — required on MSVC; **read § 2 before annotating**: which of the two macros a class gets decides whether a consumer links on Windows).
-   **GLFW Fork:** [`docs/glfw-fork.md`](../glfw-fork.md) (`dependencies/glfw` is the fork **EmeraudeEngine/glfw**, not upstream — one patch (`glfwGetKeyboardState()` / `glfwGetMouseButtonState()`, consumed behind `GLFW_EM_CUSTOM_VERSION`) and the `update-glfw.py` procedure that carries it forward. ⚠️ An unpatched GLFW still compiles through the `#else` fallback, so **a successful build is not proof the patch survived** — compare patch-ids, never SHAs).

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
