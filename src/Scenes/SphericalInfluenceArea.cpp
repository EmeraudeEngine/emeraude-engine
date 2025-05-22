/*
 * src/Scenes/SphericalInfluenceArea.cpp
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

#include "SphericalInfluenceArea.hpp"

/* Local inclusions. */
#include "AbstractEntity.hpp"
#include "Tracer.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Physics;

	SphericalInfluenceArea::SphericalInfluenceArea (const AbstractEntity & parentEntity) noexcept
		: m_parentEntity(&parentEntity)
	{

	}

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
	SphericalInfluenceArea::isUnderInfluence (const CartesianFrame< float > & /*worldCoordinates*/, const Space3D::AACuboid< float > & /*worldBoundingBox*/) const noexcept
	{
		/* FIXME: TODO ! */
		return false;
	}

	float
	SphericalInfluenceArea::influenceStrength (const CartesianFrame< float > & /*worldCoordinates*/, const Space3D::AACuboid< float > & /*worldBoundingBox*/) const noexcept
	{
		/* FIXME: TODO ! */
		return 0.0F;
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
