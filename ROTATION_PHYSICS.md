# Rotation Physics Implementation

## Overview

This document describes the rotation physics system implemented in Emeraude-Engine. The system provides realistic angular motion including torque application, angular drag, and angular momentum transfer during collisions.

## Features

### 1. Torque Application
Objects can now have torque applied to them, causing angular acceleration based on their moment of inertia.

**Formula:** `α = I⁻¹ × T`
- `α`: Angular acceleration (rad/s²)
- `I`: Inertia tensor (3×3 matrix)
- `T`: Torque vector (N⋅m)

### 2. Moment of Inertia
Each object has an inertia tensor that defines its resistance to rotation. Helper functions are provided to calculate inertia for common shapes.

### 3. Angular Drag
Objects experience angular damping based on an angular drag coefficient, simulating air resistance and friction.

### 4. Collision-Induced Rotation
When objects collide, they transfer angular momentum based on the impact point relative to their center of mass.

## Usage

### Setting Up an Object for Rotation

```cpp
// 1. Get the physical object properties
auto& physProps = entity.bodyPhysicalProperties();

// 2. Calculate and set the moment of inertia tensor
// For a cuboid (box):
const float width = 2.0f;  // meters
const float height = 1.0f; // meters
const float depth = 1.5f;  // meters
const float mass = physProps.mass();

auto inertiaTensor = Physics::Inertia::computeCuboid(mass, width, height, depth);
physProps.setInertia(inertiaTensor);

// 3. Set angular drag coefficient (optional, default is 0.1)
// 0.0 = no drag (perpetual rotation)
// 1.0 = immediate stop
physProps.setAngularDrag(0.15f);
```

### Available Inertia Calculation Functions

All functions are in the `EmEn::Physics::Inertia` namespace:

#### Solid Cuboid (Box)
```cpp
auto inertia = Physics::Inertia::computeCuboid(mass, width, height, depth);
```
Formula:
- `Ixx = m × (h² + d²) / 12`
- `Iyy = m × (w² + d²) / 12`
- `Izz = m × (w² + h²) / 12`

#### Solid Sphere
```cpp
auto inertia = Physics::Inertia::computeSphere(mass, radius);
```
Formula: `I = (2/5) × m × r²`

#### Solid Cylinder
```cpp
auto inertia = Physics::Inertia::computeCylinder(mass, radius, height);
```
Formula:
- `Ixx = Izz = (1/12) × m × (3r² + h²)`
- `Iyy = (1/2) × m × r²` (rotation around Y axis)

#### Hollow Sphere (Shell)
```cpp
auto inertia = Physics::Inertia::computeHollowSphere(mass, radius);
```
Formula: `I = (2/3) × m × r²`

#### Capsule
```cpp
auto inertia = Physics::Inertia::computeCapsule(mass, radius, cylinderHeight);
```
Approximation using combined cylinder and hemisphere inertias.

### Applying Torque Manually

```cpp
// Get the movable trait
auto* movableTrait = entity.getMovableTrait();

// Create a torque vector (axis × magnitude)
// Example: Torque around Y axis with magnitude 10 N⋅m
Libs::Math::Vector<3, float> torque{0.0f, 10.0f, 0.0f};

// Apply the torque
movableTrait->addTorque(torque);
```

### Setting Initial Angular Velocity

```cpp
auto* movableTrait = entity.getMovableTrait();

// Set angular velocity (rad/s) around an axis
// Example: Rotate around Y axis at 2 rad/s
Libs::Math::Vector<3, float> angularVelocity{0.0f, 2.0f, 0.0f};
movableTrait->setAngularVelocity(angularVelocity);
```

### Checking Rotation State

```cpp
auto* movableTrait = entity.getMovableTrait();

// Check if object is spinning
if (movableTrait->isSpinning()) {
    // Get angular velocity vector
    const auto& angularVel = movableTrait->angularVelocity();

    // Get angular speed (magnitude)
    float angularSpeed = movableTrait->angularSpeed(); // rad/s
}
```

## Physics Simulation Details

### Update Cycle
- Physics updates run at 60Hz (16.67ms per cycle)
- Angular velocity is integrated: `ω_new = ω_old + α × Δt`
- Angular drag is applied: `ω_new = ω_old × (1 - drag)`
- Rotation is applied to the entity transform

### Collision-Induced Rotation

When two objects collide, the system automatically:
1. Calculates the lever arm from center of mass to collision point: `r = collision_pos - center_pos`
2. Computes the impulse force at the collision point based on relative velocity and masses
3. Calculates torque: `τ = r × F`
4. Applies torque to both objects (or just the moving object for static collisions)

This happens automatically for:
- Scene boundary collisions
- Ground collisions
- Static entity collisions
- Movable entity collisions

### Angular Drag

The angular drag coefficient controls how quickly rotation slows down:
- **0.0**: No drag - object rotates forever (space-like)
- **0.1**: Low drag - slow deceleration (default)
- **0.5**: Medium drag - moderate deceleration
- **1.0**: High drag - rapid deceleration (water-like)

The damping is applied per frame: `velocity *= (1 - drag)`

## Example: Complete Setup

```cpp
// Create an entity with physical properties
auto entity = createEntity();

// Set up basic physics (mass, surface, drag coefficient)
auto& physProps = entity.bodyPhysicalProperties();
physProps.setProperties(
    10.0f,  // mass (kg)
    1.0f,   // surface area (m²)
    Physics::DragCoefficient::Cube<float>, // drag coefficient
    0.5f,   // bounciness
    0.5f    // stickiness
);

// Calculate and set moment of inertia for a 2×1×1.5 meter box
auto inertia = Physics::Inertia::computeCuboid(
    physProps.mass(),
    2.0f,  // width
    1.0f,  // height
    1.5f   // depth
);
physProps.setInertia(inertia);

// Set angular drag for realistic slowdown
physProps.setAngularDrag(0.15f);

// Give it an initial spin (optional)
auto* movableTrait = entity.getMovableTrait();
movableTrait->setAngularVelocity({0.0f, 3.14f, 0.0f}); // ~180°/s around Y axis

// Apply an initial torque (optional)
movableTrait->addTorque({0.0f, 5.0f, 0.0f}); // 5 N⋅m around Y axis
```

## Technical Notes

### Coordinate System
- X: Width (right)
- Y: Height (up)
- Z: Depth (forward)

### Units
- Mass: kilograms (kg)
- Distance: meters (m)
- Torque: Newton-meters (N⋅m)
- Angular velocity: radians per second (rad/s)
- Angular acceleration: radians per second squared (rad/s²)
- Inertia: kilogram-meter squared (kg⋅m²)

### Performance
- Inertia calculation is done once during setup, not every frame
- Angular physics adds minimal overhead to the simulation
- Torque calculations use efficient matrix-vector operations

## Future Improvements

Potential enhancements:
1. More physically accurate angular drag (based on shape and environment density)
2. Gyroscopic effects for spinning objects
3. Additional inertia tensor helpers for complex shapes
4. Rolling friction simulation
5. Advanced collision response with friction coefficients

## References

- Physics formulas: Classical mechanics, rigid body dynamics
- Collision response: Chris Hecker's Rigid Body Dynamics articles
- Inertia tensors: Engineering mechanics textbooks
