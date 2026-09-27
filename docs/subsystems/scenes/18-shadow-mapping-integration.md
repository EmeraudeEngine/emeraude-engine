## Shadow Mapping Integration

The Scene handles shadow map rendering and lighting pass selection. See [`docs/shadow-mapping.md`](../../shadow-mapping.md) for complete shadow mapping architecture.

### Pass Type Selection (Shadow + Color Projection)

Each light's `RenderPassType` is selected at render time based on 4 conditions:

```cpp
const bool useShadow = shadowMapsEnabled
    && light->isShadowCastingEnabled()
    && light->hasShadowDescriptorSet()
    && instance->isShadowReceivingEnabled();
const bool useColorProjection = light->hasColorProjectionTexture();

// 4-branch selection per light type:
if ( useShadow && useColorProjection )
    passType = RenderPassType::SpotLightPassFull;
else if ( useShadow )
    passType = RenderPassType::SpotLightPassShadowMap;
else if ( useColorProjection )
    passType = RenderPassType::SpotLightPassColorMap;
else
    passType = RenderPassType::SpotLightPass;
```

Same pattern applies to directional (with CSM variants) and point lights.

**Why this matters:** Without the global shadow check, disabling shadows via settings caused Vulkan validation errors because shadow map images remained in `VK_IMAGE_LAYOUT_UNDEFINED` but descriptor sets still tried to bind them.

### Descriptor Set Architecture

Each light creates a descriptor set with 2 bindings:

| Binding | Content | Inactive fallback |
|---------|---------|-------------------|
| 0 | Light UBO (dynamic offset) | Always present |
| 1 | Shadow map sampler | Not created (no shadow descriptor set) |

Lights without shadow use only the shared UBO descriptor set (binding 0). Shadow-enabled lights get a dedicated descriptor set with both bindings.

**Color projection uses the global bindless system** — the light UBO carries a `uint` bindless index (`ColorProjectionIndex`, encoded as `bit_cast<float>`). The texture is registered in `BindlessTextureManager` via `ObserverTrait` notification when async loading completes. See: `Saphir/AGENTS.md` → Bindless Color Projection Sampling.

**Code references:**
- `Scene.rendering.cpp:renderLightedSelection()` - Pass type selection logic
- `Component/SpotLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Component/DirectionalLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Component/PointLight.cpp:createShadowDescriptorSet()` - 2-binding shadow descriptor
- `Component/AbstractLightEmitter.cpp:registerColorProjectionInBindless()` - Bindless registration
- `Component/AbstractLightEmitter.cpp:onNotification()` - Async texture load callback
