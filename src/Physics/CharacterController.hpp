/*
 * src/Physics/CharacterController.hpp
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
#include <cstdint>

/* Local inclusions for usages. */
#include "Math/Space3D/Capsule.hpp"
#include "Math/Vector.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief What a character controller asks the world: sweeps of its capsule, its penetration, pushing a body.
	 * @note The scene implements it during its physics step (`Scene::resolveCollisions()`), under the physics octree lock.
	 */
	class EMEN_API CharacterWorldInterface
	{
		public:

			/** @brief The first contact of a sweep. */
			struct Hit final
			{
				/** The world contact point, on the obstacle's surface. */
				Base::Math::Vector< 3, float > point;
				/** The obstacle's surface normal there, pointing back towards the character (unit). */
				Base::Math::Vector< 3, float > normal;
				/** The obstacle's velocity at the contact point (zero for the ground and the static solids). */
				Base::Math::Vector< 3, float > velocity;
				/** A stable key of the obstacle (an entity's creation number, or the ground's key). */
				uint64_t key{0};
				/** The fraction of the motion travelled before the contact, in [0, 1]. */
				float fraction{1.0F};
				/** The obstacle's index for push() when it is a dynamic body. */
				uint32_t bodyIndex{0};
				/** Whether the obstacle is a dynamic body (it can be pushed). */
				bool dynamic{false};
			};

			virtual ~CharacterWorldInterface () = default;

			/**
			 * @brief Sweeps a capsule along a motion against everything solid but the character itself.
			 * @param capsule A reference to the world capsule at its start.
			 * @param motion A reference to the motion.
			 * @param hit A reference to the first contact, written when the function answers true.
			 * @return bool True on a contact before the end of the motion.
			 */
			[[nodiscard]]
			virtual bool sweep (const Base::Math::Space3D::Capsule< float > & capsule, const Base::Math::Vector< 3, float > & motion, Hit & hit) const noexcept = 0;

			/**
			 * @brief Returns the displacement that takes a capsule out of the ground, the static solids and the kinematic
			 * bodies it overlaps (never out of a dynamic body: those do not push a character).
			 * @param capsule A reference to the world capsule.
			 * @return Base::Math::Vector< 3, float > A zero vector when nothing overlaps.
			 */
			[[nodiscard]]
			virtual Base::Math::Vector< 3, float > penetrationCorrection (const Base::Math::Space3D::Capsule< float > & capsule) const noexcept = 0;

			/**
			 * @brief Applies an impulse to a dynamic body at a world point.
			 * @param bodyIndex The body's index, from a Hit.
			 * @param point A reference to the world point.
			 * @param impulse A reference to the impulse (N·s).
			 * @return void
			 */
			virtual void push (uint32_t bodyIndex, const Base::Math::Vector< 3, float > & point, const Base::Math::Vector< 3, float > & impulse) noexcept = 0;

		protected:

			CharacterWorldInterface () = default;
			CharacterWorldInterface (const CharacterWorldInterface &) = default;
			CharacterWorldInterface (CharacterWorldInterface &&) = default;
			CharacterWorldInterface & operator= (const CharacterWorldInterface &) = default;
			CharacterWorldInterface & operator= (CharacterWorldInterface &&) = default;
	};

	/**
	 * @brief A KINEMATIC character controller: the character is MOVED, not simulated (physics overhaul P4, decisions 9).
	 * @note Each physics step: (1) out of what it overlaps; (2) the wanted horizontal velocity (instant on the ground,
	 * blended in the air by the air control), gravity when airborne, a jump as a launch velocity, the velocity of what
	 * it stands on; (3) COLLIDE AND SLIDE — the capsule is swept, stops a skin width before the first obstacle and
	 * slides along it (a walkable slope keeps the horizontal speed, a wall or a steeper slope only blocks, a ceiling
	 * stops a rise), with a STEP-UP (up, forward, down, with a headroom test) against a low obstacle; a dynamic body hit
	 * is pushed with a bounded force; (4) a GROUND PROBE snaps it back onto a walkable surface within the snap distance,
	 * so walking down stairs and slopes does not go airborne. The approach of K. Fauerby, "Improved Collision Detection
	 * and Response" (2003), Jolt's `CharacterVirtual` (MIT) and Godot's `move_and_slide()` — no code taken from them.
	 * @note The character's capsule stands on its FEET: the position is the bottom of the capsule.
	 */
	class EMEN_API CharacterController final
	{
		public:

			/** @brief What happened during one step (the scene emits it after its physics step). */
			struct Events final
			{
				/** The fall speed at a landing (m/s, positive), valid when landed. */
				float landingSpeed{0.0F};
				bool landed{false};
				bool leftGround{false};
				bool hitWall{false};
			};

			/** @brief The default radius (m), a human. */
			static constexpr float DefaultRadius{0.3F};
			/** @brief The default height, feet to head (m). */
			static constexpr float DefaultHeight{1.8F};
			/** @brief The default step height (m): citadel's 0.29 m steps pass. */
			static constexpr float DefaultStepHeight{0.35F};
			/** @brief The default steepest walkable slope (degrees). */
			static constexpr float DefaultMaxSlopeDegrees{45.0F};
			/** @brief The default ground snap distance (m). */
			static constexpr float DefaultSnapDistance{0.3F};
			/** @brief The default air control, in [0, 1] (0 = none, 1 = as on the ground). */
			static constexpr float DefaultAirControl{0.3F};
			/** @brief The default push force on a dynamic body (N). */
			static constexpr float DefaultPushForce{300.0F};
			/** @brief The distance kept between the capsule and what it touches (m). */
			static constexpr float SkinWidth{0.01F};
			/** @brief The bounded number of slide iterations per step. */
			static constexpr uint32_t MaxSlideIterations{4};

			/**
			 * @brief Constructs a controller with the default parameters.
			 */
			CharacterController () noexcept = default;

			/**
			 * @brief Sets the capsule size.
			 * @param radius The radius (m), finite and > 0.
			 * @param height The height, feet to head (m), finite and >= 2 × radius.
			 * @return bool False for an invalid size (nothing changes).
			 */
			bool setSize (float radius, float height) noexcept;

			/**
			 * @brief Sets the step height.
			 * @param height The highest obstacle climbed by walking into it (m), finite and >= 0 (0 = no step-up).
			 * @return bool False for an invalid value.
			 */
			bool setStepHeight (float height) noexcept;

			/**
			 * @brief Sets the steepest walkable slope.
			 * @param degrees The angle (degrees), finite, in [0, 89].
			 * @return bool False for an invalid value.
			 */
			bool setMaxSlope (float degrees) noexcept;

			/**
			 * @brief Sets the ground snap distance.
			 * @param distance The distance (m), finite and >= 0.
			 * @return bool False for an invalid value.
			 */
			bool setSnapDistance (float distance) noexcept;

			/**
			 * @brief Sets the air control.
			 * @param control The fraction of the ground control kept in the air, finite, in [0, 1].
			 * @return bool False for an invalid value.
			 */
			bool setAirControl (float control) noexcept;

			/**
			 * @brief Sets the push force on dynamic bodies.
			 * @param force The force (N), finite and >= 0.
			 * @return bool False for an invalid value.
			 */
			bool setPushForce (float force) noexcept;

			/**
			 * @brief Sets the velocity the character wants to move at (its vertical part is ignored).
			 * @param velocity A reference to the world velocity (m/s), finite.
			 * @return bool False for a non-finite velocity.
			 */
			bool setWantedVelocity (const Base::Math::Vector< 3, float > & velocity) noexcept;

			/**
			 * @brief Requests a jump on the next step, effective only when the character is grounded.
			 * @param launchSpeed The launch speed (m/s), finite and > 0.
			 * @return bool False for an invalid speed.
			 */
			bool jump (float launchSpeed) noexcept;

			/**
			 * @brief Runs one step.
			 * @param world A reference to the world queries.
			 * @param feet A reference to the feet position at the step's start (the capsule's bottom).
			 * @param upward A reference to the world up direction (unit, against gravity).
			 * @param gravity The gravity acceleration (m/s², >= 0, along −upward).
			 * @param deltaTime The step (s), > 0.
			 * @return Base::Math::Vector< 3, float > The feet position at the step's end.
			 */
			[[nodiscard]]
			Base::Math::Vector< 3, float > step (CharacterWorldInterface & world, const Base::Math::Vector< 3, float > & feet, const Base::Math::Vector< 3, float > & upward, float gravity, float deltaTime) noexcept;

			/**
			 * @brief Returns the capsule in the character's local space (feet at the origin, Y up).
			 * @return Base::Math::Space3D::Capsule< float >
			 */
			[[nodiscard]]
			Base::Math::Space3D::Capsule< float > localCapsule () const noexcept;

			/** @brief Returns whether the character stands on a walkable surface. */
			[[nodiscard]]
			bool
			isGrounded () const noexcept
			{
				return m_grounded;
			}

			/** @brief Returns the normal of the surface it stands on (valid when grounded). */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			groundNormal () const noexcept
			{
				return m_groundNormal;
			}

			/** @brief Returns the key of what it stands on (0 when airborne). */
			[[nodiscard]]
			uint64_t
			supportKey () const noexcept
			{
				return m_supportKey;
			}

			/** @brief Returns the velocity of the last step (m/s), what the character really did. */
			[[nodiscard]]
			const Base::Math::Vector< 3, float > &
			velocity () const noexcept
			{
				return m_velocity;
			}

			/** @brief Returns the vertical speed (m/s, positive up). */
			[[nodiscard]]
			float
			verticalSpeed () const noexcept
			{
				return m_verticalSpeed;
			}

			/** @brief Returns the events of the last step. */
			[[nodiscard]]
			const Events &
			events () const noexcept
			{
				return m_events;
			}

			/** @brief Returns the radius (m). */
			[[nodiscard]]
			float
			radius () const noexcept
			{
				return m_radius;
			}

			/** @brief Returns the height, feet to head (m). */
			[[nodiscard]]
			float
			height () const noexcept
			{
				return m_height;
			}

			/** @brief Returns the step height (m). */
			[[nodiscard]]
			float
			stepHeight () const noexcept
			{
				return m_stepHeight;
			}

			/** @brief Returns the cosine of the steepest walkable slope. */
			[[nodiscard]]
			float
			walkableCosine () const noexcept
			{
				return m_walkableCosine;
			}

			/** @brief Returns the ground snap distance (m). */
			[[nodiscard]]
			float
			snapDistance () const noexcept
			{
				return m_snapDistance;
			}

			/** @brief Returns the air control. */
			[[nodiscard]]
			float
			airControl () const noexcept
			{
				return m_airControl;
			}

			/** @brief Returns the push force (N). */
			[[nodiscard]]
			float
			pushForce () const noexcept
			{
				return m_pushForce;
			}

		private:

			/** @brief The world capsule standing on these feet. */
			[[nodiscard]]
			Base::Math::Space3D::Capsule< float > capsuleAt (const Base::Math::Vector< 3, float > & feet, const Base::Math::Vector< 3, float > & upward) const noexcept;

			/** @brief Collide and slide a motion from a position; answers the position reached (and whether it climbed a step). */
			[[nodiscard]]
			Base::Math::Vector< 3, float > slide (CharacterWorldInterface & world, Base::Math::Vector< 3, float > position, Base::Math::Vector< 3, float > motion, const Base::Math::Vector< 3, float > & upward, bool canStep, float deltaTime, bool & stepped) noexcept;

			/** @brief Whether the surface just beyond a contact point, along a horizontal direction, is walkable (a step's top). */
			[[nodiscard]]
			bool walkableBeyond (const CharacterWorldInterface & world, const Base::Math::Vector< 3, float > & point, const Base::Math::Vector< 3, float > & direction, const Base::Math::Vector< 3, float > & upward) const noexcept;

			/** @brief The ground probe: a walkable surface under the feet within a length, answers true with the hit. */
			[[nodiscard]]
			bool probeGround (const CharacterWorldInterface & world, const Base::Math::Vector< 3, float > & position, const Base::Math::Vector< 3, float > & upward, float length, CharacterWorldInterface::Hit & hit) const noexcept;

			/** @brief Tries to climb a low obstacle with a horizontal motion; answers true with the position reached. */
			[[nodiscard]]
			bool stepUp (const CharacterWorldInterface & world, const Base::Math::Vector< 3, float > & position, const Base::Math::Vector< 3, float > & horizontalMotion, const Base::Math::Vector< 3, float > & upward, Base::Math::Vector< 3, float > & reached) const noexcept;

			Base::Math::Vector< 3, float > m_wantedVelocity;
			Base::Math::Vector< 3, float > m_horizontalVelocity;
			Base::Math::Vector< 3, float > m_velocity;
			Base::Math::Vector< 3, float > m_groundNormal{0.0F, 1.0F, 0.0F};
			Base::Math::Vector< 3, float > m_supportVelocity;
			uint64_t m_supportKey{0};
			Events m_events;
			float m_radius{DefaultRadius};
			float m_height{DefaultHeight};
			float m_stepHeight{DefaultStepHeight};
			float m_walkableCosine{0.70710678F};
			float m_snapDistance{DefaultSnapDistance};
			float m_airControl{DefaultAirControl};
			float m_pushForce{DefaultPushForce};
			float m_verticalSpeed{0.0F};
			float m_jumpSpeed{0.0F};
			bool m_grounded{false};
			bool m_jumpRequested{false};
	};
}
