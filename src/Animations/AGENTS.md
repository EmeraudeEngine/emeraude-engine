# Animations System

Context for developing the Emeraude Engine animations system.

## Module Overview

**Status: UNDER CONSTRUCTION** - Skeletal animation system currently in development.

## Animations-Specific Rules

### System Scope
- **Skeletal animations**: Skinning, mesh deformation via skeleton
- **Animation interfaces**: Possibility to animate values via interfaces
- **NOT transform animations**: Use scene graph for position/rotation/scale animation

### Architecture (to be defined)
The skeletal animation system is currently under active development. Architecture and features will be documented once stabilized.

### Planned Features
- Skeletons and bones
- Skinning (vertex deformation by bones)
- Timeline and keyframes
- Skeletal animation blending
- Interfaces for custom value animation

## Development Commands

```bash
# Animation tests (when available)
ctest -R Animations
./test --filter="*Animation*"
```

## Important Files

To be documented upon system stabilization.

## Development Patterns

To be documented upon system stabilization.

## Critical Points

- **Under active development**: Architecture subject to change
- Consult developers before major modifications

## Detailed Documentation

Documentation to be created once the system is stabilized.
