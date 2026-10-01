/*
 * src/Scenes/Scene.physics.cpp
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

#include "Scene.hpp"

/* STL inclusions. */
#include <algorithm>
#include <cmath>
#include <limits>
#include <span>

/* Local inclusions. */
#include "Math/RigidBody.hpp"
#include "Math/Space3D/Contacts/ContactManifold.hpp"
#include "Physics/BodyInertia.hpp"
#include "Physics/NarrowPhase.hpp"

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
	using namespace Physics;

	namespace
	{
		/**
		 * @brief Applies complete collision response: velocity bounce + grounded state.
		 * @param movable The movable trait to update.
		 * @param surfaceNormal The dominant collision surface normal.
		 * @param groundPenetration The ground penetration depth (0 if no direct ground collision).
		 * @param dominantSource The source of the dominant collision (Ground, Boundary, or Entity).
		 * @param groundedOnEntity Pointer to the entity we collided with (if source is Entity).
		 */
		void
		applyCollisionResponse (MovableTrait * movable, const Vector< 3, float > & surfaceNormal, float groundPenetration, GroundedSource dominantSource, const MovableTrait * groundedOnEntity) noexcept
		{
			auto velocity = movable->linearVelocity();
			const float vn = Vector< 3, float >::dotProduct(velocity, surfaceNormal);

			/* Apply velocity bounce if moving into surface.
			 * vn > 0 means velocity is going INTO the surface (same direction as normal). */
			if ( vn > 0.0F )
			{
				velocity -= surfaceNormal * vn * (1.0F + movable->getBodyPhysicalProperties().bounciness());
				movable->setLinearVelocity(velocity);
			}

			/* Apply grounded response if standing on a surface.
			 * Surface is considered "ground" if:
			 * - Direct ground collision (groundPenetration > 0), OR
			 * - Normal points downward (Y < -0.7 in Y-up = surface faces up) */
			constexpr auto GroundNormalThreshold{0.7F}; /* ~45 degrees */
			const bool isOnSurface = (groundPenetration > 0.0F) || (surfaceNormal[Y] < -GroundNormalThreshold);

			/* Only apply grounded response if not bouncing away (velocity Y near zero or negative). */
			if ( isOnSurface && velocity[Y] <= 0.1F )
			{
				velocity[Y] = 0.0F;
				movable->setLinearVelocity(velocity);

				/* Set grounded with appropriate source.
				 * Priority: Ground > Boundary > Entity (ground is always ground if detected). */
				if ( groundPenetration > 0.0F )
				{
					movable->setGrounded(GroundedSource::Ground);
				}
				else
				{
					movable->setGrounded(dominantSource, groundedOnEntity);
				}
			}
		}
	}

	namespace
	{
		/** @brief The persistent key B of a ground manifold (no entity has this creation number). */
		constexpr uint64_t GroundKey{std::numeric_limits< uint64_t >::max()};

		/** @brief The default ground material (P2: GroundLevelInterface carries none): the body's own feel dominates. */
		constexpr float GroundFriction{1.0F};
		constexpr float GroundRestitution{0.0F};

		/** @brief A support is a contact whose normal is within ~45° of the gravity direction. */
		constexpr float SupportCosine{0.7F};

		/** @brief Under this approach speed (m/s) a contact is no impact: a body RESTING on another approaches at ~1e-6
		 * m/s, and notifying NodeCollision for it fired once per cycle per resting body (1001 per cycle in
		 * balls-of-steel, 2026-10-01) — the former pipeline fired nothing at rest. */
		constexpr float ImpactSpeedThreshold{0.05F};

		/**
		 * @brief Converts a base contact manifold into a solver manifold (the margin is removed from the depth).
		 */
		void
		appendManifold (const Space3D::ContactManifold< float > & contact, uint32_t bodyA, uint32_t bodyB, uint64_t keyA, uint64_t keyB, uint64_t keySub, float friction, float restitution, std::vector< SoftStepSolver::Manifold > & manifolds) noexcept
		{
			if ( contact.empty() )
			{
				return;
			}

			SoftStepSolver::Manifold manifold;
			manifold.normal = contact.normal();
			manifold.keyA = keyA;
			manifold.keyB = keyB;
			manifold.keySub = keySub;
			manifold.bodyA = bodyA;
			manifold.bodyB = bodyB;
			manifold.friction = friction;
			manifold.restitution = restitution;

			for ( const auto & point : contact.points() )
			{
				SoftStepSolver::Point solverPoint;
				solverPoint.position = point.position();
				/* Shape A was inflated by the speculative margin: the true separation is margin − depth. */
				solverPoint.separation = SoftStepSolver::SpeculativeMargin - point.depth();
				solverPoint.featureId = point.featureId();

				manifold.points.push_back(solverPoint);
			}

			manifolds.push_back(manifold);
		}

		/**
		 * @brief Collects the ground triangles under a body into solver manifolds.
		 */
		class GroundContactCollector final : public GroundTriangleVisitor
		{
			public:

				GroundContactCollector (const CollisionModelInterface & model, const CartesianFrame< float > & frame, uint32_t bodyIndex, uint64_t creationNumber, float friction, float restitution, std::vector< SoftStepSolver::Manifold > & manifolds) noexcept
					: m_model{model},
					m_frame{frame},
					m_manifolds{manifolds},
					m_creationNumber{creationNumber},
					m_bodyIndex{bodyIndex},
					m_friction{friction},
					m_restitution{restitution}
				{

				}

				void
				onTriangle (const Space3D::Triangle< float > & triangle, uint32_t featureId) noexcept override
				{
					/* One-sided and solid below (NarrowPhase::generateGround()). */
					if ( NarrowPhase::generateGround(m_model, m_frame, triangle, SoftStepSolver::SpeculativeMargin, m_claimedLowPoints, m_contact) )
					{
						/* A = the body, B = the static world; the sub key tells the triangles apart. */
						appendManifold(m_contact, m_bodyIndex, 0, m_creationNumber, GroundKey, static_cast< uint64_t >(featureId) + 1, m_friction, m_restitution, m_manifolds);
					}
				}

			private:

				const CollisionModelInterface & m_model;
				const CartesianFrame< float > & m_frame;
				std::vector< SoftStepSolver::Manifold > & m_manifolds;
				Space3D::ContactManifold< float > m_contact;
				uint64_t m_creationNumber;
				uint32_t m_bodyIndex;
				uint32_t m_claimedLowPoints{0};
				float m_friction;
				float m_restitution;
		};

		/** @brief Whether two world AABBs overlap, the first inflated by a margin. */
		[[nodiscard]]
		bool
		boxesOverlap (const Space3D::AACuboid< float > & boxA, const Space3D::AACuboid< float > & boxB, float margin) noexcept
		{
			for ( size_t axis = 0; axis < 3; ++axis )
			{
				if ( boxA.minimum(axis) - margin > boxB.maximum(axis) || boxB.minimum(axis) > boxA.maximum(axis) + margin )
				{
					return false;
				}
			}

			return true;
		}
	}

	void
	Scene::resolveCollisions (std::vector< std::shared_ptr< AbstractEntity > > & movedEntities, std::vector< PhysicsImpact > & impacts) const noexcept
	{
		movedEntities.clear();
		impacts.clear();

		if ( m_physicsOctree == nullptr )
		{
			return;
		}

		/* Lock the physics octree for the duration of the simulation to prevent
		 * concurrent modifications from other threads (e.g., checkEntityLocationInOctrees). */
		const std::scoped_lock lock{m_physicsOctreeAccess};

		constexpr float PhysicsStepSeconds = WorldPhysicsUpdateCycleDurationS< float >;
		constexpr float SpeculativeContactMargin = SoftStepSolver::SpeculativeMargin;

		auto & bodies = m_physicsBodies;
		auto & bodyEntities = m_physicsBodyEntities;
		auto & manifolds = m_physicsManifolds;
		auto & bodyIndices = m_physicsBodyIndices;

		bodies.clear();
		bodyEntities.clear();
		manifolds.clear();
		bodyIndices.clear();

		/* ============================================================
		 * 1. THE BODIES, in creation order (index 0 = the static world).
		 * ============================================================ */
		m_physicsOctree->forEachSector([&bodyEntities] (const OctreeSector< AbstractEntity, true > & /*sector*/, const std::vector< std::shared_ptr< AbstractEntity > > & candidates, size_t ownedOffset) {
			for ( size_t index = ownedOffset; index < candidates.size(); ++index )
			{
				bodyEntities.push_back(candidates[index]);
			}
		});

		std::ranges::sort(bodyEntities, [] (const auto & lhs, const auto & rhs) {
			return lhs->creationNumber() < rhs->creationNumber();
		});

		/* The static world first. */
		bodies.emplace_back();
		bodyEntities.insert(bodyEntities.begin(), nullptr);

		for ( size_t entityIndex = 1; entityIndex < bodyEntities.size(); ++entityIndex )
		{
			const auto & entity = bodyEntities[entityIndex];
			const auto frame = entity->getWorldCoordinates();

			if ( !entity->hasMovableAbility() || entity->getMovableTrait() == nullptr )
			{
				/* A static entity: part of the static world (it never moves). */
				bodyIndices[entity.get()] = 0;
				bodies.emplace_back();

				continue;
			}

			auto * movable = entity->getMovableTrait();
			const auto rotation = frame.getRotationMatrix3();
			const auto * model = entity->collisionModel();
			SoftStepSolver::Body body;
			/* P3: the body turns about the centroid of its shape (decision 8b). */
			body.centerOffset = model != nullptr ? rotation * model->centerOfMassOffset(frame.scalingFactor()) : Vector< 3, float >{};
			body.position = frame.position() + body.centerOffset;
			body.orientation = frame.toQuaternion();

			if ( movable->isMovable() )
			{
				const auto & properties = movable->getBodyPhysicalProperties();

				body.movable = movable;
				body.linearVelocity = movable->linearVelocity();
				body.angularVelocity = movable->isRotationPhysicsEnabled() ? movable->angularVelocity() : Vector< 3, float >{};
				body.inverseMass = properties.inverseMass();
				/* P3: the explicit tensor, else the shape's as a uniform solid (decision 8c); zero = no rotation. */
				body.inverseInertia = movable->isRotationPhysicsEnabled() ? worldInverseInertia(localInverseInertia(properties, model, frame.scalingFactor()), rotation) : Matrix< 3, float >{std::array< float, 9 >{}};
				body.gravity = !movable->isFreeFlyModeEnabled() && !properties.isMassNull();
				/* A sleeping body is solid but still; contact with an awake one wakes it (below). */
				body.dynamic = !entity->isSimulationPaused();
			}
			else
			{
				/* KINEMATIC (an animated, non-movable node): infinite mass, the velocity of its own motion. */
				const auto creation = entity->creationNumber();

				if ( const auto previous = m_kinematicLastPositions.find(creation); previous != m_kinematicLastPositions.end() )
				{
					body.linearVelocity = (body.position - previous->second) * (1.0F / PhysicsStepSeconds);
				}

				m_kinematicLastPositions[creation] = body.position;
			}

			bodyIndices[entity.get()] = static_cast< uint32_t >(bodies.size());
			bodies.push_back(body);
		}

		/* ============================================================
		 * 2. THE PAIRS (the octree's pairing contract: owned × owned, owned × inherited, each pair once).
		 * ============================================================ */
		const auto isActive = [&bodies] (uint32_t index) {
			return index != 0 && bodies[index].movable != nullptr && bodies[index].dynamic;
		};

		const auto isAsleep = [&bodies] (uint32_t index) {
			return index != 0 && bodies[index].movable != nullptr && !bodies[index].dynamic;
		};

		Space3D::ContactManifold< float > contact;

		const auto testPair = [&] (const std::shared_ptr< AbstractEntity > & entityA, const std::shared_ptr< AbstractEntity > & entityB) {
			/* Canonical order: A has the lower creation number, whatever the octree's order. */
			const auto & first = entityA->creationNumber() < entityB->creationNumber() ? entityA : entityB;
			const auto & second = entityA->creationNumber() < entityB->creationNumber() ? entityB : entityA;
			const uint32_t indexA = bodyIndices[first.get()];
			const uint32_t indexB = bodyIndices[second.get()];

			/* At least one active body; two sleeping ones (or a sleeping one and the static world) need no test. */
			if ( !isActive(indexA) && !isActive(indexB) )
			{
				return;
			}

			if ( !first->hasCollisionModel() || !second->hasCollisionModel() )
			{
				return;
			}

			const auto frameA = first->getWorldCoordinates();
			const auto frameB = second->getWorldCoordinates();
			const auto * modelA = first->collisionModel();
			const auto * modelB = second->collisionModel();

			if ( !boxesOverlap(modelA->getAABB(frameA), modelB->getAABB(frameB), SpeculativeContactMargin) )
			{
				return;
			}

			if ( !NarrowPhase::generate(*modelA, frameA, *modelB, frameB, SpeculativeContactMargin, contact) )
			{
				return;
			}

			/* Wake a sleeping body touched by an active one: it takes part in this step. */
			if ( isAsleep(indexA) )
			{
				bodies[indexA].dynamic = true;
				first->pauseSimulation(false);
			}

			if ( isAsleep(indexB) )
			{
				bodies[indexB].dynamic = true;
				second->pauseSimulation(false);
			}

			const auto & materialA = first->bodyPhysicalProperties();
			const auto & materialB = second->bodyPhysicalProperties();
			/* Box2D's mixing: friction = geometric mean, restitution = the larger (owner, P2). */
			const float friction = std::sqrt(materialA.stickiness() * materialB.stickiness());
			const float restitution = std::max(materialA.bounciness(), materialB.bounciness());

			appendManifold(contact, indexA, indexB, first->creationNumber(), second->creationNumber(), 0, friction, restitution, manifolds);
		};

		m_physicsOctree->forEachSector([&testPair] (const OctreeSector< AbstractEntity, true > & /*sector*/, const std::vector< std::shared_ptr< AbstractEntity > > & candidates, size_t ownedOffset) {
			for ( size_t indexA = ownedOffset; indexA < candidates.size(); ++indexA )
			{
				for ( size_t indexB = indexA + 1; indexB < candidates.size(); ++indexB )
				{
					testPair(candidates[indexA], candidates[indexB]);
				}

				for ( size_t indexB = 0; indexB < ownedOffset; ++indexB )
				{
					testPair(candidates[indexA], candidates[indexB]);
				}
			}
		});

		/* ============================================================
		 * 3. THE GROUND: its rendered triangles under every active body.
		 * ============================================================ */
		if ( m_groundLevel != nullptr )
		{
			for ( size_t index = 1; index < bodies.size(); ++index )
			{
				if ( !isActive(static_cast< uint32_t >(index)) )
				{
					continue;
				}

				const auto & entity = bodyEntities[index];

				if ( !entity->hasCollisionModel() )
				{
					continue;
				}

				const auto frame = entity->getWorldCoordinates();
				const auto * model = entity->collisionModel();
				const auto & material = entity->bodyPhysicalProperties();
				const auto bounds = model->getAABB(frame);
				const Space3D::AACuboid< float > region{bounds.maximum() + Vector< 3, float >{SpeculativeContactMargin, SpeculativeContactMargin, SpeculativeContactMargin}, bounds.minimum() - Vector< 3, float >{SpeculativeContactMargin, SpeculativeContactMargin, SpeculativeContactMargin}};

				GroundContactCollector collector{*model, frame, static_cast< uint32_t >(index), entity->creationNumber(), std::sqrt(material.stickiness() * GroundFriction), std::max(material.bounciness(), GroundRestitution), manifolds};

				static_cast< void >(m_groundLevel->visitTriangles(region, collector));
			}
		}

		/* ============================================================
		 * 4. SOLVE, in a deterministic order.
		 * ============================================================ */
		std::ranges::sort(manifolds, [] (const auto & lhs, const auto & rhs) {
			if ( lhs.keyA != rhs.keyA )
			{
				return lhs.keyA < rhs.keyA;
			}

			if ( lhs.keyB != rhs.keyB )
			{
				return lhs.keyB < rhs.keyB;
			}

			return lhs.keySub < rhs.keySub;
		});

		m_softStepSolver.step(bodies, manifolds, m_environmentPhysicalProperties.surfaceGravity(), PhysicsStepSeconds);

		/* ============================================================
		 * 5. WRITE BACK the dynamic bodies.
		 * ============================================================ */
		for ( size_t index = 1; index < bodies.size(); ++index )
		{
			auto & body = bodies[index];

			if ( body.movable == nullptr || !body.dynamic )
			{
				continue;
			}

			body.movable->setLinearVelocity(body.linearVelocity);

			if ( body.movable->isRotationPhysicsEnabled() )
			{
				body.movable->setAngularVelocity(body.angularVelocity);
			}

			bool moved = false;

			/* The solver moved the centre of mass; the node turns about its origin (rotateFromPhysics() below), so
			 * the origin also takes the swing of the offset: Δorigin = Δcom + c − ΔR · c. */
			const auto originDelta = body.deltaPosition + body.centerOffset - (body.deltaRotation * body.centerOffset);

			if ( originDelta.lengthSquared() > 0.0F )
			{
				body.movable->moveFromPhysics(originDelta);
				moved = true;
			}

			float angle = 0.0F;
			Vector< 3, float > axis;

			body.deltaRotation.toAngleAxis(angle, axis);

			if ( std::abs(angle) > std::numeric_limits< float >::epsilon() )
			{
				body.movable->rotateFromPhysics(angle, axis);
				moved = true;
			}

			if ( moved )
			{
				movedEntities.push_back(bodyEntities[index]);
			}
		}

		/* ============================================================
		 * 6. EVENTS and GROUNDED STATE, from the manifolds.
		 * ============================================================ */
		const auto down = m_environmentPhysicalProperties.surfaceGravity().normalized();

		for ( const auto & manifold : manifolds )
		{
			bool pushing = false;
			float approach = 0.0F;

			for ( const auto & point : manifold.points )
			{
				pushing = pushing || point.maxNormalImpulse > 0.0F || point.normalImpulse > 0.0F;
				approach = std::max(approach, -point.relativeVelocity);
			}

			if ( !pushing )
			{
				continue;
			}

			const bool impact = approach > ImpactSpeedThreshold;

			const float alignment = Vector< 3, float >::dotProduct(manifold.normal, down);
			auto & bodyA = bodies[manifold.bodyA];
			auto & bodyB = bodies[manifold.bodyB];

			/* The impact as the former pipeline reported it: mass × approach speed / step (NodeCollision). COLLECTED,
			 * not emitted: this step holds the physics octree lock and a handler may create or remove entities. */
			if ( bodyA.movable != nullptr && bodyA.dynamic && impact )
			{
				impacts.push_back({bodyEntities[manifold.bodyA], approach * bodyA.movable->getBodyPhysicalProperties().mass() / PhysicsStepSeconds});
			}

			if ( bodyB.movable != nullptr && bodyB.dynamic && impact )
			{
				impacts.push_back({bodyEntities[manifold.bodyB], approach * bodyB.movable->getBodyPhysicalProperties().mass() / PhysicsStepSeconds});
			}

			/* A stands on B when the normal (A → B) points down; B on A when it points up. */
			if ( bodyA.movable != nullptr && bodyA.dynamic && alignment > SupportCosine )
			{
				if ( manifold.keyB == GroundKey )
				{
					bodyA.movable->setGrounded(GroundedSource::Ground);
				}
				else
				{
					bodyA.movable->setGrounded(GroundedSource::Entity, bodyB.movable);
				}
			}

			if ( bodyB.movable != nullptr && bodyB.dynamic && alignment < -SupportCosine )
			{
				bodyB.movable->setGrounded(GroundedSource::Entity, bodyA.movable);
			}
		}

		/* ============================================================
		 * 7. THE WORLD BOUNDARIES: hard clip + bounce, after the solver (owner, P2).
		 * ============================================================ */
		for ( size_t index = 1; index < bodies.size(); ++index )
		{
			auto & body = bodies[index];

			if ( body.movable == nullptr || !body.dynamic )
			{
				continue;
			}

			const auto & entity = bodyEntities[index];
			Vector< 3, float > positionCorrection{0.0F, 0.0F, 0.0F};
			Vector< 3, float > dominantNormal{0.0F, 0.0F, 0.0F};
			float maxPenetration = 0.0F;

			this->accumulateBoundaryCorrection(entity, positionCorrection, dominantNormal, maxPenetration);

			if ( maxPenetration <= 0.0F )
			{
				continue;
			}

			/* Only the world box's FLOOR grounds (its normal points down, from the body into the wall). */
			const auto source = dominantNormal[Y] < -SupportCosine ? GroundedSource::Boundary : GroundedSource::None;

			body.movable->moveFromPhysics(positionCorrection);
			applyCollisionResponse(body.movable, dominantNormal, 0.0F, source, nullptr);

			if ( std::ranges::find(movedEntities, entity) == movedEntities.end() )
			{
				movedEntities.push_back(entity);
			}
		}
	}

	void
	Scene::clipInsideBoundaries (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		const auto position = entity->getWorldCoordinates().position();

		/* No collision model means Point behavior. */
		if ( !entity->hasCollisionModel() )
		{
			if ( position[X] > m_boundary )
			{
				entity->setXPosition(m_boundary, TransformSpace::World);
			}
			else if ( position[X] < -m_boundary )
			{
				entity->setXPosition(-m_boundary, TransformSpace::World);
			}

			if ( position[Y] > m_boundary )
			{
				entity->setYPosition(m_boundary, TransformSpace::World);
			}
			else if ( position[Y] < -m_boundary )
			{
				entity->setYPosition(-m_boundary, TransformSpace::World);
			}

			if ( position[Z] > m_boundary )
			{
				entity->setZPosition(m_boundary, TransformSpace::World);
			}
			else if ( position[Z] < -m_boundary )
			{
				entity->setZPosition(-m_boundary, TransformSpace::World);
			}

			return;
		}

		const auto * model = entity->collisionModel();
		const auto worldCoords = entity->getWorldCoordinates();

		switch ( model->modelType() )
		{
			case CollisionModelType::Point :
			{
				if ( position[X] > m_boundary )
				{
					entity->setXPosition(m_boundary, TransformSpace::World);
				}
				else if ( position[X] < -m_boundary )
				{
					entity->setXPosition(-m_boundary, TransformSpace::World);
				}

				if ( position[Y] > m_boundary )
				{
					entity->setYPosition(m_boundary, TransformSpace::World);
				}
				else if ( position[Y] < -m_boundary )
				{
					entity->setYPosition(-m_boundary, TransformSpace::World);
				}

				if ( position[Z] > m_boundary )
				{
					entity->setZPosition(m_boundary, TransformSpace::World);
				}
				else if ( position[Z] < -m_boundary )
				{
					entity->setZPosition(-m_boundary, TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;
				const auto limit = m_boundary - radius;

				if ( position[X] > limit )
				{
					entity->setXPosition(limit, TransformSpace::World);
				}
				else if ( position[X] < -limit )
				{
					entity->setXPosition(-limit, TransformSpace::World);
				}

				if ( position[Y] > limit )
				{
					entity->setYPosition(limit, TransformSpace::World);
				}
				else if ( position[Y] < -limit )
				{
					entity->setYPosition(-limit, TransformSpace::World);
				}

				if ( position[Z] > limit )
				{
					entity->setZPosition(limit, TransformSpace::World);
				}
				else if ( position[Z] < -limit )
				{
					entity->setZPosition(-limit, TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::Box :
			{
				const auto aabb = model->getAABB(worldCoords);

				if ( aabb.maximum(X) > m_boundary )
				{
					entity->moveX(m_boundary - aabb.maximum(X), TransformSpace::World);
				}
				else if ( aabb.minimum(X) < -m_boundary )
				{
					entity->moveX(-m_boundary - aabb.minimum(X), TransformSpace::World);
				}

				if ( aabb.maximum(Y) > m_boundary )
				{
					entity->moveY(m_boundary - aabb.maximum(Y), TransformSpace::World);
				}
				else if ( aabb.minimum(Y) < -m_boundary )
				{
					entity->moveY(-m_boundary - aabb.minimum(Y), TransformSpace::World);
				}

				if ( aabb.maximum(Z) > m_boundary )
				{
					entity->moveZ(m_boundary - aabb.maximum(Z), TransformSpace::World);
				}
				else if ( aabb.minimum(Z) < -m_boundary )
				{
					entity->moveZ(-m_boundary - aabb.minimum(Z), TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::Capsule :
				/* TODO: Implement Capsule boundary clipping. */
				break;
		}
	}

	void
	Scene::accumulateBoundaryCorrection (const std::shared_ptr< AbstractEntity > & entity, Vector< 3, float > & positionCorrection, Vector< 3, float > & dominantNormal, float & maxPenetration) const noexcept
	{
		/* No collision model means no boundary correction. */
		if ( !entity->hasCollisionModel() )
		{
			return;
		}

		const auto * model = entity->collisionModel();
		const auto worldCoords = entity->getWorldCoordinates();
		const auto position = worldCoords.position();

		/* Helper lambda to accumulate a single boundary collision. */
		auto accumulateCollision = [&positionCorrection, &dominantNormal, &maxPenetration] (const Vector< 3, float > & normal, float penetration) {
			/* Accumulate position correction (move opposite to normal). */
			positionCorrection -= normal * penetration;

			/* Track dominant collision for velocity bounce. */
			if ( penetration > maxPenetration )
			{
				maxPenetration = penetration;
				dominantNormal = normal;
			}
		};

		switch ( model->modelType() )
		{
			case CollisionModelType::Point :
			{
				if ( position[X] > m_boundary )
				{
					accumulateCollision({1.0F, 0.0F, 0.0F}, position[X] - m_boundary);
				}
				else if ( position[X] < -m_boundary )
				{
					accumulateCollision({-1.0F, 0.0F, 0.0F}, -m_boundary - position[X]);
				}

				if ( position[Y] > m_boundary )
				{
					accumulateCollision({0.0F, 1.0F, 0.0F}, position[Y] - m_boundary);
				}
				else if ( position[Y] < -m_boundary )
				{
					accumulateCollision({0.0F, -1.0F, 0.0F}, -m_boundary - position[Y]);
				}

				if ( position[Z] > m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, 1.0F}, position[Z] - m_boundary);
				}
				else if ( position[Z] < -m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, -1.0F}, -m_boundary - position[Z]);
				}
			}
				break;

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;

				if ( position[X] + radius > m_boundary )
				{
					accumulateCollision({1.0F, 0.0F, 0.0F}, (position[X] + radius) - m_boundary);
				}
				else if ( position[X] - radius < -m_boundary )
				{
					accumulateCollision({-1.0F, 0.0F, 0.0F}, -m_boundary - (position[X] - radius));
				}

				if ( position[Y] + radius > m_boundary )
				{
					accumulateCollision({0.0F, 1.0F, 0.0F}, (position[Y] + radius) - m_boundary);
				}
				else if ( position[Y] - radius < -m_boundary )
				{
					accumulateCollision({0.0F, -1.0F, 0.0F}, -m_boundary - (position[Y] - radius));
				}

				if ( position[Z] + radius > m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, 1.0F}, (position[Z] + radius) - m_boundary);
				}
				else if ( position[Z] - radius < -m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, -1.0F}, -m_boundary - (position[Z] - radius));
				}
			}
				break;

			case CollisionModelType::Box :
			{
				const auto aabb = model->getAABB(worldCoords);

				if ( aabb.maximum(X) > m_boundary )
				{
					accumulateCollision({1.0F, 0.0F, 0.0F}, aabb.maximum(X) - m_boundary);
				}
				else if ( aabb.minimum(X) < -m_boundary )
				{
					accumulateCollision({-1.0F, 0.0F, 0.0F}, -m_boundary - aabb.minimum(X));
				}

				if ( aabb.maximum(Y) > m_boundary )
				{
					accumulateCollision({0.0F, 1.0F, 0.0F}, aabb.maximum(Y) - m_boundary);
				}
				else if ( aabb.minimum(Y) < -m_boundary )
				{
					accumulateCollision({0.0F, -1.0F, 0.0F}, -m_boundary - aabb.minimum(Y));
				}

				if ( aabb.maximum(Z) > m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, 1.0F}, aabb.maximum(Z) - m_boundary);
				}
				else if ( aabb.minimum(Z) < -m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, -1.0F}, -m_boundary - aabb.minimum(Z));
				}
			}
				break;

			case CollisionModelType::Capsule :
				/* TODO: Implement Capsule boundary correction. */
				break;
		}
	}

}
