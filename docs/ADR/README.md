# Architecture Decision Records (ADR)

This directory contains the Architecture Decision Records for Emeraude Engine. Each ADR documents a significant architectural decision that affects the entire engine or large subsystems.

## ADR Index

| ADR | Title | Status | Category |
|-----|-------|--------|----------|
| [ADR-001](ADR-001-Y-down-coordinate-system.md) | Y-Down Coordinate System | Accepted | Core |
| [ADR-002](ADR-002-vulkan-only-graphics-api.md) | Vulkan-Only Graphics API | Accepted | Graphics |
| [ADR-003](ADR-003-fail-safe-resource-management.md) | Fail-Safe Resource Management | Accepted | Resources |
| [ADR-004](ADR-004-saphir-shader-generation.md) | Saphir Automatic Shader Generation System | Accepted | Graphics |
| [ADR-005](ADR-005-graphics-instancing-system.md) | Graphics Instancing System Architecture | Accepted | Graphics |
| [ADR-006](ADR-006-component-composition-over-inheritance.md) | Component Composition Over Inheritance | Accepted | Architecture |
| [ADR-007](ADR-007-physics-four-entity-architecture.md) | Physics Four-Entity Architecture | Accepted | Physics |
| [ADR-008](ADR-008-double-buffering-thread-safety.md) | Double Buffering for Thread Safety | Accepted | Threading |
| [ADR-009](ADR-009-service-locator-pattern.md) | Service Locator Pattern | Accepted | Architecture |
| [ADR-010](ADR-010-vulkan-abstraction-layer.md) | Vulkan Abstraction Layer | Accepted | Graphics |

## Categories

### Core Foundation
- **ADR-001**: Y-Down Coordinate System - Fundamental spatial convention affecting all subsystems
- **ADR-009**: Service Locator Pattern - Dependency injection mechanism throughout engine

### Graphics & Rendering  
- **ADR-002**: Vulkan-Only Graphics API - Exclusive use of modern Vulkan API
- **ADR-004**: Saphir Shader Generation - Automatic GLSL generation from parameters
- **ADR-005**: Graphics Instancing System - Efficient rendering architecture for duplicates
- **ADR-010**: Vulkan Abstraction Layer - High-level abstraction over raw Vulkan

### Resource Management
- **ADR-003**: Fail-Safe Resource Management - Never-crash resource loading philosophy

### Scene & Entity Management
- **ADR-006**: Component Composition Over Inheritance - Entity-component architecture
- **ADR-007**: Physics Four-Entity Architecture - Specialized physics collision semantics

### Threading & Performance
- **ADR-008**: Double Buffering Thread Safety - Simulation/rendering thread separation

## Decision Dependencies

```
ADR-001 (Y-Down) 
    ├── ADR-002 (Vulkan) - Aligns with Vulkan's Y-down viewport
    ├── ADR-007 (Physics) - Y-down gravity implementation
    └── ADR-008 (Double Buffer) - Y-down in both active/render frames

ADR-002 (Vulkan-Only)
    ├── ADR-004 (Saphir) - Generates Vulkan-compatible shaders
    ├── ADR-005 (Instancing) - Built on Vulkan abstractions
    └── ADR-010 (Abstraction) - Makes Vulkan usable

ADR-003 (Fail-Safe)
    ├── ADR-004 (Saphir) - Shader failures handled gracefully
    ├── ADR-005 (Instancing) - Resources shared safely
    └── ADR-006 (Components) - Components use fail-safe resources

ADR-006 (Components)
    ├── ADR-008 (Double Buffer) - Components use double-buffered data
    └── ADR-009 (Services) - Components access services via ServiceProvider

ADR-009 (Services)
    └── ADR-003 (Fail-Safe) - Resources use ServiceProvider for dependencies
```

## ADR Format

Each ADR follows this standard format:

- **Status**: Accepted/Deprecated/Superseded
- **Context**: Why this decision was necessary
- **Decision**: What was decided
- **Consequences**: Benefits and drawbacks
- **Implementation**: Key technical details
- **Related ADRs**: Dependencies and relationships
- **References**: Relevant documentation

## Reading Guide

### For New Developers
1. Start with **ADR-001** (Y-Down) - affects all development
2. Read **ADR-003** (Fail-Safe) - fundamental engine philosophy
3. Read **ADR-006** (Components) - basic scene architecture

### For Graphics Developers
1. **ADR-002** (Vulkan-Only) - graphics API decision
2. **ADR-010** (Vulkan Abstraction) - how to use graphics APIs
3. **ADR-004** (Saphir) - automatic shader generation
4. **ADR-005** (Instancing) - rendering architecture

### For Gameplay Developers
1. **ADR-006** (Components) - entity composition patterns
2. **ADR-007** (Physics) - physics entity types and behaviors
3. **ADR-008** (Double Buffer) - threading considerations
4. **ADR-009** (Services) - accessing engine systems

### For Systems Developers
1. **ADR-009** (Services) - dependency injection architecture
2. **ADR-008** (Double Buffer) - multi-threading design
3. **ADR-003** (Fail-Safe) - error handling philosophy

## Evolution

ADRs capture decisions at specific points in time. When architectural decisions change:

1. **Mark old ADR as "Superseded"** with reference to replacement
2. **Create new ADR** documenting the new decision
3. **Update this index** with status changes
4. **Update dependency graph** to reflect new relationships

## Implementation Status

All ADRs listed above are **Accepted** and actively implemented in Emeraude Engine v0.8.33+. These decisions form the stable architectural foundation of the engine.