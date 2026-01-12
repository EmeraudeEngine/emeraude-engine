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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "MovableTrait.hpp"

/* Local inclusions. */
#include "Tracer.hpp"
#include "Libs/Math/Base.hpp"

namespace EmEn::Physics
{
	using namespace Libs;
	using namespace Libs::Math;

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
	MovableTrait::updateSimulation (const EnvironmentPhysicalProperties & envProperties) noexcept
	{
		const auto & objectProperties = this->getBodyPhysicalProperties();

		/* Decay grounded state each frame. */
		this->updateGroundedState();

		/* Apply ground friction and gravity based on grounded source.
		 * Ground/Boundary: stable surfaces - full friction, no gravity
		 * Entity: dynamic surfaces - friction but gravity still applies (can fall off) */
		const bool isOnStableSurface = this->isGroundedOnTerrain() || this->isGroundedOnBoundary();

		if ( this->isGrounded() )
		{
			const auto frictionFactor = 1.0F - objectProperties.stickiness();

			m_linearVelocity[X] *= frictionFactor;
			m_linearVelocity[Z] *= frictionFactor;

			/* Clamp downward velocity to prevent micro-bounces.
			 * Only for stable surfaces (Ground/Boundary).
			 * Y-down: positive Y = moving down. */
			if ( isOnStableSurface && m_linearVelocity[Y] > 0.0F )
			{
				m_linearVelocity[Y] = 0.0F;
			}

			m_linearSpeed = m_linearVelocity.length();
		}

		/* Apply gravity if:
		 * - Not grounded at all, OR
		 * - Grounded on Entity (can fall off dynamic surfaces)
		 * Exception: free fly mode or massless objects. */
		const bool shouldApplyGravity = !isOnStableSurface && !this->isFreeFlyModeEnabled() && !objectProperties.isMassNull();

		if ( shouldApplyGravity )
		{
			m_linearVelocity[Y] += envProperties.steppedSurfaceGravity();
			m_linearSpeed = m_linearVelocity.length();
		}

		/* Apply the drag force if there is linear speed. */
		if ( m_linearSpeed > 0.0F )
		{
			const auto dragMagnitude = Physics::getDragMagnitude(
				objectProperties.dragCoefficient(),
				envProperties.atmosphericDensity(),
				m_linearSpeed,
				objectProperties.surface()
			);

			const auto force = m_linearVelocity.normalized().scale(-dragMagnitude);

			this->addForce(force);
		}

		bool isMoveOccurs = false;

		/* Apply the movement */
		if ( m_linearSpeed > 0.0F )
		{
			/* Dispatch the final move to the entity according to the new velocity. */
			this->moveFromPhysics(m_linearVelocity * EngineUpdateCycleDurationS< float >);

			isMoveOccurs = true;
		}

		/* Apply the drag force on rotation if there is angular speed and rotation is enabled. */
		if ( m_rotationEnabled && m_angularSpeed > 0.0F )
		{
			/*
			 * Angular drag is implemented as a simple damping coefficient.
			 * A more physically accurate implementation would use:
			 * Td = B * m * (V / Vt) * L^2 * w
			 * Where:
			 *   Td = drag torque
			 *   B = angular drag coefficient
			 *   V = volume of a submerged portion of polyhedron
			 *   Vt = total volume of polyhedron
			 *   L = approximation of the average width of the polyhedron
			 *   w = angular velocity
			 *
			 * For now, we use a simplified damping approach where the angular drag coefficient
			 * (0.0 to 1.0) determines how much angular velocity is retained each frame.
			 * 0.0 = no drag (perpetual rotation), 1.0 = immediate stop.
			 */
			const auto angularDrag = objectProperties.angularDragCoefficient();

			/* Apply damping: velocity *= (1 - drag) */
			const auto dampingFactor = 1.0F - angularDrag;
			m_angularVelocity *= dampingFactor;
			m_angularSpeed = m_angularVelocity.length();

			/* Dispatch the final rotation to the entity according to the new angular velocity. */
			this->rotateFromPhysics(2.0F * std::numbers::pi_v< float > / EngineUpdateCycleDurationS< float >, m_angularVelocity);

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
		/* Don't decay grounded state if Y velocity is negligible. */
		if ( std::abs(m_linearVelocity[Y]) < 0.001F )
		{
			return;
		}

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
