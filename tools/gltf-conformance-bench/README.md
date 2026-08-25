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

- **The framing is calculated, never guessed.** Bounds come from walking the glTF node hierarchy
  *with* its transformations (`gltf_bounds.py`); the mesh bbox alone lies —
  `MetalRoughSpheresNoTextures` is one sphere instanced 123 times, and its mesh bbox is ~0.
- **Bit-exact `(0,0,0)` and "crushed by the exposure" are different bugs.** A black frame is not
  proof that something failed to load. Zero exactly means never drawn; small non-zero values mean
  drawn and under-exposed.
- **The hue of a near-grey pixel is noise.** The iridescence grids report a 130° hue spread at a
  saturation of 0.03. Report the saturation.
- **A control exonerates the instrument.** Before blaming a material, capture `MetalRoughSpheres`
  in the same run: if its mirror-to-diffuse progression reads correctly, the IBL and the exposure
  are not the cause.
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