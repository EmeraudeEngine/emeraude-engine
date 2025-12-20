/*
 * src/Physics/PointCollisionModel.cpp
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

#include "PointCollisionModel.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/Point.hpp"
#include "Libs/Math/Space3D/Collisions/PointSphere.hpp"
#include "Libs/Math/Space3D/Collisions/PointCuboid.hpp"
#include "Libs/Math/Space3D/Collisions/CapsulePoint.hpp"
#include "SphereCollisionModel.hpp"
#include "AABBCollisionModel.hpp"
#include "CapsuleCollisionModel.hpp"

namespace EmEn::Physics
{
	using namespace Libs::Math;
	using namespace Libs::Math::Space3D;

	CollisionDetectionResults
	PointCollisionModel::isCollidingWith (const CartesianFrame< float > & thisWorldFrame, const CollisionModelInterface & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
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
	PointCollisionModel::collideWithPoint ([[maybe_unused]] const CartesianFrame< float > & thisWorldFrame, [[maybe_unused]] const PointCollisionModel & other, [[maybe_unused]] const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		/* Two points cannot collide (zero volume). */
		return {};
	}

	CollisionDetectionResults
	PointCollisionModel::collideWithSphere (const CartesianFrame< float > & thisWorldFrame, const SphereCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldPoint = this->toWorldPoint(thisWorldFrame);
		const auto worldSphere = other.toWorldSphere(otherWorldFrame);

		if ( isColliding(worldPoint, worldSphere) )
		{
			results.m_collisionDetected = true;
			results.m_contact = worldPoint;

			/* Compute MTV to push point out of sphere. */
			const auto pointToCenter = worldSphere.position() - worldPoint;
			const auto distance = pointToCenter.length();

			if ( distance > std::numeric_limits< float >::epsilon() )
			{
				results.m_depth = worldSphere.radius() - distance;
				results.m_impactNormal = -pointToCenter / distance;
				results.m_MTV = results.m_impactNormal * results.m_depth;
			}
			else
			{
				/* Point is at sphere center. */
				results.m_depth = worldSphere.radius();
				results.m_impactNormal = Vector< 3, float >::negativeY();
				results.m_MTV = results.m_impactNormal * results.m_depth;
			}
		}

		return results;
	}

	CollisionDetectionResults
	PointCollisionModel::collideWithAABB (const CartesianFrame< float > & thisWorldFrame, const AABBCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldPoint = this->toWorldPoint(thisWorldFrame);
		const auto worldAABB = other.toWorldAABB(otherWorldFrame);

		if ( isColliding(worldPoint, worldAABB) )
		{
			results.m_collisionDetected = true;
			results.m_contact = worldPoint;

			/* Compute MTV to push point out of AABB. */
			const auto & minB = worldAABB.minimum();
			const auto & maxB = worldAABB.maximum();

			/* Find the smallest distance to each face. */
			const float distToMaxX = maxB[0] - worldPoint[0];
			const float distToMinX = worldPoint[0] - minB[0];
			const float distToMaxY = maxB[1] - worldPoint[1];
			const float distToMinY = worldPoint[1] - minB[1];
			const float distToMaxZ = maxB[2] - worldPoint[2];
			const float distToMinZ = worldPoint[2] - minB[2];

			float minDist = distToMaxX;
			results.m_impactNormal = Vector< 3, float >::positiveX();

			if ( distToMinX < minDist )
			{
				minDist = distToMinX;
				results.m_impactNormal = Vector< 3, float >::negativeX();
			}

			if ( distToMaxY < minDist )
			{
				minDist = distToMaxY;
				results.m_impactNormal = Vector< 3, float >::positiveY();
			}

			if ( distToMinY < minDist )
			{
				minDist = distToMinY;
				results.m_impactNormal = Vector< 3, float >::negativeY();
			}

			if ( distToMaxZ < minDist )
			{
				minDist = distToMaxZ;
				results.m_impactNormal = Vector< 3, float >::positiveZ();
			}

			if ( distToMinZ < minDist )
			{
				minDist = distToMinZ;
				results.m_impactNormal = Vector< 3, float >::negativeZ();
			}

			results.m_depth = minDist;
			results.m_MTV = results.m_impactNormal * minDist;
		}

		return results;
	}

	CollisionDetectionResults
	PointCollisionModel::collideWithCapsule (const CartesianFrame< float > & thisWorldFrame, const CapsuleCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldPoint = this->toWorldPoint(thisWorldFrame);
		const auto worldCapsule = other.toWorldCapsule(otherWorldFrame);

		if ( isColliding(worldPoint, worldCapsule) )
		{
			results.m_collisionDetected = true;
			results.m_contact = worldPoint;

			/* Compute MTV to push point out of capsule. */
			const auto closestOnAxis = worldCapsule.closestPointOnAxis(worldPoint);
			const auto axisToPoint = worldPoint - closestOnAxis;
			const auto distance = axisToPoint.length();

			if ( distance > std::numeric_limits< float >::epsilon() )
			{
				results.m_depth = worldCapsule.radius() - distance;
				results.m_impactNormal = axisToPoint / distance;
				results.m_MTV = results.m_impactNormal * results.m_depth;
			}
			else
			{
				/* Point is on the capsule axis. */
				results.m_depth = worldCapsule.radius();
				results.m_impactNormal = Vector< 3, float >::negativeY();
				results.m_MTV = results.m_impactNormal * results.m_depth;
			}
		}

		return results;
	}
}
