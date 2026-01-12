/*
 * src/Scenes/SphericalInfluenceArea.cpp
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

#include "SphericalInfluenceArea.hpp"

/* Local inclusions. */
#include "AbstractEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Physics;

	SphericalInfluenceArea::SphericalInfluenceArea (const AbstractEntity & parentEntity, float outerRadius, float innerRadius) noexcept
		: SphericalInfluenceArea(parentEntity)
	{
		if ( outerRadius > innerRadius )
		{
			this->setOuterRadius(outerRadius);
			this->setInnerRadius(innerRadius);
		}
		else
		{
			this->setOuterRadius(innerRadius);
			this->setInnerRadius(outerRadius);
		}
	}

	bool
	SphericalInfluenceArea::isUnderInfluence (const CartesianFrame< float > & worldCoordinates, const Space3D::Sphere< float > & worldBoundingSphere) const noexcept
	{
		const auto range = m_outerRadius + worldBoundingSphere.radius();

		return (worldCoordinates.position() - m_parentEntity->getWorldCoordinates().position()).length() <= range;
	}

	float
	SphericalInfluenceArea::influenceStrength (const CartesianFrame< float > & worldCoordinates, const Space3D::Sphere< float > & worldBoundingSphere) const noexcept
	{
		const auto distance = (worldCoordinates.position() - m_parentEntity->getWorldCoordinates().position()).length();

		const auto targetBoundingRadius = worldBoundingSphere.radius();

		/* Outside the outer radius, so no influence at all. */
		if ( distance > m_outerRadius + targetBoundingRadius )
		{
			return 0.0F;
		}

		/* Inside the inner radius, full influence. */
		if ( distance < m_innerRadius + targetBoundingRadius )
		{
			return 1.0F;
		}

		/* Compute fallout distance. */
		const auto falloutDistance = m_outerRadius - m_innerRadius;

		return (distance + targetBoundingRadius - m_innerRadius) / falloutDistance;
	}

	bool
	SphericalInfluenceArea::isUnderInfluence (const CartesianFrame< float > & /*worldCoordinates*/, const Space3D::AACuboid< float > & worldBoundingBox) const noexcept
	{
		const auto modifierPosition = m_parentEntity->getWorldCoordinates().position();

		/* NOTE: worldBoundingBox is already in world coordinates (from CollisionModel::getAABB(worldFrame)). */
		const auto & boxMin = worldBoundingBox.minimum();
		const auto & boxMax = worldBoundingBox.maximum();

		/* Find the closest point on the AABB to the sphere center. */
		const Vector< 3, float > closestPoint(
			std::clamp(modifierPosition[X], boxMin[X], boxMax[X]),
			std::clamp(modifierPosition[Y], boxMin[Y], boxMax[Y]),
			std::clamp(modifierPosition[Z], boxMin[Z], boxMax[Z])
		);

		/* Check if that point is within the outer radius. */
		const auto distance = (closestPoint - modifierPosition).length();

		return distance <= m_outerRadius;
	}

	float
	SphericalInfluenceArea::influenceStrength (const CartesianFrame< float > & /*worldCoordinates*/, const Space3D::AACuboid< float > & worldBoundingBox) const noexcept
	{
		const auto modifierPosition = m_parentEntity->getWorldCoordinates().position();

		/* NOTE: worldBoundingBox is already in world coordinates (from CollisionModel::getAABB(worldFrame)). */
		const auto & boxMin = worldBoundingBox.minimum();
		const auto & boxMax = worldBoundingBox.maximum();

		/* Find the closest point on the AABB to the sphere center. */
		const Vector< 3, float > closestPoint(
			std::clamp(modifierPosition[X], boxMin[X], boxMax[X]),
			std::clamp(modifierPosition[Y], boxMin[Y], boxMax[Y]),
			std::clamp(modifierPosition[Z], boxMin[Z], boxMax[Z])
		);

		const auto distance = (closestPoint - modifierPosition).length();

		/* Outside the outer radius, no influence. */
		if ( distance > m_outerRadius )
		{
			return 0.0F;
		}

		/* Inside the inner radius, full influence. */
		if ( distance < m_innerRadius )
		{
			return 1.0F;
		}

		/* Compute falloff between inner and outer radius. */
		const auto falloutDistance = m_outerRadius - m_innerRadius;

		return 1.0F - ((distance - m_innerRadius) / falloutDistance);
	}

	bool
	SphericalInfluenceArea::isUnderInfluence (const Vector< 3, float > & worldPosition) const noexcept
	{
		const auto distance = (worldPosition - m_parentEntity->getWorldCoordinates().position()).length();

		return distance <= m_outerRadius;
	}

	float
	SphericalInfluenceArea::influenceStrength (const Vector< 3, float > & worldPosition) const noexcept
	{
		const auto distance = (worldPosition - m_parentEntity->getWorldCoordinates().position()).length();

		/* Outside the outer radius, no influence. */
		if ( distance > m_outerRadius )
		{
			return 0.0F;
		}

		/* Inside the inner radius, full influence. */
		if ( distance < m_innerRadius )
		{
			return 1.0F;
		}

		/* Compute falloff between inner and outer radius. */
		const auto falloutDistance = m_outerRadius - m_innerRadius;

		return 1.0F - ((distance - m_innerRadius) / falloutDistance);
	}

	void
	SphericalInfluenceArea::setOuterRadius (float outerRadius) noexcept
	{
		if ( outerRadius <= 0.0F )
		{
			Tracer::warning(ClassId, "Radius must be positive !");

			return;
		}

		m_outerRadius = outerRadius;

		if ( m_innerRadius > m_outerRadius )
		{
			m_innerRadius = m_outerRadius;
		}
	}

	float
	SphericalInfluenceArea::outerRadius () const noexcept
	{
		return m_outerRadius;
	}

	void
	SphericalInfluenceArea::setInnerRadius (float innerRadius) noexcept
	{
		if ( innerRadius <= 0.0F )
		{
			Tracer::warning(ClassId, "Radius must be positive !");

			return;
		}

		m_innerRadius = innerRadius;

		if ( m_innerRadius > m_outerRadius )
		{
			m_outerRadius = m_innerRadius;
		}
	}

	float
	SphericalInfluenceArea::innerRadius () const noexcept
	{
		return m_innerRadius;
	}
}
