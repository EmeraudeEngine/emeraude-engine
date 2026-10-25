---
id: dynamic-sky-moon-relief
title: Dynamic sky — the moon's relief along the terminator (a height or normal map)
status: open
priority: unranked
scope: Graphics/Material (SkyAtmosphereResource), Graphics/Renderable (DynamicSkyResource manifest)
opened: 2026-10-25
tags: [sky, moon, texture, design]
---

# Dynamic sky — the moon's relief along the terminator (a height or normal map)

## Why

The moon's disc is a smooth sphere with an albedo map (graphics doc 42 § "The moons"). Along the terminator the real
Moon shows its relief — crater rims lit, their floors in shadow — which an albedo map cannot give. The first plan of the
disc (the former item `dynamic-sky-moon`) named "an optional normal map".

## What remains

- A second map per moon (manifest `"Moons": [{"Surface": …, "Relief": …}]`): a normal map, or a height map whose
  normals are derived. NASA's SVS CGI Moon Kit (svs.gsfc.nasa.gov/4720) ships a displacement map (LRO LOLA) beside the
  colour map projet-alpha uses.
- The shading normal perturbs μ0 and μ in the lunar reflectance; the disc's silhouette stays the sphere.
- The light: the photometry table (`MoonSurfacePhotometry`) is computed with the smooth sphere; whether the relief
  changes the disc-integrated light measurably (it darkens the crescent a little: shadows) is to be measured before
  deciding to fold it into the table.

## ⚠️ Traps

- Normal maps follow the Khronos convention (+Y up in the image; graphics conventions); an equirectangular normal map's
  tangent frame is east / north.
- At the 0.5° of a real moon the relief is a few pixels: it shows through a long lens only.

## References

- Graphics doc 42 § "The moons"; `SkyAtmosphereResource` (the moons' block), `MoonSurfacePhotometry`.
