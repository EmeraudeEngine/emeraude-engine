/*
 * src/Physics/ConstraintSolver.hpp
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
#include <vector>
#include <algorithm>

/* Local inclusions. */
#include "ContactManifold.hpp"
#include "MovableTrait.hpp"
#include "Libs/Math/Vector.hpp"

namespace EmEn::Physics
{
	/**
	 * @brief Sequential Impulse constraint solver for rigid body dynamics.
	 * @note Implements Erin Catto's iterative impulse-based physics solver.
	 */
	class ConstraintSolver final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ConstraintSolver"};

			/**
			 * @brief Constructs a constraint solver with default iteration counts.
			 */
			ConstraintSolver () noexcept = default;

			/**
			 * @brief Constructs a constraint solver with custom iteration counts.
			 * @param velocityIterations Number of velocity constraint iterations (default: 8).
			 * @param positionIterations Number of position correction iterations (default: 3).
			 */
			ConstraintSolver (int velocityIterations, int positionIterations) noexcept
				: m_velocityIterations{velocityIterations},
				  m_positionIterations{positionIterations}
			{

			}

			/**
			 * @brief Solves all contact constraints in the given manifolds.
			 * @param manifolds A vector of contact manifolds to solve.
			 * @param deltaTime The physics time step in seconds.
			 * @return void
			 */
			void
			solve (std::vector< ContactManifold > & manifolds, float deltaTime) noexcept
			{
				if ( manifolds.empty() || deltaTime <= 0.0F )
				{
					return;
				}

				// Prepare all manifolds (compute relative positions, effective mass, etc.)
				for ( auto & manifold : manifolds )
				{
					manifold.prepare();
					prepareContacts(manifold, deltaTime);
				}

				// Phase 1: Velocity constraints (iterative impulse resolution)
				for ( int iter = 0; iter < m_velocityIterations; ++iter )
				{
					for ( auto & manifold : manifolds )
					{
						solveVelocityConstraints(manifold);
					}
				}

				// Phase 2: Position constraints (Baumgarte stabilization)
				for ( int iter = 0; iter < m_positionIterations; ++iter )
				{
					for ( auto & manifold : manifolds )
					{
						solvePositionConstraints(manifold);
					}
				}
			}

			/**
			 * @brief Sets the number of velocity iterations.
			 * @param iterations Number of iterations (typical: 6-10).
			 * @return void
			 */
			void
			setVelocityIterations (int iterations) noexcept
			{
				m_velocityIterations = std::max(1, iterations);
			}

			/**
			 * @brief Sets the number of position iterations.
			 * @param iterations Number of iterations (typical: 2-4).
			 * @return void
			 */
			void
			setPositionIterations (int iterations) noexcept
			{
				m_positionIterations = std::max(1, iterations);
			}

		private:

			/**
			 * @brief Prepares contact points by computing effective mass and velocity bias.
			 * @param manifold The contact manifold to prepare.
			 * @param deltaTime The physics time step.
			 * @return void
			 */
			void
			prepareContacts (ContactManifold & manifold, float deltaTime) noexcept
			{
				constexpr float baumgarteSlop = 0.01F;      // Penetration allowance (1cm)
				constexpr float baumgarteFactor = 0.2F;     // Position correction strength

				for ( auto & contact : manifold.contacts() )
				{
					MovableTrait * bodyA = contact.bodyA;
					MovableTrait * bodyB = contact.bodyB;

					// Skip if both bodies are nullptr or immovable
					if ( (!bodyA || !bodyA->isMovable()) && (!bodyB || !bodyB->isMovable()) )
					{
						continue;
					}

					// Compute effective mass for this contact
					float massInvA = bodyA && bodyA->isMovable() ? bodyA->getBodyPhysicalProperties().inverseMass() : 0.0F;
					float massInvB = bodyB && bodyB->isMovable() ? bodyB->getBodyPhysicalProperties().inverseMass() : 0.0F;

					// Angular contribution to effective mass
					float angularContribution = 0.0F;

					if ( bodyA && bodyA->isMovable() && bodyA->isRotationPhysicsEnabled() )
					{
						auto rA_cross_n = Libs::Math::Vector< 3, float >::crossProduct(contact.rA, contact.normal);
						auto temp = bodyA->inverseWorldInertia() * rA_cross_n;
						angularContribution += Libs::Math::Vector< 3, float >::dotProduct(rA_cross_n, temp);
					}

					if ( bodyB && bodyB->isMovable() && bodyB->isRotationPhysicsEnabled() )
					{
						auto rB_cross_n = Libs::Math::Vector< 3, float >::crossProduct(contact.rB, contact.normal);
						auto temp = bodyB->inverseWorldInertia() * rB_cross_n;
						angularContribution += Libs::Math::Vector< 3, float >::dotProduct(rB_cross_n, temp);
					}

					contact.effectiveMass = massInvA + massInvB + angularContribution;

					if ( contact.effectiveMass > 0.0F )
					{
						contact.effectiveMass = 1.0F / contact.effectiveMass;
					}

					// Compute velocity bias for position correction (Baumgarte stabilization)
					float penetrationError = std::max(contact.penetrationDepth - baumgarteSlop, 0.0F);
					contact.velocityBias = (baumgarteFactor / deltaTime) * penetrationError;
				}
			}

			/**
			 * @brief Solves velocity constraints for a manifold (applies impulses).
			 * @param manifold The contact manifold.
			 * @return void
			 */
			void
			solveVelocityConstraints (ContactManifold & manifold) noexcept
			{
				for ( auto & contact : manifold.contacts() )
				{
					MovableTrait * bodyA = contact.bodyA;
					MovableTrait * bodyB = contact.bodyB;

					// Skip if both bodies are immovable
					if ( (!bodyA || !bodyA->isMovable()) && (!bodyB || !bodyB->isMovable()) )
					{
						continue;
					}

					// Compute relative velocity at contact point
					Libs::Math::Vector< 3, float > vA{0.0F, 0.0F, 0.0F};
					Libs::Math::Vector< 3, float > vB{0.0F, 0.0F, 0.0F};

					if ( bodyA && bodyA->isMovable() )
					{
						vA = bodyA->linearVelocity();
						if ( bodyA->isRotationPhysicsEnabled() )
						{
							vA += Libs::Math::Vector< 3, float >::crossProduct(bodyA->angularVelocity(), contact.rA);
						}
					}

					if ( bodyB && bodyB->isMovable() )
					{
						vB = bodyB->linearVelocity();
						if ( bodyB->isRotationPhysicsEnabled() )
						{
							vB += Libs::Math::Vector< 3, float >::crossProduct(bodyB->angularVelocity(), contact.rB);
						}
					}

					auto relativeVelocity = vB - vA;
					float normalVelocity = Libs::Math::Vector< 3, float >::dotProduct(relativeVelocity, contact.normal);

					// Compute restitution (average bounciness of both bodies)
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

					// Compute impulse magnitude
					float targetVelocity = -normalVelocity - restitution * std::max(0.0F, -normalVelocity) + contact.velocityBias;
					float lambda = targetVelocity * contact.effectiveMass;

					// Accumulate and clamp impulse (non-penetration constraint: impulse >= 0)
					float oldImpulse = contact.accumulatedNormalImpulse;
					contact.accumulatedNormalImpulse = std::max(0.0F, oldImpulse + lambda);
					lambda = contact.accumulatedNormalImpulse - oldImpulse;

					// Apply impulse
					auto impulse = contact.normal * lambda;

					if ( bodyA && bodyA->isMovable() )
					{
						bodyA->applyLinearImpulse(-impulse);
						if ( bodyA->isRotationPhysicsEnabled() )
						{
							auto angularImpulse = Libs::Math::Vector< 3, float >::crossProduct(contact.rA, -impulse);
							bodyA->applyAngularImpulse(angularImpulse);
						}
					}

					if ( bodyB && bodyB->isMovable() )
					{
						bodyB->applyLinearImpulse(impulse);
						if ( bodyB->isRotationPhysicsEnabled() )
						{
							auto angularImpulse = Libs::Math::Vector< 3, float >::crossProduct(contact.rB, impulse);
							bodyB->applyAngularImpulse(angularImpulse);
						}
					}
				}
			}

			/**
			 * @brief Solves position constraints (corrects penetration directly).
			 * @param manifold The contact manifold.
			 * @return void
			 */
			void
			solvePositionConstraints (ContactManifold & manifold) noexcept
			{
				constexpr float positionCorrectionSlop = 0.005F;  // 5mm allowance
				constexpr float positionCorrectionFactor = 0.3F;  // Correction strength

				for ( auto & contact : manifold.contacts() )
				{
					MovableTrait * bodyA = contact.bodyA;
					MovableTrait * bodyB = contact.bodyB;

					// Skip if both bodies are immovable
					if ( (!bodyA || !bodyA->isMovable()) && (!bodyB || !bodyB->isMovable()) )
					{
						continue;
					}

					// Only correct significant penetrations
					float penetration = contact.penetrationDepth - positionCorrectionSlop;
					if ( penetration <= 0.0F )
					{
						continue;
					}

					// Compute position correction magnitude
					float correction = positionCorrectionFactor * penetration * contact.effectiveMass;

					// Apply position correction
					auto correctionVector = contact.normal * correction;

					if ( bodyA && bodyA->isMovable() )
					{
						float massInvA = bodyA->getBodyPhysicalProperties().inverseMass();
						auto posA = bodyA->worldPosition() - correctionVector * massInvA;
						bodyA->applySimulatedPosition(posA);
					}

					if ( bodyB && bodyB->isMovable() )
					{
						float massInvB = bodyB->getBodyPhysicalProperties().inverseMass();
						auto posB = bodyB->worldPosition() + correctionVector * massInvB;
						bodyB->applySimulatedPosition(posB);
					}
				}
			}

			int m_velocityIterations{8};   // Number of velocity constraint solver iterations
			int m_positionIterations{3};   // Number of position correction iterations
	};
}
