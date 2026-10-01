## Rigid-body rotation (physics overhaul P3, 2026-10-02)

Owner decisions: `docs/physics-overhaul.md` § 1.8. Code: `Physics/BodyInertia.hpp`, the collision models'
`centerOfMassOffset()` / `solidInertia()`, `Scene::resolveCollisions()` (steps 1 and 5).

### Who rotates

- **Every dynamic body, by default** (`MovableTrait::m_rotationEnabled` is `true`): a crate tumbles, a ball rolls.
- **Characters do not**: projet-alpha's `Actor::AbstractLiving` (Player, Paladin, Fox, Drone) calls
  `enableRotationPhysics(false)` — a character turns by steering, and the P4 kinematic controller never takes its
  rotation from the contacts either. Turn it off for any other body that must stay upright.
- A point model never rotates (no tensor), and neither does a body with no usable shape yet or a null mass.

### About which point: the centre of mass

The solver works on the CENTRE OF MASS = the centroid of the collision shape (`centerOfMassOffset(scaling)`, in the
entity's axes; Box2D v3 `localCenter`). The scene step places `Body::position` there and keeps the world offset
`Body::centerOffset`; the write-back moves the node's ORIGIN by `Δcom + c − ΔR · c`, because `rotateFromPhysics()` turns
the node about its origin. A model whose origin is at its base (most glTF props, the humanoids) therefore tumbles about
its middle, not about its feet. Box: the local box's centroid × the frame scaling; sphere: the origin; capsule: the
middle of its axis (unscaled, as the narrow phase uses them). `Node::getWorldCenterOfMass()` answers the same point.

### Inertia: explicit, or derived from the shape

- `BodyPhysicalProperties::inertiaTensor()` is a `std::optional`: an author sets one with `setInertiaTensor()` (or a
  value in the 7-argument `setProperties()` / the constructor); `{}` or `resetInertiaTensor()` means DERIVED. The JSON
  never sets it (`InertiaKey` is not read — owner, decision 8c).
- Derived = the shape as a uniform solid of the body's mass (`solidInertia(mass, scaling)` → emeraude-base
  `RigidBody::solidBoxInertia` / `solidSphereInertia` / `solidCapsuleInertia`; a capsule whose axis is not Y turns its
  tensor: `I = I⊥ Id + (I∥ − I⊥) a aᵀ`).
- `Physics::localInverseInertia(properties, model, scaling)` picks the explicit tensor, else the derived one, and
  inverts it (zero = no rotation); `worldInverseInertia(localInverse, R)` = `R · I⁻¹ · Rᵀ`. The scene step computes it
  every step from the WORLD frame (a child node is right too); nothing caches it any more.
- An entity with several massive components keeps no explicit tensor: its inertia is derived from its collision shape
  and its total mass. `BodyPhysicalProperties::merge()` sums two explicit tensors (exact about a common centre of mass).
- ⚠️ Before P3 every toolkit body carried the IDENTITY (a 1 m, 10 kg cube has 1.67 kg·m²): six times too little
  inertia for it, far too much for a light object.

### Angular drag

`ω *= Physics::getAngularDragFactor(c, dt)` = `(1 − c)^(dt / WorldPhysicsUpdateCycleDurationS)`: `c` keeps its meaning
("the fraction of ω lost per logic cycle", default 0.1) and is integrated exactly for any step (decision 8e). At the
fixed cycle it is bit-for-bit the former `1 − c`.

### Removed in P3 (no caller)

`MovableTrait::applyAngularImpulse()`, `updateInverseWorldInertia()`, `inverseWorldInertia()` (the cache was refreshed
from the LOCAL rotation on every move of every node) and `setCenterOfMass()` / `centerOfMass()` (an author offset that
nothing used and that decision 8b replaces).

### Measured (Linux, `collision-debug`, 3 recorded launches bit-identical)

`docs/physics-overhaul.md` § 1b, P3. Rolling: once on the ground the slope ball rolls WITHOUT SLIPPING, |v| = |ω| r
within 2 % (3.159 / 3.101 m/s at cycle 120, 0.566 / 0.555 at cycle 240), both dying out through the angular drag; on
the slope it still slips (5.3 vs 3.4 m/s at cycle 60).
