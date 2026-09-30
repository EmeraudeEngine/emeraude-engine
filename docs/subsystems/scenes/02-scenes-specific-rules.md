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

### Available Components
**Rendering:** Visual, MultipleVisuals — ⚠️ both take `Graphics::RenderableInstance::Lighting` (Lit/Unlit) as a REQUIRED last constructor argument since 2026-09-25: the former unlit default blacked out three sets of content (the last one every forest tree, lit only by the GI)
**Lights:** DirectionalLight, PointLight, SpotLight, SunCourse (drives a DirectionalLight along the day), SkyFollowsSun (scales the background's luminance with a SunCourse)
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

> `SunCourse` (2026-09-13) drives a **`DirectionalLight` along the daily course of the sun**: each
> logic cycle it places its PIVOT node on the unit vector toward the sun (the light in its
> position-to-origin mode, `useDirectionVector(false)`, so the position IS the direction), writes the
> illuminance from the **Kasten & Young (1989) air mass** through a Beer-Lambert extinction referenced
> at the zenith (`Options::zenithIlluminance`, 100 klx by default; ~30 lx on the horizon), writes the
> colour from a temperature sliding 5800 K → 2000 K with the same air mass, and **disables the light
> while the sun is below the horizon** — at night it lights nothing. Great circle through the sunrise
> and noon points at CONSTANT angular speed: day = night = `Options::dayDuration` (120 s default).
> Built in one call by `Toolkit::generateSunCourse(name, course, shadows)` (a movable Node, the light
> under a `DirectionalShadowOptions`, the component suffixed "Course"). ⚠️ **The sun only** (owner
> decision): the sky, ambient and IBL stay the background's — a scene swapping a sky's sun for it passes
> `applyStars = false`. ⚠️ The phase is an **integer cycle counter** over an integer revolution length,
> never a float accumulated per cycle (owner rule: recompute from a stored reference) — `setPhase()`
> lands on the same direction every time. Like `NodeAnimation` it holds WEAK references and asks for
> its own removal when the node or the light dies.
>
> `SkyFollowsSun` (2026-09-13, owner decision) is the sky's HALF, deliberately a separate component:
> bound to a `SunCourse`, it maps the elevation through a smoothstep (`duskElevation` −6° → 0,
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
