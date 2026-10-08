/*
 * src/Physics/MovableTrait.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>

/* Local inclusions for usages. */
#include "BodyPhysicalProperties.hpp"
#include "EnvironmentPhysicalProperties.hpp"

/* Forward declarations. */
namespace EmEn::Scenes
{
	class AbstractEntity;
}


namespace EmEn::Physics
{
	/**
	 * @brief Identifies the type of surface an entity is grounded on.
	 */
	enum class EMEN_API GroundedSource : uint8_t
	{
		None,	  ///< Not grounded.
		Ground,	///< Grounded on terrain/ground.
		Boundary,  ///< Grounded on world boundary.
		Entity	 ///< Grounded on a StaticEntity or Node.
	};
	/**
	 * @brief Gives the ability to move something in the 3D world with physical properties.
	 */
	class EMEN_API MovableTrait
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
			 */
			void
			setLinearVelocity (const Base::Math::Vector< 3, float > & velocity) noexcept
			{
				m_linearVelocity = velocity;
				m_linearSpeed = m_linearVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief Sets the angular velocity around a vector.
			 * @param velocity A reference to a vector.
			 */
			void
			setAngularVelocity (const Base::Math::Vector< 3, float > & velocity) noexcept
			{
				m_angularVelocity = velocity;
				m_angularSpeed = m_angularVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief Sets a minimal velocity in a direction.
			 * @param velocity A reference to a vector.
			 */
			void setMinimalVelocity (const Base::Math::Vector< 3, float > & velocity) noexcept;

			/**
			 * @brief Adds an acceleration to the velocity to the current velocity without any checking.
			 * @param acceleration A reference to vector.
			 */
			void
			addAcceleration (const Base::Math::Vector< 3, float > & acceleration) noexcept
			{
				m_linearVelocity += acceleration * WorldPhysicsUpdateCycleDurationS< float >;
				m_linearSpeed = m_linearVelocity.length();

				this->onImpulse();
			}

			/**
			 * @brief Adds a raw angular acceleration vector to the current angular velocity without any checking.
			 * @param acceleration A reference to vector.
			 */
			void
			addAngularAcceleration (const Base::Math::Vector< 3, float > & acceleration) noexcept
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
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
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
			 * @return const Base::Math::Vector< 3, float > &
			 */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
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
			 */
			void
			applyLinearImpulse (const Base::Math::Vector< 3, float > & impulse) noexcept
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
			 * @brief Adds a physical force to the object acceleration.
			 * @note Using this formula: F = m * a
			 * @param force A reference to a vector representing the force. The magnitude (length) will represent the acceleration in m/s².
			 */
			void addForce (const Base::Math::Vector< 3, float > & force) noexcept;

			/**
			 * @brief Sets the object into inertia.
			 */
			void stopMovement () noexcept;

			/**
			 * @brief Advances the body's own part of a physics cycle.
			 * @note A body INTEGRATED BY THE SCENE (it collides: the scene's physics step moves it, gravity and contacts in
			 * its sub-steps — physics overhaul P2) only gets its drag here. Any other body (no collision model) is integrated
			 * here as before: gravity, drag, then the move.
			 * @param envProperties A reference to physical environment properties.
			 * @param integratedByScene Whether the scene's physics step integrates this body.
			 * @return bool True when this call moved the body.
			 */
			bool updateSimulation (const EnvironmentPhysicalProperties & envProperties, bool integratedByScene) noexcept;

			/**
			 * @brief Sets whether this is affected by all physical interactions.
			 * @note If false, the method stopMovement() will be called.
			 * @param state The state.
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
			 *	   Disabling rotation will also reset angular velocity to zero.
			 * @param state True to enable rotation, false to disable.
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
			 * @brief Counts the consecutive physics steps the body was slow (the scene's islands decide the sleep, P5).
			 * @param slow Whether the body was slow this step.
			 */
			void
			accountSlowness (bool slow) noexcept
			{
				if ( !slow )
				{
					m_slowSteps = 0;
				}
				else if ( m_slowSteps < std::numeric_limits< uint16_t >::max() )
				{
					++m_slowSteps;
				}
			}

			/** @brief Returns the consecutive physics steps the body was slow. */
			[[nodiscard]]
			uint16_t
			slowSteps () const noexcept
			{
				return m_slowSteps;
			}

			/**
			 * @brief Sets the island the body fell asleep with (0: awake). The scene wakes the whole island when one of its
			 * bodies wakes.
			 * @param key The island's key (the lowest creation number of its bodies), or 0.
			 */
			void
			setSleepIsland (uint64_t key) noexcept
			{
				m_sleepIsland = key;

				if ( key == 0 )
				{
					m_slowSteps = 0;
				}
			}

			/** @brief Returns the island the body fell asleep with (0: none). */
			[[nodiscard]]
			uint64_t
			sleepIsland () const noexcept
			{
				return m_sleepIsland;
			}

			/**
			 * @brief Returns the world position (public accessor for physics engine).
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float >
			worldPosition () const noexcept
			{
				return this->getWorldPosition();
			}

			/**
			 * @brief Returns the world velocity of the entity.
			 * @note If not override, velocity is null.
			 * @return Base::Math::Vector< float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getWorldVelocity () const noexcept = 0;

			/**
			 * @brief Returns the world center of mass of the entity: the centroid of its collision shape (physics
			 * overhaul P3, decision 8b), its origin without one.
			 * @return Base::Math::Vector< float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getWorldCenterOfMass () const noexcept = 0;

			/**
			 * @brief Returns the object physical properties for the physics simulation.
			 * @return const BodyPhysicalProperties &
			 */
			[[nodiscard]]
			virtual const BodyPhysicalProperties & getBodyPhysicalProperties () const noexcept = 0;

			/**
			 * @brief Events when this movable has hit something.
			 * @note The impact force is expressed in Newtons (N), representing the instantaneous collision force.
			 *	   Computed as F = (m × Δv) / Δt where Δt is the physics timestep.
			 *	   Example values at 60 FPS (Δt ≈ 0.0167s):
			 *	   - Light tap: ~100 N
			 *	   - Moderate impact: ~1000 N
			 *	   - Heavy collision: ~10000 N
			 * @param impactForce The collision force in Newtons (N).
			 */
			virtual void onCollision (float impactForce) noexcept = 0;

			/**
			 * @brief Events when this movable got a new impulse or a force.
			 */
			virtual void onImpulse () noexcept = 0;

			/**
			 * @brief Moves the entity in the scene from physics simulation.
			 * @note This should make a call to LocatableInterface::move() final object method.
			 * @param positionDelta A reference to a delta vector to add to current position.
			 */
			virtual void moveFromPhysics (const Base::Math::Vector< 3, float > & positionDelta) noexcept = 0;

			/**
			 * @brief Rotates the entity int the scene from physics simulation.
			 * @note This should make a call to LocatableInterface::rotate() final object method.
			 * @param radianAngle An angle in radian.
			 * @param worldDirection A reference to a vector.
			 */
			virtual void rotateFromPhysics (float radianAngle, const Base::Math::Vector< 3, float > & worldDirection) noexcept = 0;

			/**
			 * @brief Marks that this entity is grounded on a specific source.
			 * @param source The type of surface (Ground, Boundary, or Entity).
			 * @param groundedOn Pointer to the entity we're grounded on (only for Entity source).
			 */
			void setGrounded (GroundedSource source, const MovableTrait * groundedOn = nullptr) noexcept;

			/**
			 * @brief Clears the grounded state immediately.
			 */
			void clearGrounded () noexcept;

			/**
			 * @brief Decrements the grounded grace period.
			 * @note Called each frame. Grounded state persists for a few frames after losing contact.
			 */
			void updateGroundedState () noexcept;

			/**
			 * @brief Returns whether this entity is grounded on anything.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGrounded () const noexcept;

			/**
			 * @brief Returns whether this entity is grounded on terrain.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGroundedOnTerrain () const noexcept;

			/**
			 * @brief Returns whether this entity is grounded on a boundary.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGroundedOnBoundary () const noexcept;

			/**
			 * @brief Returns whether this entity is grounded on another entity.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGroundedOnEntity () const noexcept;

			/**
			 * @brief Returns whether this entity is grounded on a specific entity.
			 * @param entity The entity to check against.
			 * @return bool
			 */
			[[nodiscard]]
			bool isGroundedOn (const MovableTrait * entity) const noexcept;

			/**
			 * @brief Returns the current grounded source.
			 * @return GroundedSource
			 */
			[[nodiscard]]
			GroundedSource groundedSource () const noexcept;

		protected:

			/**
			 * @brief Constructs a movable trait.
			 */
			MovableTrait () noexcept = default;

			/**
			 * @brief Returns the world position for the physics simulation.
			 * @return Base::Math::Vector< 3, float >
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > getWorldPosition () const noexcept = 0;

		private:

			/** @brief Grace period before losing grounded state (in frames). ~250ms at 60 FPS. */
			static constexpr uint8_t GroundedGracePeriod{15};

			Base::Math::Vector< 3, float > m_linearVelocity;
			Base::Math::Vector< 3, float > m_angularVelocity; // Omega
			const MovableTrait * m_groundedOn{nullptr}; ///< Entity we're grounded on (if source is Entity).
			uint64_t m_sleepIsland{0}; ///< The island it fell asleep with (0: awake).
			float m_linearSpeed{0.0F};
			float m_angularSpeed{0.0F};
			GroundedSource m_groundedSource{GroundedSource::None}; ///< Type of surface we're grounded on.
			uint8_t m_groundedFrames{0}; ///< Grace period countdown.
			uint16_t m_slowSteps{0}; ///< Consecutive physics steps slow enough to sleep.
			bool m_isMovable{true};
			/* On by default for every dynamic body (P3, decision 8d); a character turns it off. */
			bool m_rotationEnabled{true};
			bool m_freeFlyModeEnabled{false};
	};
}
