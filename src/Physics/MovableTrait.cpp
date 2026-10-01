/*
 * src/Physics/MovableTrait.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
 *
 * Emeraude-Engine is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * Emeraude-Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Emeraude-Engine; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "MovableTrait.hpp"

/* STL inclusions. */
#include <limits>

/* Local inclusions. */

namespace EmEn::Physics
{
	using namespace Base;
	using namespace Base::Math;

	void
	MovableTrait::setMinimalVelocity (const Vector< 3, float > & velocity) noexcept
	{
		for ( size_t axis = 0; axis < 3; ++axis )
		{
			if ( m_linearVelocity[axis] >= 0.0F && velocity[axis] >= 0.0F )
			{
				m_linearVelocity[axis] = std::max(m_linearVelocity[axis], velocity[axis]);
			}
			else if ( m_linearVelocity[axis] < 0.0F && velocity[axis] < 0.0F )
			{
				m_linearVelocity[axis] = std::min(m_linearVelocity[axis], velocity[axis]);
			}
			else
			{
				m_linearVelocity[axis] += velocity[axis];
			}
		}

		m_linearSpeed = m_linearVelocity.length();

		this->onImpulse();
	}

	void
	MovableTrait::addForce (const Vector< 3, float > & force) noexcept
	{
		const auto & objectProperties = this->getBodyPhysicalProperties();

		/* NOTE: If the object mass is null, we discard the force. */
		if ( objectProperties.isMassNull() )
		{
			return;
		}

		/* a = F * 1/m */
		this->addAcceleration(force * objectProperties.inverseMass());
	}

	void
	MovableTrait::stopMovement () noexcept
	{
		m_linearVelocity.reset();
		m_angularVelocity.reset();

		m_linearSpeed = 0.0F;
		m_angularSpeed = 0.0F;
	}

	bool
	MovableTrait::updateSimulation (const EnvironmentPhysicalProperties & envProperties, bool integratedByScene) noexcept
	{
		const auto & objectProperties = this->getBodyPhysicalProperties();

		/* Decay grounded state each frame (the physics step re-arms it from its contacts). */
		this->updateGroundedState();

		/* ⚠️ Physics overhaul P2: no gravity switch-off on a "stable surface", no downward-speed clamp, no
		 * per-tick friction multiplier any more — resting and friction are the contact solver's job, and gravity
		 * always applies (inside the step's sub-steps for a body the scene integrates). The former special cases
		 * are recorded in docs/physics-overhaul.md § 2 D. */
		if ( !integratedByScene && !this->isFreeFlyModeEnabled() && !objectProperties.isMassNull() )
		{
			m_linearVelocity += envProperties.steppedSurfaceGravity();
			m_linearSpeed = m_linearVelocity.length();
		}

		/* Apply the drag if there is linear speed (a massless body ignores forces, as addForce() does). */
		if ( m_linearSpeed > 0.0F && !objectProperties.isMassNull() )
		{
			m_linearVelocity *= Physics::getDragVelocityFactor(
				objectProperties.dragCoefficient(),
				envProperties.atmosphericDensity(),
				m_linearSpeed,
				objectProperties.surface(),
				objectProperties.inverseMass(),
				WorldPhysicsUpdateCycleDurationS< float >
			);
			m_linearSpeed = m_linearVelocity.length();
		}

		/* Apply the drag on rotation if there is angular speed and rotation is enabled.
		 * A simple damping coefficient: velocity *= 1 - drag per cycle (0 = perpetual rotation, 1 = immediate stop).
		 * NOTE: tick-rate dependent; its exact integration is item `rotational-physics`. */
		if ( m_rotationEnabled && m_angularSpeed > 0.0F )
		{
			m_angularVelocity *= 1.0F - objectProperties.angularDragCoefficient();
			m_angularSpeed = m_angularVelocity.length();
		}

		/* A body the scene integrates is moved (and rotated) by the physics step. */
		if ( integratedByScene )
		{
			return false;
		}

		bool isMoveOccurs = false;

		if ( m_linearSpeed > 0.0F )
		{
			this->moveFromPhysics(m_linearVelocity * WorldPhysicsUpdateCycleDurationS< float >);
			isMoveOccurs = true;
		}

		/* NOTE: Vector / s is NaN once |s| <= the float epsilon: 1 / speed stays finite above FLT_MIN. */
		if ( m_rotationEnabled && m_angularSpeed > std::numeric_limits< float >::min() )
		{
			this->rotateFromPhysics(
				m_angularSpeed * WorldPhysicsUpdateCycleDurationS< float >,
				m_angularVelocity * (1.0F / m_angularSpeed)
			);
			isMoveOccurs = true;
		}

		return isMoveOccurs;
	}

	void
	MovableTrait::setGrounded (GroundedSource source, const MovableTrait * groundedOn) noexcept
	{
		m_groundedSource = source;
		m_groundedOn = groundedOn;
		m_groundedFrames = GroundedGracePeriod;
	}

	void
	MovableTrait::clearGrounded () noexcept
	{
		m_groundedSource = GroundedSource::None;
		m_groundedOn = nullptr;
		m_groundedFrames = 0;
	}

	void
	MovableTrait::updateGroundedState () noexcept
	{
		/* ⚠️⚠️ The grace period must ALWAYS decay. It used to return early when the vertical
		 * velocity was negligible ("don't decay grounded state if Y velocity is negligible"),
		 * which closed a CIRCULAR LOCK with updateSimulation():
		 *   grounded on a stable surface -> gravity is not applied at all
		 *   -> the vertical velocity stays 0
		 *   -> this early return refused to decay the grace period
		 *   -> still grounded -> gravity still skipped, forever.
		 * A body resting on Ground or Boundary that LOST its support therefore never fell again:
		 * teleport it into the air and it hovers there permanently. Measured Aug 2026 on both a
		 * ball and the player (`Act.setPosition(x, 80, z)` — a console command this project uses
		 * constantly), still at Y = 80 five seconds later.
		 * ⚠️ Decaying unconditionally is safe BECAUSE contact re-arms the period every frame it is
		 * detected (`setGrounded()` resets it to GroundedGracePeriod). The countdown therefore only
		 * ever runs down while contact is genuinely absent — which is exactly when it must expire.
		 * ⚠️ Bodies grounded on an Entity were immune: updateSimulation() keeps applying gravity to
		 * them on purpose, so they could always fall off. Only the stable-surface path locked up. */
		if ( m_groundedFrames > 0 )
		{
			m_groundedFrames--;

			/* Clear the grounded source when grace period expires. */
			if ( m_groundedFrames == 0 )
			{
				m_groundedSource = GroundedSource::None;
				m_groundedOn = nullptr;
			}
		}
	}

	bool
	MovableTrait::isGrounded () const noexcept
	{
		return m_groundedFrames > 0;
	}

	bool
	MovableTrait::isGroundedOnTerrain () const noexcept
	{
		return m_groundedFrames > 0 && m_groundedSource == GroundedSource::Ground;
	}

	bool
	MovableTrait::isGroundedOnBoundary () const noexcept
	{
		return m_groundedFrames > 0 && m_groundedSource == GroundedSource::Boundary;
	}

	bool
	MovableTrait::isGroundedOnEntity () const noexcept
	{
		return m_groundedFrames > 0 && m_groundedSource == GroundedSource::Entity;
	}

	bool
	MovableTrait::isGroundedOn (const MovableTrait * entity) const noexcept
	{
		return m_groundedFrames > 0 && m_groundedSource == GroundedSource::Entity && m_groundedOn == entity;
	}

	GroundedSource
	MovableTrait::groundedSource () const noexcept
	{
		return m_groundedFrames > 0 ? m_groundedSource : GroundedSource::None;
	}

	bool
	MovableTrait::checkSimulationInertia () noexcept
	{
		constexpr auto VelocityThreshold{0.05F}; /* 5 cm/s */

		/* Check if velocity is negligible. */
		const bool isStable = (m_linearSpeed < VelocityThreshold) && (m_angularSpeed < VelocityThreshold);

		/* Sleep only allowed when ACTIVELY touching a stable surface this frame.
		 * m_groundedFrames == GroundedGracePeriod means we just touched the surface.
		 * Grace period alone (bouncing but not touching) must not allow sleep. */
		const bool isActivelyOnStableSurface =
			(m_groundedFrames == GroundedGracePeriod) &&
			(m_groundedSource == GroundedSource::Ground || m_groundedSource == GroundedSource::Boundary);

		if ( isStable && isActivelyOnStableSurface )
		{
			/* Increment stable frames counter. */
			if ( m_stableFrames < StableFramesThreshold )
			{
				m_stableFrames++;
			}

			/* After enough stable frames, entity can sleep. */
			if ( m_stableFrames >= StableFramesThreshold )
			{
				/* Clamp micro-velocities to zero. */
				m_linearVelocity.reset();
				m_angularVelocity.reset();
				m_linearSpeed = 0.0F;
				m_angularSpeed = 0.0F;

				return true;
			}
		}
		else
		{
			/* Reset stable frames counter on any significant movement. */
			m_stableFrames = 0;
		}

		return false;
	}
}
