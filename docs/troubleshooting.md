# Troubleshooting Guide

Solutions for common engine-level issues in Emeraude Engine development.

> **Application-level troubleshooting** (build, CEF, runtime) should be in the application's own `docs/troubleshooting.md`.

## Table of Contents

- [Material/Shader Issues](#materialshader-issues)
- [Physics Issues](#physics-issues)

---

## Material/Shader Issues

### Material properties not working (reflection/refraction amount)

**Symptoms:** Setting `reflectionAmount` or `refractionAmount` to 0 still shows the effect, or wrong colors appear.

**Root cause pattern:** Shared UBO offset bug - all materials read from offset 0.

**Debugging steps:**

1. **Verify C++ values are written correctly:**
   ```cpp
   // Add trace in StandardResource::updateVideoMemory()
   TraceInfo{ClassId} <<
       "updateVideoMemory for '" << this->name() << "':" "\n"
       "  UBO Index = " << m_sharedUBOIndex << "\n"
       "  UBO Offset = " << (m_sharedUBOIndex * m_sharedUniformBuffer->blockAlignedSize()) << " bytes\n"
       "  [20] reflectionAmount = " << m_materialProperties[ReflectionAmountOffset];
   ```

2. **Debug shader with visual output:**
   ```cpp
   // In LightGenerator.cpp, temporarily output UBO values as RGB:
   Code{fragmentShader, Location::Output} <<
       m_fragmentColor << ".rgb = vec3(" <<
       m_surfaceReflectionAmount << ", " <<
       m_surfaceRefractionAmount << ", " <<
       m_surfaceRefractionIOR << ");";
   ```

3. **Compare expected vs actual colors:**
   - If you set markers (0.11, 0.22, 0.33), you should see dark colors
   - If you see cyan/white (0.5, 0.95, 1.5), the shader reads wrong UBO data
   - This indicates a UBO offset problem in descriptor binding

4. **Check the critical files:**
   - `Vulkan/Buffer.hpp:getDescriptorInfo()` - Must use offset parameter
   - `Vulkan/UniformBufferObject.cpp:getDescriptorInfo()` - Must convert element index to bytes
   - `Graphics/Material/StandardResource.cpp:createDescriptorSet()` - Passes UBO index

**Solution:** Ensure `Buffer::getDescriptorInfo()` uses the offset:
```cpp
descriptorInfo.offset = static_cast<VkDeviceSize>(offset);  // NOT always 0!
```

### Shader uses undefined variable (fresnelFactor)

**Symptoms:** Shader compilation fails with "undefined variable: fresnelFactor"

**Cause:** `fresnelFactor` is only generated when BOTH reflection AND refraction components are present in the material.

**Solution:** Ensure material has both components:
```cpp
newMaterial.setReflectionComponent(cubemapTexture, 0.5F);
newMaterial.setRefractionComponent(cubemapTexture, 1.5F, 0.3F);
```

The variable is generated in `StandardResource.cpp` around line 1459.

### Wrong IOR value (always 1.0)

**Symptoms:** IOR value is always 1.0 even when set to other values like 0.5.

**Cause:** `setRefractionIOR()` clamps values to [1.0, 3.0] range.

**Solution:** IOR must be >= 1.0 (vacuum). Common values:
- Air: 1.0003
- Water: 1.33
- Glass: 1.5
- Diamond: 2.42

### SSS faces flickering / darkening (quad-level)

**Symptoms:** Individual quads (two triangles) on SSS materials darken or flicker, especially faces directly facing the camera. Most visible with `subsurfaceIntensity = 1.0`.

**Root cause:** `smoothstep(sssWrap, 1.0, NdotLWrap)` with `sssWrap = sssIntensity = 1.0` -> `smoothstep(1.0, 1.0, x)` is undefined behavior in GLSL. Produces NaN on some GPU drivers.

**Solution:** SSS wrap is clamped: `sssWrap = min(sssIntensity, 0.99)`. This ensures `edge0 < edge1` for `smoothstep`.

**Code reference:** `LightGenerator.PBR.cpp` lines 702, 716

### GPU hang with POM on large surfaces

**Symptoms:** Application becomes unresponsive when viewing large surfaces with Parallax Occlusion Mapping (height map) active.

**Root cause:** POM ray-marching is expensive per-fragment. At far distances, the effect is invisible but still consumes GPU cycles.

**Solution:** Distance-based POM fade (built-in since Feb 2026):
- Full POM within 8 world units
- Linear fade between 8-18 units (both heightScale and numLayers reduced)
- Complete skip beyond 18 units (early-out, returns original UVs)

**Code reference:** `PBRResource.cpp:generateFragmentShaderCode()` (POM section)

---

## Physics Issues

### Objects fall through floor

**Symptoms:** Physics-enabled objects pass through geometry.

**Solutions:**
1. Check boundaries are configured in scene
2. Verify the object has physics MovableTrait enabled
3. Check Y-down coordinate system (positive Y is down in Emeraude Engine)
4. Verify collision mesh is loaded for floor geometry

### Collisions not working

**Symptoms:** Objects don't collide with each other or with geometry.

**Solutions:**
1. Verify both objects have collision-enabled physics traits
2. Check collision masks and filters (ensure they overlap)

---

## Related Documentation

- [`docs/caution-points.md`](caution-points.md) - Engine critical warnings
- [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) - Graphics system
- [`src/Physics/AGENTS.md`](../src/Physics/AGENTS.md) - Physics system
- [`src/Saphir/AGENTS.md`](../src/Saphir/AGENTS.md) - Shader system
