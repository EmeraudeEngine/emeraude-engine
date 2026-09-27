## Module Overview

Format-agnostic namespace for loading multi-resource asset files (glTF, FBX, etc.) into engine resource containers. Produces a common intermediate representation (`SceneData`) that can be consumed by scene builders or mesh resources independently.

> [!IMPORTANT]
> **This layer loads *scenes*.** Whether a given file yields a single model or a complete scene
> (lights, cameras, instancers) is decided by **the format**, not by the caller — hence
> `Loaders` / `SceneData` (renamed from `AssetLoaders` / `AssetData`, 2026-08-08). It lives
> engine-side rather than in emeraude-base precisely because emeraude-base only knows raw,
> classic geometry formats; composite scene description belongs here.
>
> **Absorption rule.** A loader translates *everything* into native engine scene logic. When
> the scene layer cannot express a source concept, the missing capability is added to `Scenes`
> — never a foreign construct kept alive, never a workaround in the loader. See
> [`../../docs/scene-loaders-usd.md`](../../scene-loaders-usd.md).
