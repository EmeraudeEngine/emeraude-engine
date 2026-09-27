## 12. Post-Processing Effects

### The directory is the DOMAIN, the base class is the MECHANISM (Sep 2026)

`Graphics/Effects/` used to be split by execution mechanism — `Display/` and `Lens/` for the
direct effects, `Framebuffer/` for the indirect ones. That said nothing the base class did not
already say, and it forced the ray-traced light transport, the participating medium, the image
resolve and the whole photographic chain into ONE `Framebuffer/` bag of 16. It is now split by
**domain of application**, which makes the directory a readable map of `EffectSlot`:

| Folder / namespace | Domain | Members |
|---|---|---|
| `Effects/Lighting/` | Light transport off surfaces, from the G-buffer | RTGI, SSGI, RTR, SSR, RTAO, SSAO, ContactShadows |
| `Effects/Atmosphere/` | Participating medium | AtmosphericFog, VolumetricLight, VolumetricScattering, VolumetricClouds |
| `Effects/Resolve/` | Resolution of the sampled image — adds NO light | TAA, FXAA, FXAASharpen, Sharpen |
| `Effects/Camera/` | The physical imaging chain | DepthOfField, MotionBlur, LensFlare, VeilingGlare, ToneMapping |
| `Effects/Style/` | The look — non-physical | the 18 former `Lens/` effects |
| `Effects/Shared/` | GLSL snippet libraries, NOT effects | CSMSamplingGLSL, MarchDitherGLSL, RTAlphaTestGLSL |
