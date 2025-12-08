/*
 * src/Physics/CollisionDetection.cpp
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

#include "CollisionDetection.hpp"

/* Local inclusions. */
#include "Libs/Math/Space3D/Collisions/SamePrimitive.hpp"
#include "Libs/Math/Space3D/Collisions/SphereCuboid.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Libs/Math/OrientedCuboid.hpp"
#include "Scenes/AbstractEntity.hpp"
#include "Physics/MovableTrait.hpp"
#include "Physics/ContactManifold.hpp"

namespace EmEn::Physics
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Scenes;

	bool
	detectCollisionMovableToMovable (AbstractEntity & movableEntityA, AbstractEntity & movableEntityB, std::vector< ContactManifold > & outManifolds) noexcept
	{
		auto * movableA = movableEntityA.getMovableTrait();
		auto * movableB = movableEntityB.getMovableTrait();

		if ( movableA == nullptr || movableB == nullptr )
		{
			return false;
		}

		float penetration = 0.0F;
		Vector< 3, float > normal;

		/* Sphere-to-sphere collision detection. */
		if ( movableEntityA.collisionDetectionModel() == CollisionDetectionModel::Sphere && movableEntityB.collisionDetectionModel() == CollisionDetectionModel::Sphere )
		{
			if ( !detectSphereToSphereCollision(movableEntityA, movableEntityB, penetration, normal) )
			{
				return false;
			}
		}
		/* Box-to-box collision detection (AABB). */
		else if ( movableEntityA.collisionDetectionModel() == CollisionDetectionModel::AABB && movableEntityB.collisionDetectionModel() == CollisionDetectionModel::AABB )
		{
			if ( !detectBoxToBoxCollision(movableEntityB, movableEntityA, penetration, normal) )
			{
				return false;
			}
		}
		/* Mixed collision detection. */
		else
		{
			if ( movableEntityA.collisionDetectionModel() == CollisionDetectionModel::Sphere )
			{
				if ( !detectSphereToBoxCollision(movableEntityA, movableEntityB, penetration, normal) )
				{
					return false;
				}
			}
			else
			{
				if ( !detectSphereToBoxCollision(movableEntityB, movableEntityA, penetration, normal) )
				{
					return false;
				}

				/* Invert normal since we swapped A and B. */
				normal = -normal;
			}
		}

		/* Create contact manifold for both movable bodies. */
		ContactManifold manifold{movableA, movableB};

		/* Normal must point from bodyA towards bodyB.
		 * The collision detection already returns MTV pointing from A to B,
		 * so we just normalize it. */
		const auto contactNormal = normal.normalize();

		/* Compute contact point position.
		 * For sphere-sphere: point on surface of A towards B.
		 * For other cases: midpoint as approximation. */
		Vector< 3, float > collisionPosition;

		if ( movableEntityA.collisionDetectionModel() == CollisionDetectionModel::Sphere &&
		     movableEntityB.collisionDetectionModel() == CollisionDetectionModel::Sphere )
		{
			/* Contact point is on surface of sphere A, in direction of B. */
			const auto & posA = movableEntityA.getWorldCoordinates().position();
			const auto radiusA = movableEntityA.getWorldBoundingSphere().radius();

			collisionPosition = posA + contactNormal * radiusA;
		}
		else
		{
			/* Fallback to midpoint for other collision types. */
			collisionPosition = Vector< 3, float >::midPoint(
				movableEntityA.getWorldCoordinates().position(),
				movableEntityB.getWorldCoordinates().position()
			);
		}

		/* Add contact to manifold. */
		manifold.addContact(collisionPosition, contactNormal, penetration);

		/* Add manifold to output vector. */
		outManifolds.push_back(manifold);

		return true;
	}

	bool
	detectCollisionMovableToStatic (AbstractEntity & movableEntity, const AbstractEntity & staticEntity, std::vector< ContactManifold > & outManifolds) noexcept
	{
		auto * movable = movableEntity.getMovableTrait();

		if ( movable == nullptr )
		{
			return false;
		}

		float penetration = 0.0F;
		Vector< 3, float > normal;

		/* Sphere-to-sphere collision detection. */
		if ( movableEntity.collisionDetectionModel() == CollisionDetectionModel::Sphere && staticEntity.collisionDetectionModel() == CollisionDetectionModel::Sphere )
		{
			if ( !detectSphereToSphereCollision(movableEntity, staticEntity, penetration, normal) )
			{
				return false;
			}
		}
		/* Box-to-box collision detection (AABB). */
		else if ( movableEntity.collisionDetectionModel() == CollisionDetectionModel::AABB && staticEntity.collisionDetectionModel() == CollisionDetectionModel::AABB )
		{
			if ( !detectBoxToBoxCollision(movableEntity, staticEntity, penetration, normal) )
			{
				return false;
			}
		}
		/* Mixed collision detection. */
		else
		{
			if ( movableEntity.collisionDetectionModel() == CollisionDetectionModel::Sphere )
			{
				if ( !detectSphereToBoxCollision(movableEntity, staticEntity, penetration, normal) )
				{
					return false;
				}
			}
			else
			{
				if ( !detectSphereToBoxCollision(staticEntity, movableEntity, penetration, normal) )
				{
					return false;
				}

				/* Invert normal since we have box (movable) vs sphere (static). */
				normal = -normal;
			}
		}

		/* Create contact manifold with only the movable body (static has infinite mass). */
		ContactManifold manifold(movable);

		/* Compute contact point position (midpoint between the two entities). */
		const auto collisionPosition = Vector< 3, float >::midPoint(
			movableEntity.getWorldCoordinates().position(),
			staticEntity.getWorldCoordinates().position()
		);

		/* Normal must point from bodyA (movable) towards bodyB (static).
		 * The collision detection returns MTV pointing from static towards movable
		 * (direction to push movable out), so we invert it. */
		const auto contactNormal = -normal.normalize();

		/* Add contact to manifold. */
		manifold.addContact(collisionPosition, contactNormal, penetration);

		/* Add manifold to output vector. */
		outManifolds.push_back(manifold);

		return true;
	}

	bool
	detectSphereToSphereCollision (const AbstractEntity & sphereEntityA, const AbstractEntity & sphereEntityB, float & outPenetration, Vector< 3, float > & outNormal) noexcept
	{
		const auto sphereA = sphereEntityA.getWorldBoundingSphere();
		const auto sphereB = sphereEntityB.getWorldBoundingSphere();

		Vector< 3, float > mtv;

		if ( !Space3D::isColliding(sphereA, sphereB, mtv) )
		{
			return false;
		}

		outPenetration = mtv.length();
		outNormal = mtv;

		return true;
	}

	bool
	detectBoxToBoxCollision (const AbstractEntity & boxEntityA, const AbstractEntity & boxEntityB, float & outPenetration, Vector< 3, float > & outNormal) noexcept
	{
		/* NOTE: We check first with axis-aligned bounding box ... */
		if ( !Space3D::isColliding(boxEntityA.getWorldBoundingBox(), boxEntityB.getWorldBoundingBox()) )
		{
			return false;
		}

		/* NOTE: Then with oriented bounding box ... */
		outPenetration = OrientedCuboid< float >::isIntersecting(
			{boxEntityA.localBoundingBox(), boxEntityA.getWorldCoordinates()},
			{boxEntityB.localBoundingBox(), boxEntityB.getWorldCoordinates()},
			outNormal
		);

		return outPenetration > 0.0F;
	}

	bool
	detectSphereToBoxCollision (const AbstractEntity & sphereEntity, const AbstractEntity & boxEntity, float & outPenetration, Vector< 3, float > & outNormal) noexcept
	{
		const auto sphere = sphereEntity.getWorldBoundingSphere();
		const auto box = boxEntity.getWorldBoundingBox();

		Vector< 3, float > mtv;

		if ( !Space3D::isColliding(sphere, box, mtv) )
		{
			return false;
		}

		outPenetration = mtv.length();
		outNormal = mtv;

		return true;
	}
}
