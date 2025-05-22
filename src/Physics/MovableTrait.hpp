/*
 * src/Physics/MovableTrait.hpp
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

#pragma once

/* STL inclusions. */
#include <cstdint>
#include <array>

/* Local inclusions for usages. */
#include "PhysicalEnvironmentProperties.hpp"
#include "PhysicalObjectProperties.hpp"
#include "Collider.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Gives the ability to move something in the 3D world with physical properties.
	 */
	class MovableTrait
	{
		public:

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			MovableTrait (const MovableTrait & copy) noexcept = default;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			MovableTrait (MovableTrait && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return MovableTrait &
			 */
			MovableTrait & operator= (const MovableTrait & copy) noexcept = default;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return MovableTrait &
			 */
			MovableTrait & operator= (MovableTrait && copy) noexcept = default;

			/**
			 * @brief Destructs the movable trait.
			 */
			virtual ~MovableTrait () = default;

			/**
			 * @brief Sets the linear velocity in a direction.
			 * @param velocity A reference to a vector.
			 * @return void
			 */
			void
			setLinearVelocity (const Libs::Math::Vector< 3, float > & velocity) noexcept
			{
				m_linearVelocity = velocity;
				m_linearSpeed = m_linearVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief Sets the angular velocity around a vector.
			 * @param velocity A reference to a vector.
			 * @return void
			 */
			void
			setAngularVelocity (const Libs::Math::Vector< 3, float > & velocity) noexcept
			{
				m_angularVelocity = velocity;
				m_angularSpeed = m_angularVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief Sets a minimal velocity in a direction.
			 * @param velocity A reference to a vector.
			 * @return void
			 */
			void setMinimalVelocity (const Libs::Math::Vector< 3, float > & velocity) noexcept;

			/**
			 * @brief Adds an acceleration to the velocity to the current velocity without any checking.
			 * @param acceleration A reference to vector.
			 * @return void
			 */
			void
			addAcceleration (const Libs::Math::Vector< 3, float > & acceleration) noexcept
			{
				m_linearVelocity += acceleration * EngineUpdateCycleDurationS< float >;
				m_linearSpeed = m_linearVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief Adds a raw angular acceleration vector to the current angular velocity without any checking.
			 * @param acceleration A reference to vector.
			 * @return void
			 */
			void
			addAngularAcceleration (const Libs::Math::Vector< 3, float > & acceleration) noexcept
			{
				m_angularVelocity += acceleration;
				m_angularSpeed = m_angularVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief Returns whether the object is in motion.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasVelocity () const noexcept
			{
				return m_linearSpeed > 0.0F;
			}

			/**
			 * @brief Returns the linear velocity vector.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			linearVelocity () const noexcept
			{
				return m_linearVelocity;
			}

			/**
			 * @brief Returns the linear speed in meters per second.
			 * @return float
			 */
			[[nodiscard]]
			float
			linearSpeed () const noexcept
			{
				return m_linearSpeed;
			}

			/**
			 * @brief Returns whether the object is spinning.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSpinning () const noexcept
			{
				return m_angularSpeed > 0.0F;
			}

			/**
			 * @brief Returns the angular velocity vector.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			angularVelocity () const noexcept
			{
				return m_angularVelocity;
			}

			/**
			 * @brief Returns the angular speed.
			 * @return float
			 */
			[[nodiscard]]
			float
			angularSpeed () const noexcept
			{
				return m_angularSpeed;
			}

			/**
			 * @brief Sets the center of mass.
			 * @param centerOfMass A reference to a vector.
			 * @return void
			 */
			void
			setCenterOfMass (const Libs::Math::Vector< 3, float > & centerOfMass) noexcept
			{
				m_centerOfMass = centerOfMass;
			}

			/**
			 * @brief Returns the center of mass from the scene node position.
			 * @return const Libs::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Vector< 3, float > &
			centerOfMass () const noexcept
			{
				return m_centerOfMass;
			}

			/**
			 * @brief Returns the read-only collider.
			 * @return const Collider &
			 */
			[[nodiscard]]
			const Collider &
			collider () const noexcept
			{
				return m_collider;
			}

			/**
			 * @brief Returns the collider.
			 * @return Collider &
			 */
			[[nodiscard]]
			Collider &
			collider () noexcept
			{
				return m_collider;
			}

			/**
			 * @brief Adds a physical force to the object acceleration.
			 * @note Using this formula: F = m * a
			 * @param force A reference to a vector representing the force. The magnitude (length) will represent the acceleration in m/s².
			 * @return void
			 */
			void addForce (const Libs::Math::Vector< 3, float > & force) noexcept;

			/**
			 * @brief Adds a torque to the object angular acceleration.
			 * @param torque A reference to a vector representing the torque.
			 * @return void
			 */
			void addTorque (const Libs::Math::Vector< 3, float > & torque) noexcept;

			/**
			 * @brief Deviates the velocity direction from a normal vector when the object hit something static.
			 * @param surfaceNormal A vector representing the hit surface normal.
			 * @param surfaceBounciness A scalar to reduce the velocity after deviating the movement. 1.0F = no velocity loss from the surface. Default 0.75.
			 * @return float
			 */
			float deflect (const Libs::Math::Vector< 3, float > & surfaceNormal, float surfaceBounciness = 0.75F) noexcept;

			/**
			 * @brief Sets the object into inertia.
			 * @return void
			 */
			void stopMovement () noexcept;

			/**
			 * @brief Updates the velocity vector from the acceleration vector and return a reference to the new velocity.
			 * This will in order:
			 *  - Apply gravity force to acceleration vector.
			 *  - Apply drag force to acceleration vector.
			 *  - Add acceleration vector to velocity vector.
			 * Then will return true if a movement occurs.
			 * @param envProperties A reference to physical environment properties.
			 * @return bool
			 */
			bool updateSimulation (const PhysicalEnvironmentProperties & envProperties) noexcept;

			/**
			 * @brief Sets whether this is affected by all physical interactions.
			 * @note If false, the method stopMovement() will be called.
			 * @param state The state.
			 * @return void
			 */
			void
			setMovingAbility (bool state) noexcept
			{
				m_flags[IsMovable] = state;

				if ( !state )
				{
					this->stopMovement();
				}
			}

			/**
			 * @brief Returns whether this is affected by all physical interactions.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMovable () const noexcept
			{
				return m_flags[IsMovable];
			}

			/**
			 * @brief Sets whether this movable trait always computes physics.
			 * @param state The state.
			 * @return void
			 */
			void
			setAlwaysComputePhysicsState (bool state) noexcept
			{
				m_flags[AlwaysComputePhysics] = state;
			}

			/**
			 * @brief Returns whether this movable trait always compute physics.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			alwaysComputePhysics () const noexcept
			{
				return m_flags[AlwaysComputePhysics];
			}

			/**
			 * @brief Enables the free fly mode. In other terms, the gravity will be ignored.
			 * @param state The state.
			 */
			void
			enableFreeFlyMode (bool state) noexcept
			{
				m_flags[FreeFlyModeEnabled] = state;
			}

			/**
			 * @brief Returns whether the free fly mode is enabled or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFreeFlyModeEnabled () const noexcept
			{
				return m_flags[FreeFlyModeEnabled];
			}

			/**
			 * @brief Check for simulation inertia.
			 * @warning This method is not physically correct, and its aim is to reduce useless physics computation.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkSimulationInertia () noexcept;

			/**
			 * @brief Returns the world velocity of the entity.
			 * @note If not override, velocity is null.
			 * @return Libs::Math::Vector< float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > getWorldVelocity () const noexcept = 0;

			/**
			 * @brief Returns the world center of mass of the entity.
			 * @note If not override, velocity is null.
			 * @return Libs::Math::Vector< float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > getWorldCenterOfMass () const noexcept = 0;

			/**
			 * @brief Events when this movable has hit something.
			 * @param impactForce The force of impact.
			 * @return void
			 */
			virtual void onHit (float impactForce) noexcept = 0;

			/**
			 * @brief Events when this movable got a new impulse or a force.
			 * @return void
			 */
			virtual void onImpulse () noexcept = 0;

		protected:

			/**
			 * @brief Constructs a movable trait.
			 */
			MovableTrait () noexcept = default;

			/**
			 * @brief Returns the object physical properties for the physics simulation.
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			virtual const PhysicalObjectProperties & getObjectProperties () const noexcept = 0;

			/**
			 * @brief Returns the world position for the physics simulation.
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > getWorldPosition () const noexcept = 0;

			/**
			 * @brief Gives the new world position after simulation computed.
			 * @param worldPosition A reference to a vector.
			 * @return void
			 */
			virtual void simulatedMove (const Libs::Math::Vector< 3, float > & worldPosition) noexcept = 0;

			/**
			 * @brief Gives the new world orientation after simulation computed.
			 * @param radianAngle
			 * @param worldDirection
			 * @return Libs::Math::Vector< 3, float >
			 */
			virtual void simulatedRotation (float radianAngle, const Libs::Math::Vector< 3, float > & worldDirection) noexcept = 0;

		private:

			/* Flag names */
			static constexpr auto IsMovable{0UL};
			static constexpr auto AlwaysComputePhysics{1UL};
			static constexpr auto FreeFlyModeEnabled{2UL};

			Libs::Math::Vector< 3, float > m_lastWorldPosition;
			Libs::Math::Vector< 3, float > m_linearVelocity;
			Libs::Math::Vector< 3, float > m_angularVelocity; // Omega
			Libs::Math::Vector< 3, float > m_centerOfMass;
			float m_linearSpeed{0.0F};
			float m_angularSpeed{0.0F};
			Collider m_collider;
			uint32_t m_inertCheckCount{0};
			std::array< bool, 8 > m_flags{
				true/*IsMovable*/,
				false/*AlwaysComputePhysics*/,
				false/*FreeFlyModeEnabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
