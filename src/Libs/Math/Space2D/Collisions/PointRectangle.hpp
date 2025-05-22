/*
* src/Libs/Math/Space2D/Collisions.hpp
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

#pragma once

/* Local inclusions. */
#include "Libs/Math/Space2D/AARectangle.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a rectangle is colliding with a point.
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param rectangle A reference to a Circle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const AARectangle< precision_t > & rectangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( point[X] < rectangle.left() || point[X] > rectangle.right() )
		{
			return false;
		}

		/* NOTE: Positive Y goes downward. */
		if ( point[Y] < rectangle.top() || point[Y] > rectangle.bottom() )
		{
			return false;
		}

		return true;
	}

	/**
	 * @brief Checks if a rectangle is colliding with a point.
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param rectangle A reference to a circle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const AARectangle< precision_t > & rectangle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// TODO ...

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Point< precision_t > &, const AARectangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AARectangle< precision_t > & rectangle, const Point< precision_t > & point) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, rectangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Point< precision_t > &, const AARectangle< precision_t > &, Vector< 2, precision_t >) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AARectangle< precision_t > & rectangle, const Point< precision_t > & point, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, rectangle);
	}
}
