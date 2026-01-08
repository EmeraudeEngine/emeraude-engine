/*
 * src/Libs/Math/Space3D/Intersections/SegmentCapsule.hpp
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
#include "Libs/Math/Space3D/Segment.hpp"
#include "Libs/Math/Space3D/Capsule.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Checks if a segment is intersecting a capsule.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param capsule A reference to a capsule.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Capsule< precision_t > & capsule) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segment.isValid() || !capsule.isValid() )
		{
			return false;
		}

		/* NOTE: Use the closestPointsBetweenSegments approach.
		 * If the distance between closest points <= capsule radius, they intersect. */

		const auto & segStart = segment.startPoint();
		const auto & segEnd = segment.endPoint();
		const auto & capsuleStart = capsule.axis().startPoint();
		const auto & capsuleEnd = capsule.axis().endPoint();

		const auto d1 = segEnd - segStart;       /* Segment direction */
		const auto d2 = capsuleEnd - capsuleStart; /* Capsule axis direction */
		const auto r = segStart - capsuleStart;

		const precision_t a = Vector< 3, precision_t >::dotProduct(d1, d1);
		const precision_t e = Vector< 3, precision_t >::dotProduct(d2, d2);
		const precision_t f = Vector< 3, precision_t >::dotProduct(d2, r);

		constexpr precision_t epsilon = std::numeric_limits< precision_t >::epsilon();

		precision_t s, t;

		/* NOTE: Check if both segments are degenerate (points). */
		if ( a <= epsilon && e <= epsilon )
		{
			const auto distSq = Vector< 3, precision_t >::distanceSquared(segStart, capsuleStart);

			return distSq <= capsule.squaredRadius();
		}

		/* NOTE: Check if segment is degenerate (point). */
		if ( a <= epsilon )
		{
			s = 0;
			t = std::clamp(f / e, static_cast< precision_t >(0), static_cast< precision_t >(1));
		}
		else
		{
			const precision_t c = Vector< 3, precision_t >::dotProduct(d1, r);

			/* NOTE: Check if capsule axis is degenerate (point). */
			if ( e <= epsilon )
			{
				t = 0;
				s = std::clamp(-c / a, static_cast< precision_t >(0), static_cast< precision_t >(1));
			}
			else
			{
				/* NOTE: General non-degenerate case. */
				const precision_t b = Vector< 3, precision_t >::dotProduct(d1, d2);
				const precision_t denom = a * e - b * b;

				if ( denom != 0 )
				{
					s = std::clamp((b * f - c * e) / denom, static_cast< precision_t >(0), static_cast< precision_t >(1));
				}
				else
				{
					s = 0;
				}

				t = (b * s + f) / e;

				if ( t < 0 )
				{
					t = 0;
					s = std::clamp(-c / a, static_cast< precision_t >(0), static_cast< precision_t >(1));
				}
				else if ( t > 1 )
				{
					t = 1;
					s = std::clamp((b - c) / a, static_cast< precision_t >(0), static_cast< precision_t >(1));
				}
			}
		}

		const auto closestOnSeg = segStart + d1 * s;
		const auto closestOnCapsule = capsuleStart + d2 * t;

		const auto distSq = Vector< 3, precision_t >::distanceSquared(closestOnSeg, closestOnCapsule);

		return distSq <= capsule.squaredRadius();
	}

	/**
	 * @brief Helper to test segment intersection with a sphere and return intersection parameter.
	 * @tparam precision_t The data precision.
	 * @param segStart The start of the segment.
	 * @param segDir The segment direction vector (not normalized).
	 * @param segLengthSq The squared length of the segment.
	 * @param sphereCenter The center of the sphere.
	 * @param radiusSq The squared radius of the sphere.
	 * @param t Output: the intersection parameter [0, 1].
	 * @return bool True if segment intersects the sphere.
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	segmentSphereIntersectionParam (const Point< precision_t > & segStart, const Vector< 3, precision_t > & segDir, precision_t segLengthSq, const Point< precision_t > & sphereCenter, precision_t radiusSq, precision_t & t) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		const auto oc = segStart - sphereCenter;

		const precision_t a = segLengthSq;
		const precision_t b = static_cast< precision_t >(2) * Vector< 3, precision_t >::dotProduct(oc, segDir);
		const precision_t c = oc.lengthSquared() - radiusSq;

		const precision_t discriminant = b * b - static_cast< precision_t >(4) * a * c;

		if ( discriminant < 0 )
		{
			return false;
		}

		const precision_t sqrtDisc = std::sqrt(discriminant);
		const precision_t t1 = (-b - sqrtDisc) / (static_cast< precision_t >(2) * a);
		const precision_t t2 = (-b + sqrtDisc) / (static_cast< precision_t >(2) * a);

		/* NOTE: Find the smallest t in [0, 1]. */
		if ( t1 >= 0 && t1 <= 1 )
		{
			t = t1;

			return true;
		}

		if ( t2 >= 0 && t2 <= 1 )
		{
			t = t2;

			return true;
		}

		/* NOTE: Check if segment is entirely inside sphere. */
		if ( t1 < 0 && t2 > 1 )
		{
			t = 0;

			return true;
		}

		return false;
	}

	/**
	 * @brief Checks if a segment is intersecting a capsule and gives the intersection point.
	 * @tparam precision_t The data precision. Default float.
	 * @param segment A reference to a segment.
	 * @param capsule A reference to a capsule.
	 * @param intersection A writable reference to a point for the intersection if method returns true.
	 * @return bool
	 */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Segment< precision_t > & segment, const Capsule< precision_t > & capsule, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		if ( !segment.isValid() || !capsule.isValid() )
		{
			intersection.reset();

			return false;
		}

		const auto & segStart = segment.startPoint();
		const auto & segEnd = segment.endPoint();
		const auto & capsuleStart = capsule.axis().startPoint();
		const auto & capsuleEnd = capsule.axis().endPoint();
		//const auto radius = capsule.radius();
		const auto radiusSq = capsule.squaredRadius();

		const auto segDir = segEnd - segStart;
		const auto segLengthSq = segDir.lengthSquared();
		const auto segLength = std::sqrt(segLengthSq);

		/* NOTE: Handle degenerate segment (point). */
		if ( segLength < std::numeric_limits< precision_t >::epsilon() )
		{
			const auto distSq = Vector< 3, precision_t >::distanceSquared(segStart, capsule.closestPointOnAxis(segStart));

			if ( distSq <= radiusSq )
			{
				intersection = segStart;

				return true;
			}

			intersection.reset();

			return false;
		}

		const auto capsuleAxis = capsuleEnd - capsuleStart;
		const auto capsuleAxisLength = capsuleAxis.length();

		/* NOTE: Handle degenerate capsule (sphere). */
		if ( capsuleAxisLength < std::numeric_limits< precision_t >::epsilon() )
		{
			precision_t t;

			if ( segmentSphereIntersectionParam(segStart, segDir, segLengthSq, capsuleStart, radiusSq, t) )
			{
				intersection = segStart + segDir * t;

				return true;
			}

			intersection.reset();

			return false;
		}

		const auto capsuleDir = capsuleAxis / capsuleAxisLength;

		/* NOTE: Track the nearest intersection. */
		precision_t nearestT = std::numeric_limits< precision_t >::max();
		bool found = false;

		/* NOTE: Test intersection with the infinite cylinder, then constrain to both finite bounds. */
		const auto dp = Vector< 3, precision_t >::dotProduct(segDir, capsuleDir);
		const auto perpSegDir = segDir - capsuleDir * dp;

		const auto w = segStart - capsuleStart;
		const auto perpW = w - capsuleDir * Vector< 3, precision_t >::dotProduct(w, capsuleDir);

		const auto perpDirLengthSq = perpSegDir.lengthSquared();

		if ( perpDirLengthSq > std::numeric_limits< precision_t >::epsilon() )
		{
			const auto a = perpDirLengthSq;
			const auto b = static_cast< precision_t >(2) * Vector< 3, precision_t >::dotProduct(perpSegDir, perpW);
			const auto c = perpW.lengthSquared() - radiusSq;

			const auto discriminant = b * b - static_cast< precision_t >(4) * a * c;

			if ( discriminant >= 0 )
			{
				const auto sqrtDisc = std::sqrt(discriminant);
				const precision_t t1 = (-b - sqrtDisc) / (static_cast< precision_t >(2) * a);
				const precision_t t2 = (-b + sqrtDisc) / (static_cast< precision_t >(2) * a);

				for ( const auto t : {t1, t2} )
				{
					if ( t >= 0 && t <= 1 )
					{
						const auto hitPoint = segStart + segDir * t;
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
		}

		/* NOTE: Test intersection with the start hemisphere. */
		{
			precision_t t;

			if ( segmentSphereIntersectionParam(segStart, segDir, segLengthSq, capsuleStart, radiusSq, t) )
			{
				const auto hitPoint = segStart + segDir * t;
				const auto hitToStart = hitPoint - capsuleStart;
				const auto projOnAxis = Vector< 3, precision_t >::dotProduct(hitToStart, capsuleDir);

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

		/* NOTE: Test intersection with the end hemisphere. */
		{
			precision_t t;

			if ( segmentSphereIntersectionParam(segStart, segDir, segLengthSq, capsuleEnd, radiusSq, t) )
			{
				const auto hitPoint = segStart + segDir * t;
				const auto hitToEnd = hitPoint - capsuleEnd;
				const auto projOnAxis = Vector< 3, precision_t >::dotProduct(hitToEnd, capsuleDir);

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

		if ( found )
		{
			intersection = segStart + segDir * nearestT;

			return true;
		}

		intersection.reset();

		return false;
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Segment< precision_t > &, const Capsule< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Capsule< precision_t > & capsule, const Segment< precision_t > & segment) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, capsule);
	}

	/** @copydoc EmEn::Libs::Math::Space3D::isIntersecting(const Segment< precision_t > &, const Capsule< precision_t > &, Point< precision_t > &) noexcept */
	template< typename precision_t = float >
	[[nodiscard]]
	static
	bool
	isIntersecting (const Capsule< precision_t > & capsule, const Segment< precision_t > & segment, Point< precision_t > & intersection) noexcept requires (std::is_floating_point_v< precision_t >)
	{
		return isIntersecting(segment, capsule, intersection);
	}
}
