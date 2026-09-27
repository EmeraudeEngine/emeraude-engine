## 9. Shadow Mapping & Color Projection Global Control

The `Renderer` provides a global shadow mapping enable/disable via `isShadowMapsEnabled()`.

**Setting key:** `GraphicsShadowMappingEnabledKey` (`Core/Graphics/Renderer/ShadowMappingEnabled`)

**Integration with Scene:**
The Scene checks this setting when selecting `RenderPassType` for each light. The pass type is selected from a 4-branch matrix:

| Shadow | Color Projection | Pass Type (example: Spot) |
|--------|-------------------|---------------------------|
| No | No | `SpotLightPass` (0 samplers) |
| Yes | No | `SpotLightPassShadowMap` (1 sampler) |
| No | Yes | `SpotLightPassColorMap` (1 sampler) |
| Yes | Yes | `SpotLightPassFull` (2 samplers) |

Each pass type generates a **distinct shader program**. When a feature is inactive, its sampling code is not generated — no dummy texture samples, no wasted GPU cycles.

**Descriptor set architecture:** Each light creates a 2-binding descriptor set (UBO + shadow sampler) when shadow mapping is active, or uses the shared UBO-only descriptor set otherwise. Color projection is handled via the global `BindlessTextureManager` — the light UBO carries a bindless index, and the shader samples from the bindless texture array. See: Section 6 → Color Projection via Bindless.

**Why global control matters:**
Without the global check, disabling shadow mapping via settings caused Vulkan validation errors because shadow map images remained in `VK_IMAGE_LAYOUT_UNDEFINED` but descriptor sets still tried to bind them.

**Code references:**
- `Renderer.hpp:isShadowMapsEnabled()` - Global accessor
- `Scenes/Scene.rendering.cpp:renderLightedSelection()` - 4-branch pass type selection
- `Graphics/Types.hpp:RenderPassType` - 16-value combinatorial enum
- `Graphics/Types.hpp:renderPassUsesColorProjection()` - Color projection helper
- `SettingKeys.hpp:GraphicsShadowMappingEnabledKey` - Setting key

See [`docs/shadow-mapping.md`](../../shadow-mapping.md) for complete shadow mapping and color projection architecture.
