/*
 * src/Physics/ConstraintSolver.cpp
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

#include "ConstraintSolver.hpp"

/* STL inclusions. */
#include <vector>
#include <algorithm>

/* Local inclusions. */
#include "ContactManifold.hpp"
#include "MovableTrait.hpp"
#include "Libs/Math/Vector.hpp"

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
		constexpr float BaumgarteSlop{0.01F};      // Penetration allowance (1cm)
		constexpr float BaumgarteFactor{0.2F};     // Position correction strength

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

			/* Compute impulse magnitude. */
			const float targetVelocity = -normalVelocity - restitution * std::max(0.0F, -normalVelocity) + contact.velocityBias();
			float lambda = targetVelocity * contact.effectiveMass();

			/* Accumulate and clamp impulse (non-penetration constraint: impulse >= 0). */
			contact.updateAccumulatedNormalImpulse(lambda);

			/* Apply impulses. */
			const auto linearImpulse = contact.normal() * lambda;

			if ( bodyA && bodyA->isMovable() )
			{
				bodyA->applyLinearImpulse(-linearImpulse);

				if ( bodyA->isRotationPhysicsEnabled() )
				{
					const auto angularImpulse = Libs::Math::Vector< 3, float >::crossProduct(contact.rA(), -linearImpulse);

					bodyA->applyAngularImpulse(angularImpulse);
				}
			}

			if ( bodyB && bodyB->isMovable() )
			{
				bodyB->applyLinearImpulse(linearImpulse);

				if ( bodyB->isRotationPhysicsEnabled() )
				{
					const auto angularImpulse = Libs::Math::Vector< 3, float >::crossProduct(contact.rB(), linearImpulse);

					bodyB->applyAngularImpulse(angularImpulse);
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
