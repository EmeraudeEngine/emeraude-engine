/*
 * src/Libs/Math/Space2D/Collisions/TriangleRectangle.hpp
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

/* STL inclusions. */
#include <array>

/* Local inclusions. */
#include "Libs/Math/Space2D/Triangle.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Libs/Math/Space2D/SAT.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Checks if a triangle is colliding with a rectangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param rectangle A reference to a rectangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const AARectangle< precision_t > & rectangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		Vector< 2, precision_t > MTV;

		std::array< Vector< 2, precision_t >, 3 > triangleVertices = {
			triangle.pointA(),
			triangle.pointB(),
			triangle.pointC()
		};

		std::array< Vector< 2, precision_t >, 4 > rectangleVertices = {
			Vector< 2, precision_t >{rectangle.left(), rectangle.top()},
			Vector< 2, precision_t >{rectangle.right(), rectangle.top()},
			Vector< 2, precision_t >{rectangle.right(), rectangle.bottom()},
			Vector< 2, precision_t >{rectangle.left(), rectangle.bottom()}
		};

		return SAT::checkCollision(triangleVertices, rectangleVertices, MTV);
	}

	/**
	 * @brief Checks if a triangle is colliding with a rectangle and gives the overlapping distance.
	 * @tparam precision_t The data precision. Default float.
	 * @param triangle A reference to a triangle.
	 * @param rectangle A reference to a rectangle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const AARectangle< precision_t > & rectangle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		std::array< Vector< 2, precision_t >, 3 > triangleVertices = {
			triangle.pointA(),
			triangle.pointB(),
			triangle.pointC()
		};

		std::array< Vector< 2, precision_t >, 4 > rectangleVertices = {
			Vector< 2, precision_t >{rectangle.left(), rectangle.top()},
			Vector< 2, precision_t >{rectangle.right(), rectangle.top()},
			Vector< 2, precision_t >{rectangle.right(), rectangle.bottom()},
			Vector< 2, precision_t >{rectangle.left(), rectangle.bottom()}
		};

		return SAT::checkCollision(triangleVertices, rectangleVertices, minimumTranslationVector);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Triangle< precision_t > &, const AARectangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AARectangle< precision_t > & rectangle, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(triangle, rectangle);
	}

	/** @copydoc EmEn::Libs::Math::Space2D::isColliding(const Triangle< precision_t > &, const AARectangle< precision_t > &, Vector< 2, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const AARectangle< precision_t > & rectangle, const Triangle< precision_t > & triangle, Vector< 2, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const bool result = isColliding(triangle, rectangle, minimumTranslationVector);

		if ( result )
		{
			minimumTranslationVector = -minimumTranslationVector;
		}

		return result;
	}
}
