## Development Patterns

### Creating a Dynamic Object (Node)
```cpp
// Create as child of existing Node
auto player = scene->root()->createChild("player", initialPos);

// Add Components
player->newVisual(meshResource, castShadows, receiveShadows, "body");
player->newCamera(90.0f, 16.0f/9.0f, 0.1f, 1000.0f, "player_cam");

// Configure physics
player->bodyPhysicalProperties().setMass(80.0f);
player->enableSphereCollision(true);
```

### Creating Static Geometry (StaticEntity)
```cpp
// Create via Scene
auto building = scene->createStaticEntity("building_01");
building->setPosition(worldPos);

// Add Visual and Light
building->newVisual(buildingMesh, true, true, "main");
building->newPointLight(Color::Warm, 100.0f, 20.0f, "lamp");
```

### Hierarchy (vehicle with wheels)
```cpp
// Parent vehicle
auto vehicle = scene->root()->createChild("vehicle", vehiclePos);
vehicle->newVisual(carBodyMesh, true, true, "body");

// Child wheels (automatically follow parent)
auto wheelFL = vehicle->createChild("wheel_FL", localPos_FL);
wheelFL->newVisual(wheelMesh, true, true, "wheel");

// Move vehicle → wheels automatically follow
vehicle->applyForce(forwardVector * thrust);
```

### Toolkit — Entity Generation & Node Hierarchies

The `Toolkit` class (`Scenes/Toolkit.hpp`) provides high-level entity construction helpers. It manages a cursor position, generation policies, and material/geometry creation.

**Core workflow:**
1. `setCursor(x, y, z)` — Position for the next entity
2. `generateCuboidInstance<entity_t>(name, size, material)` — Creates geometry + material + renderable + visual component
3. Returns `BuiltEntity<entity_t, Component::Visual>` with `.entity()` and `.component()` accessors
4. Lights: `generateDirectionalLight` / `generatePointLight` / `generateSpotLight`, and since 2026-09-13
   `generateSunCourse(name, SunCourse::Options, DirectionalShadowOptions)` — the ANIMATED sun (see
   `SunCourse` above and [`docs/toolkit-system.md`](../../toolkit-system.md) § Lights)
5. Clouds (2026-09-24): `generateCloud<entity_t>(name, CloudShapeResource::Parameters, width, look)` —
   a non-collidable entity at the cursor + a `CloudVolume`; `width` is the unscaled box along local
   X in metres, the height and depth follow `CloudShapeResource::proportionsOf(parameters)` (computed
   from the parameters, since the shape may still be growing). Vary the SEED to vary the cloud.

**Generation policies (`GenPolicy`):**

| Policy | Behavior |
|--------|----------|
| `Simple` (default) | Creates a standalone entity under the scene root |
| `Parent` | Creates the next Node as a **child** of a previously set parent node |
| `Reusable` | Reuses an existing entity for the next component attachment |

**Node hierarchy creation:**
```cpp
// Create parent node at world position
const auto parent = toolkit
    .setCursor(0.0F, -1.0F, 0.0F)
    .generateCuboidInstance< Node >("Parent", 2.0F, material);

// Create child — cursor is now in parent's local space
const auto child = toolkit
    .setParentNode(parent.entity())
    .setCursor(6.0F, 0.0F, 0.0F)
    .generateCuboidInstance< Node >("Child", 2.0F, material);

// Create grandchild — cursor in child's local space
const auto grandchild = toolkit
    .setParentNode(child.entity())
    .setCursor(6.0F, 0.0F, 0.0F)
    .generateCuboidInstance< Node >("GrandChild", 2.0F, material);

// IMPORTANT: Reset to default after building hierarchy
toolkit.clearGenerationParameters();
```

**Key methods:**
- `setParentNode(shared_ptr<Node>)` — Next generated Node becomes a child of this parent
- `setReusableNode(shared_ptr<Node>)` — Attaches next component to an existing Node (no new entity)
- `setReusableStaticEntity(shared_ptr<StaticEntity>)` — Same for static entities
- `clearGenerationParameters()` — Resets policy to `Simple`, clears parent/reusable refs, resets cursor

**Available generators:**
- `generateCuboidInstance<T>(name, size, material)` / `generateCuboidInstance<T>(name, {w,h,d}, material)`
- `generateSphereInstance<T>(name, radius, material)`
- `generateRenderableInstance<T>(name, renderable)` — Generic, from pre-built renderable
- `generateEntity<T>(name)` — Empty entity (no visual)
- `generateDirectionalLight<T>(name, color, intensity, shadowRes, range)`
- `generatePointLight<T>(name, color, range, intensity, shadowRes)`
- `generateSpotLight<T>(name, color, range, intensity, angle, shadowRes)`
- `generateSunCourse(name, SunCourse::Options, DirectionalShadowOptions)` — Node only: the ANIMATED sun
  (pivot + `DirectionalLight` + `Component::SunCourse`), 2026-09-13
- `generatePerspectiveCamera<T>(name, focalLengthMM, lookAt, primary, showModel, preset)` — ⚠️ the
  framing is a LENS in millimetres, never an angle (13.096 mm = the historical 85° default;
  12 mm = 90°, 20.8 = 60°, 25.7 = 50°, 50 = 27°). See `Component/Camera.hpp`.
- `generateOrthographicCamera<T>(name, size, ...)` — unaffected: no lens under an orthographic
  projection.
- `generateCubemapCamera<T>(name, ...)` / `generateEnvironmentCubemapRenderer<T>(...)` — go through
  `setTechnicalFieldOfView(90)`, a cube face being a geometric constraint rather than a lens choice.

All generators support `<Node>` or `<StaticEntity>` as template parameter (default: `StaticEntity`).

### Exposing a Component to the Console / MCP (`Component/ConsoleAdapter.hpp`, 2026-09-27)

A component type becomes drivable from the console and the MCP server through an ADAPTER — never by
making the component itself a `ControllableTrait` (the thousands of components never driven must pay
nothing). Every type that has something to drive is covered since 2026-09-27; one file per family:
`LightConsoleAdapters.cpp` (Point/Spot/DirectionalLight), `CameraConsoleAdapter.cpp`,
`EnvironmentConsoleAdapters.cpp` (SunCourse, SkyFollowsSun, CloudVolume), `AnimationConsoleAdapters.cpp`
(NodeAnimation, ParticlesEmitter), `PhysicsConsoleAdapters.cpp` (Directional/SphericalPushModifier,
Weight), `AudioConsoleAdapters.cpp` (SoundEmitter), `VisualConsoleAdapters.cpp` (Visual,
MultipleVisuals — one template). `Microphone` has nothing to drive. Operator view (every command, what is
refused and why): `docs/ai-runtime-control.md` § "Driving an entity's components". Pattern:

- Derive `ConsoleAdapter< YourComponent >` (its console identifier is `YourComponent::ClassId`) and bind
  typed commands whose first two parameters are `entityParameter("the …")` and
  `componentParameter(ClassId)`; do the work through `this->act(entity, component, [&] (YourComponent & c)
  { … })`, which resolves the entity in the ACTIVE scene, checks the type, and runs under
  `withExclusiveActiveScene()`. `sceneManager()` serves a command that addresses no component
  (`Camera.getActive()`).
- **Addressing** (owner decision 2026-09-27): `entity` is an ADDRESS — the shortest suffix of the node
  path that is unique in the scene (`Head`, else `ACTOR_…06/Head`), or a static entity's name.
  `resolveEntity()` REFUSES an ambiguous address with its candidates (every projet-alpha actor carries a
  `Head`; the former `Scene::findNode()` lookup acted on the first one found); `entityAddresses()` gives
  every entity's address (`listEntities()` prints it). Suffixes are built on node boundaries, so a node
  name containing `/` still resolves. Sibling names are unique (`Node::children()` is a map), so the full
  path always resolves — except a static entity named like a ROOT node, which stays ambiguous.
- A setter answers `changedState(message, stateOf(component))`: the confirmation, then the component's
  NEW state as a JSON output (the same function `getState` answers), which MCP clients read as
  `structuredContent`.
- Validate ranges BEFORE `act()` (no exclusive lock taken for a refused call); describe every parameter
  with its unit; hints `ReadOnly` for getters, `Idempotent` for setters.
- ⚠️ **Refuse, never apply halfway**: a switch decided at creation (a light's shadow map, a visual's
  lighting/shadow/RT flags) is not exposed, and a value the engine would CLAMP or IGNORE is refused with
  the reason (ISO outside the camera range, spawn rate above the particle limit, a shadow setting on a
  light without shadow map, a pause on an emitter without source). Check the setter's body: a
  `std::clamp`/early `return` there means the command must check first.
- ⚠️ **Name nothing `near`/`far`**: windef.h defines both as macros (MSVC).
- An optional parameter in the middle (`std::optional`) is reachable by NAME (MCP) only: the console's
  positional syntax has no hole.
- Add an `appendXxxConsoleAdapters()` next to the component and call it from
  `Manager::onRegisterToConsole()` (the manager owns the adapters, `m_componentConsoleAdapters`, and
  registers them as its sub-objects: `Core.SceneManagerService.<Type>.*`, MCP `SceneManager_<Type>_*`).
- Keep `SceneManager_<Type>_<command>` ≤ 49 characters (MCP tool-name budget: a longer name is DROPPED
  from `tools/list`). `DirectionalPushModifier` leaves 12 characters for its commands.
- Engine fixes found while writing them: `DirectionalPushModifier::setCustomDirection()` /
  `disableCustomDirection()` had their flag calls INVERTED (a custom direction was overwritten by the
  next `move()`), fixed 2026-09-27; runtime shadow toggle → item `light-shadow-runtime-toggle`;
  `ParticlesEmitter::start(duration)` unit → item `particles-emitter-timeout-unit`. Entity positions in
  `listEntities()` → item `list-entities-positions`.

### Creating a New Component
0. ⚠️ **A component MAY move or query its own entity from `processLogics()`** (`Node::setPosition()`,
   `getComponent()`, `forEachComponent()`…): `m_componentsMutex` is a **recursive mutex** since
   2026-09-13, so the same-thread re-entry that used to deadlock the logic thread (black frame,
   live console — `SunCourse`) is legal by construction. A move requested from the loop is still
   DEFERRED by `onContainerMove()` to the end of the loop (same cycle) so the siblings are not
   moved mid-iteration — same family as the deferred `ComponentBoundariesModified` refresh. What
   stays FORBIDDEN from inside the loop is a structural change: `linkComponent()`,
   `removeComponent()`, `clearComponents()` trace an error and do nothing (they would invalidate
   the iteration) — a component leaves by returning `true` from `shouldBeRemoved()`.
1. Inherit from `Component::Abstract` (Abstract.hpp)
2. Implement `processLogics()` if per-frame logic needed
3. Implement `move()` if reaction to entity movement needed
4. Implement `onSuspend()`/`onWakeup()` (pure virtual, mandatory)
5. Register with Scene if automatic observation needed

### Suspend/Wakeup System (Scene Manager Level)
When Scene Manager changes active scene, entities and their components are suspended/woken up to release pooled resources (e.g., OpenAL audio sources).

**Architecture (Template Method Pattern):**

1. **AbstractEntity** (`AbstractEntity.hpp/.cpp`):
   - `suspend()` / `wakeup()` - Public non-virtual methods
   - Call entity's `onSuspend()`/`onWakeup()` then iterate components
   - `onSuspend()`/`onWakeup()` - Protected virtual hooks (default empty)

2. **Component::Abstract** (`Component/Abstract.hpp`):
   - `onSuspend()` / `onWakeup()` - Pure virtual protected (mandatory contract)
   - Called by `AbstractEntity` (friend class)
   - Each component must implement (even if empty)

**Call flow:**
```
Scene::disable() → entity->suspend() → entity->onSuspend()
                                     → component->onSuspend() (for each)

Scene::enable()  → entity->wakeup()  → entity->onWakeup()
                                     → component->onWakeup() (for each)
```

**Existing implementations:**
- `SoundEmitter`: Releases/reacquires audio source, remembers playing state
- Other components: Empty implementation (no pooled resources)

See `Scene.cpp:enable()`, `Scene.cpp:disable()`, `AbstractEntity.cpp:suspend()`, `AbstractEntity.cpp:wakeup()`
