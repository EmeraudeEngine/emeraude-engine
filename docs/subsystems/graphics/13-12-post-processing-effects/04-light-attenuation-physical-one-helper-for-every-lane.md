## 12. Post-Processing Effects

### Light Attenuation — Physical, ONE helper for every lane (restored 2026-09-25)

Point and spot lights fall off as a **windowed inverse square**, written ONCE in
`Graphics/Effects/Shared/LightFalloffGLSL.hpp` and used by the raster light pass (Saphir
`LightGenerator.PBR.cpp`, through `EMEN_LIGHT_FALLOFF_BODY_GLSL`) and by the three traced direct terms —
RTGI, RTR and the irradiance probes (`EMEN_LIGHT_FALLOFF_GLSL`):

```glsl
float emLightFalloff (float d, float r) /* metres; r <= 0 = unbounded, the inverse square alone */
    = saturate(1 - (d/r)^4)^2 / max(d^2, 0.01^2)
```

References: Lagarde & de Rousiers, "Moving Frostbite to Physically Based Rendering", SIGGRAPH 2014
(`smoothDistanceAtt / max(d², 0.01²)`); Karis, "Real Shading in Unreal Engine 4", SIGGRAPH 2013 (the same window).

This is what makes a light intensity in **candela** mean anything: the illuminance it produces at `d` is
`I/d²` lux. The window forces the contribution to exactly zero at the radius, so the renderer keeps culling lights
by radius (it sits near 1 over most of the range: 0.88 at half the radius); squaring it removes the visible edge.
The 1 cm clamp removes the singularity at the source without biasing anything a camera frames.

> [!CAUTION]
> **The inverse square was LOST from 2026-08-12 to 2026-09-25.** `1c1d94ba` ("the Blinn-Phong machinery is gone")
> deleted `LightGenerator.PerFragment.cpp`, which generated it, and left `max(1 - (d/r)², 0)` — no `1/d²`, the radius
> as the dimmer. On 2026-09-13 RTGI, RTR and the probes were "fixed" to copy the raster curve verbatim, so all four
> lanes agreed on the wrong law. ⚠️⚠️ **Two lanes that agree prove nothing about the law**: check the curve against
> the physics (`I/d²`), never only against the other lane. The version lost in August also used `1 / (d² + 1)` —
> Unreal's `+1` is in CENTIMETRES², and in metres it halved the illuminance at 1 m; the restored helper clamps
> instead. Every point/spot light of projet-alpha's demos was re-tuned to keep its look (the power that delivers
> the former illuminance at the light's subject), the actors kept their physical lumens: engine
> [`docs/caution-points.md`](../../../caution-points.md) § *Fixed: point and spot lights had NO inverse square
> for six weeks*.

`legacyUnitCompensation` — the TEMPORARY per-light factor that restored the pre-change
brightness while content was authored in the old units — is GONE: photometric phase 2 removed it.

**Interface** (`PostProcessEffect.hpp`):
- `create(renderer, width, height)` — Allocate GPU resources (IRTs, pipelines, descriptors)
- `destroy()` — Release resources
- `resize(renderer, width, height)` — Recreate on window resize
- `execute(commandBuffer, inputColor, inputDepth, inputNormals, constants)` — Run effect
- `requiresDepth()` / `requiresNormals()` / `requiresHDR()` — Declare input dependencies
