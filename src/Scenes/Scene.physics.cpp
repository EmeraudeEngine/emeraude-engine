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

/* Local inclusions. */
#include "Physics/CollisionDetection.hpp"

/* STL inclusions. */
#include <span>

namespace EmEn::Scenes
{
	using namespace Base;
	using namespace Base::Math;
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

	void
	Scene::resolveCollisions () const noexcept
	{
		if ( m_physicsOctree == nullptr )
		{
			return;
		}

		/* Lock the physics octree for the duration of the simulation to prevent
		 * concurrent modifications from other threads (e.g., checkEntityLocationInOctrees). */
		const std::scoped_lock lock{m_physicsOctreeAccess};

		/* ============================================================
		 * PHASE 1: STATIC COLLISIONS (Boundaries, Ground, StaticEntity)
		 * - Accumulate position corrections from ALL static collisions
		 * - Use dominant collision (deepest penetration) for velocity bounce
		 * ============================================================ */

		/* Each movable is handled ONCE, at the single sector that OWNS it (forEachSector() visits
		 * inner nodes too, which is where anything straddling a boundary lives). The statics it can
		 * touch are the inherited ones (ancestors) plus those of its own sector's subtree, reached
		 * through the subtree query in accumulateStaticEntityCorrections(). ⚠️ The former leaf-only
		 * walk corrected a straddling body in the FIRST leaf that met it, against that leaf's
		 * candidates only — the statics owned by the sibling leaves it also straddled were never
		 * tested. */
		m_physicsOctree->forEachSector([this] (const OctreeSector< AbstractEntity, true > & sector, const std::vector< std::shared_ptr< AbstractEntity > > & candidates, size_t ownedOffset) {
			/* An element the sector OWNS is fully inside it, so the sector's border flag settles
			 * the boundary question for every element below. */
			const bool entityAtBorder = sector.isTouchingRootBorder();

			const std::span< const std::shared_ptr< AbstractEntity > > inheritedCandidates{candidates.data(), ownedOffset};

			for ( size_t candidateIndex = ownedOffset; candidateIndex < candidates.size(); ++candidateIndex )
			{
				const auto & entity = candidates[candidateIndex];

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
				if ( entityAtBorder )
				{
					const float prevMax = maxPenetration;
					this->accumulateBoundaryCorrection(entity, positionCorrection, dominantNormal, maxPenetration);

					if ( maxPenetration > prevMax )
					{
						/* Only mark as grounded on Boundary if it's the floor (bottom face).
						 * Side walls and ceiling cannot ground an entity.
						 * ⚠️ +Y is UP, so the world box's FLOOR is its Y = -boundary plane, and the
						 * contact normal there points from the body towards that wall — DOWNWARD.
						 * The test below is therefore Y-UP correct as written: do not "fix" it. */
						constexpr auto GroundNormalThreshold{0.7F};

						if ( dominantNormal[Y] < -GroundNormalThreshold )
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
					this->accumulateStaticEntityCorrections(entity, sector, inheritedCandidates, positionCorrection, dominantNormal, maxPenetration, collidedEntity);

					if ( maxPenetration > prevMax )
					{
						/* Only mark as grounded on Entity if standing on top of it.
						 * Hitting the side of a wall doesn't ground you.
						 * ⚠️ +Y is UP: standing ON something means the contact normal points from
						 * the body DOWN into it. Y-UP correct as written: do not "fix" it. */
						constexpr auto GroundNormalThreshold{0.7F};

						if ( dominantNormal[Y] < -GroundNormalThreshold )
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
					const float impactForce = std::max(0.0F, impactVelocity) * movable->getBodyPhysicalProperties().mass() / WorldPhysicsUpdateCycleDurationS< float >;

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
		std::vector< std::shared_ptr< AbstractEntity > > involvedEntities;

		/* Pairing contract of OctreeSector::forEachSector(): at each sector, test owned × owned and
		 * owned × inherited, nothing else. Every pair whose bounds can overlap is produced exactly
		 * once — two elements of disjoint subtrees have disjoint bounds by the storage invariant —
		 * so there is NO cross-sector deduplication, and none must come back. ⚠️ The former version
		 * paired ALL candidates in EVERY leaf and deduplicated with a hash set: every inherited ×
		 * inherited pair was re-hashed once per leaf below it. Measured on game-logic (121 nodes):
		 * 99 % of the logic thread, 23 ms per tick. */
		const auto testPair = [&dynamicManifolds, &involvedEntities] (const std::shared_ptr< AbstractEntity > & entityA, bool entityAPaused, const std::shared_ptr< AbstractEntity > & entityB) {
			/* Skip non-movable entities, or pairs where both are paused
			 * (two sleeping bodies don't need collision testing).
			 * An active entity must still collide with paused ones. */
			if ( !entityB->hasMovableAbility() || (entityAPaused && entityB->isSimulationPaused()) )
			{
				return;
			}

			/* Detect and collect collision manifold. */
			if ( detectCollisionMovableToMovable(*entityA, *entityB, dynamicManifolds) )
			{
				involvedEntities.push_back(entityA);
				involvedEntities.push_back(entityB);
			}
		};

		m_physicsOctree->forEachSector([&testPair] (const OctreeSector< AbstractEntity, true > & /*sector*/, const std::vector< std::shared_ptr< AbstractEntity > > & candidates, size_t ownedOffset) {
			for ( size_t indexA = ownedOffset; indexA < candidates.size(); ++indexA )
			{
				const auto & entityA = candidates[indexA];

				/* Skip non-movable entities. */
				if ( !entityA->hasMovableAbility() )
				{
					continue;
				}

				const bool entityAPaused = entityA->isSimulationPaused();

				/* Owned × owned: the elements this sector owns after A. */
				for ( size_t indexB = indexA + 1; indexB < candidates.size(); ++indexB )
				{
					testPair(entityA, entityAPaused, candidates[indexB]);
				}

				/* Owned × inherited: everything the ancestors own. */
				for ( size_t indexB = 0; indexB < ownedOffset; ++indexB )
				{
					testPair(entityA, entityAPaused, candidates[indexB]);
				}
			}
		});

		/* Resolve dynamic collisions via impulse solver, then enforce boundaries. */
		if ( !dynamicManifolds.empty() )
		{
			m_constraintSolver.solve(dynamicManifolds, WorldPhysicsUpdateCycleDurationS< float >);

			/* Immediately clip all involved entities to boundaries.
			 * This ensures impulse resolution cannot push entities outside. */
			for ( const auto & entity : involvedEntities )
			{
				this->clipInsideBoundaries(entity);
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

			/* ⚠️ +Y is UP: a point is above ground while position[Y] >= groundLevel, so the
			 * violation to clip is position[Y] < groundLevel. */
			if ( position[Y] < groundLevel )
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

				if ( position[Y] < groundLevel )
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
				/* ⚠️ +Y is UP, so the lowest point of the sphere is position[Y] - radius, and
				 * resting on the surface puts its CENTRE one radius ABOVE the ground level. */
				const auto lowestPoint = position[Y] - radius;

				if ( lowestPoint < groundLevel )
				{
					entity->setYPosition(groundLevel + radius, TransformSpace::World);
				}
			}
				break;

			case CollisionModelType::AABB :
			{
				const auto aabb = model->getAABB(worldCoords);

				/* ⚠️ +Y is UP, so the box's ground-facing face is its MINIMUM-Y one. The accessors
				 * say which extremum they return rather than "bottom"/"top", so this choice is
				 * explicit at the site that makes it.
				 * ⚠️ This block is DUPLICATED THREE TIMES in this file. Changing one and not the
				 * others is a silent half-migration: bodies would rest on the ground in one code path
				 * and sink through it in another.
				 * ⚠️⚠️ The corner CHOICE and the penetration ARITHMETIC below are ONE decision. The
				 * Y-up flip moved the corners to minY* and left the subtraction reversed, which is
				 * how this block ended up half-migrated INSIDE the warning telling it not to be. */
				const std::array< Vector< 3, float >, 4 > groundFacingCorners{
					aabb.minYSouthEast(),
					aabb.minYSouthWest(),
					aabb.minYNorthWest(),
					aabb.minYNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : groundFacingCorners )
				{
					const auto groundLevel = m_groundLevel->getLevelAt(corner);
					/* ⚠️ +Y is UP: the corner is BELOW the surface when it sits at a LOWER Y, so the
					 * penetration is (ground - corner). Written the other way round it measures the
					 * CLEARANCE above the ground and reports every airborne body as colliding. */
					const auto penetration = groundLevel - corner[Y];

					if ( penetration > deepestPenetration )
					{
						deepestPenetration = penetration;
					}
				}

				if ( deepestPenetration > 0.0F )
				{
					/* ⚠️ +Y is UP: clipping a body out of the ground moves it towards +Y. */
					entity->moveY(deepestPenetration, TransformSpace::World);
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

				/* ⚠️ +Y is UP, so position[Y] < groundLevel means below ground. */
				if ( position[Y] < groundLevel )
				{
					const auto penetration = groundLevel - position[Y];
					ContactManifold manifold(movable);
					/* Normal points from bodyA (entity) towards bodyB (the ground), which is
					 * DOWNWARD now that +Y is up. */
					manifold.addContact({position[X], groundLevel, position[Z]}, {0.0F, -1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
			}
				break;

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;
				const auto groundLevel = m_groundLevel->getLevelAt(position);
				/* ⚠️ +Y is UP, so the lowest point of the sphere is position[Y] - radius. */
				const auto lowestPoint = position[Y] - radius;

				if ( lowestPoint < groundLevel )
				{
					const auto penetration = groundLevel - lowestPoint;
					ContactManifold manifold(movable);
					/* Normal points from bodyA (entity) towards bodyB (the ground), which is
					 * DOWNWARD now that +Y is up. */
					manifold.addContact({position[X], groundLevel, position[Z]}, {0.0F, -1.0F, 0.0F}, penetration);
					manifolds.push_back(manifold);
				}
			}
				break;

			case CollisionModelType::AABB :
			{
				const auto aabb = model->getAABB(worldCoords);

				/* ⚠️ +Y is UP, so the box's ground-facing face is its MINIMUM-Y one. The accessors
				 * say which extremum they return rather than "bottom"/"top", so this choice is
				 * explicit at the site that makes it.
				 * ⚠️ This block is DUPLICATED THREE TIMES in this file. Changing one and not the
				 * others is a silent half-migration: bodies would rest on the ground in one code path
				 * and sink through it in another.
				 * ⚠️⚠️ The corner CHOICE and the penetration ARITHMETIC below are ONE decision. The
				 * Y-up flip moved the corners to minY* and left the subtraction reversed, which is
				 * how this block ended up half-migrated INSIDE the warning telling it not to be. */
				const std::array< Vector< 3, float >, 4 > groundFacingCorners{
					aabb.minYSouthEast(),
					aabb.minYSouthWest(),
					aabb.minYNorthWest(),
					aabb.minYNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : groundFacingCorners )
				{
					const auto groundLevel = m_groundLevel->getLevelAt(corner);
					/* ⚠️ +Y is UP: the corner is BELOW the surface when it sits at a LOWER Y, so the
					 * penetration is (ground - corner). Written the other way round it measures the
					 * CLEARANCE above the ground and reports every airborne body as colliding. */
					const auto penetration = groundLevel - corner[Y];

					if ( penetration > deepestPenetration )
					{
						deepestPenetration = penetration;
					}
				}

				if ( deepestPenetration > 0.0F )
				{
					const auto groundLevel = m_groundLevel->getLevelAt(position);
					ContactManifold manifold(movable);
					/* Normal points from bodyA (entity) towards bodyB (the ground), which is
					 * DOWNWARD now that +Y is up. */
					manifold.addContact({position[X], groundLevel, position[Z]}, {0.0F, -1.0F, 0.0F}, deepestPenetration);
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
			 * ⚠️ getNormalAt() returns the surface's OUTWARD normal, which points UP (+Y).
			 * The file-wide contact convention is the opposite one — the normal points FROM the
			 * body INTO the surface it penetrates — which is what applyCollisionResponse() reads
			 * (vn > 0 means "moving into the surface") and what makes the correction below
			 * (positionCorrection -= normal * penetration) push the body UP. Hence the negation:
			 * it survived the Y-up flip unchanged, because BOTH sides of it flipped together. */
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

				/* ⚠️ +Y is UP, so position[Y] < groundLevel means below ground. */
				if ( position[Y] < groundLevel )
				{
					accumulateCollision(position, groundLevel - position[Y]);
				}
			}
				break;

			case CollisionModelType::Sphere :
			{
				const auto aabb = model->getAABB(worldCoords);
				const auto radius = aabb.width() * 0.5F;
				const auto groundLevel = m_groundLevel->getLevelAt(position);
				/* ⚠️ +Y is UP, so the lowest point of the sphere is position[Y] - radius. */
				const auto lowestPoint = position[Y] - radius;

				if ( lowestPoint < groundLevel )
				{
					accumulateCollision(position, groundLevel - lowestPoint);
				}
			}
				break;

			case CollisionModelType::AABB :
			{
				const auto aabb = model->getAABB(worldCoords);

				/* ⚠️ +Y is UP, so the box's ground-facing face is its MINIMUM-Y one. The accessors
				 * say which extremum they return rather than "bottom"/"top", so this choice is
				 * explicit at the site that makes it.
				 * ⚠️ This block is DUPLICATED THREE TIMES in this file. Changing one and not the
				 * others is a silent half-migration: bodies would rest on the ground in one code path
				 * and sink through it in another.
				 * ⚠️⚠️ The corner CHOICE and the penetration ARITHMETIC below are ONE decision. The
				 * Y-up flip moved the corners to minY* and left the subtraction reversed, which is
				 * how this block ended up half-migrated INSIDE the warning telling it not to be. */
				const std::array< Vector< 3, float >, 4 > groundFacingCorners{
					aabb.minYSouthEast(),
					aabb.minYSouthWest(),
					aabb.minYNorthWest(),
					aabb.minYNorthEast()
				};

				auto deepestPenetration = 0.0F;

				for ( const auto & corner : groundFacingCorners )
				{
					const auto groundLevel = m_groundLevel->getLevelAt(corner);
					/* ⚠️ +Y is UP: the corner is BELOW the surface when it sits at a LOWER Y, so the
					 * penetration is (ground - corner). Written the other way round it measures the
					 * CLEARANCE above the ground and reports every airborne body as colliding. */
					const auto penetration = groundLevel - corner[Y];

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
	Scene::accumulateStaticEntityCorrections (const std::shared_ptr< AbstractEntity > & entity, const OctreeSector< AbstractEntity, true > & sector, std::span< const std::shared_ptr< AbstractEntity > > inheritedCandidates, Vector< 3, float > & positionCorrection, Vector< 3, float > & dominantNormal, float & maxPenetration, const MovableTrait *& collidedEntity) const noexcept
	{
		/* No collision model means no collision simulation. */
		if ( !entity->hasCollisionModel() )
		{
			return;
		}

		const auto * entityModel = entity->collisionModel();
		const auto entityWorldCoords = entity->getWorldCoordinates();

		const auto accumulate = [&] (const AbstractEntity & otherEntity) {
			/* Skip self. */
			if ( entity.get() == &otherEntity )
			{
				return;
			}

			/* Skip if the other entity is movable (we only want static entities here). */
			if ( otherEntity.hasMovableAbility() )
			{
				return;
			}

			/* Skip if the other entity has no collision model. */
			if ( !otherEntity.hasCollisionModel() )
			{
				return;
			}

			const auto * otherModel = otherEntity.collisionModel();

			/* Static entities with Point model are ignored (no volume). */
			if ( otherModel->modelType() == CollisionModelType::Point )
			{
				return;
			}

			const auto otherWorldCoords = otherEntity.getWorldCoordinates();

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
					collidedEntity = otherEntity.getMovableTrait();
				}
			}
		};

		/* Statics owned by the ancestors of the entity's sector: anything straddling a boundary
		 * above it lives there — a ground plane straddles them all and sits at the root. */
		for ( const auto & otherEntity : inheritedCandidates )
		{
			accumulate(*otherEntity);
		}

		/* Statics owned by the entity's own sector and by its subtree, bounded by the entity's
		 * AABB: a body straddling the boundary between two child sectors must meet the small
		 * statics of BOTH, which no single leaf's candidate set could offer. */
		sector.forTouchedSector(entityModel->getAABB(entityWorldCoords), [&accumulate] (const OctreeSector< AbstractEntity, true > & touchedSector) {
			for ( const auto & otherEntity : touchedSector.elements() )
			{
				accumulate(*otherEntity);
			}
		});
	}
}
