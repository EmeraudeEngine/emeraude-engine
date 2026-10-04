## 12. Post-Processing Effects

### Background photometric contract — the sky is a light source, authored in the asset (Jul 2026)

A sky EMITS, so it is described by a **luminance in nits**, and that value spans seven orders of
magnitude between noon and midnight: `DaylightSkyLuminance` 8000, `OvercastSkyLuminance` 2000,
`TwilightSkyLuminance` 10, `MoonlitNightSkyLuminance` 1 (`Graphics/Renderable/SkyBoxResource.hpp`).
A background manifest (store `Backgrounds`) declares the FULL photometric description of the sky:

```json
{
	"Cubemap": "BlueSky",
	"Luminance": 8000.0,
	"AverageColor": [0.35, 0.55, 0.85],
	"AmbientIlluminance": 20000.0,
	"Stars": [
		{ "Type": "Sun", "Direction": [0.35, -0.55, 0.63], "Illuminance": 100000.0,
		  "Temperature": 5500, "AngularDiameter": 0.53, "InTexture": true }
	]
}
```

- `Cubemap` (required): resource name in the `Cubemaps` store (the key REPLACED `"Texture"`, Jul 2026).
- `Luminance` (nits, default 8000 = clear day): the emission scale of the LDR source — a night
  cubemap and a noon cubemap are not the same photometric object.
- `AverageColor` (sRGB, optional): authored/cheated average; absent = computed from the source
  (`CubemapResource::averageColor()`).
- `AmbientIlluminance` (lux, optional): what the dome pours on the ground; absent = derived as
  `E = L × factor` where the factor is MEASURED on the actual texels
  (`CubemapResource::hemisphereIlluminanceFactor()`: sRGB-decoded luma × cos(zenith) integrated
  over the sky hemisphere — π only for a uniform dome; Backrooms measures 0.11, its dome is
  mostly dark ceiling). Owner decision (review session Jul 2026): the uniform-dome π over-lit
  every non-uniform sky. HDR sources are calibrated to exactly π by construction.
- `Stars` (optional, 0..N): celestial bodies (`Graphics::CelestialBody`) to derive analytic
  directional lights from — `Direction` points TOWARD the body (engine frame, UP = +Y),
  `Illuminance` in lux, `Temperature` in kelvins (industry-standard authoring; wins over a direct
  `"Color"` when both are present) resolved via `Photometry::colorFromTemperature()` (Planckian
  locus), `AngularDiameter` in degrees, `InTexture` = anti-double-counting flag — **STRUCTURING
  since 2026-09-13**: the brightest `InTexture` body is MASKED OUT of the IBL bake (irradiance and
  prefiltered), a cone of one angular diameter around its direction that the bake reads at the rim
  (`IBLBaker::StarMask`, GLSL `maskStar()`, widened by the footprint of the source mip being read so
  the coarse mips cannot leak the body back in). The analytic star carries that energy with shadows;
  the visible skybox keeps the body. On Kloppenheim 05 the in-texture sun is 80 % of the ground
  illuminance — unmasked it was a second, unshadowed sun. The ray-traced lanes (RTGI, irradiance
  probes) apply the same mask to their raw-cubemap sky term since 2026-10-05 (`emMaskStar()`,
  `Effects/Shared/StarMaskGLSL.hpp`): without it they flashed. Zero stars is legitimate: pure ambiance
  (overcast, nebula, cave).
- ⚠️⚠️ **`Direction` is in the WORLD frame, UP = +Y — every store body was MEASURED in its picture on
  2026-09-25** (`tools/sky-manifest.py --locate`, owner: "corriger tout ce bordel"). Until then 13
  manifests carried the Y-DOWN values of July 2026: their entity landed below the ground, the star
  shone upward (no ground shadow on `basic-scenery`), and the IBL mask sat on empty sky while the
  painted body stayed in the bake. Negating Y would NOT have been enough: `StormyDays` would have
  put its sun at 34° for a disc painted at 17.8°, and `ViolentDays`, `Miramar`, `Interstellar` were
  wrong in azimuth too (up to 138°). `IndustrialSunsetPureSky` had a painted sunset sun and NO
  star (added: 920 lx, 2400 K, anchored on its sky, see the tool bullet). Which estimate was kept
  per sky (clipped plateau, unclipped disc, half-hidden glow, the author's intent where nothing is
  painted): `docs/caution-points.md` § *The sky manifests put their bodies where the pictures do not*.
- **Authoring an HDR manifest is a MEASUREMENT, not a guess: `tools/sky-manifest.py`** (Sep 2026).
  It reads the equirectangular `.hdr`, finds the sun (brightest texel, radiance-weighted centroid),
  prints its direction in the engine frame and its energy profile, integrates the sky-only and total
  horizontal illuminances, and anchors everything on ONE declared value — the sun's illuminance in
  lux (`--sun-illuminance 100000`): `Luminance = k · E_h,total / π` (what makes the loader's
  dome-of-1 calibration reproduce the picture's absolute level), `AmbientIlluminance = k · E_h,sky-only`
  (the dome without the sun the analytic star carries), `Stars[0]` = the measured sun, `InTexture`.
  Reference run: `Kloppenheim05` (Poly Haven, the Intel Sponza 2022 sky) → `Luminance` 36 910 nits,
  `AmbientIlluminance` 19 590 lx, sun at 74.5° elevation, direction (0.216, 0.964, 0.157), 97.6 % of
  its excess energy within 0.5° — a compact veiled disc, hence the one-diameter mask.
  ⚠️ **A veiled sun cannot be the anchor**: read the `sun/sky` ratio first. `IndustrialSunsetPureSky`'s
  sun carries 0.4 % of the picture's horizontal illuminance, and 10 000 lx on it gave a 52 000-nit sky
  with 163 000 lx of ambient; `--sky-luminance NITS` anchors on the sky instead and DERIVES the sun
  (4 800 nits → 920 lx).
  **`--locate NAME --store DIR [--around AZ,EL] [--preview out.png]`** measures where the body of ANY
  store sky is painted (packed cube or equirectangular, LDR or HDR) and compares it with the manifest.
  An LDR sun is a CLIPPED plateau whose outline follows the clouds: the centre of its largest
  inscribed disc is kept, not a centroid (a lit cloud edge hanging off `StormyDays`' disc dragged the
  centroid 4-7° up, with the threshold); `--around` adds the centroid of an UNCLIPPED disc (a moon) and the peak of the
  glow smoothed over 2° (a sun half hidden by clouds). The packed-cube face convention of the tool was
  verified in the engine: aimed at the measured `StormyDays` direction, the painted sun sits on the
  screen centre.

Parsing is CENTRALIZED in `AbstractBackground::parsePhotometry()` — every background type
(`SkyBoxResource`, future `DynamicSkyResource`, `ColorBackgroundResource`) goes through it.

**HDR skies (Jul 2026)**: a Cubemaps manifest with `"FileFormat": "hdr"` + `"Equirectangular"`
loads a Radiance RGBE source through the float pipeline (`CubemapResource::loadEquirectangularHDR`):
- **D6 calibration (Unity-like)**: an HDRI is RELATIVE; the loader measures the illuminance its
  upper hemisphere pours on the ground and normalizes so scale 1 = uniform dome of luminance 1
  (E = pi lux). The Background `Luminance` key then means EXACTLY the same thing for LDR and HDR,
  and the unclipped sun keeps its full relative punch (clamped to 65504, the half-float maximum —
  real blinding specular reflections).
- Faces are stored as raw **RGBA16F** texels (`hdrFaceData()`, `isHDR()`; `faces()` stays empty)
  and uploaded as `VK_FORMAT_R16G16B16A16_SFLOAT` (guaranteed filterable, half the VRAM of 32F).
- ⚠️ **`Base::PixelFactory::Color< float > CLAMPS to [0,1] on construction** — every HDR data
  path must stay on RAW floats (`sampleEquirectangularHDR()` bypasses `linearSample`/`pixel()`).
- ⚠️ **`Vulkan::Image::pixelBytes()` must know the texel format**: it drives the per-layer buffer
  offsets of the cubemap upload. Its missing 16F entries scrambled the faces of the first HDR sky
  (fixed) — extend it whenever a new texel format enters the engine.
- Debug: compile with `EMERAUDE_DEBUG_HDR_FACES` to dump tonemapped face PNGs to `/tmp` at load.
- The average color is computed at load from the calibrated radiances (`averageColor()` override).
- LDR-only consumers (`CubemapMovieResource`) explicitly reject HDR cubemaps.

**The scene consumes this in two ways:**
1. **Always** (automatic): the luminance scales every IBL contribution. It travels through the
   View UBO (`UniformBlock::Component::EnvironmentLuminance`, pushed by
   `Scene::refreshAmbientLightProperties()` to ALL render targets — main, views, textures — at
   LightSet init, on `setBackground()`, and when an async background finishes loading). The
   shader reads it via `LightGenerator::scaledIBLIntensity()`; it is a UNIFORM, not a baked
   literal, so it can change at runtime (day/night) without regenerating programs.
2. **Opt-in**: `Scene::applyBackgroundLighting(options)` derives the scene lighting — LightSet
   ambient = `AmbientIlluminance` in the hue of `AverageColor` (a unit-luminance chromaticity since 2026-09-25 —
   the colour no longer takes its luminance off the illuminance), one `DirectionalLight` per star (the first
   becomes the main directional light). Shadow mapping is NOT photometric data: it comes from
   `BackgroundLightingOptions` (classic map or CSM). Deferred automatically while the background
   resource is still loading (observer on `LoadFinished`). Scene JSON: `"ApplyLighting": true`
   in the `Background` block. Full manual = don't call it.

⚠️⚠️ **The luminance drives TWO consumers and both must hear about it**: the material's emission
(what you see looking up) AND the IBL scale above. Historically the IBL scale sat on its 8000-nit
daylight default in every scene: a material reflecting 3% of its environment received 240 nits
against 0.1 nit of moonlit diffuse, 2400x too much — Citadel's stone walls read as white neon, and
the relief detail modulating that clipped signal was mistakable for a broken normal map. Fixing
only the material would have corrected what the sky LOOKS like while leaving everything it LIGHTS
wrong.
⚠️ The luminance is also part of the sky material's IDENTITY (it is in the resource name): two
manifests sharing one cubemap at different luminances must not share a material.
⚠️ A copy-paste default of `Roughness 0.5` + `Reflection Automatic 0.1` exists in 54 of the 3917
material manifests; they were all amplifying the IBL the same way. Correct by construction now that
the scale is right, but their satin roughness is still worth reviewing surface by surface.

**Camera presets** (`Scenes/EffectsToolkit/CameraPresets.{hpp,cpp}`): full photographic
packages — optics + exposure + DoF/HDR materialization + lens effects in one call.
`Neutral` (reset), `HighQuality` (f/2.8 full frame, clean), `HumanEye` (f/8, soft peripheral
vignette), `VintageBlackAndWhite` (f/5.6 Super 35 + StylePresets::Hitchcock60s stack),
`Super8` (f/1.9 Super 8 gate, +0.3 EV, coarse grain/jitter/flicker/dust). Applying a preset
REPLACES the camera's photographic setup; two cameras can carry different presets
(active-camera switch = full look switch). Validated on Sponza (Jul 2026).

⚠️ **A style declares a FORMAT, not a lens, and NEVER steals the framing** (owner decision,
Jul 2026, once the focal length started driving the field of view). `CameraStyle::sensorWidth`
replaced `focalLength`, and applying a style reads the current field of view, mounts the format,
then re-derives the EQUIVALENT focal length — the same move a director of photography makes when
changing stock. The shot belongs to whoever placed the camera; what the format changes is the
optical CHARACTER, since the circle of confusion scales as the focal length squared over the
format width. Hence `FullFrameFormat` 36 mm, `Super35Format` 24.89, `BroadcastFormat` 8.8 (2/3"
tube), `CamcorderFormat` 6.4 (1/2"), `Super8Format` 5.79 — and that is *why* 1980s broadcast video
and Super 8 look flat while a full-frame prime separates its subject. This also deleted twelve
arbitrary focal-length values. ⚠️ A style is REFUSED on a technical camera (`isStyleable()` warns):
a cubemap face has no format, no lens and no grading.

**EXTENSION CONTRACT — consumer-defined styles** (`EffectsToolkit::CameraStyle`): an
engine consumer declares its own photographic style as a DATA block (optics, exposure,
DoF/HDR flags, and a lens-stack FACTORY — fresh effect instances per application, no
state sharing between cameras). Usable two ways: `CameraPresets::Apply(camera, style)`
directly (unlimited ad-hoc styles), or registered once behind the `CameraPreset::Custom`
token via `CameraPresets::setCustomStyle(style)` (then usable at Toolkit creation and in
runtime cycles; unset token falls back to Neutral with a warning). The factory may return
effect classes OWNED BY THE APPLICATION (subclass `DirectPostProcessEffect`, override
`generateFragmentShaderCode()` — the whole surface is public/EMEN_API): validated with
projet-alpha's `BitmapMonochromeEffect` (1-bit ordered-dither, its own GLSL — the engine
never knows the type). NOTE: a style with no HDR feeds RAW LINEAR values to the lens
effects — threshold-like effects usually want `HDR = true` so the auto-exposure
normalizes their input.

**Preset TOKEN at creation** (owner-decided idiom): the preset is part of the camera
DEFINITION — `enum class EffectsToolkit::CameraPreset` (13 values: Normal, HighQuality,
HumanEye, VintageBlackAndWhite, Super8, plus the PROMOTED StylePresets catalog —
Analog80s, VHSAnalog80s, SatelliteAnalog80s, VHSPureSignal, SatellitePureSignal,
GoldenHour, BlueHour, Retro8Bits — each with era-consistent optics: video/broadcast =
deep focus, cinema grades = photographic DoF; Retro8Bits keeps its tone mapping since 2026-09-26 —
"no photometry" rendered a raw luminance clipped to white). Taken by
`Toolkit::generatePerspectiveCamera(..., preset = CameraPreset::Normal)` (perspective
only: the thin-lens DoF model is meaningless under orthographic projection; cubemap
capture cameras are never graded). Runtime re-application goes through
`CameraPresets::Apply(camera, token)` — demo cycle order IS the enum order.
StylePresets:: functions remain the lens-stack building blocks.

**Lens effects run on DISPLAY-ENCODED values** — after the tone mapping, which applies its 1/2.2 before
writing to the UNORM swapchain — so `ColorGrading`'s contrast pivots on 0.5 and its `pow` gamma are in
the right domain. ⚠️⚠️ **A warm or cool look is a WHITE BALANCE, never a hue rotation** (2026-09-26):
`ColorGrading::setWhiteBalance(kelvin, tint)` grades the image as if lit by a black body at `kelvin`
(6500 K exactly neutral, `Photometry::linearColorFromTemperature()` gains divided by the 6500 K ones,
luminance-normalized, applied in LINEAR light). `setHue()` ROTATES every hue by the same angle and adds
no orange: every "warm" style of the catalogue used it and came out green or magenta — Golden Hour
turned the sky and the clouds green-cyan. The contrast/brightness step ends on a SOFT SHOULDER (knee
0.9, continuous value and slope, asymptote 1) instead of a hard clamp at 1. Measured audit and the
re-calibrated values: `docs/caution-points.md` § *the camera styles' warm grades*.
⚠️ When measuring a style's tint, a DESATURATING style (VHS, B&W) reads "magenta" on a green scene
only because the scene's green dominance shrinks: judge the hue on a NEUTRAL surface (a cloud).

**Camera API** (all no-op when the matching effect is absent — the options are retained):
- `enableDepthOfField(bool)` / `enableMotionBlur(bool)` / `enableBloom(bool)` / `enableHDR(bool)`
  — MATERIALIZE the DepthOfField / MotionBlur / VeilingGlare / ToneMapping effect in the scene chain (and
  remove it when disabled). ⚠️ **Canonical order: DepthOfField → MotionBlur → VeilingGlare → ToneMapping**,
  which is the physical order of events: the optics form the image, the motion smears during the
  exposure, the glass scatters what was formed, the sensor responds. All four insert themselves
  ahead of the first `runsAfterToneMapping()` effect.
- ⚠️ **The motion blur has no strength knob, by design**: its length is `setShutterSpeed()` divided
  by the frame duration — the shutter angle, i.e. the fraction of the frame during which light was
  collected (1/48 s at 24 fps is the cinematic 180-degree rule). That is what makes it
  framerate-independent, and it means the exposure time is a SHARED control: it sets the blur AND
  one third of the exposure triad. A demo no longer adds `MotionBlur` to its stack; the quality
  knobs (`Core/Graphics/PostProcessing/MotionBlur/SampleCount`, `SoftDepthExtent`) are read by the effect itself,
  like the depth of field's.
- Optics: `setAperture(fStop)`, `setFocalLength(mm)`, `setFocusDistance(m)` (implies
  manual focus, like tapping to focus), `setAutoFocus(bool)`.
- ⚠️⚠️ **THE FIELD OF VIEW IS NOT SETTABLE — it is derived.** `m_focalLength` (+ `m_sensorWidth`)
  is the SINGLE source of truth for the framing; `fieldOfView()` computes `2·atan(h/(2f))` on
  demand, `h` being `sensorHeight()` (`sensorWidth × 2/3`, 3:2 full frame) because the engine's
  field of view is VERTICAL. A camera is configured **like a real appliance** — a lens, a format,
  an aperture, a shutter, an ISO — and the projection matrices follow. `setFieldOfView()` and
  `changeFieldOfView()` **no longer exist**, `setPerspectiveProjection(distance)` no longer takes
  an angle, and `AnimationID::FieldOfView` is gone (animate `FocalLength`: a zoom IS a focal ramp).
  Reference points: 13.096 mm = the historical 85° default, 12 mm = 90°, 20.8 = 60°, 25.7 = 50°,
  50 mm = 27°.
  **Why it was done** (owner, Jul 2026): storing both an angle and a lens meant FOUR writers, and
  one of them — `setPerspectiveProjection(fov, distance)` — updated only the angle and left a stale
  focal length behind, so the panel reported a lens that did not match the image. Deriving removes
  the class of bug rather than one instance of it. Side effect: the 1 mm focal floor caps the
  derived angle at ~171°, replacing a clamp that allowed a geometrically meaningless 360°.
- An ultra-wide has enormous depth of field (the circle of confusion goes as f²), so a game-style
  framing yields a subtle DoF whatever the aperture — that is optics, not a weak effect. Visible
  background separation needs a longer lens, not a smaller f-number.
- `setSensorWidth(mm)` is the FORMAT knob, and the reference length that gives millimetres a
  meaning: it converts the thin-lens circle of confusion (in meters, on the sensor) into a
  fraction of the image, which is why the DoF needs no arbitrary scale. It behaves as a
  **constant lens**: the focal length is kept and the field of view follows, i.e. the physical crop
  factor — 11 mm sees 94.6° on full frame and 71.2° on APS-C. Changing the format therefore
  REFRAMES; it does not merely restyle the blur.
- `setTechnicalFieldOfView(degrees)` is the ONLY way an angle enters a camera, and it is **not
  photographic**: it exists where the field of view is a GEOMETRIC constraint — a cubemap face is
  strictly 90° or the six faces do not join. It stores the focal length that yields the angle (90°
  on a 24 mm-high sensor is exactly 12 mm, the round trip costing ~1e-5°) and raises
  `TechnicalProjection`, which **locks the sensor format** (`setSensorWidth()` warns and returns),
  since reframing is precisely what would break the constraint. `isTechnicalCamera()` reports it.
- Exposure: `setExposureCompensation(EV)`, `setAutoExposure(bool)`.
- ⚠️ **With auto-exposure on (the default), the aperture is not an exposure control.** The
  metering solves directly for the multiplier that puts the scene on middle grey, and the
  aperture only enters through the CLAMPS. So f/11 → f/32 changes the depth of field and nothing
  else until the metering saturates — aperture priority, exactly as a real body behaves. The
  aperture reads as stops of brightness only in manual mode, where the full APEX exposure applies.
- ⚠️⚠️ **The metering moves the ISO FIRST, then the SHUTTER (2026-09-26, owner decision)**:
  `ToneMapping::resolveExposure()` bounds the multiplier by ISO max at the AUTHORED shutter (dark
  end, unchanged — the shutter never slows) and by ISO min at `Camera::FastestShutterSpeed`
  (1/8000 s, bright end). The readback splits the result back into `meteredSensitivity()` and
  `meteredShutterSpeed()`, and `ToneMapping::effectiveShutterSpeed()` → `FrameContext::shutterSpeed`
  is what `MotionBlur` scales by — never `camera->shutterSpeed()`, the authored (slowest) one.
  Before, the metering moved the ISO alone and could not close down past ISO 100: after the
  photometric recalibration, `forest` (EV100 ≈ 14.3) at f/8 1/125 s sat 1.3 EV over, and every wider-aperture
  camera style (KeyPad5/6) 2.4-5.8 EV over — the style effects composited over a white frame
  (`docs/caution-points.md` § *every wide-aperture camera style overexposed*). `getStatus()`'s
  `Metering:` line and `getFrameDiagnostics()` (`meteredShutterSpeed`) report both values.
- ⚠️ **EV compensation respects the sensor (2026-07-26)**: with auto-ISO on, the bias shifts
  the METERING TARGET (`keyValue × 2^EC`, applied INSIDE the sensor clamp) instead of
  post-amplifying the clamped result — +3 EV saturates at the same ISO ceiling, exactly as a
  real auto-ISO body. In manual mode it stays a straight EV bias on the APEX exposure.
- **Metered values ARE read back (2026-07-26)**: both GPU-resident measurements come home
  through a per-frame-in-flight host-visible ring (one tiny persistently-mapped slot per
  frame; slot N is read when it comes around again — its fence passed — so the read never
  stalls and costs framesInFlight frames of latency, the standard pattern).
  `ToneMapping::meteredSensitivity()/meteredLuminance()` (ISO + scene average in nits,
  decoded from the RGBA16F adaptation history) and `DepthOfField::meteredFocusDistance()`
  (meters, from the 1x1 RG32F focus history). Access from the panel through
  `PostProcessStack::cameraToneMapping()/cameraDepthOfField()` — RENDER THREAD, inside the
  frame scope, like everything the panel touches.
  ⚠️ **Both copies end with a TRANSFER → HOST buffer barrier** (`TRANSFER_WRITE` → `HOST_READ`,
  2026-09-26): a fence wait alone does not make a device write visible to the host, so until then
  the metered ISO and focus distance could be read stale. The `FrameCapture`, `Recorder` and
  overflow-census readbacks carry the same barrier; any new readback must too.
- ⚠️ **Auto and manual expose IDENTICALLY (2026-07-26)**: the auto-exposure keys on
  `Photometry::MeteredMiddleGrey` (K=12.5 / (MeterCalibration=1.2 · 100) ≈ 0.104), the value the
  manual APEX triad lands a correctly metered scene on. The previous key, Reinhard's 0.18, is a
  display-side grey-card convention — NOT what a K=12.5 meter produces through
  `exposureFromValue100()` — and kept auto mode 0.79 EV hotter than the same scene shot manually,
  shifting the auto-ISO window against its own sensor bounds. The three constants live in
  `Photometry.hpp` (single source); do not reintroduce a literal.
  Two consequences worth knowing: (1) the adaptation now consumes `PushConstants::deltaTime`
  (the chain contract — no self-measured time) and its FIRST execution resets the 1x1 history
  in the shader (`resetHistory` push constant): startup convergence is instantaneous instead of
  a long transient, and a recycled NaN can no longer poison the EMA forever (same guard as the
  DoF focus history); (2) ⚠️ any exposure statistic captured BEFORE that reset existed is
  suspect — the old path could take tens of seconds to converge, and comparisons made against
  such captures mis-attributed the difference (lived: a keyValue A/B read "no change" against a
  reference that was in fact an unconverged transient; the METERED ISO readback below is what
  settled it — gltf-loader meters ~ISO 1000 at f/11 1/250, well inside the sensor bounds).
- ⚠️ **Bloom intensity = the FRACTION of above-threshold energy the lens scatters (2026-07-26)**:
  the glare chain carries the full photometric energy (sunlit stone ~20000 nits over a 1000-nit
  threshold), and the composite is `original + bloom × intensity` in nits — so 1.0 means "the
  glass scatters ALL of it" and sets any daylight scene ablaze. Camera default 0.03 (a clean
  modern lens scatters 2-5%; a hazy vintage one >10%). This became visible when the anti-firefly
  ceiling was fixed: the old fixed `clamp(…, 64)` (LDR-era, four stops BELOW the default
  threshold) crushed every source to identical, near-invisible glare; the ceiling is now
  `max(threshold, 1) × 64` — six stops of differentiation headroom — pushed to EVERY downsample
  mip (`BloomPushConstants::fireflyClamp`).
- **Automatic modes are the default** (auto-focus + auto-exposure ON at construction).
- All optics are ANIMATABLE (`AnimationID::Aperture/FocalLength/FocusDistance/
  ExposureCompensation`) — focus pulls and exposure ramps via the animation system.
- Single-pass lens effects (VHS, grain, B&W...) were ALREADY per-camera via
  `addLensEffect()`; the physical camera extends the model to multi-pass effects.
- ⚠️ **Lens-effect list = PUBLICATION contract (2026-07-26)**: `Camera::lensEffects()`
  returns a `std::shared_ptr< const Graphics::DirectEffectList >` SNAPSHOT (nullptr = no
  effect); every mutation replaces the list wholesale (copy-on-write under the camera's
  internal lock). The renderer RETAINS the snapshot it records per frame in flight
  (`Renderer::m_lensEffectsSnapshots`, indexed by `currentFrameIndex()`), so a removed
  effect survives until the frame slot's fence has passed. This replaced a bare
  `std::vector` the render thread iterated while KeyPad style-cycling mutated it from the
  logic thread (use-after-free one keypress away) — do NOT reintroduce a mutable reference
  accessor, and never destroy a direct effect in place while frames are in flight.

**Camera cut** (`Scene::switchToCamera(std::shared_ptr< Component::Camera >)`): performs a
full cut — the camera becomes the RENDERED point of view (primary video source reroute
through `AVConsole::Manager::switchPrimaryVideoSource()`, which disconnects every other
source feeding the primary output) AND the photographic authority (active camera). One
call: image + look together. Validated on Sponza with fixed showcase cameras carrying
different presets (KeyPad8 cycle in the projet-alpha demos).

⚠️ **Active-camera lifetime contract (2026-07-26)**: the scene holds the photographic
authority as a WEAK reference behind an internal publication mutex.
`Scene::activeCamera()` returns a `std::shared_ptr` — a camera whose entity
self-terminated resolves to nullptr and the effects dematerialize through the regular
per-frame polling; the shared_ptr keeps the component alive for the caller's use even if
the entity dies mid-frame. NEVER cache the raw pointer across frames (that was the
use-after-free this replaced: the render thread could copy the raw pointer, then the
entity's destruction freed the component under it — the old CameraDestroyed clear was
unlocked and could arrive too late). App-side long-lived references (the KeyPad8 player
camera) are captured as `std::weak_ptr` and locked at use.

**Materialization mechanics** (no observer, no cross-thread races):
- `Renderer::renderFrame*()` calls `PostProcessStack::syncCameraEffects(activeCamera,
  renderer)` once per frame on the render thread — a two-boolean comparison when nothing
  changed. Camera switches (`Scene::setActiveCamera`) are therefore handled automatically:
  **each camera keeps its own photographic setup; the active one shapes the pipeline.**
- On change: new effects are created render-side; removed effects retire through
  `Renderer::deferredDestructor()` (frames-in-flight safety); the scene target is retired
  so the lazy configure path rebuilds the pipeline with the new requirements (HDR may
  appear/disappear with the tone mapping).
- The effects READ the camera each frame via `FrameContext::camera` — parameter changes
  (aperture, EV...) apply immediately, zero rebuild. Fallback to the effect's local
  `Parameters` when no camera exists.
- Demos/apps DO NOT add DepthOfField/ToneMapping to their stack anymore — they enable
  them on the camera (see `projet-alpha` `GLTFLoader::onEnabled()`).

**The chain is created ON DEMAND (Jul 2026)** — the camera is never silently ignored:

- `Component::Camera::requiresPostProcessing()` answers "does this camera need the
  pipeline to exist at all" (any of DoF / motion blur / bloom / HDR, or at least one lens
  effect). When it says yes and the scene has no chain, `Renderer::renderFrame()` calls
  `Scenes::Scene::requirePostProcessStack()`, which lazily creates an empty one.
- **The bug this closes:** before, `syncCameraEffects()` only ran when the APPLICATION had
  provided a stack, so `camera->enableHDR(true)` on any other scene was a no-op with no
  diagnostic. The raw photometric radiance then reached an LDR swap-chain — a daylight
  scene came out **pure white**, a night scene **pure black**. In `projet-alpha`, 29 of the
  34 demos were in that state.
- Lifetime/threading are unchanged: the stack still belongs to the `Scene` and dies with
  it. `requirePostProcessStack()` is render-thread (frame scope) or scene-building thread
  BEFORE activation — `Manager::newScene()` does not activate, so the two never overlap.

**Master switch vs. actual work** — two distinct questions, do not conflate them:

| Question | Who answers | API |
|---|---|---|
| Is post-processing ALLOWED? | the user | `PostProcessor::enable()` / `isEnabled()`, **default ON** |
| Is there anything TO run? | the renderer | `Renderer::m_postProcessingActive` / `needsInternalTarget()` |

`m_postProcessingActive` is recomputed once per recorded frame, **after**
`syncCameraEffects()` (so effects materialized this very frame count), as
`isEnabled() && (stack has effects || camera has lens effects)`. Every decision inside the
renderer — scene-target create/destroy, strategy dispatch, direct-path composite, jitter —
keys on it, never on `isEnabled()` alone. A scene with an empty chain therefore stays on
`renderFrameDirect()` and pays nothing, which is what lets the switch default to ON.

> [!CAUTION]
> Do not "fix" a missing effect by calling `postProcessor().enable(true)` from the
> application. That call is gone from `projet-alpha` (`AbstractDemo::createScene()`) on
> purpose: making the application arm the pipeline is what produced the silent no-op above.
> Declare the effect on the camera, or add it to the scene stack — the renderer arms itself.

e### Sprite photometric contract — a flame is authored in nits (Aug 2026)

A sprite is almost always a self-illuminating object (flame, explosion, muzzle flash, neon), it is
rendered UNLIT, and on the unlit path the surface colour IS the emitted radiance — so it needs a
real luminance or it contributes nothing. The manifest carries both halves:

```json
{
	"Type": "AnimatedTexture",
	"Data": { "Name": "fire001" },
	"BlendingMode": "Screen",
	"AutoIllumination": 1.0,
	"EmissiveStrength": 10000.0
}
```

- `AutoIllumination` — the emissive **MASK**, clamped to [0,1]. It cannot carry a brightness.
- `EmissiveStrength` — the **LUMINANCE in cd/m² (nits)**. Same key and same contract as
  `BasicResource` / `StandardResource` and the glTF extension `KHR_materials_emissive_strength`;
  the emitted quantity is `autoIlluminationColor * autoIlluminationAmount * emissiveStrength`.

⚠️ **The key was added in Aug 2026 and its absence was a hard limit, not an oversight to work
around in the application**: before it, `SpriteResource::load()` parsed the amount only, so every
sprite emitted exactly 1 nit and the fire and explosions of `game-logic`-style scenes were
invisible under photometric exposure. Reference values and the full failure mode are in
`docs/caution-points.md` § "The light RADIUS is a culling bound, not a dimmer".

Applied by `Material::Interface::emissionMultiplier()` — see `src/Saphir/AGENTS.md` § "Emission on
the UNLIT path", including why it multiplies here and adds on the lit path, and why it must never
reach the albedo attachment.
