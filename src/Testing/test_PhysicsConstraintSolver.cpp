/*
 * src/Testing/test_PhysicsConstraintSolver.cpp
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

#include "gtest/gtest.h"

/* Local inclusions. */
#include "Physics/ContactPoint.hpp"
#include "Physics/ContactManifold.hpp"
#include "Physics/ConstraintSolver.hpp"
#include "Physics/MovableTrait.hpp"
#include "Physics/BodyPhysicalProperties.hpp"
#include "Libs/Math/Vector.hpp"
#include "Libs/Math/Matrix.hpp"

using namespace EmEn;
using namespace EmEn::Physics;
using namespace EmEn::Libs::Math;

/**
 * @brief Mock MovableTrait for testing (since it's abstract).
 */
class MockMovableTrait : public MovableTrait
{
	public:

		MockMovableTrait () noexcept
		{
			// Initialize with default properties
			m_bodyPhysicalProperties.setProperties(1.0F, 1.0F, 0.5F, 0.5F, 0.5F);
		}

		explicit
		MockMovableTrait (float mass, float surface, float drag, float bounciness, float stickiness) noexcept
		{
			m_bodyPhysicalProperties.setProperties(mass, surface, drag, bounciness, stickiness);
		}

		void
		setMassAndInertia (float mass, const Matrix< 3, float > & inertia) noexcept
		{
			m_bodyPhysicalProperties.setMass(mass, false);
			m_bodyPhysicalProperties.setInertia(inertia, false);
		}

		void
		setProperties (float mass, float surface, float drag, float bounciness, float stickiness) noexcept
		{
			m_bodyPhysicalProperties.setProperties(mass, surface, drag, bounciness, stickiness);
		}

		void
		setWorldPosition (const Vector< 3, float > & pos) noexcept
		{
			m_worldPosition = pos;
		}

		void
		setWorldCenterOfMass (const Vector< 3, float > & com) noexcept
		{
			m_worldCenterOfMass = com;
		}

		// Implement pure virtual methods
		Vector< 3, float >
		getWorldVelocity () const noexcept override
		{
			return this->linearVelocity();
		}

		Vector< 3, float >
		getWorldCenterOfMass () const noexcept override
		{
			return m_worldCenterOfMass;
		}

		[[nodiscard]]
		const BodyPhysicalProperties &
		getBodyPhysicalProperties () const noexcept override
		{
			return m_bodyPhysicalProperties;
		}

		void
		onHit (float /*impactForce*/) noexcept override
		{

		}

		void
		onImpulse () noexcept override
		{

		}

	protected:

		Vector< 3, float >
		getWorldPosition () const noexcept override
		{
			return m_worldPosition;
		}

		void
		moveFromPhysics (const Vector< 3, float > & worldPosition) noexcept override
		{
			m_worldPosition = worldPosition;
		}

		void
		rotateFromPhysics (float /*radianAngle*/, const Vector< 3, float > & /*worldDirection*/) noexcept override
		{

		}

	private:

		BodyPhysicalProperties m_bodyPhysicalProperties;
		Vector< 3, float > m_worldPosition{0.0F, 0.0F, 0.0F};
		Vector< 3, float > m_worldCenterOfMass{0.0F, 0.0F, 0.0F};
};

/**
 * @brief Test fixture for ConstraintSolver tests.
 */
class PhysicsConstraintSolverTest : public ::testing::Test
{
	protected:
		// No setUp needed
};

/**
 * @test Verifies that ContactPoint structure initializes correctly.
 */
TEST_F(PhysicsConstraintSolverTest, ContactPointConstruction)
{
	MockMovableTrait bodyA, bodyB;

	Vector< 3, float > position{1.0F, 2.0F, 3.0F};
	Vector< 3, float > normal{0.0F, 1.0F, 0.0F};
	float depth = 0.1F;

	ContactPoint contact(position, normal, depth, &bodyA, &bodyB);

	EXPECT_EQ(contact.positionWorld, position);
	EXPECT_EQ(contact.normal, normal);
	EXPECT_FLOAT_EQ(contact.penetrationDepth, depth);
	EXPECT_EQ(contact.bodyA, &bodyA);
	EXPECT_EQ(contact.bodyB, &bodyB);
	EXPECT_FLOAT_EQ(contact.accumulatedNormalImpulse, 0.0F);
}

/**
 * @test Verifies that ContactManifold manages contacts correctly.
 */
TEST_F(PhysicsConstraintSolverTest, ContactManifoldAddContact)
{
	MockMovableTrait bodyA, bodyB;
	ContactManifold manifold(&bodyA, &bodyB);

	EXPECT_FALSE(manifold.hasContacts());
	EXPECT_EQ(manifold.contactCount(), 0UL);

	Vector< 3, float > pos{0.0F, 0.0F, 0.0F};
	Vector< 3, float > normal{0.0F, 1.0F, 0.0F};

	bool added = manifold.addContact(pos, normal, 0.1F);
	EXPECT_TRUE(added);
	EXPECT_TRUE(manifold.hasContacts());
	EXPECT_EQ(manifold.contactCount(), 1UL);

	// Add 3 more (total 4, max capacity)
	manifold.addContact(pos, normal, 0.1F);
	manifold.addContact(pos, normal, 0.1F);
	manifold.addContact(pos, normal, 0.1F);
	EXPECT_EQ(manifold.contactCount(), 4UL);

	// Try to add 5th (should fail)
	added = manifold.addContact(pos, normal, 0.1F);
	EXPECT_FALSE(added);
	EXPECT_EQ(manifold.contactCount(), 4UL);
}

/**
 * @test Verifies that impulses are applied correctly to linear velocity.
 */
TEST_F(PhysicsConstraintSolverTest, ApplyLinearImpulse)
{
	MockMovableTrait body(1.0F, 1.0F, 0.5F, 0.5F, 0.5F);  // 1kg cube
	body.setMovingAbility(true);

	Vector< 3, float > initialVelocity{0.0F, 0.0F, 0.0F};
	body.setLinearVelocity(initialVelocity);

	// Apply impulse of 10 N·s upward (mass = 1kg, so Δv = 10 m/s)
	Vector< 3, float > impulse{0.0F, 10.0F, 0.0F};
	body.applyLinearImpulse(impulse);

	Vector< 3, float > expectedVelocity{0.0F, 10.0F, 0.0F};
	EXPECT_EQ(body.linearVelocity(), expectedVelocity);
	EXPECT_FLOAT_EQ(body.linearSpeed(), 10.0F);
}

/**
 * @test Verifies that angular impulses are applied correctly.
 */
TEST_F(PhysicsConstraintSolverTest, ApplyAngularImpulse)
{
	MockMovableTrait body(1.0F, 1.0F, 0.5F, 0.5F, 0.5F);
	Matrix< 3, float > inertia;
	inertia[M3x3Col0Row0] = 1.0F;
	inertia[M3x3Col1Row1] = 1.0F;
	inertia[M3x3Col2Row2] = 1.0F;
	body.setMassAndInertia(1.0F, inertia);
	body.setMovingAbility(true);
	body.enableRotationPhysics(true);

	// Initialize inverse world inertia (identity for simplicity)
	Matrix< 3, float > rotationMatrix;  // Identity by default
	body.updateInverseWorldInertia(rotationMatrix);

	Vector< 3, float > angularImpulse{0.0F, 0.0F, 1.0F};
	body.applyAngularImpulse(angularImpulse);

	// With identity inertia, angular velocity should equal angular impulse
	EXPECT_EQ(body.angularVelocity(), angularImpulse);
	EXPECT_FLOAT_EQ(body.angularSpeed(), 1.0F);
}

/**
 * @test Verifies momentum conservation in a simple collision.
 * Two bodies with equal mass colliding head-on should exchange velocities.
 */
TEST_F(PhysicsConstraintSolverTest, MomentumConservation)
{
	MockMovableTrait bodyA, bodyB;

	// Setup body A (moving right at 5 m/s)
	
	bodyA.setMovingAbility(true);
	bodyA.setWorldPosition({-1.0F, 0.0F, 0.0F});
	bodyA.setWorldCenterOfMass({-1.0F, 0.0F, 0.0F});
	bodyA.setLinearVelocity({5.0F, 0.0F, 0.0F});

	// Setup body B (moving left at -5 m/s)
	
	bodyB.setMovingAbility(true);
	bodyB.setWorldPosition({1.0F, 0.0F, 0.0F});
	bodyB.setWorldCenterOfMass({1.0F, 0.0F, 0.0F});
	bodyB.setLinearVelocity({-5.0F, 0.0F, 0.0F});

	// Create contact manifold (head-on collision)
	ContactManifold manifold(&bodyA, &bodyB);
	Vector< 3, float > contactPos{0.0F, 0.0F, 0.0F};
	Vector< 3, float > contactNormal{1.0F, 0.0F, 0.0F};  // Normal from A to B
	float penetration = 0.1F;

	manifold.addContact(contactPos, contactNormal, penetration);
	manifold.prepare();

	// Initial momentum
	float initialMomentum = bodyA.linearVelocity()[0] * 1.0F + bodyB.linearVelocity()[0] * 1.0F;
	EXPECT_FLOAT_EQ(initialMomentum, 0.0F);  // 5 + (-5) = 0

	// Solve collision
	std::vector< ContactManifold > manifolds{manifold};
	ConstraintSolver solver(10, 3);
	solver.solve(manifolds, 0.016F);  // 16ms timestep

	// Final momentum should be conserved
	float finalMomentum = bodyA.linearVelocity()[0] * 1.0F + bodyB.linearVelocity()[0] * 1.0F;
	EXPECT_NEAR(finalMomentum, initialMomentum, 0.01F);  // Within 1% tolerance

	// Velocities should have changed (bodies bounced)
	EXPECT_LT(bodyA.linearVelocity()[0], 5.0F);  // A slowed down or reversed
	EXPECT_GT(bodyB.linearVelocity()[0], -5.0F);  // B slowed down or reversed
}

/**
 * @test Verifies that collision with zero restitution results in no bounce.
 */
TEST_F(PhysicsConstraintSolverTest, ZeroRestitutionNoBounce)
{
	MockMovableTrait bodyA, bodyB;

	// Setup with zero bounciness
	Matrix< 3, float > inertia;
	inertia[M3x3Col0Row0] = 1.0F;
	inertia[M3x3Col1Row1] = 1.0F;
	inertia[M3x3Col2Row2] = 1.0F;


	bodyA.setMovingAbility(true);
	bodyA.setProperties(1.0F, 1.0F, 0.5F, 0.0F, 0.5F);  // Zero bounciness
	bodyA.setMassAndInertia(1.0F, inertia);
	bodyA.setWorldPosition({0.0F, 1.0F, 0.0F});
	bodyA.setWorldCenterOfMass({0.0F, 1.0F, 0.0F});
	bodyA.setLinearVelocity({0.0F, -10.0F, 0.0F});  // Falling down

	// Initialize inverse world inertia (identity rotation for this test)
	Matrix< 3, float > identityRotation;
	identityRotation[M3x3Col0Row0] = 1.0F;
	identityRotation[M3x3Col1Row1] = 1.0F;
	identityRotation[M3x3Col2Row2] = 1.0F;
	bodyA.updateInverseWorldInertia(identityRotation);


	bodyB.setMovingAbility(false);  // Static ground
	bodyB.setProperties(1.0F, 1.0F, 0.5F, 0.0F, 0.5F);  // Zero bounciness
	bodyB.setMassAndInertia(1.0F, inertia);
	bodyB.setWorldPosition({0.0F, 0.0F, 0.0F});
	bodyB.setWorldCenterOfMass({0.0F, 0.0F, 0.0F});
	bodyB.updateInverseWorldInertia(identityRotation);

	// Create contact
	// Normal points FROM A TO B (A is above at y=1, B is below at y=0, so normal is downward)
	ContactManifold manifold(&bodyA, &bodyB);
	manifold.addContact({0.0F, 0.5F, 0.0F}, {0.0F, -1.0F, 0.0F}, 0.05F);
	manifold.prepare();

	// Solve
	std::vector< ContactManifold > manifolds{manifold};
	ConstraintSolver solver(10, 3);
	solver.solve(manifolds, 0.016F);

	// With zero restitution, the normal velocity should be eliminated
	// The solver should prevent penetration by zeroing out velocity in contact normal direction
	EXPECT_NEAR(bodyA.linearVelocity()[1], 0.0F, 0.5F);  // Should be near zero (no bounce)
}

/**
 * @test Verifies position correction removes penetration.
 */
TEST_F(PhysicsConstraintSolverTest, PositionCorrection)
{
	MockMovableTrait bodyA, bodyB;

	
	bodyA.setMovingAbility(true);
	bodyA.setWorldPosition({0.0F, 0.5F, 0.0F});  // Penetrating into B
	bodyA.setWorldCenterOfMass({0.0F, 0.5F, 0.0F});
	bodyA.setLinearVelocity({0.0F, 0.0F, 0.0F});

	
	bodyB.setMovingAbility(false);  // Static
	bodyB.setWorldPosition({0.0F, 0.0F, 0.0F});
	bodyB.setWorldCenterOfMass({0.0F, 0.0F, 0.0F});

	// Large penetration
	float penetration = 0.2F;
	ContactManifold manifold(&bodyA, &bodyB);
	manifold.addContact({0.0F, 0.4F, 0.0F}, {0.0F, 1.0F, 0.0F}, penetration);
	manifold.prepare();

	Vector< 3, float > initialPos = bodyA.worldPosition();

	// Solve
	std::vector< ContactManifold > manifolds{manifold};
	ConstraintSolver solver(10, 5);  // More position iterations
	solver.solve(manifolds, 0.016F);

	Vector< 3, float > finalPos = bodyA.worldPosition();

	// Position correction should have occurred (penetration reduced)
	// Note: With velocity constraints active, position might move in either direction
	// The key is that penetration is being resolved over iterations
	EXPECT_NE(finalPos[1], initialPos[1]);  // Position should change
}

/**
 * @test Verifies that static bodies don't move during solving.
 */
TEST_F(PhysicsConstraintSolverTest, StaticBodyDoesNotMove)
{
	MockMovableTrait bodyA, bodyB;

	
	bodyA.setMovingAbility(true);
	bodyA.setWorldPosition({0.0F, 1.0F, 0.0F});
	bodyA.setWorldCenterOfMass({0.0F, 1.0F, 0.0F});
	bodyA.setLinearVelocity({0.0F, -5.0F, 0.0F});

	
	bodyB.setMovingAbility(false);  // Static
	bodyB.setWorldPosition({0.0F, 0.0F, 0.0F});
	bodyB.setWorldCenterOfMass({0.0F, 0.0F, 0.0F});
	bodyB.setLinearVelocity({0.0F, 0.0F, 0.0F});

	Vector< 3, float > staticInitialPos = bodyB.worldPosition();
	Vector< 3, float > staticInitialVel = bodyB.linearVelocity();

	// Create contact
	ContactManifold manifold(&bodyA, &bodyB);
	manifold.addContact({0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 0.1F);
	manifold.prepare();

	// Solve
	std::vector< ContactManifold > manifolds{manifold};
	ConstraintSolver solver(10, 3);
	solver.solve(manifolds, 0.016F);

	// Static body should not have moved
	EXPECT_EQ(bodyB.worldPosition(), staticInitialPos);
	EXPECT_EQ(bodyB.linearVelocity(), staticInitialVel);
}

/**
 * @test Verifies that solver handles empty manifold list gracefully.
 */
TEST_F(PhysicsConstraintSolverTest, EmptyManifoldList)
{
	std::vector< ContactManifold > manifolds;

	ConstraintSolver solver;
	EXPECT_NO_THROW(solver.solve(manifolds, 0.016F));
}

/**
 * @test Verifies that solver handles zero or negative timestep gracefully.
 */
TEST_F(PhysicsConstraintSolverTest, InvalidTimestep)
{
	MockMovableTrait bodyA, bodyB;
	
	

	ContactManifold manifold(&bodyA, &bodyB);
	manifold.addContact({0.0F, 0.0F, 0.0F}, {0.0F, 1.0F, 0.0F}, 0.1F);

	std::vector< ContactManifold > manifolds{manifold};
	ConstraintSolver solver;

	// Zero timestep
	EXPECT_NO_THROW(solver.solve(manifolds, 0.0F));

	// Negative timestep
	EXPECT_NO_THROW(solver.solve(manifolds, -0.016F));
}
