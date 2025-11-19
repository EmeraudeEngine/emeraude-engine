# ADR-008: Double Buffering for Thread Safety

## Status
**Accepted** - Implemented across Scene Graph entities for simulation/rendering separation

## Context

Game engines typically run on multiple threads to maximize performance:
- **Logic Thread**: Physics simulation, game logic, AI (usually fixed 60 Hz)
- **Render Thread**: Graphics rendering, command buffer building (variable FPS)
- **Additional Threads**: Resource loading, audio processing, networking

**The Fundamental Problem:**
```
Logic Thread (60 Hz):           Render Thread (Variable FPS):
- Update physics                - Read positions  
- Move entities                 - Build draw calls
- Modify transforms             - Submit to GPU
- Change positions              - Present frame
```

**Without Synchronization:**
- Render thread reads entity positions **while** logic thread is writing them
- Results: Torn reads, visual artifacts, objects "jumping" between frames
- Race conditions: Position partially updated (X changed, Y/Z not yet)

**Traditional Solutions:**
1. **Single threaded**: Logic and rendering on same thread (performance loss)
2. **Locks/Mutexes**: Synchronize access (contention, complexity, deadlock risk)
3. **Message passing**: Logic sends position updates to render (latency, complexity)
4. **Immutable snapshots**: Copy entire world state each frame (memory/CPU overhead)

## Decision

**Emeraude Engine implements double buffering where each entity maintains two complete transform states: one for logic thread writes and one for render thread reads.**

**Core Principle:**
> "Logic thread owns active state, render thread owns render state, atomic swap synchronizes them."

**Architecture:**
```cpp
class Node {
    CartesianFrame m_activeFrame;   // Written by logic thread (60 Hz)
    CartesianFrame m_renderFrame;   // Read by render thread (variable FPS)
    
    // Atomic swap at end of logic frame
    void swapFrames() { m_renderFrame = m_activeFrame; }
};
```

**Thread Safety Model:**
- **Logic thread**: Exclusively owns and modifies m_activeFrame
- **Render thread**: Exclusively reads from m_renderFrame (never writes)
- **Synchronization**: Single atomic assignment per frame
- **No locks required**: Read-only access eliminates contention

## Implementation Strategy

**Entity Double Buffering:**

**Node (Dynamic Entities):**
```cpp
// Logic thread (writes active)
void Node::setPosition(const Vector3& pos) {
    m_activeFrame.position = pos;  // Safe: logic thread exclusive access
}

// Render thread (reads render)  
Vector3 Node::getRenderPosition() const {
    return m_renderFrame.position;  // Safe: render-only data
}

// End of logic frame (atomic swap)
void Scene::swapAllFrames() {
    for (auto& node : allNodes) {
        node->m_renderFrame = node->m_activeFrame;  // Atomic assignment
    }
}
```

**StaticEntity (Static Entities):**
```cpp
// StaticEntity also uses double buffering for consistency
// Even though they don't move, maintains same interface
class StaticEntity {
    CartesianFrame m_activeFrame;
    CartesianFrame m_renderFrame;
    
    // Position set once during scene setup (logic thread)
    void setPosition(const Vector3& pos) {
        m_activeFrame.position = pos;
    }
    
    // Render thread reads render frame
    Vector3 getRenderPosition() const {
        return m_renderFrame.position;
    }
};
```

**Frame Synchronization Point:**
```cpp
// Scene update cycle (logic thread - 60 Hz)
void Scene::update(float deltaTime) {
    // 1. Physics integration (writes to active frames)
    physics.integrateForces(deltaTime);
    physics.resolveCollisions();
    
    // 2. Update scene graph (writes to active frames)  
    updateNodeHierarchy();
    
    // 3. Component processing (writes to active frames)
    for (auto& node : m_nodes) {
        for (auto& component : node->components()) {
            component->processLogics();
        }
    }
    
    // 4. ATOMIC SWAP - synchronization point
    for (auto& node : m_nodes) {
        node->swapFrames();  // m_renderFrame = m_activeFrame
    }
    for (auto& entity : m_staticEntities) {
        entity->swapFrames();
    }
    
    // Scene now ready for next render frame
}
```

## Consequences

### Positive
- **Lock-free rendering**: Render thread never blocks on logic thread
- **Visual stability**: No torn reads, no objects jumping between partial updates
- **Performance**: No mutex contention, no synchronization overhead
- **Temporal consistency**: Entire scene snapshot from single logic frame
- **Simple mental model**: Clear ownership (logic writes, render reads)
- **Smooth rendering**: Render thread can interpolate between frames if needed

### Negative
- **Memory overhead**: Every entity stores two complete transform states
- **One frame latency**: Rendering always one frame behind logic
- **Copy cost**: Atomic assignment of CartesianFrame each entity each frame
- **Cache impact**: Double memory footprint may affect cache locality

### Neutral
- **Industry standard**: Most real-time engines use similar approaches
- **Latency vs stability**: Trade one frame of latency for visual stability

## Performance Characteristics

**Memory Overhead:**
```cpp
// Per entity memory cost
sizeof(CartesianFrame) × 2 = ~128 bytes per entity
1000 entities = ~128 KB additional memory (negligible)
```

**CPU Overhead:**
```cpp
// Per frame cost (logic thread)
1000 entities × 1 CartesianFrame assignment = ~1000 assignments/frame
At 60 FPS = 60,000 assignments/second (negligible modern CPU cost)
```

**Cache Implications:**
- Active and render frames stored adjacently (good spatial locality)
- Render thread accesses only render frames (predictable pattern)
- Logic thread accesses only active frames (separate cache lines)

## Advanced Considerations

**Interpolation Support:**
```cpp
// Render thread can interpolate for smooth motion
Vector3 interpolatedPosition(float alpha) const {
    // alpha = 0.0 → previous frame, alpha = 1.0 → current frame
    return lerp(m_previousRenderFrame.position, m_renderFrame.position, alpha);
}
```

**Component Integration:**
```cpp
// Components read from render frames during rendering
void Visual::updateRenderTransform() {
    // Safe: render thread reading render-only data
    Matrix4 transform = m_parent->getRenderFrame().toMatrix();
    m_renderableInstance.setTransform(transform);
}
```

**Network Synchronization:**
```cpp
// Network updates write to active frame (logic thread)
void Node::applyNetworkUpdate(const NetworkTransform& update) {
    m_activeFrame.position = update.position;     // Logic thread
    m_activeFrame.rotation = update.rotation;     // Safe to write
}
```

## Alternative Approaches Considered

**Lock-Based Synchronization:**
- ❌ Mutex contention between threads
- ❌ Potential deadlocks with multiple entities
- ❌ Performance unpredictability

**Message Queues:**
- ❌ Additional latency from message passing
- ❌ Memory allocation/deallocation overhead
- ❌ Complex lifetime management

**Single-Threaded:**
- ❌ Significant performance loss
- ❌ Frame rate becomes CPU-bound

**Triple Buffering:**
- ✅ Potentially lower latency
- ❌ Additional memory overhead
- ❌ More complex synchronization

**Copy-on-Write:**
- ✅ Memory efficient for mostly-static scenes  
- ❌ Complex implementation for frequent updates
- ❌ Performance unpredictability

## Integration Points

**Physics Integration:**
- Physics updates active frames during simulation
- Physics reads active frames for collision detection
- Render thread never accesses physics-modified data directly

**Graphics Integration:**
- Visual components read render frames for transform matrices
- Renderer builds command buffers using render frame data
- Graphics thread never modifies entity positions

**Resource Loading:**
- Resources loaded on background threads
- Entity component attachment happens on logic thread
- Render thread sees completed, stable component state

## Related ADRs
- ADR-006: Component Composition Over Inheritance (components use double buffered entity data)
- ADR-007: Physics Four-Entity Architecture (physics writes to active frames)
- ADR-005: Graphics Instancing System (rendering reads from render frames)

## References
- `docs/scene-graph-architecture.md` - Complete double buffering implementation
- `src/Scenes/AGENTS.md` - Scene system double buffering context  
- `src/Graphics/AGENTS.md` - Rendering integration with double buffering