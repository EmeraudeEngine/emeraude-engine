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
	MovableTrait::addTorque (const Vector< 3, float > & torque) noexcept
	{
		const auto & objectProperties = this->getBodyPhysicalProperties();

		/* NOTE: If the object mass is null, we discard the torque. */
		if ( objectProperties.isMassNull() )
		{
			return;
		}

		/* NOTE: (Vector)Torque = (Matrix)Inertia * (Vector)Angular Acceleration (T = I * α)
		 * => "α = I⁻¹ * T"
		 *
		 * The inertia tensor is stored in PhysicalObjectProperties.
		 * For a solid cuboid, it should be set as:
		 * Ixx = (m * (h² + d²)) / 12
		 * Iyy = (m * (w² + d²)) / 12
		 * Izz = (m * (w² + h²)) / 12
		 */
		const auto & inertia = objectProperties.inertiaTensor();

		/* Calculate angular acceleration from torque: α = I⁻¹ * T */
		const auto angularAcceleration = inertia.inverse() * torque;

		/* Apply angular acceleration (scaled by timestep). */
		m_angularVelocity += angularAcceleration * EngineUpdateCycleDurationS< float >;
		m_angularSpeed = m_angularVelocity.length();

		this->onImpulse();
	}

	float
	MovableTrait::deflect (const Vector< 3, float > & surfaceNormal, float surfaceBounciness) noexcept
	{
		const auto & objectProperties = this->getBodyPhysicalProperties();

		const auto currentSpeed = m_linearSpeed;
		const auto incidentVector = m_linearVelocity.normalized();
		const auto dotProduct = Vector< 3, float >::dotProduct(incidentVector, surfaceNormal); // 1.0 or -1.0 = parallel (full hit). 0.0 = perpendicular (no hit).
		const auto totalBounciness = objectProperties.bounciness() * clampToUnit(surfaceBounciness);
		const auto modulatedBounciness = modulateNormalizedValue(totalBounciness, 1.0F - std::abs(dotProduct));

		m_linearSpeed = currentSpeed * modulatedBounciness;
		m_linearVelocity = (incidentVector - (surfaceNormal * (dotProduct * 2.0F))).scale(m_linearSpeed);

		return currentSpeed;
	}

	void
	MovableTrait::stopMovement () noexcept
	{
		m_linearVelocity.reset();
		m_angularVelocity.reset();

		m_linearSpeed = 0.0F;
		m_angularSpeed = 0.0F;

		m_inertCheckCount = 0;
	}

	bool
	MovableTrait::updateSimulation (const EnvironmentPhysicalProperties & envProperties) noexcept
	{
		const auto & objectProperties = this->getBodyPhysicalProperties();

		/* Apply the gravity. */
		if ( !this->isFreeFlyModeEnabled() && !objectProperties.isMassNull() )
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

	bool
	MovableTrait::checkSimulationInertia () noexcept
	{
		/* FIXME: Remove this ! */
		if ( m_angularSpeed > 0.0F )
		{
			return false;
		}

		/* FIXME: Should be a general settings to tweak physics engine. */
		constexpr auto MinimalDistance{0.01F};
		constexpr auto TotalInertiaCheckCount{15};

		/* Compute the last distance made by the entity. */
		const auto worldPosition = this->getWorldPosition();
		const auto distance = std::abs((worldPosition - m_lastWorldPosition).length());

		/* Save the "new" last position. */
		m_lastWorldPosition = worldPosition;

		/* Check if the distance is bigger than the threshold. */
		if ( distance > MinimalDistance )
		{
			m_inertCheckCount = 0;

			return false;
		}

		/* Check if the test reach the total of tests required
		 * to declare the entity into inertia state. */
		m_inertCheckCount++;

		if ( m_inertCheckCount < TotalInertiaCheckCount )
		{
			return false;
		}

		/* Completely stops the motion. */
		this->stopMovement();

		return true;
	}
}
