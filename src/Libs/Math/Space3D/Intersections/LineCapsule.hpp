/*
 * src/Libs/Math/Space3D/Intersections/LineCapsule.hpp
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
#include "Libs/Math/Space3D/Capsule.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Helper to test line intersection with a sphere.
	 * @tparam precision_t The data precision.
	 * @param lineOrigin The origin of the line.
	 * @param lineDirection The direction of the line (normalized).
	 * @param sphereCenter The center of the sphere.
	 * @param radiusSq The squared radius of the sphere.
	 * @param t1 Output: the first intersection parameter.
	 * @param t2 Output: the second intersection parameter.
	 * @return bool True if line intersects the sphere.
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	lineSphereIntersectionParams (const Point< precision_t > & lineOrigin, const Vector< 3, precision_t > & lineDirection, const Point< precision_t > & sphereCenter, precision_t radiusSq, precision_t & t1, precision_t & t2) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto oc = lineOrigin - sphereCenter;

		/* NOTE: Quadratic equation coefficients: at² + bt + c = 0
		 * Since direction is normalized, a = 1. */
		const precision_t b = static_cast< precision_t >(2) * Vector< 3, precision_t >::dotProduct(oc, lineDirection);
		const precision_t c = oc.lengthSquared() - radiusSq;

		const precision_t discriminant = b * b - static_cast< precision_t >(4) * c;

		if ( discriminant < 0 )
		{
			return false;
		}

		const precision_t sqrtDisc = std::sqrt(discriminant);
		t1 = (-b - sqrtDisc) / static_cast< precision_t >(2);
		t2 = (-b + sqrtDisc) / static_cast< precision_t >(2);

		return true;
	}

	/**
	 * @brief Checks if a line is intersecting a capsule.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param capsule A reference to a capsule.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const Capsule< precision_t > & capsule) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() )
		{
			return false;
		}

		const auto & lineOrigin = line.origin();
		const auto & lineDir = line.direction();
		const auto & capsuleStart = capsule.axis().startPoint();
		const auto & capsuleEnd = capsule.axis().endPoint();
		const auto radiusSq = capsule.squaredRadius();

		const auto capsuleAxis = capsuleEnd - capsuleStart;
		const auto capsuleAxisLengthSq = capsuleAxis.lengthSquared();

		/* NOTE: Handle degenerate capsule (sphere). */
		if ( capsuleAxisLengthSq < std::numeric_limits< precision_t >::epsilon() )
		{
			const auto oc = lineOrigin - capsuleStart;
			const auto projection = Vector< 3, precision_t >::dotProduct(oc, lineDir);
			const auto distSq = oc.lengthSquared() - projection * projection;

			return distSq <= radiusSq;
		}

		/* NOTE: Find the closest points between infinite line and capsule axis segment.
		 * This is a simplified line-segment to the closest point computation. */
		const auto w0 = lineOrigin - capsuleStart;

		const auto a = Vector< 3, precision_t >::dotProduct(capsuleAxis, capsuleAxis);
		const auto b = Vector< 3, precision_t >::dotProduct(capsuleAxis, lineDir);
		const auto c = Vector< 3, precision_t >::dotProduct(lineDir, lineDir); /* = 1 for normalized direction */
		const auto d = Vector< 3, precision_t >::dotProduct(capsuleAxis, w0);
		const auto e = Vector< 3, precision_t >::dotProduct(lineDir, w0);

		const auto denom = a * c - b * b;

		precision_t sc, tc;

		if ( std::abs(denom) < std::numeric_limits< precision_t >::epsilon() )
		{
			/* NOTE: Lines are parallel. */
			sc = 0;
			tc = d / a;
		}
		else
		{
			sc = (b * d - a * e) / denom;
			tc = (c * d - b * e) / denom;
		}

		/* NOTE: Clamp tc to [0, 1] to stay on the capsule axis segment.
		 * After clamping, recompute sc for the closest point on the line. */
		if ( tc < 0 )
		{
			tc = 0;
			/* NOTE: Find the closest point on the line to capsuleStart. */
			sc = -Vector< 3, precision_t >::dotProduct(lineOrigin - capsuleStart, lineDir);
		}
		else if ( tc > 1 )
		{
			tc = 1;
			/* NOTE: Find the closest point on the line to capsuleEnd. */
			sc = -Vector< 3, precision_t >::dotProduct(lineOrigin - capsuleEnd, lineDir);
		}

		/* NOTE: Closest point on the line and on capsule axis. */
		const auto closestOnLine = lineOrigin + lineDir * sc;
		const auto closestOnAxis = capsuleStart + capsuleAxis * tc;

		const auto distSq = Vector< 3, precision_t >::distanceSquared(closestOnLine, closestOnAxis);

		return distSq <= radiusSq;
	}

	/**
	 * @brief Checks if a line is intersecting a capsule and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param line A reference to a line.
	 * @param capsule A reference to a capsule.
	 * @param intersection A writable reference to a point for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Line< precision_t > & line, const Capsule< precision_t > & capsule, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !capsule.isValid() )
		{
			intersection.reset();

			return false;
		}

		const auto & lineOrigin = line.origin();
		const auto & lineDir = line.direction();
		const auto & capsuleStart = capsule.axis().startPoint();
		const auto & capsuleEnd = capsule.axis().endPoint();
		//const auto radius = capsule.radius();
		const auto radiusSq = capsule.squaredRadius();

		const auto capsuleAxis = capsuleEnd - capsuleStart;
		const auto capsuleAxisLength = capsuleAxis.length();

		/* NOTE: Handle degenerate capsule (sphere). */
		if ( capsuleAxisLength < std::numeric_limits< precision_t >::epsilon() )
		{
			precision_t t1, t2;

			if ( lineSphereIntersectionParams(lineOrigin, lineDir, capsuleStart, radiusSq, t1, t2) )
			{
				intersection = lineOrigin + lineDir * t1;

				return true;
			}

			intersection.reset();

			return false;
		}

		/* NOTE: Normalized capsule axis direction. */
		const auto capsuleDir = capsuleAxis / capsuleAxisLength;

		/* NOTE: Track the nearest intersection. */
		precision_t nearestT = std::numeric_limits< precision_t >::max();
		bool found = false;

		/* NOTE: Test intersection with the infinite cylinder.
		 * We project the problem onto the plane perpendicular to the capsule axis. */
		const auto dp = Vector< 3, precision_t >::dotProduct(lineDir, capsuleDir);
		const auto perpLineDir = lineDir - capsuleDir * dp;

		const auto w = lineOrigin - capsuleStart;
		const auto perpW = w - capsuleDir * Vector< 3, precision_t >::dotProduct(w, capsuleDir);

		const auto perpDirLengthSq = perpLineDir.lengthSquared();

		if ( perpDirLengthSq > std::numeric_limits< precision_t >::epsilon() )
		{
			/* NOTE: Solve quadratic for cylinder intersection. */
			const auto a = perpDirLengthSq;
			const auto b = static_cast< precision_t >(2) * Vector< 3, precision_t >::dotProduct(perpLineDir, perpW);
			const auto c = perpW.lengthSquared() - radiusSq;

			const auto discriminant = b * b - static_cast< precision_t >(4) * a * c;

			if ( discriminant >= 0 )
			{
				const auto sqrtDisc = std::sqrt(discriminant);
				const precision_t t1 = (-b - sqrtDisc) / (static_cast< precision_t >(2) * a);
				const precision_t t2 = (-b + sqrtDisc) / (static_cast< precision_t >(2) * a);

				/* NOTE: Check if intersection points are within the finite cylinder portion. */
				for ( const auto t : {t1, t2} )
				{
					const auto hitPoint = lineOrigin + lineDir * t;
					const auto hitToStart = hitPoint - capsuleStart;
					const auto projOnAxis = Vector< 3, precision_t >::dotProduct(hitToStart, capsuleDir);

					if ( projOnAxis >= 0 && projOnAxis <= capsuleAxisLength )
					{
						if ( t < nearestT )
						{
							nearestT = t;
							found = true;
						}
					}
				}
			}
		}

		/* NOTE: Test intersection with the start hemisphere (sphere at capsuleStart). */
		{
			precision_t t1, t2;

			if ( lineSphereIntersectionParams(lineOrigin, lineDir, capsuleStart, radiusSq, t1, t2) )
			{
				for ( const auto t : {t1, t2} )
				{
					const auto hitPoint = lineOrigin + lineDir * t;
					const auto hitToStart = hitPoint - capsuleStart;
					const auto projOnAxis = Vector< 3, precision_t >::dotProduct(hitToStart, capsuleDir);

					/* NOTE: Only accept if on the hemisphere side (proj <= 0). */
					if ( projOnAxis <= 0 )
					{
						if ( t < nearestT )
						{
							nearestT = t;
							found = true;
						}
					}
				}
			}
		}

		/* NOTE: Test intersection with the end hemisphere (sphere at capsuleEnd). */
		{
			precision_t t1, t2;

			if ( lineSphereIntersectionParams(lineOrigin, lineDir, capsuleEnd, radiusSq, t1, t2) )
			{
				for ( const auto t : {t1, t2} )
				{
					const auto hitPoint = lineOrigin + lineDir * t;
					const auto hitToEnd = hitPoint - capsuleEnd;
					const auto projOnAxis = Vector< 3, precision_t >::dotProduct(hitToEnd, capsuleDir);

					/* NOTE: Only accept if on the hemisphere side (proj >= 0). */
					if ( projOnAxis >= 0 )
					{
						if ( t < nearestT )
						{
							nearestT = t;
							found = true;
						}
					}
				}
			}
		}

		if ( found )
		{
			intersection = lineOrigin + lineDir * nearestT;

			return true;
		}

		intersection.reset();

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Line< precision_t > &, const Capsule< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Capsule< precision_t > & capsule, const Line< precision_t > & line) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, capsule);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Line< precision_t > &, const Capsule< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Capsule< precision_t > & capsule, const Line< precision_t > & line, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(line, capsule, intersection);
	}
}
