## 12. Post-Processing Effects

### A shadow/occlusion ray offsets its ORIGIN along the normal — tMin is not a bias (Sep 2026)

> [!CAUTION]
> **`rayQueryInitializeEXT`'s `tMin` and a normal-offset bias are NOT interchangeable, and
> substituting one for the other produces a FACETED terminator, not a softer one.** `tMin`
> advances the ray along its OWN direction. At a shadow terminator the light is grazing by
> definition, so advancing along it keeps the origin inside the surface's own curvature and the
> ray re-hits the neighbouring triangles. The self-shadow boundary then follows the MESH FACETS
> instead of the smooth shading normal — large, axis-aligned steps that read as a staircase.
>
> **The rule, for every ray query fired from a G-buffer surface:**
> `rayOrigin = worldPos + worldNormal * adaptiveBias`, and `tMin` a tiny CONSTANT (0.001).
> The two are decoupled on purpose: the origin offset is what escapes the surface, while a `tMin`
> equal to the adaptive bias SKIPS genuine occluders closer than it — the exact failure RTAO
> documents at wall/floor creases (a bright line where the AO must be darkest).
>
> `adaptiveBias = bias * max(1, cameraDist) * min(1 / NdotV, 10)`. Both factors matter: the
> distance term follows the growing pixel footprint, the grazing term covers rays leaving a
> nearly edge-on surface. RTAO is the reference implementation (`RTAO.cpp`, `hemispherePoint`
> block); **ContactShadows now follows it** — it did not until Sep 2026.
>
> **How it was missed for so long:** `ContactShadows` declared `requiresNormals()`, the renderer
> bound the normal attachment, the effect wrote the descriptor every frame — and the GLSL
> declared `sampler2D normalTex` and **never sampled it**. The parameter was even named
> `normalBias` while being handed to `tMin`. A `grep` for the binding finds it wired; only a
> `grep` for the READ shows the two halves were never connected.
>
> ⚠️ **Raising `tMin` hides the staircase and is NOT the fix.** Measured on the DamagedHelmet
> dome (projet-alpha `--load-demo=asset-loader --demo-options=7,0,1,0,0,0`, camera
> `setPosition(0, 0.75, 1.5)` + `lookAt(-0.013441, -3.312027, -7.637820)`), counting axis-aligned
> steps of ≥ 4 px along the mask's terminator:
>
> | Variant | Steps ≥ 4 px | Boundary perfectly flat | Cost |
> |---|---|---|---|
> | `tMin = bias` (0.01), no normal offset | **19** | 72.3 % | the defect |
> | `tMin = bias` raised ×10 (0.10) | 2 | 47.8 % | peter-panning: skips the near occluders the effect exists to draw |
> | **normal offset, bias back at 0.01** | **0** | 47.9 % | none |
>
> The middle row is the trap: it looks like a fix and costs exactly the effect's reason to exist.
