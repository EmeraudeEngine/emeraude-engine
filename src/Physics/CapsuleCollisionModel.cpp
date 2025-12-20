/*
 * src/Physics/CapsuleCollisionModel.cpp
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

#include "CapsuleCollisionModel.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/Capsule.hpp"
#include "Libs/Math/Space3D/Collisions/SamePrimitive.hpp"
#include "Libs/Math/Space3D/Collisions/CapsulePoint.hpp"
#include "Libs/Math/Space3D/Collisions/CapsuleSphere.hpp"
#include "Libs/Math/Space3D/Collisions/CapsuleCuboid.hpp"
#include "PointCollisionModel.hpp"
#include "SphereCollisionModel.hpp"
#include "AABBCollisionModel.hpp"

namespace EmEn::Physics
{
	using namespace Libs::Math;
	using namespace Libs::Math::Space3D;

	CollisionDetectionResults
	CapsuleCollisionModel::isCollidingWith (const CartesianFrame< float > & thisWorldFrame, const CollisionModelInterface & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
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
	CapsuleCollisionModel::collideWithPoint (const CartesianFrame< float > & thisWorldFrame, const PointCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldCapsule = this->toWorldCapsule(thisWorldFrame);
		const auto worldPoint = other.toWorldPoint(otherWorldFrame);

		if ( isColliding(worldPoint, worldCapsule) )
		{
			results.m_collisionDetected = true;
			results.m_contact = worldPoint;

			/* Compute MTV to push capsule away from point. */
			const auto closestOnAxis = worldCapsule.closestPointOnAxis(worldPoint);
			const auto axisToPoint = worldPoint - closestOnAxis;
			const auto distance = axisToPoint.length();

			if ( distance > std::numeric_limits< float >::epsilon() )
			{
				results.m_depth = m_localCapsule.radius() - distance;
				results.m_impactNormal = axisToPoint / distance;
				results.m_MTV = results.m_impactNormal * results.m_depth;
			}
			else
			{
				/* Point is on the capsule axis. */
				results.m_depth = m_localCapsule.radius();
				results.m_impactNormal = Vector< 3, float >::positiveY();
				results.m_MTV = results.m_impactNormal * results.m_depth;
			}
		}

		return results;
	}

	CollisionDetectionResults
	CapsuleCollisionModel::collideWithSphere (const CartesianFrame< float > & thisWorldFrame, const SphereCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldCapsule = this->toWorldCapsule(thisWorldFrame);
		const auto worldSphere = other.toWorldSphere(otherWorldFrame);

		Vector< 3, float > mtv;

		/* NOTE: isColliding(capsule, sphere, mtv) pushes capsule out of sphere. */
		if ( isColliding(worldCapsule, worldSphere, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point: closest point on capsule axis to sphere, then offset by capsule radius. */
			const auto closestOnAxis = worldCapsule.closestPointOnAxis(worldSphere.position());
			results.m_contact = closestOnAxis - (results.m_impactNormal * m_localCapsule.radius());
		}

		return results;
	}

	CollisionDetectionResults
	CapsuleCollisionModel::collideWithAABB (const CartesianFrame< float > & thisWorldFrame, const AABBCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldCapsule = this->toWorldCapsule(thisWorldFrame);
		const auto worldAABB = other.toWorldAABB(otherWorldFrame);

		Vector< 3, float > mtv;

		/* NOTE: isColliding(capsule, cuboid, mtv) pushes capsule out of cuboid. */
		if ( isColliding(worldCapsule, worldAABB, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point: closest points between capsule axis and AABB. */
			Point< float > closestOnAxis, closestOnCuboid;
			closestPointsCapsuleCuboid(worldCapsule, worldAABB, closestOnAxis, closestOnCuboid);
			results.m_contact = closestOnAxis - (results.m_impactNormal * m_localCapsule.radius());
		}

		return results;
	}

	CollisionDetectionResults
	CapsuleCollisionModel::collideWithCapsule (const CartesianFrame< float > & thisWorldFrame, const CapsuleCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldCapsuleA = this->toWorldCapsule(thisWorldFrame);
		const auto worldCapsuleB = other.toWorldCapsule(otherWorldFrame);

		Vector< 3, float > mtv;

		if ( isColliding(worldCapsuleA, worldCapsuleB, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point: closest points between the two capsule axes. */
			Point< float > closestOnA, closestOnB;
			closestPointsBetweenSegments(worldCapsuleA.axis(), worldCapsuleB.axis(), closestOnA, closestOnB);
			results.m_contact = closestOnA - (results.m_impactNormal * m_localCapsule.radius());
		}

		return results;
	}
}
