---
id: ocean-fft-surface
title: An animated sea — FFT spectral waves on a camera-following LOD grid, MoltenVK included
status: in-progress
priority: unranked
scope: Graphics/Geometry, Graphics/Renderable, Saphir/Generator, Graphics/Renderer (per-frame compute)
opened: 2026-09-28
tags: [water, ocean, fft, compute, lod, state-of-the-art]
---

# An animated sea — FFT spectral waves on a camera-following LOD grid, MoltenVK included

## Why

Owner (2026-09-28, after projet-alpha `water-world`): "le plan d'eau était inerte. Il n'y a qu'une texture normal animée
pour gérer les vagues, plutôt intéressant pour simuler les lacs. Est-ce qu'il est possible d'utiliser le mesh shader ou
une autre technique pour animer la surface de l'eau comme une mer ? Il faut penser à un fallback pour moltenvk."

Today's sea (`Renderable::BasicSeaResource`): a finite flat grid (256 m on `water-world`, a hard edge against the sky)
and the ocean material (`AbstractDemo::getOrCreateOceanMaterial()`: a 120-frame animated normal map, grab-pass
transmission, IOR 1.33, depth-based opacity). No physics reads the sea level yet (no buoyancy).

## Owner decisions (2026-09-28, AskUserQuestion)

1. **FFT spectral waves** (chosen over Gerstner sums and a Gerstner + FFT hybrid): J. Tessendorf, "Simulating Ocean
   Water", SIGGRAPH course 2001, with a directional JONSWAP spectrum (C. J. Horvath, "Empirical directional wave spectra
   for computer graphics", DigiPro 2015). One compute pass per frame, 2-3 cascades of 256² (different tile sizes, so no
   repetition shows), producing displacement (choppy: horizontal too), normals and foam (the Jacobian of the horizontal
   displacement). Compute runs on MoltenVK (Metal compute).
2. **A camera-following LOD grid displaced in the VERTEX stage** (chosen over mesh shaders + a vertex fallback, and over
   a projected grid): ONE path for every device, MoltenVK included. Waves are smooth at the vertex scale — the ripples
   come from the per-pixel normals — so mesh shaders bring no visible gain here. The sea reaches the horizon: the
   finite plane's hard edge goes away.

## The plan (proposed, to confirm)

The design mirrors the heightfield terrain, already proven: a geometry whose surface is SYNTHESIZED from textures by the
vertex stage, its frame rebuilt per pixel by the fragment stage, its textures updated every frame by the renderer.

1. **The spectrum and the FFT** (compute): initial spectrum h₀(k) from JONSWAP + directional spreading (wind speed,
   direction, fetch, per cascade), then per frame h(k, t) and an inverse FFT (Stockham or radix-2 butterfly passes) into
   displacement, normal and foam textures per cascade. Registered with `Renderer::registerSurfaceGeometry()`; recorded
   in `updateSurfaceVideoMemory()` (render thread, before the shadow maps).
2. **The geometry**: concentric rings of quads around the camera (geometry clipmaps, F. Losasso & H. Hoppe, SIGGRAPH
   2004 — the scheme the terrain's clipmap already uses), snapped to the coarsest cascade texel to avoid swimming; the
   vertex stage displaces by the cascades; a flag + a Saphir helper beside `HeightfieldSurfaceHelper`.
3. **The shading**: the per-pixel normal from the cascades replaces the animated normal map; foam from the Jacobian; the
   existing transmission, IOR and depth-based opacity unchanged.
4. **The renderable**: `Renderable::OceanResource` (decision 3).
5. **projet-alpha**: `water-world` and `terrain` take the ocean; measure the GPU cost (the FFT is ~2-3 × 256² × log₂ 256
   butterflies per frame), judge the image.
6. **Later, not now**: `getLevelAt()` answering the waves for buoyancy (a CPU copy of the displacement one frame late).

3. **A new `Renderable::OceanResource`** implementing `Scenes::SeaLevelInterface` (owner decision 2026-09-28, chosen
   over a mode of `BasicSeaResource`): `BasicSeaResource` stays the lake, with the animated normal map the owner likes
   there; each demo picks one in `onSetupSeaLevel()`.
4. **Start with step 1** (owner, 2026-09-28): the spectrum and the FFT alone, validated by dumping the displacement and
   comparing it with a CPU reference FFT, before any geometry.

## Progress (2026-09-28)

**Step 1 done**: `Graphics::OceanWaves` (`src/Graphics/OceanWaves.{hpp,cpp}`) — JONSWAP + Donelan-Banner spectrum built on
the CPU (deterministic PCG + Box-Muller draws, 3 cascades of 256² over 512 / 64 / 8 m, disjoint wave-number bands), then
per frame an evolution pass (eight real fields packed two by two), a Stockham radix-2 FFT of 256 points per row then per
column entirely in shared memory (16 KB), and a resolve pass into RGBA16F displacement and slopes. Self-test
`Core.RendererService.testOceanWaves()` against a double-precision Cooley-Tukey reference computed field by field:
**PASS**, every field within 5·10⁻⁴ of its peak (the half-float precision), 0 VUID. Default sea: Hs ≈ 2.2 m (4σ).

**Step 2 done (first light, 2026-09-28)**: the geometry is NOT the nested rings first proposed — snapping every ring to
the coarsest spacing (256 m with 9 levels) could leave the camera outside the finest square, and per-ring snapping needs
the geometry clipmaps' L-shaped fill strips. It is the terrain's CDLOD on an INFINITE plane instead (same decision: a
camera-following LOD grid displaced in the vertex stage, one path for every device), reusing the proven selection,
geomorph and per-node draw path:
- `Geometry::OceanSurfaceResource` (flags `EnableHeightfieldSurface | EnableOceanSurface`): a square of WORLD-ALIGNED
  root nodes around the camera (nodes never move in the world: nothing swims), the terrain's patch and index layout,
  its morph table; the FFT (`Graphics::OceanWaves`) recorded and submitted every frame from `updateSurfaceVideoMemory()`
  on the graphics queue, like the terrain's clipmap; the heightfield descriptor layout reused (binding 0 displacement,
  1 slopes, 2 `OceanSurface::Uniforms`).
- Saphir: `AbstractVertexStage::enableOceanSurface()` (the CDLOD placement + geomorph, then the cascades read at the
  UNDISPLACED lattice point, each cascade faded where the vertex spacing cannot hold it), `declareOceanSurface()` with
  `hfPixelNormalAt()` — the choppy surface's normal per pixel through the heightfield per-pixel frame.
- `Renderable::OceanResource` (a `SeaLevelInterface`, still answering the FLAT level); `water-world` uses it.
- Engine defect found on the way: `Material::addVolumetricTextureFallback()` asked the vertex BUFFER for UVs, so an
  animated texture on any synthesized surface (terrain or ocean) had no `sv3DTexCoord0` (shader compile error); it asks
  `surfaceProvidesPrimaryTextureCoordinates()` now.
- Measured on `water-world` (RTX 3070 Ti, validation on): 0 VUID, no shader error; the sea reaches the horizon (the
  256 m plane's hard edge is gone); 30 % of the sea pixels move by more than 10/255 in one second.

Still to do: the GPU cost (the profiler was off), the velocity of a moving surface (the vertex stage still reports
hfPosition as the previous position — TAA will smear the waves), foam from the Jacobian, mipmaps of the cascades (the
far pixels fade the small cascades instead), the shore (depth-based opacity with waves), MoltenVK on the macOS peer,
the CPU level for buoyancy (step 6), the docs.

## ⚠️ Traps

- ⚠️⚠️ The FFT's Nyquist row and column (index N/2) are their own mirror: an ODD operator (i kx, kx / |k|) breaks the
  Hermitian symmetry there and the packed partner of that field gets its imaginary part (measured 1-3 % errors on Dz,
  ∂Dx/∂x, ∂Dz/∂z). The spectrum is zero on them.
- E|h(k, t)|² must be S(k) Δk²: h(k, t) sums two independent draws and a complex unit Gaussian carries E|ξ|² = 2, so the
  amplitude is sqrt(S Δk² / 4) — a half doubled the variance (Hs 3.1 m instead of 2.2 m).

- The grab-pass transmission and the depth-based opacity read the scene BEHIND the water: a displaced surface moves that
  depth; check the shore (the water column thickness) with waves.
- Snap the grid to the texel of the coarsest cascade, or the displacement slides under the vertices (swimming).
- The ray-tracing proxy of the sea (if any) must follow the displaced surface or be excluded: a flat traced plane under
  moving waves repeats the traced-versus-drawn mismatch fixed on the terrain (docs/caution-points.md § CDLOD terrain).
- MoltenVK: check the compute image formats (storage images of RG16F / RGBA16F) against the Metal feature set.

## References

- J. Tessendorf, "Simulating Ocean Water", SIGGRAPH 2001 course notes.
- C. J. Horvath, "Empirical directional wave spectra for computer graphics", DigiPro 2015.
- F. Losasso, H. Hoppe, "Geometry Clipmaps: Terrain Rendering Using Nested Regular Grids", SIGGRAPH 2004.
- Open implementations to study (licences to check before any reuse; LGPLv3 compatibility): Crest (Unity, MIT),
  GodotOceanWaves (MIT).
