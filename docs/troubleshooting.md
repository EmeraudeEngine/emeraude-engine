# Troubleshooting Guide

Solutions for common engine-level issues in Emeraude Engine development.

> **Application-level troubleshooting** (build, CEF, runtime) should be in the application's own `docs/troubleshooting.md`.

## Table of Contents

- [Material/Shader Issues](#materialshader-issues)
- [Physics Issues](#physics-issues)
- [macOS / MoltenVK Issues](#macos--moltenvk-issues)

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

## macOS / MoltenVK Issues

### Everything lit by IBL is black (ground, metals, reflections) — MoltenVK < 1.4

**Symptoms:** On macOS the sky and the overlay render correctly, but the terrain is pure black
and the chrome/gold spheres are invisible. Metals get their whole appearance from the specular
IBL, so they vanish with it. No Vulkan error, no engine error — the frame is simply unlit.

**Root cause:** a **MoltenVK driver bug**, not an engine bug. In MoltenVK 1.2.11 (the version
shipped with the Vulkan SDK 1.2.296 installer) the **samplers** of a bindless descriptor array
are not encoded into the Metal argument buffer. The textures are — which makes the failure
mode confusing: the descriptors look correct from the Vulkan side.

**How to diagnose it (the decisive test):** replace the fetch with `textureSize()` on the same
bindless slot. `textureSize` needs no sampler:

```glsl
/* Returns the real size (e.g. 128) → the TEXTURE is bound, so only the SAMPLER is missing. */
vec4 probe = vec4(vec2(textureSize(uBindlessTexturesCube[2], 0)) / 1024.0, 0.0, 1.0);
```

If `textureSize` returns the right size while `texture()` / `textureLod()` returns 0, the
samplers of that set are unbound. Verified further: every slot of every binding of the bindless
set was dead (cube 0/1/2 *and* the 2D BRDF LUT), while a per-material sampler in another set
(the skybox, `set = 2`) worked — so the fault is set-wide, not per-slot.

**Solution:** use **MoltenVK ≥ 1.4**. Verified fixed on 1.4.2 with the same Metal3
argument-buffer path. To test without touching the Vulkan SDK install:

```bash
brew install molten-vk   # installs into its own prefix, SDK untouched
VK_DRIVER_FILES=/opt/homebrew/Cellar/molten-vk/<version>/etc/vulkan/icd.d/MoltenVK_icd.json ./<app>
```

**Ruled out during the hunt** (all measured, so do not re-investigate): descriptor writes
(sampler/view/layout all valid), the set index at bind time (`bindAtSet == shaderExpectsSet`),
`nonuniformEXT`, `textureLod` vs `texture`, the sampling direction, the bindless array sizes
(down to 80 descriptors), `UPDATE_AFTER_BIND`, and the generated MSL (correct and
self-consistent). The IBL bake itself was verified with `EMERAUDE_DEBUG_IBL_FACES`: every face
and every mip contained valid data.

### Storage images must declare a format qualifier

**Symptoms:** A compute shader writing to a storage image silently writes nothing on macOS.
With the validation layers on: `VUID-VkShaderModuleCreateInfo-pCode-08740`, "SPIR-V Capability
**StorageImageWriteWithoutFormat** was declared".

**Root cause:** an unformatted `uniform writeonly image2D` makes glslang emit the
`StorageImageWriteWithoutFormat` capability. That is a spec violation unless
`shaderStorageImageWriteWithoutFormat` is enabled, and it leaves MoltenVK/SPIRV-Cross with no
pixel format to translate the `imageStore()` to — Metal needs the format at shader-compile time.

**Solution:** always give the qualifier, matching the `VkImage` format:

```glsl
layout(set = 0, binding = 0, rgba16f) uniform writeonly image2D brdfLut;  /* VK_FORMAT_R16G16B16A16_SFLOAT */
```

**Code reference:** `Graphics/Compute/IBLBaker.cpp` (BRDF LUT + environment shaders).

### VK_KHR_portability_subset must be enabled whenever it is advertised

Per the Vulkan spec, a device exposing `VK_KHR_portability_subset` **must** enable it — this is
always the case for MoltenVK. Test the *extension*, never the platform: guarding the enable
with `if constexpr ( !IsMacOS )` skips the one platform that always needs it and leaves the
device created in a spec-violating state.

**Code reference:** `Vulkan/Instance.cpp`, graphics device features configuration.

### Shadow comparison samplers are not available (open)

**Symptoms:** with the validation layers on, macOS reports
`VUID-VkDescriptorImageInfo-mutableComparisonSamplers-04450`, "sampler comparison not
available", on the shadow-map descriptor writes.

**Root cause:** MoltenVK's portability subset reports `mutableComparisonSamplers = VK_FALSE`,
but `DummyShadowTexture` and `RenderTarget/ShadowMap` create samplers with
`compareEnable = VK_TRUE`. This affects the hand-authored dynamic-lighting path (shadow maps);
the sky-driven path uses no shadow map and is unaffected.

**Status:** not fixed. A portable shadow path has to sample the depth texture and do the
comparison in the shader instead of relying on a hardware comparison sampler.

---

## Related Documentation

- [`docs/caution-points.md`](caution-points.md) - Engine critical warnings
- [`src/Graphics/AGENTS.md`](../src/Graphics/AGENTS.md) - Graphics system
- [`src/Physics/AGENTS.md`](../src/Physics/AGENTS.md) - Physics system
- [`src/Saphir/AGENTS.md`](../src/Saphir/AGENTS.md) - Shader system
