# Emeraude Engine - AI Context

## 1. Context

**Core Framework**: Modern C++20 Vulkan 3D Engine.
**Coordinates**: **Right-Handed Y-DOWN** (Consistent across Physics, Rendering, Audio).
**Platform**: Windows 11, Linux (Debian/Ubuntu), macOS.

## 2. Architecture Map

| Category | System | Path | Context |
|---|---|---|---|
| **Framework** | Core / Tracer | [`src/AGENTS.md`](src/AGENTS.md) | App lifecycle, Logging. |
| | Libs (Math/Utils) | [`src/Libs/AGENTS.md`](src/Libs/AGENTS.md) | Foundational types. |
| | Platform | [`src/PlatformSpecific/AGENTS.md`](src/PlatformSpecific/AGENTS.md) | OS implementations. |
| | Testing | [`src/Testing/AGENTS.md`](src/Testing/AGENTS.md) | Unit tests. |
| **Graphics** | **Graphics Layer** | [`src/Graphics/AGENTS.md`](src/Graphics/AGENTS.md) | **Start Here**. High-level. |
| | Vulkan Layer | [`src/Vulkan/AGENTS.md`](src/Vulkan/AGENTS.md) | Low-level abstraction. |
| | Saphir (Shader) | [`src/Saphir/AGENTS.md`](src/Saphir/AGENTS.md) | Shader generation. |
| **Sim** | **Physics** | [`src/Physics/AGENTS.md`](src/Physics/AGENTS.md) | Physics system. |
| | Audio | [`src/Audio/AGENTS.md`](src/Audio/AGENTS.md) | OpenAL spatial audio. |
| | Input | [`src/Input/AGENTS.md`](src/Input/AGENTS.md) | Keyboard/Mouse/Pad. |
| **Data** | Resources | [`src/Resources/AGENTS.md`](src/Resources/AGENTS.md) | Async loading. |
| | Scenes | [`src/Scenes/AGENTS.md`](src/Scenes/AGENTS.md) | Scene graph. |
| | Animations | [`src/Animations/AGENTS.md`](src/Animations/AGENTS.md) | *In Dev*. |
| **Tools/UI** | Overlay (ImGui) | [`src/Overlay/AGENTS.md`](src/Overlay/AGENTS.md) | UI & Debug. |
| | Console | [`src/Console/AGENTS.md`](src/Console/AGENTS.md) | In-game terminal. |
| | AVConsole | [`src/Scenes/AVConsole/AGENTS.md`](src/Scenes/AVConsole/AGENTS.md) | Virtual devices. |
| | Tool | [`src/Tool/AGENTS.md`](src/Tool/AGENTS.md) | Editor tools. |
| **Net** | Networking | [`src/Net/AGENTS.md`](src/Net/AGENTS.md) | HTTP/Download. |

## 3. Core Axioms

### Design
1.  **POLA:** Principle of Least Astonishment. APIs must be predictable.
2.  **Pit of Success:** Make the right way easier than the wrong way (e.g. Fail-safe resources).
3.  **No Gulf:** Avoid complexity in user-facing APIs.

### Constraints
1.  **Coordinates:** **Y-DOWN** is absolute law. Gravity is `+Y`.
2.  **Vulkan:** NEVER call Vulkan directly. Use `Graphics/` abstractions.
3.  **Memory:** Use **VMA** for GPU. Use **RAII** for CPU. No raw pointers.

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

## 5. Documentation Index

-   **Philosophy:** [`docs/architecture-philosophy.md`](docs/architecture-philosophy.md) (Deep dive).
-   **Tracer:** [`docs/tracer-system.md`](docs/tracer-system.md) (Logging rules).
-   **Conventions:** [`docs/cpp-conventions.md`](docs/cpp-conventions.md).
-   **Physics:** [`docs/physics-system.md`](docs/physics-system.md).
-   **Resources:** [`docs/resource-management.md`](docs/resource-management.md).

> **Maintenance:** If you implement significant changes, ask to update docs or run `/update-docs`.