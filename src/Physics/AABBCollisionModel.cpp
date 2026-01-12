/*
 * src/Physics/AABBCollisionModel.cpp
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

#include "AABBCollisionModel.hpp"

/* Local inclusions for implementations. */
#include "Libs/Math/Space3D/Collisions/SamePrimitive.hpp"
#include "Libs/Math/Space3D/Collisions/PointCuboid.hpp"
#include "Libs/Math/Space3D/Collisions/PointSphere.hpp"
#include "Libs/Math/Space3D/Collisions/SphereCuboid.hpp"
#include "Libs/Math/Space3D/Collisions/CapsuleCuboid.hpp"
#include "PointCollisionModel.hpp"
#include "SphereCollisionModel.hpp"
#include "CapsuleCollisionModel.hpp"

namespace EmEn::Physics
{
	using namespace Libs::Math;
	using namespace Libs::Math::Space3D;

	CollisionDetectionResults
	AABBCollisionModel::isCollidingWith (const CartesianFrame< float > & thisWorldFrame, const CollisionModelInterface & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		switch ( other.modelType() )
		{
			case CollisionModelType::Point:
				return this->collideWithPoint(thisWorldFrame, static_cast< const PointCollisionModel & >(other), otherWorldFrame);

			case CollisionModelType::Sphere:
				return this->collideWithSphere(thisWorldFrame, static_cast< const SphereCollisionModel & >(other), otherWorldFrame);

			case CollisionModelType::AABB:
				return this->collideWithAABB(thisWorldFrame, static_cast< const AABBCollisionModel & >(other), otherWorldFrame);

			case CollisionModelType::Capsule:
				return this->collideWithCapsule(thisWorldFrame, static_cast< const CapsuleCollisionModel & >(other), otherWorldFrame);
		}

		return {};
	}

	CollisionDetectionResults
	AABBCollisionModel::collideWithPoint (const CartesianFrame< float > & thisWorldFrame, const PointCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldAABB = this->toWorldAABB(thisWorldFrame);
		const auto worldPoint = other.toWorldPoint(otherWorldFrame);

		if ( isColliding(worldPoint, worldAABB) )
		{
			results.m_collisionDetected = true;
			results.m_contact = worldPoint;

			/* Compute MTV to push AABB away from point. */
			const auto & minB = worldAABB.minimum();
			const auto & maxB = worldAABB.maximum();

			/* Find the smallest distance to each face (negate for MTV direction). */
			const float distToMaxX = maxB[0] - worldPoint[0];
			const float distToMinX = worldPoint[0] - minB[0];
			const float distToMaxY = maxB[1] - worldPoint[1];
			const float distToMinY = worldPoint[1] - minB[1];
			const float distToMaxZ = maxB[2] - worldPoint[2];
			const float distToMinZ = worldPoint[2] - minB[2];

			float minDist = distToMaxX;
			results.m_impactNormal = Vector< 3, float >::negativeX();

			if ( distToMinX < minDist )
			{
				minDist = distToMinX;
				results.m_impactNormal = Vector< 3, float >::positiveX();
			}

			if ( distToMaxY < minDist )
			{
				minDist = distToMaxY;
				results.m_impactNormal = Vector< 3, float >::negativeY();
			}

			if ( distToMinY < minDist )
			{
				minDist = distToMinY;
				results.m_impactNormal = Vector< 3, float >::positiveY();
			}

			if ( distToMaxZ < minDist )
			{
				minDist = distToMaxZ;
				results.m_impactNormal = Vector< 3, float >::negativeZ();
			}

			if ( distToMinZ < minDist )
			{
				minDist = distToMinZ;
				results.m_impactNormal = Vector< 3, float >::positiveZ();
			}

			results.m_depth = minDist;
			results.m_MTV = results.m_impactNormal * minDist;
		}

		return results;
	}

	CollisionDetectionResults
	AABBCollisionModel::collideWithSphere (const CartesianFrame< float > & thisWorldFrame, const SphereCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldAABB = this->toWorldAABB(thisWorldFrame);
		const auto worldSphere = other.toWorldSphere(otherWorldFrame);

		Vector< 3, float > mtv;

		/* NOTE: isColliding(cuboid, sphere, mtv) pushes cuboid out of sphere. */
		if ( isColliding(worldAABB, worldSphere, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point approximation: closest point on AABB to sphere center. */
			const auto & spherePos = worldSphere.position();
			const auto & minB = worldAABB.minimum();
			const auto & maxB = worldAABB.maximum();

			results.m_contact = Point< float >{
				std::max(minB[0], std::min(spherePos[0], maxB[0])),
				std::max(minB[1], std::min(spherePos[1], maxB[1])),
				std::max(minB[2], std::min(spherePos[2], maxB[2]))
			};
		}

		return results;
	}

	CollisionDetectionResults
	AABBCollisionModel::collideWithAABB (const CartesianFrame< float > & thisWorldFrame, const AABBCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldAABBA = this->toWorldAABB(thisWorldFrame);
		const auto worldAABBB = other.toWorldAABB(otherWorldFrame);

		Vector< 3, float > mtv;

		if ( isColliding(worldAABBA, worldAABBB, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point approximation: center of overlap region. */
			const auto overlapMinX = std::max(worldAABBA.minimum()[0], worldAABBB.minimum()[0]);
			const auto overlapMaxX = std::min(worldAABBA.maximum()[0], worldAABBB.maximum()[0]);
			const auto overlapMinY = std::max(worldAABBA.minimum()[1], worldAABBB.minimum()[1]);
			const auto overlapMaxY = std::min(worldAABBA.maximum()[1], worldAABBB.maximum()[1]);
			const auto overlapMinZ = std::max(worldAABBA.minimum()[2], worldAABBB.minimum()[2]);
			const auto overlapMaxZ = std::min(worldAABBA.maximum()[2], worldAABBB.maximum()[2]);

			results.m_contact = Point< float >{
				(overlapMinX + overlapMaxX) * 0.5F,
				(overlapMinY + overlapMaxY) * 0.5F,
				(overlapMinZ + overlapMaxZ) * 0.5F
			};
		}

		return results;
	}

	CollisionDetectionResults
	AABBCollisionModel::collideWithCapsule (const CartesianFrame< float > & thisWorldFrame, const CapsuleCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldAABB = this->toWorldAABB(thisWorldFrame);
		const auto worldCapsule = other.toWorldCapsule(otherWorldFrame);

		Vector< 3, float > mtv;

		/* NOTE: isColliding(cuboid, capsule, mtv) pushes cuboid out of capsule. */
		if ( isColliding(worldAABB, worldCapsule, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point approximation: closest point between capsule axis and AABB. */
			Point< float > closestOnAxis, closestOnCuboid;
			closestPointsCapsuleCuboid(worldCapsule, worldAABB, closestOnAxis, closestOnCuboid);
			results.m_contact = closestOnCuboid;
		}

		return results;
	}
}
