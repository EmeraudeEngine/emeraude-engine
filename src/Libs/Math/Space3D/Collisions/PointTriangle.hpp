/*
* src/Libs/Math/Space3D/Collisions.hpp
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
#include "Libs/Math/Space3D/Triangle.hpp"

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

		/* NOTE: This method checks if the point is on the same side of each edge. The side is determined by the sign of the 2D cross product. */
		const precision_t crossAB = Vector< 2, precision_t >::crossProduct(trianglePoints[1] - trianglePoints[0], point - trianglePoints[0]);
		const precision_t crossBC = Vector< 2, precision_t >::crossProduct(trianglePoints[2] - trianglePoints[1], point - trianglePoints[1]);
		const precision_t crossCA = Vector< 2, precision_t >::crossProduct(trianglePoints[0] - trianglePoints[2], point - trianglePoints[2]);

		/* NOTE: If the point is inside, the three cross products will have the same sign (or will be zero if the point is on an edge).
		 * We check both directions (clockwise and counterclockwise) to be robust. */
		const bool allNonNegative = (crossAB >= 0) && (crossBC >= 0) && (crossCA >= 0);
		const bool allNonPositive = (crossAB <= 0) && (crossBC <= 0) && (crossCA <= 0);

		return allNonNegative || allNonPositive;
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
		const auto & A = trianglePoints[0];
		const auto & B = trianglePoints[1];
		const auto & C = trianglePoints[2];

		const auto crossAB = Vector< 2, precision_t>::crossProduct(B - A, point - A);
		const auto crossBC = Vector< 2, precision_t>::crossProduct(C - B, point - B);
		const auto crossCA = Vector< 2, precision_t>::crossProduct(A - C, point - C);

		const bool allNonNegative = (crossAB >= 0) && (crossBC >= 0) && (crossCA >= 0);
		const bool allNonPositive = (crossAB <= 0) && (crossBC <= 0) && (crossCA <= 0);

		if ( !allNonNegative || !allNonPositive )
		{
			minimumTranslationVector.reset();

			return false;
		}

		/* NOTE: The point is inside, so we calculate the MTV.
		 * The MTV is the smallest vector pushing the point out of one of the sides.
		 * We calculate this vector for each side using vector rejection. */
		const auto mtvAB = (point - A).reject(B - A);
		const auto mtvBC = (point - B).reject(C - B);
		const auto mtvCA = (point - C).reject(A - C);

		/* NOTE: We find the shortest of the three thrust vectors (using the squared lengths). */
		auto min_mtv = mtvAB;
		auto min_len_sq = mtvAB.lengthSquared();

		const auto len_sq_BC = mtvBC.lengthSquared();

		if ( len_sq_BC < min_len_sq )
		{
			min_len_sq = len_sq_BC;
			min_mtv = mtvBC;
		}

		const auto len_sq_CA = mtvCA.lengthSquared();

		if ( len_sq_CA < min_len_sq )
		{
			min_mtv = mtvCA;
		}

		/* NOTE: The "rejection" vector points away from the point, so the MTV is its opposite.
		 * We convert the 2D vector to 3D to match the signature. */
		minimumTranslationVector = (-min_mtv).toVector3();

		return true;
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
