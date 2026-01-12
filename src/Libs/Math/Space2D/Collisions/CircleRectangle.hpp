/*
 * src/Libs/Math/Space2D/Collisions/CircleRectangle.hpp
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
#include "Libs/Math/Space2D/Circle.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a circle is colliding with a rectangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param circle A reference to a circle.
	 * @param rectangle A reference to a rectangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Circle< precision_t > & circle, const AARectangle< precision_t > & rectangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// Find the closest point on the rectangle to the circle center
		const precision_t clampedX = std::max(rectangle.left(), std::min(circle.position().x(), rectangle.right()));
		const precision_t clampedY = std::max(rectangle.top(), std::min(circle.position().y(), rectangle.bottom()));
		const Point< precision_t > closestPoint{clampedX, clampedY};

		const auto distanceSq = (circle.position() - closestPoint).lengthSquared();

		return distanceSq <= circle.radius() * circle.radius();
	}

	/**
	 * @brief Checks if a circle is colliding with a rectangle and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param circle A reference to a circle.
	 * @param rectangle A reference to a rectangle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Circle< precision_t > & circle, const AARectangle< precision_t > & rectangle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		// Find the closest point on the rectangle to the circle center
		const precision_t clampedX = std::max(rectangle.left(), std::min(circle.position().x(), rectangle.right()));
		const precision_t clampedY = std::max(rectangle.top(), std::min(circle.position().y(), rectangle.bottom()));
		const Point< precision_t > closestPoint{clampedX, clampedY};

		const auto delta = circle.position() - closestPoint;
		const auto distanceSq = delta.lengthSquared();

		if ( distanceSq > circle.radius() * circle.radius() )
		{
			return false;
		}

		if ( distanceSq < std::numeric_limits< precision_t >::epsilon() )
		{
			/* Circle center is inside the rectangle. */
			const precision_t dist_left = circle.position().x() - rectangle.left();
			const precision_t dist_right = rectangle.right() - circle.position().x();
			const precision_t dist_top = circle.position().y() - rectangle.top();
			const precision_t dist_bottom = rectangle.bottom() - circle.position().y();

			precision_t min_dist = dist_left;
			minimumTranslationVector = Vector< 2, precision_t >{-(min_dist + circle.radius()), 0};

			if ( dist_right < min_dist )
			{
				min_dist = dist_right;
				minimumTranslationVector = Vector< 2, precision_t >{min_dist + circle.radius(), 0};
			}

			if ( dist_top < min_dist )
			{
				min_dist = dist_top;
				minimumTranslationVector = Vector< 2, precision_t >{0, -(min_dist + circle.radius())};
			}

			if ( dist_bottom < min_dist )
			{
				minimumTranslationVector = Vector< 2, precision_t >{0, dist_bottom + circle.radius()};
			}
		}
		else
		{
			const auto distance = std::sqrt(distanceSq);
			const auto overlap = circle.radius() - distance;

			minimumTranslationVector = (delta / distance) * overlap;
		}

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Circle< precision_t > &, const AARectangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AARectangle< precision_t > & rectangle, const Circle< precision_t > & circle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(circle, rectangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Circle< precision_t > &, const AARectangle< precision_t > &, Vector< 2, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AARectangle< precision_t > & rectangle, const Circle< precision_t > & circle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const bool result = isColliding(circle, rectangle, minimumTranslationVector);

		if ( result )
		{
			minimumTranslationVector = -minimumTranslationVector;
		}

		return result;
	}
}
