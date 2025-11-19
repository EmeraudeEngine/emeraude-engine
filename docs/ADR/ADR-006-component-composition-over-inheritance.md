# ADR-006: Component Composition Over Inheritance

## Status
**Accepted** - Core principle of scene graph architecture

## Context

Game engines need to represent diverse game objects with varying behaviors:
- Players (visual, audio, physics, input, camera)
- NPCs (visual, audio, physics, AI)  
- Buildings (visual, lights, no physics)
- Vehicles (visual, audio, physics, special controls)
- Projectiles (visual, physics, collision effects)

**Traditional Inheritance Approach:**
```cpp
class GameObject { /* base */ };
class Player : public GameObject { /* player specifics */ };
class NPC : public Player { /* reuse player code */ };  
class Vehicle : public GameObject { /* vehicle specifics */ };
class FlyingVehicle : public Vehicle { /* flight mechanics */ };
```

**Problems with Inheritance:**
- **Rigid hierarchies**: Diamond problem, deep inheritance chains
- **Code duplication**: Similar behaviors reimplemented across classes
- **Inflexible combinations**: What if Player needs Vehicle capabilities?
- **Maintenance burden**: Changes require modifications across hierarchy
- **Poor composition**: Hard to mix and match behaviors dynamically

**Alternative Patterns:**
1. **Entity-Component-System (ECS)**: Separate data and behavior completely
2. **Component Composition**: Entities as containers + Components as behaviors
3. **Mixins/Traits**: Multiple inheritance simulation
4. **Composition over inheritance**: Favor has-a over is-a relationships

## Decision

**Emeraude Engine uses Component Composition architecture where entities are generic containers that gain meaning through attached components.**

**Core Principle:**
> "Entities are positions in space. Components give them meaning."

**Architecture:**
```
AbstractEntity (component management)
├── Node (dynamic + hierarchical + physics)
└── StaticEntity (static + flat + optimized)

Components attach to entities:
- Visual (rendering)
- Light (illumination) 
- Camera (viewport)
- SoundEmitter (audio)
- Physics modifiers
- Custom behaviors
```

**Design Patterns:**
- **NO inheritance hierarchies**: Never create Player extends Node
- **Component composition**: Attach/detach behaviors dynamically  
- **Generic containers**: Two entity types (Node/StaticEntity) for all objects
- **Automatic registration**: Components auto-register with relevant systems

## Architecture Details

**Entity Types (Containers Only):**

**Node**: Dynamic entity in hierarchy tree
- Supports MovableTrait (physics, velocity, forces)
- Parent-child relationships with transformations
- Double-buffered state for thread safety
- Use for: players, vehicles, projectiles, movable objects

**StaticEntity**: Optimized entity in flat map
- No physics overhead, position set once
- Direct access by name (no hierarchy traversal)
- Still supports components (Visual, Light, etc.)
- Use for: buildings, terrain, static lights, skybox

**Component Examples:**
```cpp
// Player character
auto player = scene->root()->createChild("player", startPos);
player->newVisual(characterMesh, castShadows, receiveShadows, "body");
player->newCamera(90.0f, 16.0f/9.0f, 0.1f, 1000.0f, "player_cam");
player->newSoundEmitter(footstepSound, "footsteps");
// Physics handled by Node's MovableTrait

// Street lamp (static)
auto lamp = scene->createStaticEntity("lamp_01");  
lamp->newVisual(lampMesh, true, true, "lamp_visual");
lamp->newPointLight(Color::Warm, 100.0f, 20.0f, "street_light");
// No physics - StaticEntity never moves

// Vehicle with multiple parts
auto vehicle = scene->root()->createChild("car", vehiclePos);
vehicle->newVisual(carBodyMesh, true, true, "body");
auto wheelFL = vehicle->createChild("wheel_FL", wheelPos);
wheelFL->newVisual(wheelMesh, true, true, "wheel");
// Hierarchical: moving car moves wheels automatically
```

## Implementation Strategy

**Component Lifecycle:**
```cpp
class Component {
    virtual void processLogics();  // Called every frame (optional)
    virtual void move(const CartesianFrame& newLocation);  // Parent moved (optional)
};
```

**Automatic Registration:**
```cpp
// Components register with relevant systems automatically:
Visual      → Renderer (for rendering)
Light       → LightSet (for lighting calculations)  
Camera      → AVConsole (for render targets)
SoundEmitter → Audio system (for 3D positioning)
```

**Flexible Composition:**
```cpp
// Same entity can have multiple of same component type
entity->newVisual(bodyMesh, "body");
entity->newVisual(helmetMesh, "helmet");  
entity->newVisual(weaponMesh, "weapon");

// Components can be added/removed dynamically
entity->removeComponent("helmet");
entity->newLight(Color::Blue, 50.0f, "magic_aura");
```

## Consequences

### Positive
- **Flexibility**: Any entity can have any combination of components
- **No code duplication**: Component logic exists once, used everywhere
- **Composability**: Easy to add/remove behaviors at runtime
- **Maintainability**: Changes to component affect all users automatically  
- **Performance**: Pay only for what you use (StaticEntity skips physics)
- **No rigid hierarchies**: Avoid inheritance problems completely
- **Dynamic behavior**: Add/remove components during gameplay

### Negative
- **Indirection overhead**: Component lookup cost vs direct member access
- **Runtime polymorphism**: Virtual function calls for component methods
- **Memory fragmentation**: Components allocated separately vs monolithic objects
- **Learning curve**: Different mental model than traditional OOP

### Neutral
- **Industry adoption**: Component systems are standard in modern game engines
- **Cache locality**: Trade-off between flexibility and cache performance

## Comparison with Alternatives

**vs Traditional Inheritance:**
- ✅ No rigid hierarchies, better code reuse
- ❌ Some runtime overhead vs direct member access

**vs Pure ECS:**
- ✅ Simpler mental model, better integration with scene graph
- ❌ Less cache-friendly than data-oriented ECS

**vs Monolithic Objects:**
- ✅ Flexible composition, no code duplication
- ❌ More complex object creation pattern

## Integration Points

**Scene Graph Integration:**
- AbstractEntity manages component lifetime
- Node hierarchy propagates transforms to components
- Double buffering ensures thread-safe component access

**Physics Integration:**
- Node uses MovableTrait for physics simulation
- StaticEntity skips physics for performance
- Components read entity position for spatial behaviors

**Resource Integration:**
- Visual components use RenderableInstances (graphics resources)
- SoundEmitter components use SoundResources (audio resources)
- Components follow fail-safe resource patterns

**Automatic Systems:**
- Observer pattern connects components to engine systems
- No manual registration required
- Components work immediately when attached

## Design Guidelines

**DO:**
```cpp
// Use composition
auto player = scene->root()->createChild("player");
player->newVisual(mesh);
player->newCamera(fov);

// Leverage automatic registration
// Components work immediately when attached

// Use appropriate entity type
StaticEntity for immovable objects (buildings)
Node for dynamic objects (characters, vehicles)
```

**DON'T:**
```cpp
// Don't create inheritance hierarchies
class Player : public Node { /* AVOID */ };

// Don't manage component registration manually
renderer.addVisual(visual);  // AVOID - automatic registration

// Don't use wrong entity type  
Node for static buildings;  // AVOID - use StaticEntity
StaticEntity for characters;  // AVOID - use Node
```

## Related ADRs
- ADR-008: Double Buffering Thread Safety (entities use double buffering)
- ADR-005: Graphics Instancing System (Visual components use RenderableInstances) 
- ADR-003: Fail-Safe Resource Management (components use fail-safe resources)

## References
- `docs/scene-graph-architecture.md` - Complete scene graph and component architecture
- `src/Scenes/AGENTS.md` - Scene system implementation context
- `src/Scenes/Component/Abstract.hpp` - Base component class definition