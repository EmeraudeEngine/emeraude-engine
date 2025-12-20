/*
 * src/Physics/SphereCollisionModel.cpp
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

#include "SphereCollisionModel.hpp"

/* Local inclusions for usages. */
#include "Libs/Math/Space3D/Collisions/SamePrimitive.hpp"
#include "Libs/Math/Space3D/Collisions/PointSphere.hpp"
#include "Libs/Math/Space3D/Collisions/SphereCuboid.hpp"
#include "Libs/Math/Space3D/Collisions/CapsuleSphere.hpp"
#include "PointCollisionModel.hpp"
#include "AABBCollisionModel.hpp"
#include "CapsuleCollisionModel.hpp"

namespace EmEn::Physics
{
	using namespace Libs::Math;
	using namespace Libs::Math::Space3D;

	CollisionDetectionResults
	SphereCollisionModel::isCollidingWith (const CartesianFrame< float > & thisWorldFrame, const CollisionModelInterface & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
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
	SphereCollisionModel::collideWithPoint (const CartesianFrame< float > & thisWorldFrame, const PointCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldSphere = this->toWorldSphere(thisWorldFrame);
		const auto worldPoint = other.toWorldPoint(otherWorldFrame);

		if ( isColliding(worldPoint, worldSphere) )
		{
			results.m_collisionDetected = true;
			results.m_contact = worldPoint;

			/* Compute MTV to push sphere away from point. */
			const auto centerToPoint = worldPoint - worldSphere.position();
			const auto distance = centerToPoint.length();

			if ( distance > std::numeric_limits< float >::epsilon() )
			{
				results.m_depth = m_radius - distance;
				results.m_impactNormal = centerToPoint / distance;
				results.m_MTV = results.m_impactNormal * results.m_depth;
			}
			else
			{
				/* Point is at sphere center. */
				results.m_depth = m_radius;
				results.m_impactNormal = Vector< 3, float >::positiveY();
				results.m_MTV = results.m_impactNormal * results.m_depth;
			}
		}

		return results;
	}

	CollisionDetectionResults
	SphereCollisionModel::collideWithSphere (const CartesianFrame< float > & thisWorldFrame, const SphereCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldSphereA = this->toWorldSphere(thisWorldFrame);
		const auto worldSphereB = other.toWorldSphere(otherWorldFrame);

		Vector< 3, float > mtv;

		if ( isColliding(worldSphereA, worldSphereB, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point is at the surface of the first sphere in the direction of the MTV. */
			results.m_contact = worldSphereA.position() - (results.m_impactNormal * m_radius);
		}

		return results;
	}

	CollisionDetectionResults
	SphereCollisionModel::collideWithAABB (const CartesianFrame< float > & thisWorldFrame, const AABBCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldSphere = this->toWorldSphere(thisWorldFrame);
		const auto worldAABB = other.toWorldAABB(otherWorldFrame);

		Vector< 3, float > mtv;

		if ( isColliding(worldSphere, worldAABB, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point is at the surface of the sphere in the direction of the MTV. */
			results.m_contact = worldSphere.position() - (results.m_impactNormal * m_radius);
		}

		return results;
	}

	CollisionDetectionResults
	SphereCollisionModel::collideWithCapsule (const CartesianFrame< float > & thisWorldFrame, const CapsuleCollisionModel & other, const CartesianFrame< float > & otherWorldFrame) const noexcept
	{
		CollisionDetectionResults results;

		const auto worldSphere = this->toWorldSphere(thisWorldFrame);
		const auto worldCapsule = other.toWorldCapsule(otherWorldFrame);

		Vector< 3, float > mtv;

		/* NOTE: isColliding(sphere, capsule, mtv) pushes sphere out of capsule. */
		if ( isColliding(worldSphere, worldCapsule, mtv) )
		{
			results.m_collisionDetected = true;
			results.m_MTV = mtv;
			results.m_depth = mtv.length();

			if ( results.m_depth > 0.0F )
			{
				results.m_impactNormal = mtv / results.m_depth;
			}

			/* Contact point is at the surface of the sphere in the direction of the MTV. */
			results.m_contact = worldSphere.position() - (results.m_impactNormal * m_radius);
		}

		return results;
	}
}
