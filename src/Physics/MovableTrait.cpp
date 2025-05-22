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
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;

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
		const auto & objectProperties = getObjectProperties();

		/* NOTE: If the object mass is null, we discard the force. */
		if ( objectProperties.isMassNull() )
		{
			return;
		}

		/* a = F * 1/m */
		this->addAcceleration(force * objectProperties.inverseMass());
	}

	void
	MovableTrait::addTorque (const Vector< 3, float > & /*torque*/) noexcept
	{
		const auto & objectProperties = getObjectProperties();

		/* NOTE: If the object mass is null, we discard the force. */
		if ( objectProperties.isMassNull() )
		{
			return;
		}

		/* NOTE: (Vector)Torque = (Matrix)Inertia * (Vector)Angular Acceleration (T = I * w)
		 * => "w = T / I" */
		/*const auto & cube = this->getWorldBoundingBox();

		const float sqWith = cube.width() * cube.width();
		const float sqHeight = cube.height() * cube.height();
		const float sqDepth = cube.depth() * cube.depth();

		const float inertiaValue = (objectProperties.mass() * (sqWith + sqHeight + sqDepth)) / 12.0F;

		//float ix = cube.width() * cube.width();
		//cube so all dimensions equal, but other objects may not be
		//const float inertiaValue = (physicalObjectProperties.mass() * (ix + ix + ix)) / 12.0F;

		Matrix< 3, float > inertia{};
		inertia[M3x3Col0Row0] = inertiaValue;
		inertia[M3x3Col1Row1] = inertiaValue;
		inertia[M3x3Col2Row2] = inertiaValue;

		const auto angularAcceleration = inertia.inverse() * torque;

		m_angularVelocity += angularAcceleration;
		m_angularSpeed = m_angularVelocity.length();*/

		this->onImpulse();
	}

	float
	MovableTrait::deflect (const Vector< 3, float > & surfaceNormal, float surfaceBounciness) noexcept
	{
		const auto & objectProperties = getObjectProperties();

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
	MovableTrait::updateSimulation (const PhysicalEnvironmentProperties & envProperties) noexcept
	{
		const auto & objectProperties = getObjectProperties();

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
			this->simulatedMove(m_linearVelocity * EngineUpdateCycleDurationS< float >);

			isMoveOccurs = true;
		}

		/* Apply the drag force on rotation if there is angular speed. */
		if ( m_angularSpeed > 0.0F )
		{
			/*
			 * Td = drag torque
			 * B = angular drag coefficient
			 * V = volume of a submerged portion of polyhedron
			 * Vt = total volume of polyhedron
			 * L = approximation of the average width of the polyhedron
			 * w = angular velocity
			 * Td = B * m * (V / Vt) * L^2 * w */

			// TODO: Find the correct way to apply a drag force on rotation.
			//constexpr auto dragMagnitude = 0.95F;

			//m_angularVelocity *= dragMagnitude;

			/*
			 * Drag torque is calculated by multiplying the drag force acting
			 * on the cylinder by the distance between the force and the axis of rotation.
			 * This distance is known as the lever arm and is typically denoted by the symbol "r".
			 * The formula for drag torque is: Drag torque = Drag force x Lever arm.
			 * Reference: https://www.physicsforums.com/threads/drag-torque-of-rotating-cylinder.413114/
			 */

			/* Dispatch the final rotation to the entity according to the new angular speed. */
			this->simulatedRotation(2.0F * std::numbers::pi_v< float > / EngineUpdateCycleDurationS< float >, m_angularVelocity);

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
