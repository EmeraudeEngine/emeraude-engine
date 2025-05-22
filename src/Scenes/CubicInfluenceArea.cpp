/*
 * src/Scenes/CubicInfluenceArea.cpp
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

#include "CubicInfluenceArea.hpp"

/* STL inclusions. */
#include <cmath>

/* Local inclusions. */
#include "AbstractEntity.hpp"

namespace EmEn::Scenes
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Physics;

	CubicInfluenceArea::CubicInfluenceArea (const AbstractEntity & parentEntity, float size) noexcept
		: m_parentEntity(&parentEntity), m_xSize(std::abs(size)), m_ySize(std::abs(size)), m_zSize(std::abs(size))
	{

	}

	CubicInfluenceArea::CubicInfluenceArea (const AbstractEntity & parentEntity, float xSize, float ySize, float zSize) noexcept
		: m_parentEntity(&parentEntity), m_xSize(std::abs(xSize)), m_ySize(std::abs(ySize)), m_zSize(std::abs(zSize))
	{

	}

	bool
	CubicInfluenceArea::isUnderInfluence (const CartesianFrame< float > & worldCoordinates, const Space3D::Sphere< float > & /*worldBoundingSphere*/) const noexcept
	{
		/* FIXME: TODO : Use sphere radius ! */
		const auto position = this->getPositionInModifierSpace(worldCoordinates.position());

		/* X test */
		if ( position[X] > m_xSize || position[X] < -m_xSize )
		{
			return false;
		}

		/* Y test */
		if ( position[Y] > m_ySize || position[Y] < -m_ySize )
		{
			return false;
		}

		/* Z test */
		if ( position[Z] > m_zSize || position[Z] < -m_zSize )
		{
			return false;
		}

		return true;
	}

	float
	CubicInfluenceArea::influenceStrength (const CartesianFrame< float > & worldCoordinates, const Space3D::Sphere< float > & /*worldBoundingSphere*/) const noexcept
	{
		/* FIXME: TODO : Use sphere radius ! */
		const auto position = this->getPositionInModifierSpace(worldCoordinates.position());

		/* X test */
		if ( position[X] > m_xSize || position[X] < -m_xSize )
		{
			return 0.0F;
		}

		/* Y test */
		if ( position[Y] > m_ySize || position[Y] < -m_ySize )
		{
			return 0.0F;
		}

		/* Z test */
		if ( position[Z] > m_zSize || position[Z] < -m_zSize )
		{
			return 0.0F;
		}

		return 1.0F;
	}

	bool
	CubicInfluenceArea::isUnderInfluence (const CartesianFrame< float > & /*worldCoordinates*/, const Space3D::AACuboid< float > & /*worldBoundingBox*/) const noexcept
	{
		/* FIXME: TODO ! */
		return false;
	}

	float
	CubicInfluenceArea::influenceStrength (const CartesianFrame< float > & /*worldCoordinates*/, const Space3D::AACuboid< float > & /*worldBoundingBox*/) const noexcept
	{
		/* FIXME: TODO ! */
		return 0.0F;
	}

	void
	CubicInfluenceArea::setSize (float size) noexcept
	{
		m_xSize = std::abs(size);
		m_ySize = std::abs(size);
		m_zSize = std::abs(size);
	}

	void
	CubicInfluenceArea::setSize (float xSize, float ySize, float zSize) noexcept
	{
		m_xSize = std::abs(xSize);
		m_ySize = std::abs(ySize);
		m_zSize = std::abs(zSize);
	}

	float
	CubicInfluenceArea::xSize () const noexcept
	{
		return m_xSize;
	}

	float
	CubicInfluenceArea::ySize () const noexcept
	{
		return m_ySize;
	}

	float
	CubicInfluenceArea::zSize () const noexcept
	{
		return m_zSize;
	}

	Vector< 4, float >
	CubicInfluenceArea::getPositionInModifierSpace (const Vector< 3, float > & position) const noexcept
	{
		const auto modifierSpace = m_parentEntity->getWorldCoordinates().getInvertedModelMatrix();

		/* NOTE: Don't forget the 1.0F of vector4 because it's a position. */
		return modifierSpace * Vector< 4, float >(position, 1.0F);
	}
}
