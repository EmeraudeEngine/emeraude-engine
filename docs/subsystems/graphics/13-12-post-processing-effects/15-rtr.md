## 12. Post-Processing Effects

### RTR (Ray-Traced Reflections)

4-pass pipeline using `GL_EXT_ray_query` in a fragment shader (no RT pipeline required):

1. **Trace** (half-res): Reconstructs world position from depth, traces reflection ray via TLAS,
   samples hit material (bindless albedo texture or scalar), computes Lambert lighting at hit point
2. **Blur H**: Horizontal Gaussian blur
3. **Blur V**: Vertical Gaussian blur
4. **Composite**: Blends blurred reflection with scene color using confidence (alpha)

**Descriptor sets** (trace pass):
- Set 0: RT data from `Renderer::rtDescriptorSet()` — TLAS (binding 0), mesh metadata SSBO (binding 1),
  material data SSBO (binding 2), light array SSBO (binding 3), sub-geometry table SSBO (binding 4)
- Set 1: Input textures — depth (binding 0), normals (binding 1), environment cubemap (binding 2),
  scene albedo (binding 3 — the effect declares `requiresAlbedo()`)
- Set 2: Bindless textures from `BindlessTextureManager` — sampler2D[] (binding 1)

**Primary-surface Fresnel is a COLOR (fixed Aug 2026):**
`F0 = mix(vec3(0.04), originAlbedo, originMetalness)` — a metal tints its reflection by its
albedo (gold reflects gold, a teal dome reflects teal), a dielectric reflects the physical 4%
head-on. The scalar lobe weight (max component) keeps feeding the premultiplied confidence
pipeline; the NORMALIZED tint multiplies the traced color, on both the hit and the
environment-miss paths.
> [!WARNING]
> The former model — `float F0 = mix(0.15, 0.9, metalness)`, "floor at 0.15 so dielectrics show
> visible reflections" — made every metal a WHITE mirror and boosted dielectrics 4×. Measured on
> DamagedHelmet: the dark-teal dome rendered at ~70 % of the SKY's luminance with the sky's own
> chromaticity, erasing the raster's correctly F0-tinted split-sum IBL through the composite mix.
> A "flashy" reflection is an energy bug, never a look to preserve.

> [!WARNING]
> **The MRT normal-alpha packing (`alpha = roughness + round(metalness) × 2`) quantizes the
> metalness to {0,1} at WRITE time — that quantization is load-bearing.** The decode
> (`metalness = alpha >= 2`) assumes it: a raw fractional metalness (real data since the packed
> metallic-roughness source channels are honored) corrupts BOTH decoded values (0.93 metal /
> 0.4 rough packed raw decodes as 1.0 / 0.26). Write site:
> `Saphir/Generator/SceneRendering.cpp` (MRT normal output).

**Mesh data access via buffer references:**
- `BDA` (buffer device addresses) in mesh metadata SSBO point to vertex/index buffers
- Shader reads vertex normals and UVs via `VertexBuffer`/`IndexBuffer` buffer references
- Byte offsets for normals and UVs computed from geometry flags (tangent space, etc.)

> [!CAUTION]
> **A hit's primitive index is relative to ITS sub-geometry; the index buffer is SHARED.**
> A multi-sub-geometry BLAS is built with one `VkAccelerationStructureGeometryKHR` per
> sub-geometry, each carrying a `primitiveOffset`, so `rayQueryGetIntersectionPrimitiveIndexEXT`
> restarts at 0 for every geometry. `rtHitFirstIndex(instance, geomIdx)` rebases it — that is
> what `getMeshAccessor(instance, geomIdx, primitive)` does before touching the buffer, and why
> it takes the geometry index at all. Reading the buffer with the raw primitive index fetches
> another sub-geometry's triangle: right material, wrong vertices, wrong UVs, wrong normals.
> It shipped that way until Sep 2026 and showed up as a stretched bark texture with hard dark
> bands on the reflected palm trunk of `light-and-shadow-debug` (leaves = sub-geometry 0, bark =
> sub-geometry 1). The raster was immune: it passes `firstIndex` to `vkCmdDrawIndexed`.
> Details and the full file list: `docs/caution-points.md` § "the RT hit shader read the WRONG
> TRIANGLE of a multi-layer mesh".

**Sub-geometry table** (set 0, binding 4, `GPUSubGeometryData`): one `uvec2` row per BLAS
geometry — `.x` = first index in the shared index buffer, `.y` = material index. The instance's
`GPUMeshMetaData` holds the offset of its first row and the row count; a hit reads
`offset + rayQueryGetIntersectionGeometryIndexEXT(...)`. Shared GLSL: `EMEN_RT_SUBGEOMETRY_GLSL`
(`Effects/Shared/RTAlphaTestGLSL.hpp`), spliced BEFORE each shader's own `getMeshAccessor()`.
⚠️ The rows are filled from the partition the BLAS was ACTUALLY built with —
`Geometry::Interface::BLASGeometryFirstIndices()`, or `Abstract::rtRefitInputs()` for a skinned
instance — never re-derived, so the "do not partition" cases cannot drift. There is no
per-instance ceiling (the former inline `materialIndices[4]` silently mis-materialled every
group past the fourth; `Humans/OldMan` has seven), only a scene total,
`SceneMetaData::MaxRTSubGeometries`.

> [!CAUTION]
> **The hit shading owes the raster the Lambert `1/PI`.** The direct diffuse lobe and the flat
> scene ambient are both `albedo * E / PI` — `albedo * (1-metalness) * (1-F) / PI` and
> `albedo * ambientLight.rgb / PI`. Without them RTR was **PI times too bright** on every
> reflected surface and a mirror outshone its source (measured 2026-09-13 on
> `light-and-shadow-debug`: the brick cube's reflection at 2.26× the cube itself, 0.82× after
> the fix, on a floor of albedo 1 / roughness 0 / metal where ~0.8 is the expected figure).
> ⚠️ The probe query and the bindless irradiance cube already store `E/PI` — never divide those
> again. The raster reference is `Saphir/LightGenerator.PBR.cpp` (`kD * albedo / 3.14159265`)
> and `LightGenerator.cpp` (`iblBaseColor * 0.3183098862`); RTGI and the probe volume already
> did it right for the direct term, RTR was the family's only outlier. ⚠️ The probe volume's flat AMBIENT term
> lacked its 1/PI (and was clamped to 1 lx by a `Color< float >`) until 2026-09-25.

> [!CAUTION]
> **The radius falloff is the RASTER curve, verbatim**: `max(1 - dot(d/r, d/r), 0)`, and NO
> attenuation when the radius is 0. RTR, RTGI and `IrradianceProbeVolume` each carried
> `clamp(1 - d/r, 0, 1)²` plus an inverse-square fallback until Sep 2026 — a different curve
> (0.25 against 0.75 at half the radius), so a traced surface was lit by a third of what the
> rendered one received at mid-range. Three copies of one formula: change them together.

**Self-reflection rejection:** `dot(hitNormal, worldNormal) > 0.9` prevents flat surfaces
from reflecting themselves (e.g. floor reflecting floor).

**Critical synchronization requirements:**
1. Mesh/material SSBOs must be **per-frame** (see Frame Synchronization section)
2. View matrices must use **`readStateIndex`** overloads (see Scenes/AGENTS.md)

**Code references:**
- `Effects/Lighting/RTR.hpp` — Parameters, API
- `Effects/Lighting/RTR.cpp` — GLSL shaders (inline), descriptor layouts, pipeline creation
- `Scenes/SceneMetaData.hpp` — TLAS, mesh metadata, material data management
- `Scenes/GPUMeshMetaData.hpp` — GPU-side mesh metadata struct layout
