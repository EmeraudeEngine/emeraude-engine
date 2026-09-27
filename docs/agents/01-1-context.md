## 1. Context

**Core Framework**: Modern C++20 Vulkan 3D Engine.
**Coordinates**: **Right-Handed Y-UP** — `+X` right, `+Y` up, `-Z` forward (consistent across Physics, Rendering, Scene graph, Audio). Same axes as glTF 2.0, USD and FBX, so imports are the identity. ⚠️ The engine was Y-DOWN until Aug 2026 — any statement to that effect is stale, see [`docs/coordinate-system.md`](../coordinate-system.md).
**Platform**: Windows 11, Linux (Debian/Ubuntu), macOS.
**Graphics API**: **Vulkan-only** — no D3D11, no D3D12, no Metal, no OpenGL. Single backend, mastered in full.
