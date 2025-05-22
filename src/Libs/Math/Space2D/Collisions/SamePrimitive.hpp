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
#include "Libs/Math/Space2D/Triangle.hpp"
#include "Libs/Math/Space2D/Circle.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"

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
		// TODO ...

		return false;
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
		// TODO ...

		return false;
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
		// TODO ...

		return false;
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
		// TODO ...

		return false;
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
		// TODO ...

		return false;
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
		// TODO ...

		return false;
	}
}
