/*
* src/Libs/Math/Space3D/Intersections.hpp
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
#include "Libs/Math/Space3D/Line.hpp"
#include "Libs/Math/Space3D/Sphere.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a line is intersecting a sphere.
	 * @todo Same as segment-sphere except the range is infinite, check for duplicate code.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param sphere A reference to a sphere.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const Sphere< precision_t > & sphere) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !sphere.isValid() )
		{
			return false;
		}

		/* NOTE: Find the shortest distance between the center of the sphere and the line.
		 * We use the Pythagorean theorem to avoid calculating the closest point. */
		const auto lineToCenter = sphere.position() - line.origin();

		/* NOTE: Project the lineToCenter vector onto the line direction.
		 * Since the line direction is normalized, the result is the distance 'a' from the right triangle. */
		const auto projectionLength = Vector<3, precision_t>::dotProduct(lineToCenter, line.direction());

		/* NOTE: The squared distance 'c²' is the squared length of the lineToCenter vector. */
		const auto centerToOriginDistSq = lineToCenter.lengthSquared();

		/* NOTE: The squared distance from the sphere to line 'b²' is c² - a². */
		const auto distanceToLineSq = centerToOriginDistSq - (projectionLength * projectionLength);

		/* NOTE: Compare this distance squared with the radius of the sphere squared. */
		return distanceToLineSq <= sphere.squaredRadius();
	}

	/**
	 * @brief Checks if a line is intersecting a sphere and gives the intersection point.
	 * @todo Same as segment-sphere except the range is infinite, check for duplicate code.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param sphere A reference to a sphere.
	 * @param intersection A writable reference to a vector for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const Sphere< precision_t > & sphere, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !sphere.isValid() )
		{
			intersection.reset();

			return false;
		}

		const auto & origin = line.origin();
		const auto & direction = line.direction();
		const auto & center = sphere.position();
		const auto radiusSq = sphere.squaredRadius();

		const auto oc = origin - center;

		/* NOTE: Coefficients of the quadratic equation at² + bt + c = 0
		 * a = dot(direction, direction), which is 1 because the direction is normalized. */
		const precision_t a = static_cast< precision_t >(1);
		const precision_t b = static_cast< precision_t >(2) * Vector< 3, precision_t >::dotProduct(oc, direction);
		const precision_t c = oc.lengthSquared() - radiusSq;

		/* Calculate the discriminant. */
		const precision_t discriminant = b * b - static_cast< precision_t >(4) * a * c;

		/* NOTE: If the discriminant is negative, the line never touches the sphere. */
		if ( discriminant < 0 )
		{
			intersection.reset();

			return false;
		}

		/* NOTE: An intersection exists. Calculate the 't' of the nearest intersection point. */
		const precision_t t = (-b - std::sqrt(discriminant)) / (static_cast< precision_t >(2) * a);

		/* NOTE: Calculate the coordinates of the intersection point. */
		intersection = origin + direction * t;

		return true;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Line< precision_t > &, Sphere< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Sphere< precision_t > & sphere, const Line< precision_t > & line) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, sphere);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Line< precision_t > &, Sphere< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Sphere< precision_t > & sphere, const Line< precision_t > & line, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, sphere, intersection);
	}
}
