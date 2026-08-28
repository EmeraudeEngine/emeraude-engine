# glTF conformance bench

Captures the Khronos [glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)
test models through the engine, under the framing each test's README imposes, so the renderer's
glTF 2.0 conformance can be **measured** rather than eyeballed.

The assets are vendored as a sparse checkout under `dependencies/glTF-Sample-Assets`.

## Running it

```sh
# Framing plan only — no engine needed, useful to check the bounds computation.
./bench.py --plan

# Launch the engine WITHOUT a demo, from the build directory.
./projet-alpha --disable-cef &

# Capture everything, or a subset.
./bench.py
./bench.py NormalTangentTest NormalTangentMirrorTest
```

Captures and `bench-report.json` land in `./gltf-bench-captures` unless `--out` says otherwise.

> [!IMPORTANT]
> **Launch the engine without `--load-demo`.** A running demo scene is never disturbed by the
> dropped-files pipeline: `Core.openFiles()` answers *"a scene is running, the file was ignored"*
> and the bench captures nothing.

## What the harness is

The engine's own `+ModelViewer` scene, opened through `Core.openFiles()`. It brings the three
things a conformance capture needs and that a demo scene cannot give:

| | why it matters |
|---|---|
| Framing from the real world extents | the models span a factor of **3800** in radius |
| **Manual** sunny-sixteen exposure | auto-exposure meters the whole frame — any measurement on it is a claim about the SENSOR, not about the material |
| A sky (`Core/Viewers/Background`) | a reflective, transmissive, clearcoat, sheen or iridescent material has nothing to reflect without one |

The camera is then placed explicitly on the `ViewerCamera` node with `setNodePosition` /
`setNodeLookAt`; the orbit controller only reclaims the node on a pointer event, which never
happens over TCP.

## Reading the results

**This script captures; it does not judge.** Each Khronos model ships a `README.md` giving the
literal pass criterion and the views to use, and — far more useful — its **named failure images**:

```
NormalTangentTest/screenshot/incorrect-flipped-y.png
NormalTangentMirrorTest/screenshot/supplied-tangents-ignored.png
OrientationTest/screenshot/OrientationTestFail.png
AlphaBlendModeTest/screenshot/BlendFail.jpg, OpaqueFail.jpg, PremultipliedAlphaFail.jpg
SpecularTest/screenshot/purple.jpg
```

Comparing a capture against `screenshot.png` alone tells you something is wrong but never *what*.
The failure images name the defect, which is what turns a capture into a fix.

## Traps this bench has already paid for

- **⚠️⚠️ Duplicate names in an asset are the norm, and they used to collapse whole grids.**
  Neither glTF nor FBX imposes uniqueness on names; the identity of a mesh or a material is its
  INDEX. Until 2026-08-28 the loaders keyed their resources on the name, so the second mesh named
  `Sphere` received the first one's geometry AND material, silently. `ClearCoatTest` ships
  **eighteen meshes named `ClearCoatSampleMesh`** with eighteen different materials and rendered as
  eighteen copies of material 0; `MetalRoughSpheresNoTextures` has `Sphere` ×98, `SpecularTest`
  `OneSample` ×20, `TransmissionTest` `Sphere` ×12 plus three different materials all named
  `BlueTransWithMask`, `TransmissionRoughnessTest` two different images both named `RoughnessGrid`.
  **It reads exactly like an un-wired extension**, and three bench runs charged it to
  `KHR_materials_*`. When a whole grid renders as one cell, dump the asset's names before touching
  a shader: `python3 -c "import json;a=json.load(open(f));print([m.get('name') for m in a['meshes']])"`.
  Fixed in the engine (`Scenes::Loaders::buildResourceKey()`); the assets did not change.
- **⚠️ Verify a loader-wide change with a PIXEL DIFF against the previous run, partitioned by the
  property you changed.** The captures of a previous pass are kept under
  `~/.local/share/LNIsle/projet-alpha/captures/bench-gltf-<date>/`, and the framing is
  deterministic, so a full-frame diff is available and it is far stronger than re-reading
  individual captures. For the 2026-08-28 resource-key fix it gave: models with **unique** names,
  32 captures, max delta **≤ 2 / 255** (ordinary temporal dither) — the change is a bit-exact
  **no-op** where it must be; models with **duplicate** names, 12 captures, max delta **243 / 255**
  and up to 15.1 % of pixels. Change if and only if predicted. Without that partition, "13 % of
  the frame changed" says nothing about whether the change was the intended one.
- **⚠️ Never sample a small grid on guessed pixel coordinates.** On
  `MetalRoughSpheresNoTextures` (5.5 % of frame height) a guessed 7×7 window reported a per-cell
  std of 51.97 on a capture where all 98 spheres were provably identical — the window had drifted
  into the sky, and the number was plausible enough to publish. Fit the lattice to the image
  first: high-pass (subtract a Gaussian blur), then brute-force the pitch and offset that maximise
  the row and column profiles.
- **The framing is calculated, never guessed.** Bounds come from walking the glTF node hierarchy
  *with* its transformations (`gltf_bounds.py`); the mesh bbox alone lies —
  `MetalRoughSpheresNoTextures` is one sphere instanced 123 times, and its mesh bbox is ~0.
- **Bit-exact `(0,0,0)` and "crushed by the exposure" are different bugs.** A black frame is not
  proof that something failed to load. Zero exactly means never drawn; small non-zero values mean
  drawn and under-exposed.
- **The hue of a near-grey pixel is noise.** The iridescence grids report a 130° hue spread at a
  saturation of 0.03. Report the saturation.
- **A control exonerates the instrument — and nothing else.** Before blaming a material, capture
  `MetalRoughSpheres` in the same run: if its mirror-to-diffuse progression reads correctly, the
  IBL and the exposure are not the cause. That is *all* it says. It carries a **single** material,
  so two bench runs used it to close questions it cannot answer — per-material data reaching the
  shader among them. Read what a control declares before letting it clear anything.
- **A reference screenshot shot in a dark studio is not a criterion.** `EmissiveStrengthTest` looks
  like a failure next to its reference — none of the cubes glows — purely because `+ModelViewer`
  meters a daylight sky at sunny-sixteen. On the RATIOS between cubes it passes cleanly (2.21,
  2.28, 1.97, 1.59 against a declared 1-2-4-8-16 doubling, the last compressed by the tone curve).
  The same caveat applies to `SheenCloth`, `AnisotropyStrengthTest` and both iridescence grids,
  which Khronos all shoot on black.
- **Variation along a test's axis is not proof the axis works.** The anisotropy grid varies 14.3 %
  down its anisotropy axis at roughness 0 and 2.8 % at roughness 1 — exactly what a sky gradient
  over spheres at different heights produces, with no anisotropy at all. Measure the highlight
  SHAPE (aspect ratio of the brightest pixels), not its level: reference 3.65 → 1.62 across the
  axis, ours 3.29 → 2.67.
- **The iridescence models are 3-D grids of spheres.** A dead-on `front` view collapses them into
  overlapping rows and reads nothing. They need a three-quarter view, as their reference does.
- **`AlphaBlendModeTest` declares itself conforming even when it is not.** Its on-model check
  marks turn green because the red "X" decal has zero alpha and is discarded. Never read the
  ticks — measure the alpha ramp.
- **The screenshot filename is a one-second timestamp.** Two captures inside the same second
  collide, so every capture is copied out immediately from the reported path.
- **The near plane is hard-coded** at `0.1 / sqrt(1 + tan²(fov/2)·(aspect²+1))` ≈ 0.089 m
  whatever the scene scale (`ViewMatrices2DUBO.cpp`). Sub-decimetre assets sit inside it at their
  computed framing distance and render nothing; the bench pushes the camera out and reports the
  frame coverage it loses rather than saving an empty capture.

## Files

| file | role |
|---|---|
| `bench.py` | the driver: framing plan, camera placement, capture loop |
| `gltf_bounds.py` | world-space bounds of a glTF/GLB by walking the node hierarchy with transforms |
| `../emeraude_console.py` | the shared remote-console client |