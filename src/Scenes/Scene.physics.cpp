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
 * https://github.com/londnoir/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "Scene.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Physics;

	/**
	 * @brief Applies complete collision response: velocity bounce + grounded state.
	 * @param movable The movable trait to update.
	 * @param surfaceNormal The dominant collision surface normal.
	 * @param groundPenetration The ground penetration depth (0 if no direct ground collision).
	 * @param dominantSource The source of the dominant collision (Ground, Boundary, or Entity).
	 * @param groundedOnEntity Pointer to the entity we collided with (if source is Entity).
	 */
	static void
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
		 * - Normal points downward (Y > 0.7 in Y-down = surface faces up) */
		constexpr auto GroundNormalThreshold{0.7F}; /* ~45 degrees */
		const bool isOnSurface = (groundPenetration > 0.0F) || (surfaceNormal[Y] > GroundNormalThreshold);

		/* Only apply grounded response if not bouncing away (velocity Y near zero or positive). */
		if ( isOnSurface && velocity[Y] >= -0.1F )
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

	void
	Scene::simulatePhysics () const noexcept
	{
		if ( m_physicsOctree == nullptr )
		{
			return;
		}

		/* Lock the physics octree for the duration of the simulation to prevent
		 * concurrent modifications from other threads (e.g., checkEntityLocationInOctrees). */
		const std::lock_guard< std::mutex > lock{m_physicsOctreeAccess};

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

				if ( movable == nullptr || !movable->isMovable() )
				{
					continue;
				}

				/* Accumulation variables. */
				Vector< 3, float > positionCorrection{0.0F, 0.0F, 0.0F};
				Vector< 3, float > dominantNormal{0.0F, 0.0F, 0.0F};
				float maxPenetration = 0.0F;
				GroundedSource dominantSource{GroundedSource::None};
				const MovableTrait * dominantEntity{nullptr};

				/* 1.1 - Boundary collisions (only for sectors at world border). */
				if ( sectorAtBorder )
				{
					const float prevMax = maxPenetration;
					this->accumulateBoundaryCorrection(entity, positionCorrection, dominantNormal, maxPenetration);

					if ( maxPenetration > prevMax )
					{
						/* Only mark as grounded on Boundary if it's the floor (bottom face).
						 * Side walls and ceiling cannot ground an entity.
						 * In Y-down, floor normal points in +Y direction (downward). */
						constexpr auto GroundNormalThreshold{0.7F};

						if ( dominantNormal[Y] > GroundNormalThreshold )
						{
							dominantSource = GroundedSource::Boundary;
						}
						dominantEntity = nullptr;
					}
				}

				/* 1.2 - Ground collisions (track separately for grounded state). */
				Vector< 3, float > groundNormal{0.0F, 0.0F, 0.0F};
				float groundPenetration = 0.0F;
				{
					const float prevMax = maxPenetration;
					this->accumulateGroundCorrection(entity, positionCorrection, dominantNormal, maxPenetration, groundNormal, groundPenetration);

					if ( maxPenetration > prevMax )
					{
						dominantSource = GroundedSource::Ground;
						dominantEntity = nullptr;
					}
				}

				/* 1.3 - StaticEntity collisions. */
				{
					const float prevMax = maxPenetration;
					const MovableTrait * collidedEntity = nullptr;
					this->accumulateStaticEntityCorrections(entity, leafSector, positionCorrection, dominantNormal, maxPenetration, collidedEntity);

					if ( maxPenetration > prevMax )
					{
						/* Only mark as grounded on Entity if standing on top of it.
						 * Hitting the side of a wall doesn't ground you.
						 * In Y-down, floor-like normal points in +Y direction. */
						constexpr auto GroundNormalThreshold{0.7F};

						if ( dominantNormal[Y] > GroundNormalThreshold )
						{
							dominantSource = GroundedSource::Entity;
							dominantEntity = collidedEntity;
						}
					}
				}

				/* Apply corrections if any collision occurred. */
				if ( maxPenetration > 0.0F )
				{
					/* Compute impact force from velocity component along collision normal.
					 * This is done BEFORE applyCollisionResponse modifies velocity.
					 * momentum = mass × velocity (N·s), then convert to force (N) by dividing by Δt.
					 * F = (m × Δv) / Δt */
					const float impactVelocity = Vector< 3, float >::dotProduct(movable->linearVelocity(), dominantNormal);
					const float impactForce = std::max(0.0F, impactVelocity) * movable->getBodyPhysicalProperties().mass() / EngineUpdateCycleDurationS< float >;

					/* Apply position correction (move out of collision). */
					movable->moveFromPhysics(positionCorrection);

					/* Apply velocity bounce + grounded response. */
					applyCollisionResponse(movable, dominantNormal, groundPenetration, dominantSource, dominantEntity);

					/* Notify entity of collision event. */
					if ( impactForce > 0.0F )
					{
						movable->onCollision(impactForce);
					}
				}
			}
		});

		/* ============================================================
		 * PHASE 2: DYNAMIC COLLISIONS (Node vs Node)
		 * - Detection via collision models
		 * - Resolution via Sequential Impulse Solver
		 * ============================================================ */

		std::vector< ContactManifold > dynamicManifolds;
		std::unordered_set< uint64_t > testedEntityPairs;
		std::vector< std::shared_ptr< AbstractEntity > > involvedEntities;

		m_physicsOctree->forLeafSectors([&dynamicManifolds, &testedEntityPairs, &involvedEntities] (const OctreeSector< AbstractEntity, true > & leafSector) {
			const auto & elements = leafSector.elements();

			for ( auto elementIt = elements.begin(); elementIt != elements.end(); ++elementIt )
			{
				const auto & entityA = *elementIt;

				/* Skip non-movable or paused entities. */
				if ( !entityA->hasMovableAbility() || entityA->isSimulationPaused() )
				{
					continue;
				}

				auto elementItCopy = elementIt;

				for ( ++elementItCopy; elementItCopy != elements.end(); ++elementItCopy )
				{
					const auto & entityB = *elementItCopy;

					/* Skip non-movable or paused entities. */
					if ( !entityB->hasMovableAbility() || entityB->isSimulationPaused() )
					{
						continue;
					}

					/* Avoid duplicate pair testing across sectors. */
					if ( !testedEntityPairs.insert(createEntityPairKey(entityA, entityB)).second )
					{
						continue;
					}

					/* Detect and collect collision manifold. */
					if ( detectCollisionMovableToMovable(*entityA, *entityB, dynamicManifolds) )
					{
						involvedEntities.push_back(entityA);
						involvedEntities.push_back(entityB);
					}
				}
			}
		});

		/* Resolve dynamic collisions via impulse solver, then enforce boundaries. */
		if ( !dynamicManifolds.empty() )
		{
			m_constraintSolver.solve(dynamicManifolds, EngineUpdateCycleDurationS< float >);

			/* Immediately clip all involved entities to boundaries.
			 * This ensures impulse resolution cannot push entities outside. */
			for ( const auto & entity : involvedEntities )
			{
				this->clipInsideBoundaries(entity);
			}
		}
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

			case CollisionModelType::AABB :
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
	Scene::clipAboveGround (const std::shared_ptr< AbstractEntity > & entity) const noexcept
	{
		if ( m_groundLevel == nullptr )
		{
			/* NOTE: There is no ground in this scene. */
			return;
		}

		const auto position = entity->getWorldCoordinates().position();

		/* No collision model means Point behavior. */
		if ( !entity->hasCollisionModel() )
		{
			const auto groundLevel = m_groundLevel->getLevelAt(position);

			/* NOTE: Y- is up, so position[Y] must be <= groundLevel to be above ground. */
			if ( position[Y] > groundLevel )
			{
				entity->setYPosition(groundLevel, TransformSpace::World);
			}

			return;
		}

		const auto * model = entity->collisionModel();
		const auto worldCoords = entity->getWorldCoordinates();

		switch ( model->modelType() )
		{
			case CollisionModelType::Point :
			{
				const auto groundLevel = m_groundLevel->getLevelAt(position);

				if ( position[Y] > groundLevel )
				{
					entity->setYPosition(groundLevel, TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;
				const auto groundLevel = m_groundLevel->getLevelAt(position);
				/* NOTE: Y- is up, so the lowest point of the sphere is position[Y] + radius. */
				const auto lowestPoint = position[Y] + radius;

				if ( lowestPoint > groundLevel )
				{
					entity->setYPosition(groundLevel - radius, TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::AABB :
			{
				const auto aabb = model->getAABB(worldCoords);

				/* NOTE: Y- is up, so "bottom" of the box has maximum Y values.
				 * Check all four bottom corners and use the deepest penetration. */
				const std::array< Vector< 3, float >, 4 > bottomCorners{
					aabb.bottomSouthEast(),
					aabb.bottomSouthWest(),
					aabb.bottomNorthWest(),
					aabb.bottomNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : bottomCorners )
				{
					const auto groundLevel = m_groundLevel->getLevelAt(corner);
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

			case CollisionModelType::Capsule :
				/* TODO: Implement Capsule ground clipping. */
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

		/* No collision model means no collision simulation. */
		if ( !entity->hasCollisionModel() )
		{
			return;
		}

		const auto * model = entity->collisionModel();
		const auto worldCoords = entity->getWorldCoordinates();
		const auto position = worldCoords.position();

		switch ( model->modelType() )
		{
			case CollisionModelType::Point :
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

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;

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

			case CollisionModelType::AABB :
			{
				const auto aabb = model->getAABB(worldCoords);

				/* X+ boundary: normal points from entity towards wall (X+). */
				if ( aabb.maximum(X) > m_boundary )
				{
					const auto penetration = aabb.maximum(X) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({m_boundary, position[Y], position[Z]}, {1.0F, 0.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* X- boundary: normal points from entity towards wall (X-). */
				else if ( aabb.minimum(X) < -m_boundary )
				{
					const auto penetration = -m_boundary - aabb.minimum(X);
					ContactManifold manifold(movable);
					manifold.addContact({-m_boundary, position[Y], position[Z]}, {-1.0F, 0.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}

				/* Y+ boundary: normal points from entity towards wall (Y+). */
				if ( aabb.maximum(Y) > m_boundary )
				{
					const auto penetration = aabb.maximum(Y) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({position[X], m_boundary, position[Z]}, {0.0F, 1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* Y- boundary: normal points from entity towards wall (Y-). */
				else if ( aabb.minimum(Y) < -m_boundary )
				{
					const auto penetration = -m_boundary - aabb.minimum(Y);
					ContactManifold manifold(movable);
					manifold.addContact({position[X], -m_boundary, position[Z]}, {0.0F, -1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}

				/* Z+ boundary: normal points from entity towards wall (Z+). */
				if ( aabb.maximum(Z) > m_boundary )
				{
					const auto penetration = aabb.maximum(Z) - m_boundary;
					ContactManifold manifold(movable);
					manifold.addContact({position[X], position[Y], m_boundary}, {0.0F, 0.0F, 1.0F}, penetration);
					manifolds.push_back(manifold);
				}
				/* Z- boundary: normal points from entity towards wall (Z-). */
				else if ( aabb.minimum(Z) < -m_boundary )
				{
					const auto penetration = -m_boundary - aabb.minimum(Z);
					ContactManifold manifold(movable);
					manifold.addContact({position[X], position[Y], -m_boundary}, {0.0F, 0.0F, -1.0F}, penetration);
					manifolds.push_back(manifold);
				}
			}
				break;

			case CollisionModelType::Capsule :
				/* TODO: Implement Capsule boundary collision. */
				break;
		}
	}

	void
	Scene::detectGroundCollision (const std::shared_ptr< AbstractEntity > & entity, std::vector< ContactManifold > & manifolds) const noexcept
	{
		if ( m_groundLevel == nullptr )
		{
			return;
		}

		auto * movable = entity->getMovableTrait();

		if ( movable == nullptr )
		{
			return;
		}

		/* No collision model means no collision simulation. */
		if ( !entity->hasCollisionModel() )
		{
			return;
		}

		const auto * model = entity->collisionModel();
		const auto worldCoords = entity->getWorldCoordinates();
		const auto position = worldCoords.position();

		switch ( model->modelType() )
		{
			case CollisionModelType::Point :
			{
				const auto groundLevel = m_groundLevel->getLevelAt(position);

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

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;
				const auto groundLevel = m_groundLevel->getLevelAt(position);
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

			case CollisionModelType::AABB :
			{
				const auto aabb = model->getAABB(worldCoords);

				/* NOTE: Y- is up, so "bottom" of the box has maximum Y values.
				 * Check all four bottom corners and use the deepest penetration. */
				const std::array< Vector< 3, float >, 4 > bottomCorners{
					aabb.bottomSouthEast(),
					aabb.bottomSouthWest(),
					aabb.bottomNorthWest(),
					aabb.bottomNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : bottomCorners )
				{
					const auto groundLevel = m_groundLevel->getLevelAt(corner);
					const auto penetration = corner[Y] - groundLevel;

					if ( penetration > deepestPenetration )
					{
						deepestPenetration = penetration;
					}
				}

				if ( deepestPenetration > 0.0F )
				{
					const auto groundLevel = m_groundLevel->getLevelAt(position);
					ContactManifold manifold(movable);
					/* Normal points from bodyA (entity) towards bodyB (ground/Y+). */
					manifold.addContact({position[X], groundLevel, position[Z]}, {0.0F, 1.0F, 0.0F}, deepestPenetration);
					manifolds.push_back(manifold);
				}
			}
				break;

			case CollisionModelType::Capsule :
				/* TODO: Implement Capsule ground collision. */
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

			case CollisionModelType::AABB :
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

	void
	Scene::accumulateGroundCorrection (const std::shared_ptr< AbstractEntity > & entity, Vector< 3, float > & positionCorrection, Vector< 3, float > & dominantNormal, float & maxPenetration, Vector< 3, float > & groundNormal, float & groundPenetration) const noexcept
	{
		if ( m_groundLevel == nullptr )
		{
			return;
		}

		/* No collision model means no ground correction. */
		if ( !entity->hasCollisionModel() )
		{
			return;
		}

		const auto * model = entity->collisionModel();
		const auto worldCoords = entity->getWorldCoordinates();
		const auto position = worldCoords.position();

		/* Helper lambda to accumulate ground collision.
		 * Gets the actual terrain normal at the contact position. */
		auto accumulateCollision = [this, &positionCorrection, &dominantNormal, &maxPenetration, &groundNormal, &groundPenetration] (const Vector< 3, float > & contactPosition, float penetration) {
			/* Get actual terrain normal at this position.
			 * getNormalAt() returns normal pointing UP (away from ground, Y-).
			 * We negate it to get normal pointing INTO ground (Y+) for consistent bounce math. */
			const auto normal = -m_groundLevel->getNormalAt(contactPosition);

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

		switch ( model->modelType() )
		{
			case CollisionModelType::Point :
			{
				const auto groundLevel = m_groundLevel->getLevelAt(position);

				/* NOTE: Y- is up, so position[Y] > groundLevel means below ground. */
				if ( position[Y] > groundLevel )
				{
					accumulateCollision(position, position[Y] - groundLevel);
				}
			}
				break;

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;
				const auto groundLevel = m_groundLevel->getLevelAt(position);
				/* NOTE: Y- is up, so the lowest point of the sphere is position[Y] + radius. */
				const auto lowestPoint = position[Y] + radius;

				if ( lowestPoint > groundLevel )
				{
					accumulateCollision(position, lowestPoint - groundLevel);
				}
			}
				break;

			case CollisionModelType::AABB :
			{
				const auto aabb = model->getAABB(worldCoords);

				/* NOTE: Y- is up, so "bottom" of the box has maximum Y values.
				 * Check all four bottom corners and use the deepest penetration. */
				const std::array< Vector< 3, float >, 4 > bottomCorners{
					aabb.bottomSouthEast(),
					aabb.bottomSouthWest(),
					aabb.bottomNorthWest(),
					aabb.bottomNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : bottomCorners )
				{
					const auto groundLevel = m_groundLevel->getLevelAt(corner);
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

			case CollisionModelType::Capsule :
				/* TODO: Implement Capsule ground correction. */
				break;
		}
	}

	void
	Scene::accumulateStaticEntityCorrections (const std::shared_ptr< AbstractEntity > & entity, const OctreeSector< AbstractEntity, true > & sector, Vector< 3, float > & positionCorrection, Vector< 3, float > & dominantNormal, float & maxPenetration, const MovableTrait *& collidedEntity) const noexcept
	{
		/* No collision model means no collision simulation. */
		if ( !entity->hasCollisionModel() )
		{
			return;
		}

		const auto * entityModel = entity->collisionModel();
		const auto entityWorldCoords = entity->getWorldCoordinates();

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

			/* Skip if the other entity has no collision model. */
			if ( !otherEntity->hasCollisionModel() )
			{
				continue;
			}

			const auto * otherModel = otherEntity->collisionModel();

			/* Static entities with Point model are ignored (no volume). */
			if ( otherModel->modelType() == CollisionModelType::Point )
			{
				continue;
			}

			const auto otherWorldCoords = otherEntity->getWorldCoordinates();

			/* Use the collision model interface for collision detection.
			 * This handles all combinations through double dispatch. */
			const auto results = entityModel->isCollidingWith(entityWorldCoords, *otherModel, otherWorldCoords);

			if ( results.m_collisionDetected && results.m_depth > 0.0F )
			{
				/* MTV points in the direction to move the entity OUT of collision. */
				positionCorrection += results.m_MTV;

				/* Track dominant collision for velocity bounce. */
				if ( results.m_depth > maxPenetration )
				{
					maxPenetration = results.m_depth;
					/* Normal points INTO the static entity (for bounce calculation). */
					dominantNormal = -results.m_impactNormal;
					/* Track the entity we collided with (for grounded source). */
					collidedEntity = otherEntity->getMovableTrait();
				}
			}
		}
	}
}
