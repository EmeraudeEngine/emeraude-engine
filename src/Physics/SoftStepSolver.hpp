/*
 * src/Physics/SoftStepSolver.hpp
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

/* STL inclusions. */
#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

/* Project configuration. */
#include "emeraude_export.hpp"

/* Local inclusions for usages. */
#include "Math/Matrix.hpp"
#include "Math/PiecewiseLinear.hpp"
#include "Math/Quaternion.hpp"
#include "Math/Vector.hpp"
#include "StaticVector.hpp"

namespace EmEn::Physics
{
	class MovableTrait;

	/**
	 * @brief The contact solver of the physics step: Box2D v3's "soft step" in 3D (physics overhaul P2).
	 * @note Per step: the contacts are prepared once (anchors, masses, pre-solve normal speed, warm start from the
	 * persistent cache); then, for each sub-step: integrate the velocities (gravity), warm start, solve with the soft
	 * bias, integrate the positions, relax (solve without bias); after the sub-steps the restitution is applied once,
	 * from the pre-solve speed, and the accumulated impulses are stored back into the cache by feature id.
	 * The soft contact (a spring of contactHertz, damping ratio contactDampingRatio, pushout capped by
	 * maxPushoutVelocity) replaces the Baumgarte bias and the former position pass; a separated point within the
	 * speculative margin is solved too (its bias lets the bodies close the gap, never more).
	 * @note Determinism: the caller hands the bodies and the manifolds in a stable order (creation numbers); nothing in
	 * the solve depends on addresses or on unordered containers' iteration order.
	 * @note References: E. Catto, "Solver2D" (2024) and Box2D v3 `contact_solver.c`, `solver.c` (MIT) — the algorithm is
	 * followed, no code is copied; E. Catto, "Iterative Dynamics with Temporal Coherence" (GDC 2005) for warm starting.
	 */
	class EMEN_API SoftStepSolver final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SoftStepSolver"};

			/** @brief The sub-steps per physics step (Box2D v3's default). */
			static constexpr uint32_t SubStepCount{4};

			/** @brief The soft contact's stiffness, capped to a quarter of the sub-step rate (Box2D v3: 30 Hz). */
			static constexpr float ContactHertz{30.0F};

			/** @brief The soft contact's damping ratio (Box2D v3: 10, heavily damped). */
			static constexpr float ContactDampingRatio{10.0F};

			/** @brief The fastest a penetration is pushed out (m/s). */
			static constexpr float MaxPushoutVelocity{3.0F};

			/** @brief The restitution passes: one sequential pass favours the first point of a manifold (a box dropped
			 * flat tipped 5°, measured 2026-10-01); a few passes converge on the simultaneous answer. */
			static constexpr uint32_t RestitutionIterations{4};

			/** @brief Under this approach speed (m/s) a contact does not bounce. */
			static constexpr float RestitutionThreshold{1.0F};

			/** @brief The speculative margin (m): a contact is created (and solved) up to this gap. */
			static constexpr float SpeculativeMargin{0.02F};

			/**
			 * @brief A body as the solver sees it. Index 0 of the body list is the static world (ground, static entities).
			 */
			struct Body final
			{
				/** The trait to write back to; nullptr for the static world and the kinematic bodies. */
				MovableTrait * movable{nullptr};
				/** The world position of the CENTRE OF MASS (the point the body turns about). */
				Base::Math::Vector< 3, float > position;
				/** The world offset of the centre of mass from the node's origin (the solver ignores it; the scene's
				 * write-back moves the origin by deltaPosition + centerOffset − deltaRotation · centerOffset). */
				Base::Math::Vector< 3, float > centerOffset;
				Base::Math::Quaternion< float > orientation;
				Base::Math::Vector< 3, float > linearVelocity;
				Base::Math::Vector< 3, float > angularVelocity;
				/** The world inverse inertia (fixed over the step; zero when the rotation physics is off). */
				Base::Math::Matrix< 3, float > inverseInertia{std::array< float, 9 >{}};
				/** The accumulated translation of the step. */
				Base::Math::Vector< 3, float > deltaPosition;
				/** The accumulated rotation of the step (world, applied on the left). */
				Base::Math::Quaternion< float > deltaRotation;
				float inverseMass{0.0F};
				/** Whether gravity applies (a dynamic body with a mass, not in free fly). */
				bool gravity{false};
				/** Whether the solver moves it (a dynamic body); a kinematic body only carries a velocity. */
				bool dynamic{false};
			};

			/** @brief One contact point of a manifold. */
			struct Point final
			{
				/** The world contact point (halfway between the surfaces), as generated. */
				Base::Math::Vector< 3, float > position;
				/** The anchors from each body's position (computed by prepare). */
				Base::Math::Vector< 3, float > anchorA;
				Base::Math::Vector< 3, float > anchorB;
				/** The separation at the step's start (negative = penetrating). */
				float separation{0.0F};
				float adjustedSeparation{0.0F};
				float normalMass{0.0F};
				float tangentMass1{0.0F};
				float tangentMass2{0.0F};
				float normalImpulse{0.0F};
				float tangentImpulse1{0.0F};
				float tangentImpulse2{0.0F};
				float maxNormalImpulse{0.0F};
				/** The normal relative speed before the solve (negative = approaching), for the restitution. */
				float relativeVelocity{0.0F};
				uint32_t featureId{0};
			};

			/** @brief A contact manifold between body A and body B (normal from A to B). */
			struct Manifold final
			{
				Base::Math::Vector< 3, float > normal;
				Base::Math::Vector< 3, float > tangent1;
				Base::Math::Vector< 3, float > tangent2;
				Base::StaticVector< Point, 4 > points;
				/** The persistent key: the creation numbers of A and B, and a sub key (a ground triangle's feature id + 1). */
				uint64_t keyA{0};
				uint64_t keyB{0};
				uint64_t keySub{0};
				uint32_t bodyA{0};
				uint32_t bodyB{0};
				float friction{0.0F};
				float restitution{0.0F};
			};

			/**
			 * @brief A vehicle's wheel (decision 15): a soft suspension along the contact normal and a tyre's longitudinal
			 * and lateral friction, solved with the contacts every sub-step; its spin is a degree of freedom of its own
			 * (an inertia about its axle). A wheel off the ground only spins (its torques, its damping, its brake).
			 * @note The accumulated impulses are per SUB-STEP and re-applied at each one (the warm start), as the contacts'.
			 */
			struct Wheel final
			{
				/** Where it touches (world), the normal from the ground towards the chassis, the rolling and the side
				 * directions in the contact plane (unit). */
				Base::Math::Vector< 3, float > contactPoint;
				Base::Math::Vector< 3, float > normal{0.0F, 1.0F, 0.0F};
				Base::Math::Vector< 3, float > forward;
				Base::Math::Vector< 3, float > side;
				/** The anchors from the bodies' centres (prepare). */
				Base::Math::Vector< 3, float > anchorChassis;
				Base::Math::Vector< 3, float > anchorGround;
				/** The tyre's friction over the slip ratio and over the slip angle (degrees). */
				const Base::Math::PiecewiseLinear< float, 16 > * longitudinalFriction{nullptr};
				const Base::Math::PiecewiseLinear< float, 16 > * lateralFriction{nullptr};
				uint32_t chassis{0};
				uint32_t ground{0};
				/** The suspension's length at the step's start and its maximum (its rest: the spring pushes towards it). */
				float suspensionLength{0.0F};
				/** The length as a function of the motion: length = adjusted + n · (moved chassis point − moved ground
				 * point), the anchors' own offset taken out (prepare), as a contact's adjusted separation. */
				float adjustedSuspensionLength{0.0F};
				float suspensionMaxLength{0.0F};
				float suspensionFrequency{1.5F};
				float suspensionDamping{0.5F};
				float radius{0.3F};
				float inertia{1.0F};
				float angularDamping{0.0F};
				/** In: the spin at the step's start; out: at its end (rad / s). */
				float angularVelocity{0.0F};
				float driveTorque{0.0F};
				float brakeTorque{0.0F};
				/** The solver's: effective masses, slips (degrees for the angle), friction coefficients, impulses. */
				float suspensionMass{0.0F};
				/** Along the rolling direction: with the wheel's spin (a free wheel), and the bodies' alone (a locked one). */
				float longitudinalMass{0.0F};
				float rollingMass{0.0F};
				float lateralMass{0.0F};
				float slipRatio{0.0F};
				float slipAngle{0.0F};
				float longitudinalCoefficient{0.0F};
				float lateralCoefficient{0.0F};
				float suspensionImpulse{0.0F};
				float longitudinalImpulse{0.0F};
				float lateralImpulse{0.0F};
				/** A wheel its brake stops within the sub-step is LOCKED (its spin 0): the brake's surplus becomes this
				 * impulse bound on the tyre's rolling friction, between the ground and the chassis (Jolt's model). */
				float brakeImpulse{0.0F};
				bool locked{false};
				bool contact{false};
			};

			/**
			 * @brief Constructs a solver with an empty contact cache.
			 */
			SoftStepSolver () noexcept = default;

			/**
			 * @brief Solves one physics step: integrates the dynamic bodies and resolves the contacts.
			 * @pre bodies[0] is the static world; every manifold's bodyA / bodyB indexes the list; the manifolds are sorted
			 * by key (deterministic order).
			 * @param bodies The bodies; the dynamic ones are integrated in place (velocities, position, orientation, deltas).
			 * @param manifolds The manifolds; their impulses are written back (for the caller's events and grounded state).
			 * @param gravity A reference to the gravity acceleration (m/s²).
			 * @param deltaTime The step (s), > 0.
			 */
			void step (std::vector< Body > & bodies, std::vector< Manifold > & manifolds, std::vector< Wheel > & wheels, const Base::Math::Vector< 3, float > & gravity, float deltaTime) noexcept;

			/**
			 * @brief Forgets every cached impulse (a scene change).
			 */
			void
			clearCache () noexcept
			{
				m_cache.clear();
			}

		private:

			/** @brief The impulses of one point kept between steps. */
			struct CachedPoint final
			{
				uint32_t featureId{0};
				float normalImpulse{0.0F};
				float tangentImpulse1{0.0F};
				float tangentImpulse2{0.0F};
			};

			/** @brief The impulses of one manifold kept between steps. */
			struct CachedManifold final
			{
				Base::StaticVector< CachedPoint, 4 > points;
				uint64_t lastStep{0};
			};

			/** @brief The persistent key of a manifold. */
			struct CacheKey final
			{
				uint64_t keyA{0};
				uint64_t keyB{0};
				uint64_t keySub{0};

				[[nodiscard]]
				bool
				operator== (const CacheKey & other) const noexcept
				{
					return keyA == other.keyA && keyB == other.keyB && keySub == other.keySub;
				}
			};

			/** @brief Hashes a CacheKey (only lookups; iteration order never matters). */
			struct CacheKeyHash final
			{
				[[nodiscard]]
				size_t
				operator() (const CacheKey & key) const noexcept
				{
					uint64_t hash = key.keyA * 0x9E3779B97F4A7C15ULL;
					hash ^= key.keyB + 0x9E3779B97F4A7C15ULL + (hash << 6U) + (hash >> 2U);
					hash ^= key.keySub + 0x9E3779B97F4A7C15ULL + (hash << 6U) + (hash >> 2U);

					return static_cast< size_t >(hash);
				}
			};

			/**
			 * @brief Computes the anchors, the effective masses and the pre-solve speed; warm-starts from the cache.
			 */
			void prepare (const std::vector< Body > & bodies, std::vector< Manifold > & manifolds) noexcept;

			/**
			 * @brief Applies the accumulated impulses of every point (the warm start of a sub-step).
			 */
			static void warmStart (std::vector< Body > & bodies, const std::vector< Manifold > & manifolds) noexcept;

			/**
			 * @brief One sequential pass over every contact point: the normal constraint, then the friction.
			 * @param biasRate The soft contact's bias rate (1/s).
			 * @param massScale The soft contact's mass scale.
			 * @param impulseScale The soft contact's impulse scale.
			 * @param inverseSubStep 1 / the sub-step (1/s).
			 * @param useBias True for the soft solve, false for the relax pass (no bias, rigid).
			 */
			static void solveContacts (std::vector< Body > & bodies, std::vector< Manifold > & manifolds, float biasRate, float massScale, float impulseScale, float inverseSubStep, bool useBias) noexcept;

			/**
			 * @brief The wheels' anchors, effective masses, slips and friction coefficients, from the step's start.
			 */
			static void prepareWheels (const std::vector< Body > & bodies, std::vector< Wheel > & wheels) noexcept;

			/**
			 * @brief Re-applies the wheels' accumulated impulses (a sub-step's warm start).
			 */
			static void warmStartWheels (std::vector< Body > & bodies, std::vector< Wheel > & wheels) noexcept;

			/**
			 * @brief One pass over the wheels: the suspension (soft, with its own frequency), then the tyre's friction.
			 * @param subStep The sub-step (s).
			 * @param integrateSpin True once per sub-step (the main pass): the wheels' torques, damping and brake.
			 */
			static void solveWheels (std::vector< Body > & bodies, std::vector< Wheel > & wheels, float subStep, bool integrateSpin) noexcept;

			/**
			 * @brief Applies the restitution once, from the pre-solve normal speed.
			 */
			static void applyRestitution (std::vector< Body > & bodies, std::vector< Manifold > & manifolds) noexcept;

			/**
			 * @brief Stores the accumulated impulses by feature id and forgets the manifolds absent from this step.
			 */
			void storeImpulses (const std::vector< Manifold > & manifolds) noexcept;

			std::unordered_map< CacheKey, CachedManifold, CacheKeyHash > m_cache;
			uint64_t m_stepCount{0};
	};
}
