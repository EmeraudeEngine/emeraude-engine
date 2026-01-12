/*
 * src/Physics/ConstraintSolver.cpp
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

#include "ConstraintSolver.hpp"

/* STL inclusions. */
#include <cmath>
#include <vector>
#include <algorithm>

/* Local inclusions. */
#include "Constants.hpp"
#include "Libs/Math/Vector.hpp"
#include "ContactManifold.hpp"
#include "MovableTrait.hpp"

namespace EmEn::Physics
{
	void
	ConstraintSolver::solve (std::vector< ContactManifold > & manifolds, float deltaTime) noexcept
	{
		/*if ( manifolds.empty() || deltaTime <= 0.0F )
		{
			return;
		}*/

		/* Prepare all manifolds (compute relative positions, effective mass, etc.). */
		for ( auto & manifold : manifolds )
		{
			manifold.prepare();

			this->prepareContacts(manifold, deltaTime);
		}

		/* Phase 1: Velocity constraints (iterative impulse resolution). */
		for ( uint32_t iter = 0; iter < m_velocityIterations; ++iter )
		{
			for ( auto & manifold : manifolds )
			{
				this->solveVelocityConstraints(manifold);
			}
		}

		/* Phase 2: Position constraints (Baumgarte stabilization). */
		for ( uint32_t iter = 0; iter < m_positionIterations; ++iter )
		{
			for ( auto & manifold : manifolds )
			{
				this->solvePositionConstraints(manifold);
			}
		}
	}

	void
	ConstraintSolver::prepareContacts (ContactManifold & manifold, float deltaTime) noexcept
	{
		constexpr float BaumgarteSlop{0.01F};	  // Penetration allowance (1cm)
		constexpr float BaumgarteFactor{0.2F};	 // Position correction strength

		for ( auto & contact : manifold.contacts() )
		{
			const MovableTrait * bodyA = contact.bodyA();
			const MovableTrait * bodyB = contact.bodyB();

			/* Skip if both bodies are nullptr or immovable. */
			if ( (!bodyA || !bodyA->isMovable()) && (!bodyB || !bodyB->isMovable()) )
			{
				continue;
			}

			/* Compute effective mass for this contact. */
			const float massInvA = bodyA && bodyA->isMovable() ? bodyA->getBodyPhysicalProperties().inverseMass() : 0.0F;
			const float massInvB = bodyB && bodyB->isMovable() ? bodyB->getBodyPhysicalProperties().inverseMass() : 0.0F;

			/* Angular contribution to effective mass. */
			float angularContribution = 0.0F;

			if ( bodyA && bodyA->isMovable() && bodyA->isRotationPhysicsEnabled() )
			{
				auto rA_cross_n = Libs::Math::Vector< 3, float >::crossProduct(contact.rA(), contact.normal());
				auto temp = bodyA->inverseWorldInertia() * rA_cross_n;
				angularContribution += Libs::Math::Vector< 3, float >::dotProduct(rA_cross_n, temp);
			}

			if ( bodyB && bodyB->isMovable() && bodyB->isRotationPhysicsEnabled() )
			{
				auto rB_cross_n = Libs::Math::Vector< 3, float >::crossProduct(contact.rB(), contact.normal());
				auto temp = bodyB->inverseWorldInertia() * rB_cross_n;
				angularContribution += Libs::Math::Vector< 3, float >::dotProduct(rB_cross_n, temp);
			}

			contact.setEffectiveMass(massInvA + massInvB + angularContribution);

			/* Compute effective mass for tangent directions (friction). */
			float angularContributionT1 = 0.0F;
			float angularContributionT2 = 0.0F;

			if ( bodyA && bodyA->isMovable() && bodyA->isRotationPhysicsEnabled() )
			{
				auto rA_cross_t1 = Libs::Math::Vector< 3, float >::crossProduct(contact.rA(), contact.tangent1());
				auto tempT1 = bodyA->inverseWorldInertia() * rA_cross_t1;
				angularContributionT1 += Libs::Math::Vector< 3, float >::dotProduct(rA_cross_t1, tempT1);

				auto rA_cross_t2 = Libs::Math::Vector< 3, float >::crossProduct(contact.rA(), contact.tangent2());
				auto tempT2 = bodyA->inverseWorldInertia() * rA_cross_t2;
				angularContributionT2 += Libs::Math::Vector< 3, float >::dotProduct(rA_cross_t2, tempT2);
			}

			if ( bodyB && bodyB->isMovable() && bodyB->isRotationPhysicsEnabled() )
			{
				auto rB_cross_t1 = Libs::Math::Vector< 3, float >::crossProduct(contact.rB(), contact.tangent1());
				auto tempT1 = bodyB->inverseWorldInertia() * rB_cross_t1;
				angularContributionT1 += Libs::Math::Vector< 3, float >::dotProduct(rB_cross_t1, tempT1);

				auto rB_cross_t2 = Libs::Math::Vector< 3, float >::crossProduct(contact.rB(), contact.tangent2());
				auto tempT2 = bodyB->inverseWorldInertia() * rB_cross_t2;
				angularContributionT2 += Libs::Math::Vector< 3, float >::dotProduct(rB_cross_t2, tempT2);
			}

			contact.setEffectiveMassTangent1(massInvA + massInvB + angularContributionT1);
			contact.setEffectiveMassTangent2(massInvA + massInvB + angularContributionT2);

			/* Compute velocity bias for position correction (Baumgarte stabilization). */
			const float penetrationError = std::max(contact.penetrationDepth() - BaumgarteSlop, 0.0F);

			contact.setVelocityBias((BaumgarteFactor / deltaTime) * penetrationError);
		}
	}

	void
	ConstraintSolver::solveVelocityConstraints (ContactManifold & manifold) noexcept
	{
		for ( auto & contact : manifold.contacts() )
		{
			MovableTrait * bodyA = contact.bodyA();
			MovableTrait * bodyB = contact.bodyB();

			/* Skip if both bodies are immovable. */
			if ( (!bodyA || !bodyA->isMovable()) && (!bodyB || !bodyB->isMovable()) )
			{
				continue;
			}

			/* Compute relative velocity at contact point. */
			Libs::Math::Vector< 3, float > velocityA;
			Libs::Math::Vector< 3, float > velocityB;

			if ( bodyA && bodyA->isMovable() )
			{
				velocityA = bodyA->linearVelocity();

				if ( bodyA->isRotationPhysicsEnabled() )
				{
					velocityA += Libs::Math::Vector< 3, float >::crossProduct(bodyA->angularVelocity(), contact.rA());
				}
			}

			if ( bodyB && bodyB->isMovable() )
			{
				velocityB = bodyB->linearVelocity();

				if ( bodyB->isRotationPhysicsEnabled() )
				{
					velocityB += Libs::Math::Vector< 3, float >::crossProduct(bodyB->angularVelocity(), contact.rB());
				}
			}

			auto relativeVelocity = velocityB - velocityA;
			const float normalVelocity = Libs::Math::Vector< 3, float >::dotProduct(relativeVelocity, contact.normal());

			/* Compute restitution (average bounciness of both bodies). */
			float restitution = 0.0F;

			if ( bodyA && bodyB )
			{
				restitution = (bodyA->getBodyPhysicalProperties().bounciness() + bodyB->getBodyPhysicalProperties().bounciness()) * 0.5F;
			}
			else if ( bodyA )
			{
				restitution = bodyA->getBodyPhysicalProperties().bounciness();
			}
			else if ( bodyB )
			{
				restitution = bodyB->getBodyPhysicalProperties().bounciness();
			}

			/* Compute impulse magnitude using standard impulse formula:
			 * j = -(1 + e) * Vn / effective_mass
			 * where Vn is the relative velocity along the normal.
			 *
			 * When objects are approaching (Vn < 0), we need to apply a separating impulse.
			 * The restitution coefficient determines how much the objects "bounce back". */
			const float targetVelocity = -(1.0F + restitution) * normalVelocity + contact.velocityBias();
			float lambda = targetVelocity * contact.effectiveMass();

			/* Accumulate and clamp impulse (non-penetration constraint: impulse >= 0). */
			contact.updateAccumulatedNormalImpulse(lambda);

			/* Apply impulses. */
			const auto linearImpulse = contact.normal() * lambda;
			const auto & normal = contact.normal();

			/* Check if this is a ground collision for each body.
			 * In Y-down system, normal points from bodyA to bodyB.
			 * - If normal.Y > 0.7 (pointing down), bodyA is above and grounded.
			 * - If normal.Y < -0.7 (pointing up), bodyB is above and grounded.
			 * Threshold of 0.7 allows surfaces up to ~45 degrees to count as ground. */
			constexpr auto GroundNormalThreshold{0.7F};

			if ( bodyA && bodyA->isMovable() )
			{
				bodyA->applyLinearImpulse(-linearImpulse);

				/* Body A is grounded if normal points downward (A is on top).
				 * Only ground against static surfaces, not other dynamic bodies. */
				if ( normal[Libs::Math::Y] > GroundNormalThreshold && (!bodyB || !bodyB->isMovable()) )
				{
					/* Ground on Entity since this is Node-to-Node collision resolution. */
					bodyA->setGrounded(GroundedSource::Entity, bodyB);
				}

				if ( bodyA->isRotationPhysicsEnabled() )
				{
					const auto angularImpulse = Libs::Math::Vector< 3, float >::crossProduct(contact.rA(), -linearImpulse);

					bodyA->applyAngularImpulse(angularImpulse);
				}
			}

			if ( bodyB && bodyB->isMovable() )
			{
				bodyB->applyLinearImpulse(linearImpulse);

				/* Body B is grounded if normal points upward (B is on top).
				 * Only ground against static surfaces, not other dynamic bodies. */
				if ( normal[Libs::Math::Y] < -GroundNormalThreshold && (!bodyA || !bodyA->isMovable()) )
				{
					/* Ground on Entity since this is Node-to-Node collision resolution. */
					bodyB->setGrounded(GroundedSource::Entity, bodyA);
				}

				if ( bodyB->isRotationPhysicsEnabled() )
				{
					const auto angularImpulse = Libs::Math::Vector< 3, float >::crossProduct(contact.rB(), linearImpulse);

					bodyB->applyAngularImpulse(angularImpulse);
				}
			}

			/* Notify bodies of collision event.
			 * Convert impulse (N·s) to force (N) by dividing by delta time.
			 * F = J / Δt where J is the impulse magnitude. */
			const float impactForce = std::abs(lambda) / EngineUpdateCycleDurationS< float >;

			if ( impactForce > 0.0F )
			{
				if ( bodyA && bodyA->isMovable() )
				{
					bodyA->onCollision(impactForce);
				}

				if ( bodyB && bodyB->isMovable() )
				{
					bodyB->onCollision(impactForce);
				}
			}

			/* ============================================================
			 * FRICTION IMPULSES (Coulomb friction model)
			 * ============================================================ */

			/* Compute friction coefficient (average stickiness of both bodies). */
			float friction = 0.0F;

			if ( bodyA && bodyB )
			{
				friction = (bodyA->getBodyPhysicalProperties().stickiness() + bodyB->getBodyPhysicalProperties().stickiness()) * 0.5F;
			}
			else if ( bodyA )
			{
				friction = bodyA->getBodyPhysicalProperties().stickiness();
			}
			else if ( bodyB )
			{
				friction = bodyB->getBodyPhysicalProperties().stickiness();
			}

			/* Maximum friction impulse is proportional to normal force (Coulomb's law). */
			const float maxFriction = friction * contact.accumulatedNormalImpulse();

			/* Skip friction if no normal force or no friction coefficient. */
			if ( maxFriction <= 0.0F )
			{
				continue;
			}

			/* Recompute relative velocity after normal impulse was applied. */
			velocityA.reset();
			velocityB.reset();

			if ( bodyA && bodyA->isMovable() )
			{
				velocityA = bodyA->linearVelocity();

				if ( bodyA->isRotationPhysicsEnabled() )
				{
					velocityA += Libs::Math::Vector< 3, float >::crossProduct(bodyA->angularVelocity(), contact.rA());
				}
			}

			if ( bodyB && bodyB->isMovable() )
			{
				velocityB = bodyB->linearVelocity();

				if ( bodyB->isRotationPhysicsEnabled() )
				{
					velocityB += Libs::Math::Vector< 3, float >::crossProduct(bodyB->angularVelocity(), contact.rB());
				}
			}

			relativeVelocity = velocityB - velocityA;

			/* Tangent 1 friction. */
			{
				const float tangentVelocity1 = Libs::Math::Vector< 3, float >::dotProduct(relativeVelocity, contact.tangent1());
				float lambdaT1 = -tangentVelocity1 * contact.effectiveMassTangent1();

				contact.updateAccumulatedTangentImpulse(lambdaT1, 0, maxFriction);

				const auto frictionImpulse1 = contact.tangent1() * lambdaT1;

				if ( bodyA && bodyA->isMovable() )
				{
					bodyA->applyLinearImpulse(-frictionImpulse1);

					if ( bodyA->isRotationPhysicsEnabled() )
					{
						bodyA->applyAngularImpulse(Libs::Math::Vector< 3, float >::crossProduct(contact.rA(), -frictionImpulse1));
					}
				}

				if ( bodyB && bodyB->isMovable() )
				{
					bodyB->applyLinearImpulse(frictionImpulse1);

					if ( bodyB->isRotationPhysicsEnabled() )
					{
						bodyB->applyAngularImpulse(Libs::Math::Vector< 3, float >::crossProduct(contact.rB(), frictionImpulse1));
					}
				}
			}

			/* Tangent 2 friction. */
			{
				const float tangentVelocity2 = Libs::Math::Vector< 3, float >::dotProduct(relativeVelocity, contact.tangent2());
				float lambdaT2 = -tangentVelocity2 * contact.effectiveMassTangent2();

				contact.updateAccumulatedTangentImpulse(lambdaT2, 1, maxFriction);

				const auto frictionImpulse2 = contact.tangent2() * lambdaT2;

				if ( bodyA && bodyA->isMovable() )
				{
					bodyA->applyLinearImpulse(-frictionImpulse2);

					if ( bodyA->isRotationPhysicsEnabled() )
					{
						bodyA->applyAngularImpulse(Libs::Math::Vector< 3, float >::crossProduct(contact.rA(), -frictionImpulse2));
					}
				}

				if ( bodyB && bodyB->isMovable() )
				{
					bodyB->applyLinearImpulse(frictionImpulse2);

					if ( bodyB->isRotationPhysicsEnabled() )
					{
						bodyB->applyAngularImpulse(Libs::Math::Vector< 3, float >::crossProduct(contact.rB(), frictionImpulse2));
					}
				}
			}
		}
	}

	void
	ConstraintSolver::solvePositionConstraints (ContactManifold & manifold) noexcept
	{
		constexpr float positionCorrectionSlop = 0.001F;  // 1mm allowance (was 5mm)
		constexpr float positionCorrectionFactor = 0.8F;  // Correction strength (was 0.3)

		for ( auto & contact : manifold.contacts() )
		{
			MovableTrait * bodyA = contact.bodyA();
			MovableTrait * bodyB = contact.bodyB();

			/* Skip if both bodies are immovable. */
			if ( (!bodyA || !bodyA->isMovable()) && (!bodyB || !bodyB->isMovable()) )
			{
				continue;
			}

			/* Only correct significant penetrations. */
			const float penetration = contact.penetrationDepth() - positionCorrectionSlop;

			if ( penetration <= 0.0F )
			{
				continue;
			}

			/* Compute position correction magnitude. */
			const float correction = positionCorrectionFactor * penetration * contact.effectiveMass();

			/* Apply position correction. */
			auto correctionVector = contact.normal() * correction;

			if ( bodyA && bodyA->isMovable() )
			{
				const float massInvA = bodyA->getBodyPhysicalProperties().inverseMass();
				const auto deltaA = -correctionVector * massInvA;

				bodyA->moveFromPhysics(deltaA);
			}

			if ( bodyB && bodyB->isMovable() )
			{
				const float massInvB = bodyB->getBodyPhysicalProperties().inverseMass();
				const auto deltaB = correctionVector * massInvB;

				bodyB->moveFromPhysics(deltaB);
			}
		}
	};
}
