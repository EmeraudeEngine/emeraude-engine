## Legacy (Blinn-Phong) Specular — Energy Normalisation (Jul 2026)

The non-PBR specular is a **normalised BRDF times an irradiance**, exactly like its diffuse sibling.
It is now the `BasicResource` tier alone: since the material merge, `declareSurfaceSpecular()` has a
single caller, and the legacy `StandardResource` this was written for no longer exists.
Emitted at three sites, all computing the same expression in their own space:

| Generator | Space | Normal used |
|---|---|---|
| `LightGenerator.PerVertex.cpp` | view (vertex shader, interpolated result) | `NormalViewSpace` |
| `LightGenerator.PerFragment.cpp` | view | `twoSidedN` |
| `LightGenerator.PerFragment.NormalMap.cpp` | **tangent** | `m_surfaceNormalVector` |

```glsl
const float specularExponent = max(<Material.Shininess>, 1.0);
SpecularFactor = pow(max(dot(N, H), 0.0), specularExponent)
               * ((specularExponent + 2.0) / 25.132741228718345)   /* (n + 2) / (8*pi) */
               * DiffuseFactor;                                     /* carries N.L * LightFactor */
```

Three things to keep straight:

1. **`(n + 2) / (8*pi)`, not `(n + 2) / (2*pi)`.** The latter belongs to the modified-Phong BRDF or to
   the Blinn NDF taken alone; using it is a factor of 4.
2. **The `N.L` arrives via `DiffuseFactor`, not `LightFactor`.** `DiffuseFactor` is already
   `N.L * LightFactor`, so the shadow/attenuation factor is applied exactly once. Multiplying by both
   would square it.
3. **`max(..., 1.0)` on the exponent** is a real floor, C++ API and manifest alike:
   `BasicResource` takes the manifest `Shininess` RAW as a Blinn-Phong exponent (default 200),
   with nothing keeping an authored value above 1. Without the floor, an exponent under 1 gives a
   lobe that never decays.

Why it matters photometrically: unnormalised, the term was `specularColor * illuminance * pow(...)`,
a raw multiple of the illuminance with no cosine — a 0.5 grey specular under a 50000 lx sun returned
22350 nits, five times the luminance of the sky above it. Now the diffuse and the specular of one
material are on the same scale, and both are commensurable with lights authored in lux/candela.
Full diagnosis and measurements: `docs/caution-points.md`, "The legacy specular was not
energy-normalised".

> [!WARNING]
> **The manifest key `Shininess` means two different things depending on the container.**
> `BasicResource` reads it as the raw exponent above; the merged `StandardResource` reads it as an
> authored **glossiness** `[0,1]` and converts it as `roughness = 1 - glossiness`. The old
> `StandardResource::specularExponentFromGlossiness()` bridge died with the legacy material — do
> not look for it, and never apply a conversion twice.

> [!NOTE]
> The **PBR low-quality** specular approximation in `LightGenerator.cpp` (`lqSpecPower`) is still
> unnormalised and still multiplies the raw illuminance. ⚠️ VOID since Aug 2026: `lqSpecPower`
> lived in `generateFinalFragmentOutput()`, written for the Gouraud path, and was deleted with it.
