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

/* STL inclusions. */
#include <array>

/* Local inclusions. */
#include "Libs/Math/Space2D/Triangle.hpp"
#include "Libs/Math/Space2D/Circle.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Math/Space2D/SAT.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if triangles are colliding.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangleA A reference to a triangle.
	 * @param triangleB A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangleA, const Triangle< precision_t > & triangleB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		Vector< 2, precision_t > MTV;

		std::array< Vector< 2, precision_t >, 3 > verticesA = {triangleA.pointA(), triangleA.pointB(), triangleA.pointC()};
		std::array< Vector< 2, precision_t >, 3 > verticesB = {triangleB.pointA(), triangleB.pointB(), triangleB.pointC()};

		return SAT::checkCollision(verticesA, verticesB, MTV);
	}

	/**
	 * @brief Checks if triangles are colliding and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangleA A reference to a triangle.
	 * @param triangleB A reference to a triangle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangleA, const Triangle< precision_t > & triangleB, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		std::array< Vector< 2, precision_t >, 3 > verticesA = {triangleA.pointA(), triangleA.pointB(), triangleA.pointC()};
		std::array< Vector< 2, precision_t >, 3 > verticesB = {triangleB.pointA(), triangleB.pointB(), triangleB.pointC()};

		return SAT::checkCollision(verticesA, verticesB, minimumTranslationVector);
	}

	/**
	 * @brief Checks if circles are colliding.
	 * @tparam precision_t The data precision. Default float.
	 * @param circleA A reference to a circle.
	 * @param circleB A reference to a circle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Circle< precision_t > & circleA, const Circle< precision_t > & circleB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto distanceSq = Vector< 2, precision_t >::distanceSquared(circleA.position(), circleB.position());
		const auto radiiSum = circleA.radius() + circleB.radius();
		const auto radiiSumSq = radiiSum * radiiSum;

		return distanceSq <= radiiSumSq;
	}

	/**
	 * @brief Checks if circles are colliding and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param circleA A reference to a circle.
	 * @param circleB A reference to a circle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Circle< precision_t > & circleA, const Circle< precision_t > & circleB, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto delta = circleB.position() - circleA.position();
		const auto distanceSq = delta.lengthSquared();
		const auto radiiSum = circleA.radius() + circleB.radius();

		if ( distanceSq > radiiSum * radiiSum )
		{
			return false;
		}

		const auto distance = std::sqrt(distanceSq);
		const auto overlap = radiiSum - distance;

		if ( distance > std::numeric_limits< precision_t >::epsilon() )
		{
			minimumTranslationVector = (delta / distance) * overlap;
		}
		else
		{
			/* Circles are at the same position, push apart along X-axis. */
			minimumTranslationVector = Vector< 2, precision_t >{radiiSum, 0};
		}

		return true;
	}

	/**
	 * @brief Checks if rectangles are colliding.
	 * @tparam precision_t The data precision. Default float.
	 * @param rectangleA A reference to a rectangle.
	 * @param rectangleB A reference to a rectangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AARectangle< precision_t > & rectangleA, const AARectangle< precision_t > & rectangleB) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto minAx = rectangleA.left();
		const auto minAy = rectangleA.top();
		const auto maxAx = rectangleA.right();
		const auto maxAy = rectangleA.bottom();

		const auto minBx = rectangleB.left();
		const auto minBy = rectangleB.top();
		const auto maxBx = rectangleB.right();
		const auto maxBy = rectangleB.bottom();

		return !(maxAx < minBx || minAx > maxBx ||
				 maxAy < minBy || minAy > maxBy);
	}

	/**
	 * @brief Checks if rectangles are colliding and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param rectangleA A reference to a rectangle.
	 * @param rectangleB A reference to a rectangle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AARectangle< precision_t > & rectangleA, const AARectangle< precision_t > & rectangleB, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !isColliding(rectangleA, rectangleB) )
		{
			return false;
		}

		const auto minAx = rectangleA.left();
		const auto minAy = rectangleA.top();
		const auto maxAx = rectangleA.right();
		const auto maxAy = rectangleA.bottom();

		const auto minBx = rectangleB.left();
		const auto minBy = rectangleB.top();
		const auto maxBx = rectangleB.right();
		const auto maxBy = rectangleB.bottom();

		const precision_t overlapX1 = maxAx - minBx;
		const precision_t overlapX2 = maxBx - minAx;
		const precision_t overlapY1 = maxAy - minBy;
		const precision_t overlapY2 = maxBy - minAy;

		const precision_t overlapX = std::min(overlapX1, overlapX2);
		const precision_t overlapY = std::min(overlapY1, overlapY2);

		if ( overlapX < overlapY )
		{
			minimumTranslationVector = {(overlapX1 < overlapX2) ? -overlapX : overlapX, 0};
		}
		else
		{
			minimumTranslationVector = {0, (overlapY1 < overlapY2) ? -overlapY : overlapY};
		}

		return true;
	}
}
