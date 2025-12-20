/*
 * src/Scenes/Scene.physics.cpp
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

#include "Scene.hpp"

/* Local inclusions. */
#include "Component/AbstractModifier.hpp"
#include "Libs/Math/OrientedCuboid.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Physics;

	void
	Scene::simulatePhysics () noexcept
	{
		if ( m_physicsOctree == nullptr )
		{
			return;
		}

		/* ============================================================
		 * PHASE 1: STATIC COLLISIONS (Boundaries, Ground, StaticEntity)
		 * - Accumulate position corrections from ALL static collisions
		 * - Use dominant collision (deepest penetration) for velocity bounce
		 * ============================================================ */

		m_physicsOctree->forLeafSectors([this] (const OctreeSector< AbstractEntity, true > & leafSector) {
			const bool sectorAtBorder = leafSector.isTouchingRootBorder();

			for ( const auto & entity : leafSector.elements() )
			{
				/* Skip non-movable or paused entities. */
				if ( !entity->hasMovableAbility() || entity->isSimulationPaused() )
				{
					continue;
				}

				auto * movable = entity->getMovableTrait();

				if ( movable == nullptr )
				{
					continue;
				}

				/* Accumulation variables. */
				Vector< 3, float > positionCorrection{0.0F, 0.0F, 0.0F};
				Vector< 3, float > dominantNormal{0.0F, 0.0F, 0.0F};
				float maxPenetration = 0.0F;

				/* 1.1 - Boundary collisions (only for sectors at world border). */
				if ( sectorAtBorder )
				{
					this->accumulateBoundaryCorrection(entity, positionCorrection, dominantNormal, maxPenetration);
				}

				/* 1.2 - Ground collisions (track separately for grounded state). */
				Vector< 3, float > groundNormal{0.0F, 0.0F, 0.0F};
				float groundPenetration = 0.0F;
				this->accumulateGroundCorrection(entity, positionCorrection, dominantNormal, maxPenetration, groundNormal, groundPenetration);

				/* 1.3 - StaticEntity collisions. */
				this->accumulateStaticEntityCorrections(entity, leafSector, positionCorrection, dominantNormal, maxPenetration);

				/* Apply corrections if any collision occurred. */
				if ( maxPenetration > 0.0F )
				{
					/* Apply position correction (move out of collision). */
					movable->moveFromPhysics(positionCorrection);

					/* Apply velocity correction (bounce on dominant normal).
					 * NOTE: Normal points TOWARDS surface (down for ground, Y+).
					 * vn > 0 means velocity is going INTO the surface (same direction as normal). */
					auto velocity = movable->linearVelocity();
					const float vn = Vector< 3, float >::dotProduct(velocity, dominantNormal);

					if ( vn > 0.0F )
					{
						/* Reflect velocity: remove component going into surface, add bounce. */
						velocity -= dominantNormal * vn * (1.0F + movable->getBodyPhysicalProperties().bounciness());
						movable->setLinearVelocity(velocity);
					}

					/* Mark as grounded if there's a ground collision (regardless of dominant)
					 * AND the entity is not bouncing away (velocity Y should be near zero or positive). */
					/* NOTE: Temporarily disabled for physics testing.
					if ( groundPenetration > 0.0F && velocity[Y] >= -0.1F )
					{
						movable->setGrounded();
					}
					*/
				}
			}
		});

		/* ============================================================
		 * PHASE 2: DYNAMIC COLLISIONS (Node vs Node)
		 * - Detection + Impulse solver (no direct position correction)
		 * - TODO: Implement after static collisions are validated
		 * ============================================================ */

		/* NOTE: Node vs Node collisions are temporarily disabled.
		 * The impulse-based solver will handle these once static collisions are stable. */
	}

	uint64_t
	Scene::createEntityPairKey (const std::shared_ptr< AbstractEntity > & entityA, const std::shared_ptr< AbstractEntity > & entityB) noexcept
	{
		const auto ptrA = reinterpret_cast< uintptr_t >(entityA.get());
		const auto ptrB = reinterpret_cast< uintptr_t >(entityB.get());

		return ptrA < ptrB
			? (static_cast< uint64_t >(ptrA) << 32) | static_cast< uint64_t >(ptrB & 0xFFFFFFFF)
			: (static_cast< uint64_t >(ptrB) << 32) | static_cast< uint64_t >(ptrA & 0xFFFFFFFF);
	}

	void
	Scene::detectCollisionInSector (const OctreeSector< AbstractEntity, true > & sector, std::vector< ContactManifold > & manifolds, std::unordered_set< uint64_t > & testedEntityPairs) const noexcept
	{
		const bool sectorAtBorder = sector.isTouchingRootBorder();

		for ( const auto & entity : sector.elements() )
		{
			/* Skip entities that are not movable or have simulation paused. */
			if ( !entity->hasMovableAbility() || entity->isSimulationPaused() )
			{
				continue;
			}

			/* 1.1.1 - Boundary collision (only for sectors at the world border). */
			if ( sectorAtBorder )
			{
				this->detectBoundaryCollision(entity, manifolds);
			}

			/* 1.1.2 - Ground collision. */
			this->detectGroundCollision(entity, manifolds);
		}

		/* 1.1.3 - Entity-Entity collisions within this sector. */
		const auto & elements = sector.elements();

		for ( auto elementIt = elements.begin(); elementIt != elements.end(); ++elementIt )
		{
			/* NOTE: The entity A can be a node or a static entity. */
			const auto & entityA = *elementIt;
			const bool entityAHasMovableAbility = entityA->hasMovableAbility();

			/* Copy the iterator to iterate through the next elements with it without modify the initial one. */
			auto elementItCopy = elementIt;

			for ( ++elementItCopy; elementItCopy != elements.end(); ++elementItCopy )
			{
				/* NOTE: The entity B can also be a node or a static entity. */
				const auto & entityB = *elementItCopy;
				const bool entityBHasMovableAbility = entityB->hasMovableAbility();

				/* Both entities are static or both entities are paused. */
				if ( (!entityAHasMovableAbility && !entityBHasMovableAbility) || (entityA->isSimulationPaused() && entityB->isSimulationPaused()) )
				{
					continue;
				}

				/* Check for cross-sector collision duplicates using global set.
				 * O(1) lookup instead of O(n) linear search in hasCollisionWith(). */
				if ( !testedEntityPairs.insert(createEntityPairKey(entityA, entityB)).second )
				{
					/* Pair already tested in another sector, skip. */
					continue;
				}

				if ( entityAHasMovableAbility )
				{
					/* NOTE: Here the entity A is movable.
					 * We will check the collision from entity A. */
					if ( entityBHasMovableAbility )
					{
						/* Generate contact manifolds for impulse-based resolution. */
						detectCollisionMovableToMovable(*entityA, *entityB, manifolds);
					}
					else
					{
						if ( entityA->isSimulationPaused() )
						{
							continue;
						}

						detectCollisionMovableToStatic(*entityA, *entityB, manifolds);
					}
				}
				else
				{
					if ( entityB->isSimulationPaused() )
					{
						continue;
					}

					/* NOTE: Here the entity A is static, and B cannot be static.
					 * We will check the collision from entity B. */
					detectCollisionMovableToStatic(*entityB, *entityA, manifolds);
				}
			}
		}
	}

	void
	Scene::clipInsideBoundaries (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		switch ( entity->collisionDetectionModel() )
		{
			case CollisionDetectionModel::Point :
			{
				const auto position = entity->getWorldCoordinates().position();

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

			case CollisionDetectionModel::Sphere :
			{
				const auto position = entity->getWorldCoordinates().position();
				const auto radius = entity->getWorldBoundingSphere().radius();
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

			case CollisionDetectionModel::AABB :
			{
				const auto AABB = entity->getWorldBoundingBox();

				if ( AABB.maximum(X) > m_boundary )
				{
					entity->moveX(m_boundary - AABB.maximum(X), TransformSpace::World);
				}
				else if ( AABB.minimum(X) < -m_boundary )
				{
					entity->moveX(-m_boundary - AABB.minimum(X), TransformSpace::World);
				}

				if ( AABB.maximum(Y) > m_boundary )
				{
					entity->moveY(m_boundary - AABB.maximum(Y), TransformSpace::World);
				}
				else if ( AABB.minimum(Y) < -m_boundary )
				{
					entity->moveY(-m_boundary - AABB.minimum(Y), TransformSpace::World);
				}

				if ( AABB.maximum(Z) > m_boundary )
				{
					entity->moveZ(m_boundary - AABB.maximum(Z), TransformSpace::World);
				}
				else if ( AABB.minimum(Z) < -m_boundary )
				{
					entity->moveZ(-m_boundary - AABB.minimum(Z), TransformSpace::World);
				}
			}
				break;
		}
	}

	void
	Scene::clipAboveGround (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		if ( m_groundPhysics == nullptr )
		{
			/* NOTE: There is no ground in this scene. */
			return;
		}

		switch ( entity->collisionDetectionModel() )
		{
			case CollisionDetectionModel::Point :
			{
				const auto position = entity->getWorldCoordinates().position();
				const auto groundLevel = m_groundPhysics->getLevelAt(position);

				/* NOTE: Y- is up, so position[Y] must be <= groundLevel to be above ground. */
				if ( position[Y] > groundLevel )
				{
					entity->setYPosition(groundLevel, TransformSpace::World);
				}
			}
				break;

			case CollisionDetectionModel::Sphere :
			{
				const auto position = entity->getWorldCoordinates().position();
				const auto radius = entity->getWorldBoundingSphere().radius();
				const auto groundLevel = m_groundPhysics->getLevelAt(position);
				/* NOTE: Y- is up, so the lowest point of the sphere is position[Y] + radius. */
				const auto lowestPoint = position[Y] + radius;

				if ( lowestPoint > groundLevel )
				{
					entity->setYPosition(groundLevel - radius, TransformSpace::World);
				}
			}
				break;

			case CollisionDetectionModel::AABB :
			{
				const auto AABB = entity->getWorldBoundingBox();

				/* NOTE: Y- is up, so "bottom" of the box has maximum Y values.
				 * Check all four bottom corners and use the deepest penetration. */
				const std::array< Vector< 3, float >, 4 > bottomCorners{
					AABB.bottomSouthEast(),
					AABB.bottomSouthWest(),
					AABB.bottomNorthWest(),
					AABB.bottomNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : bottomCorners )
				{
					const auto groundLevel = m_groundPhysics->getLevelAt(corner);
					const auto penetration = corner[Y] - groundLevel;

					if ( penetration > deepestPenetration )
					{
						deepestPenetration = penetration;
					}
				}

				if ( deepestPenetration > 0.0F )
				{
					/* NOTE: Move up (Y-) by the penetration amount. */
					entity->moveY(-deepestPenetration, TransformSpace::World);
				}
			}
				break;
		}
	}

	void
	Scene::detectBoundaryCollision (const std::shared_ptr< AbstractEntity > & entity, std::vector< ContactManifold > & manifolds) const noexcept
	{
		auto * movable = entity->getMovableTrait();

		if ( movable == nullptr )
		{
			return;
		}

		const auto position = entity->getWorldCoordinates().position();

		switch ( entity->collisionDetectionModel() )
		{
			case CollisionDetectionModel::Point :
			{
				/* X+ boundary: entity beyond +X wall, normal points from entity towards wall (X+). */
				if ( position[X] > m_boundary )
				{
					ContactManifold manifold(movable);
					manifold.addContact({m_boundary, position[Y], position[Z]}, {1.0F, 0.0F, 0.0F}, position[X] - m_boundary);
					manifolds.push_back(manifold);
				}
				/* X- boundary: entity beyond -X wall, normal points from entity towards wall (X-). */
				else if ( position[X] < -m_boundary )
				{
					ContactManifold manifold(movable);
					manifold.addContact({-m_boundary, position[Y], position[Z]}, {-1.0F, 0.0F, 0.0F}, -m_boundary - position[X]);
					manifolds.push_back(manifold);
				}

				/* Y+ boundary: entity beyond +Y wall, normal points from entity towards wall (Y+). */
				if ( position[Y] > m_boundary )
				{
					ContactManifold manifold(movable);
					manifold.addContact({position[X], m_boundary, position[Z]}, {0.0F, 1.0F, 0.0F}, position[Y] - m_boundary);
					manifolds.push_back(manifold);
				}
				/* Y- boundary: entity beyond -Y wall, normal points from entity towards wall (Y-). */
				else if ( position[Y] < -m_boundary )
				{
					ContactManifold manifold(movable);
					manifold.addContact({position[X], -m_boundary, position[Z]}, {0.0F, -1.0F, 0.0F}, -m_boundary - position[Y]);
					manifolds.push_back(manifold);
				}

				/* Z+ boundary: entity beyond +Z wall, normal points from entity towards wall (Z+). */
				if ( position[Z] > m_boundary )
				{
					ContactManifold manifold(movable);
					manifold.addContact({position[X], position[Y], m_boundary}, {0.0F, 0.0F, 1.0F}, position[Z] - m_boundary);
					manifolds.push_back(manifold);
				}
				/* Z- boundary: entity beyond -Z wall, normal points from entity towards wall (Z-). */
				else if ( position[Z] < -m_boundary )
				{
					ContactManifold manifold(movable);
					manifold.addContact({position[X], position[Y], -m_boundary}, {0.0F, 0.0F, -1.0F}, -m_boundary - position[Z]);
					manifolds.push_back(manifold);
				}
			}
				break;

			case CollisionDetectionModel::Sphere :
			{
				const auto radius = entity->getWorldBoundingSphere().radius();

				/* X+ boundary: normal points from entity towards wall (X+). */
				if ( position[X] + radius > m_boundary )
				{
					const auto penetration = (position[X] + radius) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({m_boundary, position[Y], position[Z]}, {1.0F, 0.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* X- boundary: normal points from entity towards wall (X-). */
				else if ( position[X] - radius < -m_boundary )
				{
					const auto penetration = -m_boundary - (position[X] - radius);
					ContactManifold manifold(movable);
					manifold.addContact({-m_boundary, position[Y], position[Z]}, {-1.0F, 0.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}

				/* Y+ boundary: normal points from entity towards wall (Y+). */
				if ( position[Y] + radius > m_boundary )
				{
					const auto penetration = (position[Y] + radius) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({position[X], m_boundary, position[Z]}, {0.0F, 1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* Y- boundary: normal points from entity towards wall (Y-). */
				else if ( position[Y] - radius < -m_boundary )
				{
					const auto penetration = -m_boundary - (position[Y] - radius);
					ContactManifold manifold(movable);
					manifold.addContact({position[X], -m_boundary, position[Z]}, {0.0F, -1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}

				/* Z+ boundary: normal points from entity towards wall (Z+). */
				if ( position[Z] + radius > m_boundary )
				{
					const auto penetration = (position[Z] + radius) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({position[X], position[Y], m_boundary}, {0.0F, 0.0F, 1.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* Z- boundary: normal points from entity towards wall (Z-). */
				else if ( position[Z] - radius < -m_boundary )
				{
					const auto penetration = -m_boundary - (position[Z] - radius);
					ContactManifold manifold(movable);
					manifold.addContact({position[X], position[Y], -m_boundary}, {0.0F, 0.0F, -1.0F}, penetration);
					manifolds.push_back(manifold);
				}
			}
				break;

			case CollisionDetectionModel::AABB :
			{
				const auto AABB = entity->getWorldBoundingBox();

				/* X+ boundary: normal points from entity towards wall (X+). */
				if ( AABB.maximum(X) > m_boundary )
				{
					const auto penetration = AABB.maximum(X) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({m_boundary, position[Y], position[Z]}, {1.0F, 0.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* X- boundary: normal points from entity towards wall (X-). */
				else if ( AABB.minimum(X) < -m_boundary )
				{
					const auto penetration = -m_boundary - AABB.minimum(X);
					ContactManifold manifold(movable);
					manifold.addContact({-m_boundary, position[Y], position[Z]}, {-1.0F, 0.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}

				/* Y+ boundary: normal points from entity towards wall (Y+). */
				if ( AABB.maximum(Y) > m_boundary )
				{
					const auto penetration = AABB.maximum(Y) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({position[X], m_boundary, position[Z]}, {0.0F, 1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* Y- boundary: normal points from entity towards wall (Y-). */
				else if ( AABB.minimum(Y) < -m_boundary )
				{
					const auto penetration = -m_boundary - AABB.minimum(Y);
					ContactManifold manifold(movable);
					manifold.addContact({position[X], -m_boundary, position[Z]}, {0.0F, -1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}

				/* Z+ boundary: normal points from entity towards wall (Z+). */
				if ( AABB.maximum(Z) > m_boundary )
				{
					const auto penetration = AABB.maximum(Z) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({position[X], position[Y], m_boundary}, {0.0F, 0.0F, 1.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* Z- boundary: normal points from entity towards wall (Z-). */
				else if ( AABB.minimum(Z) < -m_boundary )
				{
					const auto penetration = -m_boundary - AABB.minimum(Z);
					ContactManifold manifold(movable);
					manifold.addContact({position[X], position[Y], -m_boundary}, {0.0F, 0.0F, -1.0F}, penetration);
					manifolds.push_back(manifold);
				}
			}
				break;
		}

		/*const auto newCollisions = manifolds.size() - initialManifoldCount;

		if ( newCollisions > 0 )
		{
			TraceInfo{ClassId} <<
				"Boundary collision detected: entity='" << entity->name() << "' "
				"pos=(" << position[X] << ", " << position[Y] << ", " << position[Z] << ") "
				"contacts=" << newCollisions;
		}*/
	}

	void
	Scene::detectGroundCollision (const std::shared_ptr< AbstractEntity > & entity, std::vector< ContactManifold > & manifolds) const noexcept
	{
		if ( m_groundPhysics == nullptr )
		{
			return;
		}

		auto * movable = entity->getMovableTrait();

		if ( movable == nullptr )
		{
			return;
		}

		const auto position = entity->getWorldCoordinates().position();

		switch ( entity->collisionDetectionModel() )
		{
			case CollisionDetectionModel::Point :
			{
				const auto groundLevel = m_groundPhysics->getLevelAt(position);

				/* NOTE: Y- is up, so position[Y] > groundLevel means below ground. */
				if ( position[Y] > groundLevel )
				{
					const auto penetration = position[Y] - groundLevel;
					ContactManifold manifold(movable);
					/* Normal points from bodyA (entity) towards bodyB (ground/Y+). */
					manifold.addContact({position[X], groundLevel, position[Z]}, {0.0F, 1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
			}
				break;

			case CollisionDetectionModel::Sphere :
			{
				const auto radius = entity->getWorldBoundingSphere().radius();
				const auto groundLevel = m_groundPhysics->getLevelAt(position);
				/* NOTE: Y- is up, so the lowest point of the sphere is position[Y] + radius. */
				const auto lowestPoint = position[Y] + radius;

				if ( lowestPoint > groundLevel )
				{
					const auto penetration = lowestPoint - groundLevel;
					ContactManifold manifold(movable);
					/* Normal points from bodyA (entity) towards bodyB (ground/Y+). */
					manifold.addContact({position[X], groundLevel, position[Z]}, {0.0F, 1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
			}
				break;

			case CollisionDetectionModel::AABB :
			{
				const auto AABB = entity->getWorldBoundingBox();

				/* NOTE: Y- is up, so "bottom" of the box has maximum Y values.
				 * Check all four bottom corners and use the deepest penetration. */
				const std::array< Vector< 3, float >, 4 > bottomCorners{
					AABB.bottomSouthEast(),
					AABB.bottomSouthWest(),
					AABB.bottomNorthWest(),
					AABB.bottomNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : bottomCorners )
				{
					const auto groundLevel = m_groundPhysics->getLevelAt(corner);
					const auto penetration = corner[Y] - groundLevel;

					if ( penetration > deepestPenetration )
					{
						deepestPenetration = penetration;
					}
				}

				if ( deepestPenetration > 0.0F )
				{
					const auto groundLevel = m_groundPhysics->getLevelAt(position);
					ContactManifold manifold(movable);
					/* Normal points from bodyA (entity) towards bodyB (ground/Y+). */
					manifold.addContact({position[X], groundLevel, position[Z]}, {0.0F, 1.0F, 0.0F}, deepestPenetration);
					manifolds.push_back(manifold);
				}
			}
				break;
		}

		/*const auto newCollisions = manifolds.size() - initialManifoldCount;

		if ( newCollisions > 0 )
		{
			TraceInfo{ClassId} <<
				"Ground collision detected: entity='" << entity->name() << "' "
				"pos=(" << position[X] << ", " << position[Y] << ", " << position[Z] << ") "
				"contacts=" << newCollisions;
		}*/
	}

	void
	Scene::applyModifiers (Node & node) const noexcept
	{
		this->forEachModifiers([&node] (const auto & modifier) {
			/* NOTE: Avoid working on the same Node. */
			if ( &node == &modifier.parentEntity() )
			{
				return;
			}

			/* FIXME: Use AABB when usable */
			const auto modifierForce = modifier.getForceAppliedToEntity(node.getWorldCoordinates(), node.getWorldBoundingSphere());

			node.addForce(modifierForce);
		});
	}

	void
	Scene::accumulateBoundaryCorrection (const std::shared_ptr< AbstractEntity > & entity, Vector< 3, float > & positionCorrection, Vector< 3, float > & dominantNormal, float & maxPenetration) const noexcept
	{
		const auto position = entity->getWorldCoordinates().position();

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

		switch ( entity->collisionDetectionModel() )
		{
			case CollisionDetectionModel::Point :
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

			case CollisionDetectionModel::Sphere :
			{
				const auto radius = entity->getWorldBoundingSphere().radius();

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

			case CollisionDetectionModel::AABB :
			{
				const auto AABB = entity->getWorldBoundingBox();

				if ( AABB.maximum(X) > m_boundary )
				{
					accumulateCollision({1.0F, 0.0F, 0.0F}, AABB.maximum(X) - m_boundary);
				}
				else if ( AABB.minimum(X) < -m_boundary )
				{
					accumulateCollision({-1.0F, 0.0F, 0.0F}, -m_boundary - AABB.minimum(X));
				}

				if ( AABB.maximum(Y) > m_boundary )
				{
					accumulateCollision({0.0F, 1.0F, 0.0F}, AABB.maximum(Y) - m_boundary);
				}
				else if ( AABB.minimum(Y) < -m_boundary )
				{
					accumulateCollision({0.0F, -1.0F, 0.0F}, -m_boundary - AABB.minimum(Y));
				}

				if ( AABB.maximum(Z) > m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, 1.0F}, AABB.maximum(Z) - m_boundary);
				}
				else if ( AABB.minimum(Z) < -m_boundary )
				{
					accumulateCollision({0.0F, 0.0F, -1.0F}, -m_boundary - AABB.minimum(Z));
				}
			}
				break;
		}
	}

	void
	Scene::accumulateGroundCorrection (const std::shared_ptr< AbstractEntity > & entity, Vector< 3, float > & positionCorrection, Vector< 3, float > & dominantNormal, float & maxPenetration, Vector< 3, float > & groundNormal, float & groundPenetration) const noexcept
	{
		if ( m_groundPhysics == nullptr )
		{
			return;
		}

		const auto position = entity->getWorldCoordinates().position();

		/* Helper lambda to accumulate ground collision.
		 * Gets the actual terrain normal at the contact position. */
		auto accumulateCollision = [this, &positionCorrection, &dominantNormal, &maxPenetration, &groundNormal, &groundPenetration] (const Vector< 3, float > & contactPosition, float penetration) {
			/* Get actual terrain normal at this position.
			 * getNormalAt() returns normal pointing UP (away from ground, Y-).
			 * We negate it to get normal pointing INTO ground (Y+) for consistent bounce math. */
			const auto normal = -m_groundPhysics->getNormalAt(contactPosition);

			/* Accumulate position correction (move opposite to normal = up). */
			positionCorrection -= normal * penetration;

			/* Track ground-specific collision for grounded state. */
			groundNormal = normal;
			groundPenetration = penetration;

			/* Track dominant collision for velocity bounce. */
			if ( penetration > maxPenetration )
			{
				maxPenetration = penetration;
				dominantNormal = normal;
			}
		};

		switch ( entity->collisionDetectionModel() )
		{
			case CollisionDetectionModel::Point :
			{
				const auto groundLevel = m_groundPhysics->getLevelAt(position);

				/* NOTE: Y- is up, so position[Y] > groundLevel means below ground. */
				if ( position[Y] > groundLevel )
				{
					accumulateCollision(position, position[Y] - groundLevel);
				}
			}
				break;

			case CollisionDetectionModel::Sphere :
			{
				const auto radius = entity->getWorldBoundingSphere().radius();
				const auto groundLevel = m_groundPhysics->getLevelAt(position);
				/* NOTE: Y- is up, so the lowest point of the sphere is position[Y] + radius. */
				const auto lowestPoint = position[Y] + radius;

				if ( lowestPoint > groundLevel )
				{
					accumulateCollision(position, lowestPoint - groundLevel);
				}
			}
				break;

			case CollisionDetectionModel::AABB :
			{
				const auto AABB = entity->getWorldBoundingBox();

				/* NOTE: Y- is up, so "bottom" of the box has maximum Y values.
				 * Check all four bottom corners and use the deepest penetration. */
				const std::array< Vector< 3, float >, 4 > bottomCorners{
					AABB.bottomSouthEast(),
					AABB.bottomSouthWest(),
					AABB.bottomNorthWest(),
					AABB.bottomNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : bottomCorners )
				{
					const auto groundLevel = m_groundPhysics->getLevelAt(corner);
					const auto penetration = corner[Y] - groundLevel;

					if ( penetration > deepestPenetration )
					{
						deepestPenetration = penetration;
					}
				}

				if ( deepestPenetration > 0.0F )
				{
					accumulateCollision(position, deepestPenetration);
				}
			}
				break;
		}
	}

	void
	Scene::accumulateStaticEntityCorrections (const std::shared_ptr< AbstractEntity > & entity, const OctreeSector< AbstractEntity, true > & sector, Vector< 3, float > & positionCorrection, Vector< 3, float > & dominantNormal, float & maxPenetration) const noexcept
	{
		/* Iterate through all entities in this sector looking for static entities. */
		for ( const auto & otherEntity : sector.elements() )
		{
			/* Skip self. */
			if ( entity.get() == otherEntity.get() )
			{
				continue;
			}

			/* Skip if the other entity is movable (we only want static entities here). */
			if ( otherEntity->hasMovableAbility() )
			{
				continue;
			}

			/* Collision detection based on both entities' collision models.
			 * Static entities with Point model are ignored (no volume). */
			Vector< 3, float > mtv{0.0F, 0.0F, 0.0F};
			bool collisionDetected = false;

			switch ( otherEntity->collisionDetectionModel() )
			{
				case CollisionDetectionModel::Point :
					/* Static entities with no volume are skipped. */
					continue;

				case CollisionDetectionModel::Sphere :
				{
					const auto staticSphere = otherEntity->getWorldBoundingSphere();

					switch ( entity->collisionDetectionModel() )
					{
						case CollisionDetectionModel::Point :
						{
							const auto position = entity->getWorldCoordinates().position();

							collisionDetected = Space3D::isColliding(position, staticSphere, mtv);
						}
							break;

						case CollisionDetectionModel::Sphere :
						{
							const auto entitySphere = entity->getWorldBoundingSphere();

							collisionDetected = Space3D::isColliding(entitySphere, staticSphere, mtv);
						}
							break;

						case CollisionDetectionModel::AABB :
						{
							const auto entityAABB = entity->getWorldBoundingBox();

							collisionDetected = Space3D::isColliding(entityAABB, staticSphere, mtv);
						}
							break;
					}
				}
					break;

				case CollisionDetectionModel::AABB :
				{
					const auto staticAABB = otherEntity->getWorldBoundingBox();

					switch ( entity->collisionDetectionModel() )
					{
						case CollisionDetectionModel::Point :
						{
							const auto position = entity->getWorldCoordinates().position();

							collisionDetected = Space3D::isColliding(position, staticAABB, mtv);
						}
							break;

						case CollisionDetectionModel::Sphere :
						{
							const auto entitySphere = entity->getWorldBoundingSphere();

							collisionDetected = Space3D::isColliding(entitySphere, staticAABB, mtv);
						}
							break;

						case CollisionDetectionModel::AABB :
						{
							const auto entityAABB = entity->getWorldBoundingBox();

							/* NOTE: Broad phase: Quick AABB overlap test. */
							if ( Space3D::isColliding(entityAABB, staticAABB) )
							{
								/* NOTE: Narrow phase: Precise OBB collision using SAT algorithm.
								 * This handles rotated objects correctly by testing all 15 separating axes. */
								const auto entityOBB = OrientedCuboid< float >{entity->localBoundingBox(), entity->getWorldCoordinates()};
								const auto staticOBB = OrientedCuboid< float >{otherEntity->localBoundingBox(), otherEntity->getWorldCoordinates()};

								Vector< 3, float > direction;
								const auto penetration = OrientedCuboid< float >::isIntersecting(entityOBB, staticOBB, direction);

								if ( penetration > 0.0F )
								{
									collisionDetected = true;
									mtv = direction * penetration;
								}
							}
						}
							break;
					}
				}
					break;
			}

			if ( collisionDetected )
			{
				const auto penetration = mtv.length();

				if ( penetration > 0.0F )
				{
					/* MTV points in the direction to move the entity OUT of collision. */
					positionCorrection += mtv;

					/* Track dominant collision for velocity bounce. */
					if ( penetration > maxPenetration )
					{
						maxPenetration = penetration;
						/* Normal points INTO the static entity (for bounce calculation). */
						dominantNormal = -mtv.normalize();
					}
				}
			}
		}
	}
}
