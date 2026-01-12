/*
 * src/Libs/Math/Space2D/Collisions/PointRectangle.hpp
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

#pragma once

/* Local inclusions. */
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Math/Space2D/Point.hpp"

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
		const auto min = rectangle.topLeft();
		const auto max = rectangle.bottomRight();

		return (point.x() >= min.x() && point.x() <= max.x() &&
				point.y() >= min.y() && point.y() <= max.y());
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
		if ( !isColliding(point, rectangle) )
		{
			return false;
		}

		const auto min = rectangle.topLeft();
		const auto max = rectangle.bottomRight();

		const precision_t dist_left = point.x() - min.x();
		const precision_t dist_right = max.x() - point.x();
		const precision_t dist_top = point.y() - min.y();
		const precision_t dist_bottom = max.y() - point.y();

		precision_t min_dist = dist_left;
		minimumTranslationVector = {-dist_left, 0};

		if ( dist_right < min_dist )
		{
			min_dist = dist_right;
			minimumTranslationVector = {dist_right, 0};
		}

		if ( dist_top < min_dist )
		{
			min_dist = dist_top;
			minimumTranslationVector = {0, -dist_top};
		}

		if ( dist_bottom < min_dist )
		{
			minimumTranslationVector = {0, dist_bottom};
		}

		return true;
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
		const bool result = isColliding(point, rectangle, minimumTranslationVector);

		if ( result )
		{
			minimumTranslationVector = -minimumTranslationVector;
		}

		return result;
	}
}
