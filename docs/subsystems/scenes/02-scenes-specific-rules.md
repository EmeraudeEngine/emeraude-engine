## Scenes-Specific Rules

### Philosophy: Composition Over Inheritance
- **Generic entities**: Node and StaticEntity are position containers
- **Components give meaning**: Visual, Light, Camera, SoundEmitter, etc.
- **NEVER subclass**: Use Component composition instead of Player extends Node
- **Maximum flexibility**: Add/remove behaviors dynamically

### Architecture: Two Entity Types

**Node (Dynamic)**: Hierarchical tree with physics, parent-relative transforms
**StaticEntity (Static)**: Optimized flat map, no physics, absolute transforms

See [`../../docs/scene-graph-architecture.md`](../../scene-graph-architecture.md) for complete details.

### Coordinate Convention
- **Y-UP mandatory** in CartesianFrame — `localYAxis()` is `m_upward`, and `downwardVector()` is its INVERSE (they are opposites, not aliases)
- Local transforms for Nodes (parent-relative)
- World space recalculated on demand (no cache currently)

### Component setters refuse a non-finite value (2026-09-30)

Every numeric setter of `Scenes/Component/` (68 of them: intensities, radii, biases, camera optics, particle
sizes / rates, beam and path points, weights, the sun's phase…) starts with `Abstract::acceptsFinite(name, values…)`:
a NaN or an infinity is **refused**, the setter keeps its previous value and logs `Component '<name>': <setter>()
refused a non-finite value …` (owner ruling, plan Ave Robustus — the setters are public engine API). Each setter's own
sign / range policy (`std::abs`, clamps) is unchanged. The console / MCP adapters refuse such values earlier, with an
explicit reply. Checked types: `float`, `Vector< N, float >`, `Color< float >`, `std::span< const Vector< 3, float > >`;
not the setters taking a spline or a `CartesianFrame` (`setBezierPath`, `DirectionalLight::setDirection`,
`MultipleVisuals::setLocalCoordinates`). A new numeric setter adds the same first line.

A camera's view / far distance must be POSITIVE (owner ruling 2026-10-01, triad 7e): `Camera::setPerspectiveProjection`,
`setDistance`, `setFar` and the far of `setOrthographicProjection` ignore a value <= 0 with a warning and keep the previous
one (0 used to reach the projection: a far plane at 0, below the near one). `Core/Graphics/ViewDistance` outside
(0, `MaxGraphicsViewDistance` = 1 000 000] m warns and takes the default 10 km (`Toolkit::viewDistanceSetting()`, the
cameras the toolkit builds). The orthographic near stays `min(0, near)` (the box always includes the camera plane).

### Available Components
**Rendering:** Visual, MultipleVisuals — ⚠️ both take `Graphics::RenderableInstance::Lighting` (Lit/Unlit) as a REQUIRED last constructor argument since 2026-09-25: the former unlit default blacked out three sets of content (the last one every forest tree, lit only by the GI)
**Lights:** DirectionalLight, PointLight, SpotLight, CelestialClock (drives the sun's and the moons' DirectionalLights from a calendar), SkyFollowsSun (scales the background's luminance with a CelestialClock's sun)
**Audio:** SoundEmitter, Microphone
**Physics:** DirectionalPushModifier, SphericalPushModifier, Weight
**Animation:** NodeAnimation
**Utilities:** Camera, ParticlesEmitter
**Atmosphere:** CloudVolume (a volumetric cloud placed by its entity, Sep 2026)

> `NodeAnimation` plays the clips that move a **node hierarchy** rather than a skeleton — glTF TRS
> tracks on plain nodes (a rotating bezel, a swinging door). `SceneDataConsumer` installs ONE on the
> **root** of an imported hierarchy and binds `weak_ptr` targets, because a single clip drives many
> nodes. Its playback surface mirrors `Animations::SkeletalAnimator` on purpose.
> ⚠️ **Node mode only** — a `StaticEntity` bakes its world frame at build time and has no local frame
> left to animate; the consumer warns instead of silently dropping the animation.
> ⚠️ **An animated node is exempt from flattening** — see [`Animations/AGENTS.md`](../../../src/Animations/AGENTS.md).

> `CelestialClock` (2026-10-24, replaces `SunCourse`) drives **the sky's bodies from a calendar**: a planet
> (`Base::Math::Astronomy::Planet` — sidereal day, orbital period, axial tilt, eccentricity, perihelion; Earth is
> the default), a latitude, a date (days since the vernal equinox) and a time of day (0 midnight, 0.5 noon) place
> the SUN and the MOONS. Each logic cycle it places each body's PIVOT node on the unit vector toward it (the light
> in its position-to-origin mode, `useDirectionVector(false)`), writes the sun's illuminance from the **Kasten &
> Young (1989) air mass** through a Beer-Lambert extinction referenced at the zenith (`SunPhotometry`, 100 klx by
> default) and its colour from a temperature sliding 5800 K → 2000 K with the same air mass, and **disables the sun
> while it is below the horizon**. A MOON lights the scene while it is up and the sun is down: full-moon
> illuminance (0.3 lx above the atmosphere) × its BRIGHTNESS × the extinction, 4100 K, no shadow map. The brightness
> (`moonBrightness()`, 2026-10-25, graphics doc 42 § "The moons") is the lunar phase law — Hapke's reflectance as
> Jensen et al. (SIGGRAPH 2001) give it, integrated over the disc: 0.077 at quarter (it was a Lambert sphere's 0.32) —
> × the surface brightness (the dynamic sky's moon map: the textured disc over a uniform one, 1 without a map) + the
> EARTHSHINE (`Moon::earthshine`, the planet's geometric albedo × (its radius / the moon's distance)², Earth ≈ 1.19e-4,
> × the Lommel-Seeliger law of the planet's own phase × `Moon::earthshineGain`, 1 = physical; a non-finite or negative
> pair is refused for the physical value). `getState()` gives each moon's `phaseAngleDegrees` and
> `surfaceBrightness`. The astronomy is emeraude-base's `Math/Astronomy.hpp` (generic two-body orbits, owner decision: no
> perturbation, ~1° for the Sun), unit-tested there. ⚠️ **Time** is seconds since the epoch in DOUBLE, recomputed
> every cycle from a stored reference and an integer cycle count — never accumulated (owner rule); `timeScale`
> simulated seconds per real second. ⚠️ **Frame**: East-North-Up mapped with `Options::northDirection` (default −Z)
> and up +Y, east = north × up (+X by default). ⚠️ **The former `SunCourse` is `CelestialClock::simpleCourse()`**
> (owner decision, "a special case"): a planet without tilt nor orbit, sidereal day = two day durations, observed at
> the latitude 90° − the noon elevation — EXACTLY the old great circle (base test
> `MathAstronomy.AStillPlanetReproducesTheSimpleSunCourse`; measured on `terrain`: 60.0° and 92 589 lx at noon, as
> before). Built in one call by `Toolkit::generateCelestialClock(name, options, sunShadows)` (the sun's movable Node
> and light, the component suffixed "Clock", one unshadowed node + light per moon suffixed "Moon<index>").
> ⚠️ **The sky's bodies only** (owner decision 2026-09-13): the sky, ambient and IBL stay the background's. It also
> exposes the celestial pole, the local sidereal angle and the EQUATORIAL FRAME (the world vectors of right ascension 0° and 90° and of the pole: the star field's frame, graphics doc 42 § "The stars (lot S2)"). Like `NodeAnimation` it
> holds WEAK references and asks for its own removal when the sun's node or light dies.
> It **writes the scene's sky state** (owner decision 2026-10-24): `publishStateForRendering()` fills a
> `Scenes::SkyState` per render state — the sun and moons with their illuminance ABOVE the atmosphere (the zenith
> illuminance × exp(extinction)), their unit-luminance colour at the zenith temperature (no horizon reddening: a
> physical sky reddens it itself), angular diameter and phase, and for a moon its orbit's pole, full illuminance and
> earthshine (what the sky draws its disc with) — and the scene keeps its clock
> (`CelestialClockCreated` / `CelestialClockDestroyed`; the last one added wins) behind
> `Scene::skyStateForRendering(readStateIndex)`, which the renderer's physical sky reads (graphics doc 42).
> ⚠️ **With a dynamic sky** (2026-10-25, owner decisions; graphics doc 42 § "The light"), the clock follows the scene's
> background each logic cycle, running or not (`followAtmosphere()`): a loaded `DynamicSkyResource` makes its lights
> come from that atmosphere's transmittance (a CPU twin of the sky's table, the observer on the ground) and the star's
> PHYSICAL illuminance above the atmosphere — `SunPhotometry::unitDistanceIlluminance` (133 800 lx at 1 AU) over the
> square of the planet's distance (`Planet::semiMajorAxis`, eccentric) — with its LINEAR colour
> (`SunPhotometry::starTemperature`, 5772 K); the sky state carries the same star. The sun fades through the horizon
> (penumbra) instead of a hard switch; `isDaytime()` is "the sun's light is on". It warns when its `Planet::name` and
> the atmosphere's `planet` are both set and differ (a simple course is unnamed). Without a dynamic sky nothing changes
> (`zenithIlluminance`, `extinction` and the temperatures keep driving the light).
> **The calendar** (`Options::timeModel = TimeModel::Calendar`, 2026-10-26, owner decisions; engine item
> `celestial-clock-real-date-and-place`): OUR sky at a real instant — `Options::julianDay` (UT) and the observer's
> `latitude` AND `longitude` (east positive) — from emeraude-base's `Math/SolarSystem.hpp`: the Sun (Standish's JPL
> elements), the Moon (Schlyter: ~1–3.5′, measured against PyEphem) seen from the observer's place (its parallax), the
> local sidereal time (IAU 1982 + the longitude), and the stars' J2000 frame PRECESSED to the date. Earth only (another
> `Planet::name` falls back to the planet model, traced) and one moon, the Moon (further moons are dropped, traced); its
> apparent diameter and full illuminance follow its distance from the observer (× (384 400 km / d)², and the Sun's
> 1 / d²), the Sun's diameter its distance. The time is seconds since J2000.0 UT; `setInstant(julianDayUT)`,
> `julianDay()`; `setTimeOfDay()` moves within the LOCAL MEAN SOLAR day at the longitude; `setDate()` is refused (a day
> since the equinox is the planet model's). `day()` / `timeOfDay()` are the local mean solar day number and time.
> Console: `setCalendarDate(year, month, day, hour, minute, second)` (UTC) and `setNow()` (the system clock) switch a
> clock to the calendar, `setDate()` brings it back to the planet model, `setLongitude()`; `getState()` adds
> `timeModel`, `longitudeDegrees`, `julianDayUT` (a string: the JSON writer keeps 5 digits), `utc` (ISO 8601) and each
> moon's `angularDiameterDegrees`. The calendar also places the eight PLANETS (`planetCount()`, `planet(index)`,
> `planetMagnitude()`, `planetName()`; `SkyState::planets`; `getState` → `planets`): position, Mallama & Hilton 2018
> magnitude, colour, diameter, phase — drawn by the sky's star pass (graphics doc 42 § "The planets"). Measured from Liège (2026-10-26, sky-lab option 10) against PyEphem with the
> refraction off: the Sun 11–15″, Betelgeuse through the frame 17–19″, the Moon 31″–3.8′ (Schlyter), its diameter
> within 0.25 %; the 2026-08-12 eclipse seen from Liège (Sun and Moon 31″ apart, phase 0). The engine's birthplace,
> the default place of a real sky: `Identification::BirthplaceLatitude` / `BirthplaceLongitude` (Liège, owner).
>
> `SkyFollowsSun` (2026-09-13, owner decision) is the sky's HALF, deliberately a separate component:
> bound to a `CelestialClock`, it maps the sun's elevation through a smoothstep (`duskElevation` −6° → 0,
> `dayElevation` +10° → 1, `nightFactor` 0 by default) and writes `dayLuminance × factor` to the
> scene's background (`AbstractBackground::setLuminance()`), then `Scene::refreshAmbientLightProperties()`
> — ONE knob for the IBL scale (diffuse AND specular: with the sky driving the ambient the scalar
> ambient is zero and the irradiance cubemap × `environmentLuminance` IS the ambient), the sky term
> read by the post-processing and, through the new `AbstractBackground::onLuminanceChanged()` hook,
> the skybox's drawn emission (`SkyBoxResource` → `StandardResource::setEmissiveStrengthValue()`, a
> dynamic material property). The DAY luminance is the manifest's, captured the first time the
> background is seen loaded (and again after a sky swap); writes are quantised to 1/1024 so a
> constant day or night pushes nothing. Its destructor gives the sky its day back. A demo composes
> the two: `sun.entity()->componentBuilder< SkyFollowsSun >(...).setup(bind(sun.component()))`.

> `CloudVolume` (2026-09-24) is a **volumetric cloud placed by its entity**: the entity's position is
> the centre of the cloud box, its orientation turns it, its SCALE stretches it — so a cloud is
> placed, turned and resized like any entity, the editor gizmo included (verified 2026-09-24: the
> editor selects a cloud and draws its gizmo). It carries a shape (`Graphics::CloudShapeResource`,
> grown on the thread pool, shared by name), the box half extents in metres, and a DIMENSIONLESS
> `Look` — ⚠️ owner decision: **scaling a cloud keeps its look**, the renderer derives the extinction
> from the cloud's current height (`opticalThickness` is a vertical optical depth, not a density).
> The scene files it in `Scenes::CloudSet` (`Scene::cloudSet()`) on `AbstractEntity::CloudVolumeCreated`,
> removes it on `CloudVolumeDestroyed`; joining registers the shape in the scene's bindless **3D**
> array (once loaded — it observes the resource), and the render thread reads the ENTITY's published
> frame (`getWorldCoordinatesStateForRendering(readStateIndex)`) plus the component's published look.
> The set's emptiness is the switch of the whole feature: the post-process stack files the cloud
> pass on the first frame it is not empty (`PostProcessStack::syncSceneEffects()`, owner decision
> "placing a cloud is enough"). ⚠️ **A cloud is NOT solid**: its render box also becomes the collision
> model by default — `Toolkit::generateCloud()` calls `setCollidable(false)` BEFORE linking the
> component, which keeps the bounding primitives the editor picks on. `m_cloudSet` is declared AFTER
> `m_bindlessTextureSet` in `Scene`: a cloud frees its bindless slot when it dies, so the set must
> outlive it. Rendering: [`Graphics/AGENTS.md`](../../../src/Graphics/AGENTS.md) § VolumetricClouds.
>
> **The clouds' shadow (stage 2 lot 1, 2026-09-24)** is split between the set and the sun. The
> `CloudSet` OWNS the Beer shadow map (`Graphics::CloudShadowMap`, created on the render thread the
> first frame it is needed, from `Core/Graphics/PostProcessing/Clouds/Shadow*`, registered in the
> scene's bindless **2D** array — the slot is released by `~CloudSet`) and records it
> (`Scene::recordCloudShadowMap()`, before the scene pass). The main sun CARRIES it (owner decision:
> the term belongs to the light): `Scene::updateCloudShadows()`, on the logic thread right after the
> CSM cascades, calls `DirectionalLight::updateCloudShadow(slot, camera, coverage, resolution)` on
> `mainDirectionalLight()` and `disableCloudShadow()` on every other directional light — the matrix
> and the slot then travel in the light's published block like its shadow matrix. ⚠️ The map is
> recorded ONLY on a frame whose PUBLISHED sun block already names its slot
> (`DirectionalLight::cloudShadowIndex(readStateIndex)`): before that, the matrix in the block is not
> the map's, and a lit shader reading the map would disagree with the pass that wrote it.

### Editor Subsystem

Standalone scene editor for entity picking and gizmo manipulation. See [`Editor/AGENTS.md`](../../../src/Scenes/Editor/AGENTS.md).

- **Owned by**: `Scenes::Manager` (auto-deactivates on scene change)
- **Namespace**: `Scenes::Editor` (Manager) + `Scenes::Editor::Gizmo` (Abstract, Translate)
- **Activation**: Shift+F3 via Core → `Scenes::Manager::toggleEditorMode()`
- **Rendering**: Standalone pipeline (not scene entities), renders before overlay

### Controllers (NodeController & OrbitController)

Two input-driven manipulators live BY VALUE inside every `Scene` and are registered/unregistered
by `Scene::enable()`/`Scene::disable()`. Both are **inert without a controlled node** and consume
no event until one is attached.

| Controller | Input | Registered as | Behavior |
|------------|-------|---------------|----------|
| `NodeController` | Keyboard (numpad) + gamepad | Keyboard listener | Debug node manipulation (move/yaw/pitch/roll) |
| `OrbitController` | Pointer | Pointer listener | Orbits a camera node around a fixed target : left-drag = azimuth/elevation, wheel = dolly |

`OrbitController` rules:
- **Canonical state** `(target, azimuth, elevation, distance)` — the node position is RECOMPUTED
  from it on every event, never integrated incrementally. The dolly distance derives from an
  INTEGER step index over a reference distance (`reference × factor^index`, re-anchored by
  `setDistance()`) — the project's anti-FP-accumulation rule.
- The controlled node **must be a direct child of the scene root** (positioned in Parent space,
  which equals world space only at level 1; `Node::lookAt()` operates on the local frame).
- Elevation is clamped near the poles (`PoleGap`) : the aim derives the up vector, an orbit
  crossing a pole would flip the view.
- Used by the `Viewers/` scenes; any scene can use it via `scene->orbitController()`.

### Viewer Scenes (`Viewers/`)

Engine-built, ready-to-enable scenes displaying ONE dropped/opened file. These are the **Core
default behaviors** for files dropped onto the window (see `Core::openFiles()`); reserved scene
names use the manual-resource `+` prefix so Core can recognize (and replace) them.

| Builder | Scene name | Content |
|---------|------------|---------|
| `Viewers::ImageViewer` | `+ImageViewer` | Unlit quad at the image aspect ratio, double-sided (mirrored from behind, like a slide). LightSet DISABLED + camera out of HDR ⇒ texels reach the screen unmodified |
| `Viewers::ModelViewer` | `+ModelViewer` | Composite asset imported via `Manager::createSceneLoader()` + `SceneDataConsumer`, OR raw geometry file (`VertexFactory::FileIO::isReadableExtension()` : OBJ, STL, MDx, ee3d) wearing a neutral clay material (mid-grey, dull, dielectric, EXPLICIT lit path). Neutral lighting (ambient 80.72 lux delivered — `Core/Viewers/AmbientIlluminance` — + key 100 000 lux + cool fill 15 000 lux opposite, so the orbit shadow side stays readable), HDR camera with **MANUAL sunny-16 exposure**, and a **default sky** (`installBackground()`, setting `Core/Viewers/Background`, default `GreenLandscape`) supplying the background AND the IBL. `ModelViewer::handlesFile()` is the single "can I display this?" decision site |

⚠️ Lessons already paid for:
- **Never rely on auto-exposure in a viewer**: the metering averages the whole frame, and a small
  lit model over the black void is crushed to pure white. The lighting is ours, so the exposure is
  paired manually (100 000 lux ↔ f/16, 1/100 s, ISO 100).
- **The lit path must be EXPLICIT on a bare Visual**: `setLightingState(true)` in the component
  setup, exactly like the SceneDataConsumer does per mesh — left on the unlit path, the clay's raw
  [0,1] albedo goes through the photometric exposure and reads black (measured on the Bunny).
- **Extents are NOT valid right after `SceneDataConsumer::build()`** — mesh resources load on the
  thread pool. `ModelViewer` waits (bounded budget) for `getWorldRenderBoundingBox()` validity
  before framing; on timeout it falls back to a default framing.
- Framing uses the bounding-SPHERE fit (`distance = margin × radius / sin(fov/2)`), a 50 mm focal,
  and a three-quarter start orientation.
- Resource names carry a per-session counter (`+ImageViewerImage3`, ...) — containers return the
  EXISTING resource for a known name, a fixed name would show the first image forever.
- **A model viewer without an environment cannot show a material.** `ModelViewer::installBackground()`
  puts a sky on the scene: reflective, transmissive, clearcoat, sheen and iridescent materials have
  literally nothing to reflect without one, and the Khronos normal/tangent tests ask by name for
  "an environment map that contains a clear horizon line". The resource name is a SETTING
  (`Core/Viewers/Background`, default `GreenLandscape`) because the skybox belongs to the
  CONSUMER's data store, not to the engine — an unknown name degrades to no background with a
  warning, never to a viewer failure. ⚠️ The direct lighting stays manual on purpose: deriving it
  from the sky (`applyBackgroundLighting()`) would make every sky change the SUBJECT's exposure,
  and the viewer exposure is deliberately fixed. The background feeds the reflections, not the key
  light. The flat ~80 lux ambient (`Core/Viewers/AmbientIlluminance`, 80.72 lx delivered) is only a floor for the no-sky case; a loaded sky's irradiance
  dominates it by two orders of magnitude.

### Scene Loader Registry

`Scenes::Manager::createSceneLoader(filepath)` is the **single dispatch point** for composite
asset formats: it instantiates the loader whose `supportsExtension()` accepts the (dotted,
lowercased) extension — GLTF, FBX, USD, WAD — and returns it as `std::unique_ptr<Loaders::Interface>`,
or nullptr. No extension list is duplicated anywhere: `supportsExtension()` stays the single
source of truth. Loaders are cheap per-call objects; a new instance is returned every time.

### Level Interfaces (Ground & Sea)

Two interfaces define scene-wide physical levels for gameplay queries:

| Interface | Purpose | Implementations |
|-----------|---------|-----------------|
| `GroundLevelInterface` | Ground/terrain queries | `BasicGroundResource`, `TerrainResource` |
| `SeaLevelInterface` | Water surface queries | `BasicSeaResource` |

**GroundLevelInterface** (`Scenes/GroundLevelInterface.hpp`):
- `getLevelAt(worldPosition)` - Ground height at position
- `getLevelAt(x, z, deltaY)` - Returns position with Y = ground level + delta
- `getNormalAt(worldPosition)` - Surface normal at position
- `updateVisibility(cameraPosition)` - LOD/visibility hint (logic thread, once per cycle; `TerrainResource` re-centres its RAY-TRACING PROXY from here through the engine `ThreadPool`, never a raw `std::thread` — the publication is the render thread's; the clipmap it draws with follows the camera on the RENDER thread every frame, `Geometry::Interface::updateSurfaceVideoMemory()`, `src/Graphics/AGENTS.md` § "Adaptive geometries")

**SeaLevelInterface** (`Scenes/SeaLevelInterface.hpp`):
- `getLevel()` - Constant water height
- `getLevelAt(worldPosition)` - Water height at position (flat = constant)
- `getLevelAt(x, z, deltaY)` - Returns position with Y = water level + delta
- `getNormalAt(worldPosition)` - Water surface normal (flat = {0,1,0})
- `isSubmerged(worldPosition)` - True if position.Y < water level
- `getDepthAt(worldPosition)` - Depth below water (positive = submerged)
- `updateVisibility(cameraPosition)` - Visibility hint

**Scene accessors:**
```cpp
scene->groundPhysics()     // Returns GroundLevelInterface*
scene->seaLevelPhysics()   // Returns SeaLevelInterface*
```

**Code references:**
- `Scenes/GroundLevelInterface.hpp` - Ground interface definition
- `Scenes/SeaLevelInterface.hpp` - Sea level interface definition
- `Graphics/Renderable/BasicGroundResource.hpp` - Flat ground implementation
- `Graphics/Renderable/BasicSeaResource.hpp` - Flat water implementation
- `Graphics/Renderable/TerrainResource.hpp` - Heightmap terrain implementation

### Modifier System & Influence Areas

Modifiers (DirectionalPushModifier, SphericalPushModifier) apply forces to entities within their influence area.

**Influence Area Types:**
- `SphericalInfluenceArea`: Sphere with inner/outer radius for falloff. See `SphericalInfluenceArea.cpp`
- `CubicInfluenceArea`: Oriented box with local space transformation. See `CubicInfluenceArea.cpp`

**Modifier API (Semantic Dispatch):**

Two overloads with clear semantic separation:

```cpp
// For entities (Node, StaticEntity) - encapsulates collision model lookup
Vector<3,float> getForceAppliedTo(const LocatableInterface& entity) const noexcept;

// For particles/points - direct position with optional bounding radius
Vector<3,float> getForceAppliedTo(const CartesianFrame<float>& worldPosition, float radius = 0.0F) const noexcept;
```

**Entity overload internals** - Dispatches based on `CollisionModelType`:
- `Point` → uses `influenceStrength(position)` (point-based)
- `Sphere` → creates Sphere from `getRadius()`, uses Sphere overload
- `AABB/Capsule` → uses `getAABB(worldCoordinates)`, uses AACuboid overload
- No collision model → fallback to point-based

**Particle/Point overload**:
- `radius > 0.0F` → creates Sphere on the fly
- `radius == 0.0F` (default) → uses point-based influence

**Influence Area Interface:**

Three overload families for different use cases:
```cpp
// Bounding volume tests (entities with collision models)
float influenceStrength(const CartesianFrame<float>&, const Sphere<float>&);
float influenceStrength(const CartesianFrame<float>&, const AACuboid<float>&);

// Point test (particles, fallback for entities without collision)
float influenceStrength(const Vector<3,float>& worldPosition);
```

**How modifiers work:**
1. `Scene::forEachModifiers()` iterates all modifiers
2. For entities: calls `modifier->getForceAppliedTo(*this)` - entity passed directly
3. For particles: calls `modifier->getForceAppliedTo(worldCoordinates, m_size * 0.5F)` - radius passed
4. Modifier internally dispatches to correct `influenceStrength()` overload
5. Returns force vector applied to entity's physics

**Code references:**
- `InfluenceAreaInterface.hpp` - Pure virtual interface (Sphere, AABB, Point overloads)
- `SphericalInfluenceArea.cpp:influenceStrength()` - Distance-based falloff (inner/outer radius)
- `CubicInfluenceArea.cpp:influenceStrength()` - Local space box containment test
- `AbstractModifier.hpp:getForceAppliedTo()` - Virtual interface (entity vs particle)
- `SphericalPushModifier.cpp:getForceAppliedTo()` - Radial force with type dispatch
- `DirectionalPushModifier.cpp:getForceAppliedTo()` - Directional force with type dispatch
- `Node.cpp:879` - Entity call site (passes `*this`)
- `Particle.cpp:404` - Particle call site (passes `worldCoordinates, m_size * 0.5F`)

**Future improvement:** Modifiers should be integrated into physics octree for O(log n) lookups instead of O(n) iteration.

### Observer System
- **Automatic registration**: Scene observes Component additions
- Visual → rendering registration
- Camera/Microphone → AVConsole registration
- Lights → LightSet registration
- **NEVER manual registration**

### Spatial Optimization
- **Octrees per Scene**: One for physics, one for rendering
- **Frustum culling**: Active during tree traversal. **Sprites are excluded** from frustum culling because billboard rotation (vertex shader) changes the screen-space extent, but culling uses CPU-side AABB from the flat quad geometry (Z=0). See: `Scene.rendering.cpp` frustum check.
- **Depth limit**: `DefaultMaxDepth` (16 levels) prevents infinite subdivision when entities cluster
- Future optimization: Culling by Octree sector
