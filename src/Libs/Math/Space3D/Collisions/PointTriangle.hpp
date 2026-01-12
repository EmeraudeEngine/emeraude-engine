/*
 * src/Libs/Math/Space3D/Collisions/PointTriangle.hpp
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
#include "Libs/Math/Space3D/Triangle.hpp"
#include "Libs/Math/Space3D/SAT.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a point is colliding with a triangle.
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param triangle A reference to a triangle.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Triangle< precision_t > & triangle) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !triangle.isValid() )
		{
			return false;
		}

		const auto & trianglePoints = triangle.points();

		/* NOTE: Use barycentric coordinates for 3D point-in-triangle test. */
		return SAT::pointInTriangle(point, trianglePoints[0], trianglePoints[1], trianglePoints[2]);
	}

	/**
	 * @brief Checks if a point is colliding with a triangle and gives the minimum translation vector (MTV).
	 * @tparam precision_t The data precision. Default float.
	 * @param point A reference to a point.
	 * @param triangle A reference to a triangle.
	 * @param minimumTranslationVector A writable reference to a vector.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Point< precision_t > & point, const Triangle< precision_t > & triangle, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !triangle.isValid() )
		{
			minimumTranslationVector.reset();

			return false;
		}

		const auto & trianglePoints = triangle.points();
		const auto & pointA = trianglePoints[0];
		const auto & pointB = trianglePoints[1];
		const auto & pointC = trianglePoints[2];

		/* NOTE: Use the SAT helper function for 3D point-in-triangle test with MTV. */
		return SAT::pointInTriangleWithMTV(point, pointA, pointB, pointC, minimumTranslationVector);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Point< precision_t > &, const Triangle< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Point< precision_t > & point) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, triangle);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isColliding(const Point< precision_t > &, const Triangle< precision_t > &, Vector< 3, precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	bool
	isColliding (const Triangle< precision_t > & triangle, const Point< precision_t > & point, Vector< 3, precision_t > & minimumTranslationVector) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isColliding(point, triangle, minimumTranslationVector);
	}
}
