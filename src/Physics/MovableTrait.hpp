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

/* Local inclusions for usages. */
#include "EnvironmentPhysicalProperties.hpp"
#include "BodyPhysicalProperties.hpp"

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
			 * @brief [PHYSICS-NEW-SYSTEM] Applies a linear impulse directly to the velocity.
			 * @note Impulse = instant change in momentum (J = m*Δv). Used by constraint solver.
			 * @param impulse The impulse vector in N·s.
			 * @return void
			 */
			void
			applyLinearImpulse (const Libs::Math::Vector< 3, float > & impulse) noexcept
			{
				if ( !m_isMovable )
				{
					return;
				}

				m_linearVelocity += impulse * this->getBodyPhysicalProperties().inverseMass();
				m_linearSpeed = m_linearVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief [PHYSICS-NEW-SYSTEM] Applies an angular impulse directly to the angular velocity.
			 * @note Angular impulse L = I * Δω. Used by constraint solver for rotational response.
			 * @param angularImpulse The angular impulse vector.
			 * @return void
			 */
			void
			applyAngularImpulse (const Libs::Math::Vector< 3, float > & angularImpulse) noexcept
			{
				if ( !m_isMovable || !m_rotationEnabled )
				{
					return;
				}

				m_angularVelocity += m_inverseWorldInertia * angularImpulse;
				m_angularSpeed = m_angularVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief [PHYSICS-NEW-SYSTEM] Updates the inverse world inertia tensor from the current orientation.
			 * @note Call this after rotation changes. I_world = R * I_local * R^T.
			 * @param rotationMatrix The current orientation as a 3x3 rotation matrix.
			 * @return void
			 */
			void
			updateInverseWorldInertia (const Libs::Math::Matrix< 3, float > & rotationMatrix) noexcept
			{
				const auto & localInertia = this->getBodyPhysicalProperties().inertiaTensor();

				// Transform inertia tensor to world space: I_world = R * I_local * R^T
				auto rotationTransposed = rotationMatrix;
				rotationTransposed.transpose();
				auto worldInertia = rotationMatrix * localInertia * rotationTransposed;

				// Compute and cache the inverse
				m_inverseWorldInertia = worldInertia.inverse();
			}

			/**
			 * @brief [PHYSICS-NEW-SYSTEM] Returns the inverse world inertia tensor.
			 * @note This is the cached transformed and inverted inertia tensor.
			 * @return const Libs::Math::Matrix< 3, float > &
			 */
			[[nodiscard]]
			const Libs::Math::Matrix< 3, float > &
			inverseWorldInertia () const noexcept
			{
				return m_inverseWorldInertia;
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
			 * @brief Adds a physical force to the object acceleration.
			 * @note Using this formula: F = m * a
			 * @param force A reference to a vector representing the force. The magnitude (length) will represent the acceleration in m/s².
			 * @return void
			 */
			void addForce (const Libs::Math::Vector< 3, float > & force) noexcept;

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
			 *  - Add the acceleration vector to the velocity vector.
			 * Then will return true if a movement occurs.
			 * @param envProperties A reference to physical environment properties.
			 * @return bool
			 */
			bool updateSimulation (const EnvironmentPhysicalProperties & envProperties) noexcept;

			/**
			 * @brief Sets whether this is affected by all physical interactions.
			 * @note If false, the method stopMovement() will be called.
			 * @param state The state.
			 * @return void
			 */
			void
			setMovingAbility (bool state) noexcept
			{
				m_isMovable = state;

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
				return m_isMovable;
			}

			/**
			 * @brief Enables or disables rotation physics for this entity.
			 * @note When disabled, torque will not be applied and collisions won't induce rotation.
			 *       Disabling rotation will also reset angular velocity to zero.
			 * @param state True to enable rotation, false to disable.
			 * @return void
			 */
			void
			enableRotationPhysics (bool state) noexcept
			{
				m_rotationEnabled = state;

				if ( !state )
				{
					m_angularVelocity.reset();
					m_angularSpeed = 0.0F;
				}
			}

			/**
			 * @brief Returns whether rotation physics is enabled for this entity.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isRotationPhysicsEnabled () const noexcept
			{
				return m_rotationEnabled;
			}

			/**
			 * @brief Enables the free fly mode. In other terms, the gravity will be ignored.
			 * @param state The state.
			 */
			void
			enableFreeFlyMode (bool state) noexcept
			{
				m_freeFlyModeEnabled = state;
			}

			/**
			 * @brief Returns whether the free fly mode is enabled or not.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isFreeFlyModeEnabled () const noexcept
			{
				return m_freeFlyModeEnabled;
			}

			/**
			 * @brief Check for simulation inertia.
			 * @warning This method is not physically correct, and its aim is to reduce useless physics computation.
			 * @return bool
			 */
			[[nodiscard]]
			bool checkSimulationInertia () noexcept;

			/**
			 * @brief Returns the world position (public accessor for physics engine).
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Libs::Math::Vector< 3, float >
			worldPosition () const noexcept
			{
				return this->getWorldPosition();
			}

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
			 * @brief Returns the object physical properties for the physics simulation.
			 * @return const BodyPhysicalProperties &
			 */
			[[nodiscard]]
			virtual const BodyPhysicalProperties & getBodyPhysicalProperties () const noexcept = 0;

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

			/**
			 * @brief Moves the entity in the scene from physics simulation.
			 * @note This should make a call to LocatableInterface::move() final object method.
			 * @param positionDelta A reference to a delta vector to add to current position.
			 * @return void
			 */
			virtual void moveFromPhysics (const Libs::Math::Vector< 3, float > & positionDelta) noexcept = 0;

			/**
			 * @brief Rotates the entity int the scene from physics simulation.
			 * @note This should make a call to LocatableInterface::rotate() final object method.
			 * @param radianAngle An angle in radian.
			 * @param worldDirection A reference to a vector.
			 * @return Libs::Math::Vector< 3, float >
			 */
			virtual void rotateFromPhysics (float radianAngle, const Libs::Math::Vector< 3, float > & worldDirection) noexcept = 0;

			/**
			 * @brief Marks that this entity had a collision this frame.
			 * @note Called by the constraint solver when resolving collisions.
			 * @return void
			 */
			void
			setHadCollision () noexcept
			{
				m_hadCollision = true;
			}

			/**
			 * @brief Marks that this entity is grounded (standing on a surface).
			 * @note Called by the constraint solver when collision normal points upward.
			 * @return void
			 */
			void setGrounded () noexcept;

			/**
			 * @brief Decrements the grounded grace period.
			 * @note Called each frame. Grounded state persists for a few frames after losing contact.
			 * @return void
			 */
			void updateGroundedState () noexcept;

			/**
			 * @brief Returns whether this entity is grounded.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGrounded () const noexcept;

		protected:

			/**
			 * @brief Constructs a movable trait.
			 */
			MovableTrait () noexcept = default;

			/**
			 * @brief Returns the world position for the physics simulation.
			 * @return Libs::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Libs::Math::Vector< 3, float > getWorldPosition () const noexcept = 0;

		private:

			Libs::Math::Vector< 3, float > m_linearVelocity;
			Libs::Math::Vector< 3, float > m_angularVelocity; // Omega
			Libs::Math::Vector< 3, float > m_centerOfMass;
			Libs::Math::Matrix< 3, float > m_inverseWorldInertia; // Cached I^-1 in world space
			float m_linearSpeed{0.0F};
			float m_angularSpeed{0.0F};
			uint8_t m_groundedFrames{0};
			bool m_isMovable{true};
			bool m_rotationEnabled{false};
			bool m_freeFlyModeEnabled{false};
			bool m_hadCollision{false};
	};
}
