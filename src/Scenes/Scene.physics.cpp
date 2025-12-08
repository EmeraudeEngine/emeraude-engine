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
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Physics;

	constexpr auto TracerTag{"Scene/Physics"};

	void
	Scene::simulatePhysics () noexcept
	{
		if ( m_physicsOctree == nullptr )
		{
			return;
		}

		/* ============================================================
		 * PHASE 0: UPDATE GROUNDED STATE (Grace Period Decay)
		 * ============================================================ */

		/*for ( const auto & entity : m_physicsOctree->elements() )
		{
			if ( entity->isSimulationPaused() )
			{
				continue;
			}

			if ( auto * movableEntity = entity->getMovableTrait(); movableEntity != nullptr )
			{
				movableEntity->updateGroundedState();
			}
		}*/

		/* ============================================================
		 * PHASE 1: COLLISION DETECTION (Generate Manifolds)
		 * ============================================================ */

		/* FIXME: Poor memory management ! */
		std::vector< ContactManifold > manifolds;
		std::unordered_set< uint64_t > testedEntityPairs;

		/* 1.1 - Detect collisions: Boundaries, Ground, and Entity-Entity. */
		m_physicsOctree->forLeafSectors([this, &manifolds, &testedEntityPairs] (const OctreeSector< AbstractEntity, true > & sector) {
			this->detectCollisionCollisionInSector(sector, manifolds, testedEntityPairs);
		});

		/* ============================================================
		 * PHASE 2: COLLISION RESOLUTION (Impulse Solver)
		 * ============================================================ */

		if ( !manifolds.empty() )
		{
			m_constraintSolver.solve(manifolds, EngineUpdateCycleDurationS< float >);
		}

		/* ============================================================
		 * PHASE 3: SAFETY CLIPPING (Prevent Escapes)
		 * ============================================================ */

		/*m_physicsOctree->forLeafSectors([this] (const OctreeSector< AbstractEntity, true > & leafSector) {
			const bool sectorAtBorder = leafSector.isTouchingRootBorder();

			for ( const auto & entity : leafSector.elements() )
			{
				// Skip non-movable entities.
				if ( !entity->hasMovableAbility() )
				{
					continue;
				}

				// 3.1 - Clip inside boundaries (only for sectors at the world border).
				if ( sectorAtBorder )
				{
					this->clipInsideBoundaries(entity);
				}

				// 3.2 - Clip above ground.
				this->clipAboveGround(entity);
			}
		});*/

		/* ============================================================
		 * PHASE 4: SLEEP CHECK (Pause Idle Entities)
		 * ============================================================ */

		/*for ( const auto & entity : m_physicsOctree->elements() )
		{
			auto * movableEntity = entity->getMovableTrait();

			if ( movableEntity == nullptr )
			{
				continue;
			}

			if ( !entity->isSimulationPaused() && (!movableEntity->isMovable() || movableEntity->checkSimulationInertia()) )
			{
				entity->pauseSimulation(true);
			}
		}*/
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
	Scene::detectCollisionCollisionInSector (const OctreeSector< AbstractEntity, true > & sector, std::vector< ContactManifold > & manifolds, std::unordered_set< uint64_t > & testedEntityPairs) const noexcept
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

		const auto initialManifoldCount = manifolds.size();
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
			TraceInfo{TracerTag} <<
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

		const auto initialManifoldCount = manifolds.size();
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
			TraceInfo{TracerTag} <<
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
}
