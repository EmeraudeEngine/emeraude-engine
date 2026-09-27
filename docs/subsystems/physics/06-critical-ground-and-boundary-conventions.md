## Critical: Ground and Boundary Conventions (Y-UP, Aug 2026)

> [!CRITICAL]
> **The static-collision normal points FROM the body INTO the surface it penetrates.** That is what
> `applyCollisionResponse()` reads (`vn = dot(velocity, normal) > 0` means "moving into the surface")
> and what makes `positionCorrection -= normal * penetration` push the body OUT. It is the opposite
> of a surface's outward normal, hence the deliberate negation of `getNormalAt()` in
> `Scene::accumulateGroundCorrection()`.
>
> **Consequences now that +Y is up:**
> - A GROUND contact normal is `{0,-1,0}` (downward), never `{0,+1,0}`.
> - "Is this surface a floor?" is therefore `normal[Y] < -0.7`, **not** `> +0.7`. Three sites test
>   it (boundary, static entity, `applyCollisionResponse`) and all three are correct as written.
> - A body is below ground when `position[Y] < groundLevel`; the penetration is
>   `groundLevel - corner[Y]`; a sphere's lowest point is `position[Y] - radius`; clipping out of
>   the ground moves towards **+Y**.
>
> ⚠️ **The ground block is DUPLICATED THREE TIMES** (`clipAboveGround`, `detectGroundCollision`,
> `accumulateGroundCorrection` in `Scene.physics.cpp`) and the corner CHOICE (`minY*`) and the
> penetration ARITHMETIC are ONE decision. The Y-up flip moved the corners and left the subtraction
> reversed, so every airborne body reported a collision — a half-migration that landed INSIDE the
> comment warning against it. Only `accumulateGroundCorrection` is currently wired; the other two
> have no caller today.
>
> **The grounded micro-bounce clamp — it ate the jump (fixed Aug 2026).**
> `MovableTrait::updateSimulation()` zeroes the vertical velocity of a body resting on a stable
> surface, to kill micro-bounces. The test must clamp the **DOWNWARD** velocity, which is
> **NEGATIVE** now that +Y is up: `m_linearVelocity[Y] < 0.0F`. It read `> 0.0F` under Y-down,
> where positive meant sinking — left as it was, it clamped every UPWARD velocity, and since a
> body is still grounded on the very frame it pushes off, **the player's jump was destroyed
> before it could move**: the force was applied, the velocity went positive, and this line zeroed
> it every tick.
> ⚠️⚠️ **Signature to recognise, it is unmistakable**: `velocity[Y]` reads exactly ONE frame's
> worth of impulse instead of an accumulating value (here `cyclesLeft × 0.03`, decreasing with the
> ramp), while the POSITION never moves. That means the velocity is being wiped between frames —
> not that the force is missing. Measured against a healthy jump: velocity accumulates 0.48 → 4.05
> over the 16 push frames, `isGrounded` drops on frame 17, then gravity decays it by exactly
> 9.81/60 per frame.
>
> ⚠️⚠️ **BOUNDARIES CARRY NO CONVENTION.** The world box is `[-boundary, +boundary]³` and
> `clipInsideBoundaries()` / `accumulateBoundaryCorrection()` treat the three axes identically —
> there is nothing to migrate there, and a Y-sign "fix" applied to them is a regression. The only
> Y-dependent boundary decision is which wall grounds an entity, and it lives in
> `Scene::resolveCollisions()`.
