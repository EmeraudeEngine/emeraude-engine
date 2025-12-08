/*
 * src/Physics/MovableTrait.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2025 - Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>
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

		/* Apply the gravity only if not grounded. */
		if ( !this->isGrounded() && !this->isFreeFlyModeEnabled() && !objectProperties.isMassNull() )
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
	MovableTrait::setGrounded () noexcept
	{
		m_groundedFrames = 15;
	}

	void
	MovableTrait::updateGroundedState () noexcept
	{
		if ( std::abs(m_linearVelocity[Y]) < 0.001F )
		{
			return;
		}

		if ( m_groundedFrames > 0 )
		{
			m_groundedFrames--;
		}
	}

	bool
	MovableTrait::isGrounded () const noexcept
	{
		return m_groundedFrames > 0;
	}

	bool
	MovableTrait::checkSimulationInertia () noexcept
	{
		TraceInfo{"PHYSICS"} <<
			"Is movable : " << this->isMovable() << "\n"
			"Is grounded : " << this->isGrounded() << "\n"
			"Velocity : " << this->linearVelocity() << "\n";

		/* No collision this frame, entity might be in free fall - don't pause. */
		if ( !m_hadCollision )
		{
			return false;
		}

		constexpr auto VelocityClampThreshold{0.05F}; /* 1 cm/s */

		/* Clamp micro-velocities to zero to eliminate solver oscillations. */
		if ( m_linearSpeed > 0.0F && m_linearSpeed < VelocityClampThreshold )
		{
			m_linearVelocity.reset();
			m_linearSpeed = 0.0F;
		}

		if ( m_angularSpeed > 0.0F && m_angularSpeed < VelocityClampThreshold )
		{
			m_angularVelocity.reset();
			m_angularSpeed = 0.0F;
		}

		/* If no velocity, entity is at rest. */
		if ( m_linearSpeed == 0.0F && m_angularSpeed == 0.0F )
		{
			/* Reset collision flag when pausing. */
			m_hadCollision = false;

			this->setGrounded();

			return true;
		}

		return false;
	}
}
