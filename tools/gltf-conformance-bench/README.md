# glTF conformance bench

Captures the Khronos [glTF-Sample-Assets](https://github.com/KhronosGroup/glTF-Sample-Assets)
test models through the engine, under the framing each test's README imposes, so the renderer's
glTF 2.0 conformance can be **measured** rather than eyeballed.

The assets are vendored as a partial clone (`blob:none`) of the Khronos corpus. ⚠️ **It is a
submodule of `projet-alpha`, not of the engine** (owner decision, 2026-08-31: test data belongs to
the testbed), so it sits two levels above this script's engine root — and on a workstation where
the engine is *symlinked* into the consumer from a sibling directory, no arithmetic on the resolved
script path can reach it. `asset_directory_candidates()` therefore tries the consumer tree (from
the unresolved path), the resolved engine root's siblings, and finally the engine itself, in that
order; `--assets` overrides the lot.

A second, much smaller tree — `assets/`, next to this script — holds **our own** probes for the gaps
Khronos does not cover; `pick_asset()` searches it first. Every asset in it must be produced by a
versioned generator (today: `make-volume-probe.py`), because a binary blob nobody can regenerate is
worse than no test at all.

## Running it

```sh
# Framing plan only — no engine needed, useful to check the bounds computation.
./bench.py --plan

# Launch the engine WITHOUT a demo, from the build directory.
./projet-alpha --disable-cef &

# Capture everything, or a subset.
./bench.py
./bench.py NormalTangentTest NormalTangentMirrorTest

# One model and its Draco twin (a model of DRACO_MODELS always captures BOTH variants).
./bench.py Avocado

# Everything except the compressed-variant A/B, which doubles the captures it covers.
./bench.py --no-draco
```

Captures and `bench-report.json` land in `./gltf-bench-captures` unless `--out` says otherwise.
A partial run **merges** into an existing report rather than replacing it: the report carries the
bounds and framing distance every later measurement reads back, and a one-model re-capture must not
erase a twenty-minute full run (it did, once).

> [!IMPORTANT]
> **Launch the engine without `--load-demo`.** A running demo scene is never disturbed by the
> dropped-files pipeline: `Core.openFiles()` answers *"a scene is running, the file was ignored"*
> and the bench captures nothing.

> [!WARNING]
> **Portable since 2026-09-22 (macOS run).** A capture path with spaces (`~/Library/Application Support/…`)
> used to be cut at the first space, so every view "never landed on disk". And a session whose settings
> have no `Core/Viewers` section restored nothing, so `Core.shutdown()` saved the last posed environment:
> absent keys are now restored to the engine defaults (`ENVIRONMENT_DEFAULTS`, keep them in sync with
> `SettingKeys.hpp`).

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

## The compressed-variant A/B (Draco, added 2026-09-18)

Sixteen models are captured **twice** — once from the plain glTF, once from its `glTF-Draco`
sibling — and compared per pixel. The table is `DRACO_MODELS` in `bench.py`; `--no-draco` skips
the lot, and a `[draco]` suffix marks the compressed row everywhere (plan, capture filename,
report record).

**It asks a different question from the rest of this bench.** Everywhere else the criterion is
"does the renderer obey the spec", read against a Khronos reference image. Here it is "does the
compressed asset render the *same thing* as its uncompressed source" — there is no reference image,
the reference **is** the other capture.

> [!CAUTION]
> **The framing is computed from the plain asset and reused verbatim for the compressed one.**
> Draco quantisation dilates the bounding sphere by half a quantum — measured at a constant
> **+0.0977 %** on the radius across `Box`, `Avocado`, `BoomBox` and `WaterBottle`. Letting each
> variant compute its own bounds moves the camera by that much, and the A/B then measures the
> camera move instead of the codec. This is why a `[draco]` row in `--plan` shows its twin's radius
> and distance, and why `pick_asset()` **refuses** a missing variant instead of falling back to
> `rglob`: a fallback would hand back the plain file and the A/B would compare an asset with
> itself, reporting a perfect match for a variant that was never loaded.

**How to read the three numbers** (`differingPercent`, `meanAbsDelta`, `maxAbsDelta`, per view in
`bench-report.json` under `dracoDelta`): Draco is **lossy**, so bit-equality is not the criterion
and `annotate_draco_deltas()` deliberately writes **no verdict**. Measured 2026-09-18:

| | differing | mean | max |
|---|---|---|---|
| `Box` (quantised positions land exactly) | **0.0000 %** usually | 0.0000 | 0 |
| `Avocado` (curved, fully textured) | 10.8 % | **0.1737** | 192 |
| `CesiumMan` (skinned) | 3.2 % | **0.1128** | 211 |

> [!NOTE]
> **One LSB is the CAPTURE's noise floor, not a codec signal.** `Box` is the bit-identical case, but
> not reliably: the same run repeated shows an intermittent residual of **max 1/255 on ~0.44 %** of
> its pixels. `BrainStem` carries a comparable ~0.02 % run-to-run wobble in **both** variants. A row
> whose maximum is 1 has nothing to say about the codec.

A **tiny mean with a high maximum confined to silhouettes** is the quantisation signature — that is
a pass. A mean that *moves*, or a maximum spread over a **region** or a **block**, is a defect.
Picking a numeric threshold here would freeze one asset's quantisation grid into a rule for all.

> [!IMPORTANT]
> **The entity count is the one hard criterion, and it is flagged.** The two variants declare the
> same nodes, so a deficit on the compressed side means a primitive failed to decode and vanished
> silently — `dracoEntityMismatch` in the report. It is the same structural control the rest of the
> bench runs, and it is what a per-pixel criterion cannot see.

> [!CAUTION]
> **A KEYSTROKE ON THE WINDOW SILENTLY CORRUPTS A CAPTURE, and the numbers alone will not say so.**
> The bench drives a window that sits on someone's screen. Pressing **space** over it cycles the
> model viewer's animation (`Core::cycleViewerAnimation`, reached from `KeyCode::KeySpace`), so the
> subject is captured mid-stride instead of at rest. `Core.cycleAnimation()` over the console does
> the same. Measured 2026-09-18: one stray space bar during a run took `CesiumMan`'s A/B from a mean
> of **0.10/255 to 9.22**, and `BrainStem`'s to 8.88 — an 80x jump that reads exactly like the
> "mean that moves" signature of a real defect.
>
> Two things make it diagnosable, and both must be used:
> - **`m_viewerAnimationIndex` resets to 0 on every model open**, so only the capture between that
>   keystroke and the next load is affected. In an A/B that means the *plain* capture is corrupted
>   and its `[draco]` twin is not, which is a nonsensical asymmetry — the codec cannot make one
>   variant walk.
> - **The capture SHOWS it**: the overlay reads `Animation 1/1: <clip>` in the corner, and the
>   subject's pose differs. Look at the image before trusting a number that moved.
>
> The cheap confirmation is to compare the SAME variant across two runs: a stable pair and a moving
> one isolates the contaminated capture immediately. `BrainStem` additionally carries a genuine
> ~0.02 % run-to-run non-determinism in **both** variants, so treat that as its floor, not a signal.

> [!IMPORTANT]
> **The A/B compares only the SUBJECT, in a fixed environment — both were needed** (2026-09-18).
>
> The background used to be non-deterministic: the viewer's environment cubemap streams, and the
> scene logs its choice before the texture is resident, so one capture of a pair could be drawn with
> the default and its twin with the landscape. Measured, that moved `RiggedSimple` by **+28.39**/255
> of mean and `CarConcept` by −1.78 while the subject stayed pixel-identical — indistinguishable
> from a codec defect, with the entity count matching so the structural control stayed silent.
>
> ⚠️ **Cropping to the subject does NOT fix that, and it was tried first.** A tight box around a
> slender model is still mostly background, and removing the identical sky only *concentrates* the
> difference: `RiggedSimple` went 28.43 → 38.40. What kills it is having nothing to stream —
> `DRACO_AB_ENVIRONMENT` poses the bit-exact black backdrop (`Background` and `EnvironmentCubemap`
> both `""`) on every A/B row that does not declare an environment of its own. The viewer's key
> light still lights the subject; what disappears is the environment reflection, which a GEOMETRY
> codec test has no business measuring.
>
> The crop is kept for a different reason: the mean now describes the subject instead of being
> diluted over two megapixels of mostly-identical sky. ⚠️ It projects the **AABB**, not the bounding
> sphere — the framing makes every subject subtend the same angle, so a sphere-based box comes out
> the SAME SIZE for every model and leaves a slender subject swimming in background.
>
> ⚠️⚠️ **Numbers from before 2026-09-18 are NOT comparable with these.** The mean is now taken over
> the subject's box and the environment reflection is gone; every figure roughly doubled for that
> reason alone. Do not read the change as a regression.
>
> **Result**: over two consecutive full runs, 23 of 33 rows reproduce to the fourth decimal and the
> largest drift is **0.062**/255, against 28.39 before.

> [!WARNING]
> **Before believing any A/B, check the comparator discriminates.** Two *different* models must
> come out far apart — 58.5 % of pixels and a mean of 95.6/255, measured. A comparison harness that
> silently reports zero is indistinguishable from a perfect match. And make sure the second asset
> has actually **finished loading**: a capture taken mid-swap compares a frame with itself and
> reports a flawless result for entirely the wrong reason (it did, during this work).

> [!CAUTION]
> **A row flagged `textureEncodingsDiffer` is NOT a geometry-codec measurement.** Two Khronos models
> ship their Draco variant with a *different texture encoding* than their plain one —
> `SunglassesKhronos` is PNG against **WebP**, `CarConcept` PNG against **KTX2** — so their delta
> carries two codecs at once. The bench detects this by reading both assets' image encodings (`jpg`
> and `jpeg` normalised: without that, three more models look confounded and are not) and says so in
> the run and in the report. The rows are kept because the LOAD is worth exercising; only the pixel
> comparison is confounded. ⚠️ This also means the `CarConcept` figure published on 2026-09-18
> (mean 0.22 / 0.49) was never a Draco number.

> [!NOTE]
> **A run is no longer sensitive to what the engine loaded before it — since 2026-09-18.** It used
> to be: the glTF resource prefix was keyed on the file *stem*, so both variants of a model shared
> one resource namespace and the second load served the first's cache. Hand-loading a variant with
> `Core.openFiles()` and then benching it in the same session reported **98.16 % of pixels, mean
> 39.28/255** for `SunglassesKhronos`, against **6.35 % / 0.10** from a clean session. The prefix is
> now derived from the whole path, and that contaminated scenario reproduces 6.3451 % exactly.
> Recorded because the shape recurs: **a defect that only moves a number, with nothing visibly
> broken, is the hardest kind to catch — and a bench is precisely where it surfaces first.**

**`DRACO_BLOCKED` is empty since 2026-09-18** — `SunglassesKhronos` was its only occupant, blocked
by `EXT_texture_webp` rather than by anything to do with Draco, and that extension is now supported.
The table stays: a variant that cannot be benched belongs in it **with its reason**, because an
absent row reads as "never tried", which is not the same claim.

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
- **⚠️ `TransmissionRoughnessTest` is the refraction criterion — read it by ROW, not by column.**
  It is a GRID: the **IOR varies by row** (2.42 Diamond / 1.76 Sapphire / 1.50 Glass / 1.33 Water /
  1.00 Air, top to bottom, labelled on the asset itself) and the roughness by column. Fitting runs
  to a diff mask finds the nine ROUGHNESS columns and tells you nothing about refraction — partition
  by row instead, from the label geometry.
  Its value is that **IOR 1.0 is a physical zero**: at `eta = 1` a refracted ray is unbent, it stays
  on the camera ray, and a perspective projection maps every point of a camera ray to the same pixel,
  so the screen displacement is exactly nought. A correct screen-space refraction therefore produces
  a monotone ladder ending in a collapse — measured after the Aug 2026 rewrite, changed pixels per
  row: **12964 / 12659 / 11981 / 10487 / 1615**. The residue on the last row is the silhouette, where
  `dot(N, I) > 0`. A criterion that does NOT collapse on the bottom row means the displacement no
  longer follows the IOR.
  Its companion control is `TransmissionTest`, whose spheres are **thin-walled** (no
  `KHR_materials_volume`, so `thicknessFactor` 0): any change to the refraction ray must leave it
  **bit-exact**, and it did (max diff 0.0).
- **⚠️ `VolumeAbsorptionProbe` is OURS, not Khronos'**, and it is the only asset here that
  exercises `KHR_materials_volume` at all. Of the 60 glTF files in reach, 15 materials declare the
  extension, 2 declare an `attenuationColor` and **zero** declare an `attenuationDistance` — whose
  default is **+infinity** — so the absorption of every Khronos model is the identity whether the
  feature works or not. Three identical transmissive balls differing only in the volume they
  declare; the control lives inside the single capture, which beats a before/after because it
  cannot drift. Read the share of disc pixels where green exceeds red by more than 25/255:
  **0.0 % / 0.0 % / 34.2 %** left to right. ⚠️ The MIDDLE ball being 0.0 % is the point — a colour
  *without* a distance must NOT tint, and that is the rule a well-meaning fix would break.
  Regenerate with `./make-volume-probe.py`; never hand-drop a binary nobody can rebuild.
  ⚠️ Those percentages are the OLD reading and no longer apply. **The live criterion since
  2026-08-29 is the G/R ratio at each ball's centre, against the backdrop the asset now carries:**

  | ball | centre RGB | G/R |
  |---|---|---|
  | `NoVolume` | (198.5, 197.1, 194.0) | **0.993** |
  | `ColourNoDistance` | (198.5, 197.1, 194.0) | **0.993** |
  | `ColourAndDistance` | (4.6, 67.7, 7.5) | **14.717** |

  The backdrop reads (201, 200, 196), so the two neutral balls transmit it essentially intact and are
  **identical to each other** — that middle ball staying neutral is still the point of the asset. The
  third keeps ~13 % of the green and almost none of the red or blue, which is `0.6^4` as the medium
  prescribes. A regression shows as the third ball's ratio collapsing toward 1, or — worse — as the
  middle ball drifting away from the first.

  ⚠️⚠️ Getting there took two fixes, and the second was in the ASSET: its generated spheres were
  wound the wrong way round. A closed convex mesh looks identical either way, but the outward normal
  then faces away from the camera, `NdotV` clamps to 0 and the Fresnel term pins at 1 — total
  reflection, zero transmission, whatever the transmission path does. See `docs/caution-points.md`
  § "Inverted triangle winding is INVISIBLE on a closed mesh".
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
- **⚠️ The viewer's environment is CONFIGURABLE, and five of these tests need it changed.** Khronos
  shoots `SheenCloth`, `AnisotropyStrengthTest` and both iridescence grids on black, and
  `SpecularTest` against a bright environment; `+ModelViewer` meters a daylight sky at sunny-sixteen.
  Three settings, and the first two are **independent axes** — what is behind the subject, and what
  the subject reflects:
  `Core/Viewers/Background` (empty = a bit-exact black backdrop), `Core/Viewers/EnvironmentCubemap`
  (empty = whatever the background installed) and `Core/Viewers/AmbientIlluminance` (80.72 lux
  delivered by default — renamed 2026-09-25 from `AmbientIntensity` = 200 × a dimming colour — and that flat
  ambient washes out a sheen rim).
  Measured on `SheenCloth`: rim-to-backdrop contrast **8.1× with the defaults, 390× with
  `Background = ""` and `AmbientIlluminance = 0`** — a 48× gain, backdrop exactly (0,0,0).
  ⚠️ **Black is not the universal answer**: `SpecularTest` is too DARK, not washed out (F0 ≤ 0.04 on
  a black dielectric), and needs a bright *reflected* environment instead. Read what a test declares
  before choosing its environment.
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
- **The near plane WAS hard-coded** at `0.1 / sqrt(1 + tan²(fov/2)·(aspect²+1))` ≈ 0.089 m whatever
  the scene scale, in four copies. Sub-decimetre assets sat inside it and rendered nothing; the bench
  pushed the camera out and reported the frame coverage it lost rather than saving an empty capture —
  `MetalRoughSpheresNoTextures` capped at **5.5 %** of frame height, `BoomBox` at **14.1 %**. Since
  2026-08-28 the engine derives it from the subject and both plan at the nominal **38.9 %**.
  ⚠️ The clamp in `clamp_distance()` is KEPT as a guard and made subject-relative: a `*` in the plan
  output now means the engine's rule and this script have drifted apart, not that an asset is small.
  ⚠️ And the near plane alone was not enough — `+ModelViewer` held two more decimetre floors (the
  framing radius, and the orbit controller's lower distance limit, which `setDistance()` clamps
  against). **One scale floor is never alone.**

## Files

| file | role |
|---|---|
| `bench.py` | the driver: framing plan, camera placement, capture loop |
| `gltf_bounds.py` | world-space bounds of a glTF/GLB by walking the node hierarchy with transforms |
| `png_compare.py` | dependency-free PNG decode + per-pixel comparison, for the compressed-variant A/B (numpy is used when importable, as a speed-up only) |
| `../emeraude_console.py` | the shared remote-console client |
## Traps the 2026-09-14 re-judgement added

- **⚠️⚠️ A missing object shows you the object BEHIND it.** Probing the iridescence grids
  sphere-by-sphere reported the seat-less half as *saturated and correlated with its declared film
  thickness* — the exact opposite of the truth, which is that those 197 spheres are not in the scene
  at all. In a 7×7×7 grid almost every cell has another cell behind it, so the probe was reading a
  neighbour. Restricting the reading to **isolated** spheres (nothing of the lattice in front of or
  behind, from the projected discs) settled it in one pass: with a seat, luminance 203…216; without,
  19.8…31.1, against a background of 16.7…32.7. **Prove nothing else can be under a pixel before you
  read it.**
- **⚠️⚠️ In an outdoor environment, a sphere's brightest pixels are an IMAGE OF THE SKY, not a BRDF
  lobe.** The highlight-elongation metric for `AnisotropyStrengthTest` — the shape metric this bench
  has wanted for three runs — came back as pure noise (1.23 … 6.82, no structure, the anisotropy-0
  row no more isotropic than the rest). Five metrics have now been confounded on this test, and the
  lesson is that the metric was never the problem: no shape metric can work while the reflected
  environment is a landscape. **Pose the test in a dark environment with a distinct source, then
  measure.** Same root cause blocks `SpecularTest` (too dark) and `SheenCloth` (washed out).
- **⚠️ A listing you truncated is not a listing.** This section first claimed
  `Core.SettingsService` had **no `set`** — and therefore that a test could not be posed in its own
  environment at all. False: `set(key, value)` has been there since the console was unified. The
  "evidence" was a `help` dump piped through `grep | head`, and `set` sorts one line past `save`,
  i.e. exactly where the default `head -10` cut. **Use `<path>.lsfunc()` to enumerate a level's
  commands, and never conclude a capability is missing from a truncated pipe.** The bench now poses
  a per-test environment through that command (`ENVIRONMENTS` in `bench.py`) and restores the
  session's values afterwards.
- **⚠️ Always restore what you posed.** `Core.shutdown()` *saves the settings on the way out*, so a
  value the bench leaves behind lands in the user's `settings.json` permanently. `run_bench()` reads
  the session's three viewer keys before touching anything and puts them back at the end, and also
  between models, so one test's environment never lands on another's capture.
- **⚠️ `ModelViewer` can miss the extents and frame by fallback.** `TextureTransformTest` logged
  *"The imported content published no extents in time, using the fallback framing"* on both
  attempts — reproducible. The warning is in the engine log and nowhere in `bench-report.json`:
  **read the engine log for that line before trusting a capture's framing.**
- **⚠️ `OrientationTest` cannot be judged from an axis view.** The six axis views each show an arrow
  but never the target it must point at — the targets sit on the far rim, out of frame. It now gets
  `three-qtr` and `three-qtr-rear`, which show the RGB (quaternion) and CMY (matrix) sets with their
  targets, the way its own reference screenshot does.
- **⚠️ The plan's `coverage~38.9 %` is stale arithmetic**; the real subtended height at
  `DISTANCE_FACTOR = 5.142` is ~83 %, which is what the captures show. Do not conclude anything about
  framing from that column.
