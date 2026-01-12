/*
 * src/Libs/Math/BezierCurve.hpp
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
#include <cstddef>
#include <functional>
#include <iostream>
#include <type_traits>
#include <vector>

/* Local inclusions for usages. */
#include "Vector.hpp"

namespace EmEn::Libs::Math
{
	/**
	 * @brief Bezier curve.
	 * @tparam dim_t The dimension of the point.
	 * @tparam number_t The type of number. Default float.
	 */
	template< size_t dim_t, typename number_t = float >
	requires (dim_t == 2 || dim_t == 3 || dim_t == 4) && std::is_arithmetic_v< number_t >
	class BezierCurve final
	{
		public:

			using Callback = std::function< bool (float time, const Vector< dim_t, number_t > & position) >;

			/**
			 * @brief Constructs a bezier curve.
			 */
			BezierCurve () noexcept = default;

			/**
			 * @brief Adds a control point.
			 * @param position The absolute position of the control point.
			 * @return Vector< dim_t, type_t > &
			 */
			Vector< dim_t, number_t > &
			addPoint (const Vector< dim_t, number_t > & position) noexcept
			{
				return m_points.emplace_back(position);
			}

			/**
			 * @brief Close the curve by reusing the first point.
			 * @return void
			 */
			void
			close () noexcept
			{
				m_closed = true;
			}

			/**
			 * @brief Creates a line from the control point.
			 * @param segments The number of segments to synthesize the line.
			 * @param callback A function that receives the point and the interval on the line.
			 * @return bool
			 */
			bool
			synthesize (size_t segments, const Callback & callback) const noexcept
			{
				if ( m_points.size() < 3 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", the curve needs at least 3 points !" "\n";

					return false;
				}

				if ( segments < 2 )
				{
					std::cerr << __PRETTY_FUNCTION__ << ", the segment parameter must be at least 2 !" "\n";

					return false;
				}

				const auto timeStep = 1.0F / static_cast< float >(segments);

				/* NOTE: We are using segment count + 1 here. */
				for ( size_t index = 0; index <= segments; index++ )
				{
					/* More precise calculation to avoid error accumulation */
					const auto timePoint = std::min(1.0F, static_cast< float >(index) * timeStep);
					const auto interpolatedPoint = this->synthesizePoint(timePoint);

					if ( !callback(timePoint, interpolatedPoint) )
					{
						return false;
					}
				}

				return true;
			}

		private:

			/**
			 * @brief Gets a point on the synthesized line at a time.
			 * @param globalTimePoint A time from 0.0 to 1.0
			 * @return Vector
			 */
			[[nodiscard]]
			Vector< dim_t, number_t >
			synthesizePoint (float globalTimePoint) const noexcept
			{
				/* Case where the curve has only one quadratic segment. */
				if ( m_points.size() == 3 && !m_closed )
				{
					return Vector< dim_t, number_t >::quadraticBezierInterpolation(m_points[0], m_points[1], m_points[2], globalTimePoint);
				}

				/* Managing extreme cases for open curves. */
				if ( !m_closed )
				{
					if ( globalTimePoint <= 0.0F )
					{
						return m_points.front();
					}

					if ( globalTimePoint >= 1.0F )
					{
						return m_points.back();
					}
				}

				const size_t pointCount = m_points.size();

				/* For an open curve, we have N-2 segments for N points. For a closed curve, we have N segments. */
				const size_t numSegments = m_closed ? pointCount : pointCount - 2;

				/* Safety if not enough points for a segment */
				if ( numSegments == 0 )
				{
					return m_points.front();
				}

				/* Calculate which segment we are in and the local time (between 0 and 1) in this segment. */
				const float scaledTime = globalTimePoint * numSegments;
				size_t segmentIndex = static_cast< size_t >(std::floor(scaledTime));

				/* Ensure the index does not overflow due to inaccuracies on globalTimePoint = 1.0 */
				if ( segmentIndex >= numSegments )
				{
					segmentIndex = numSegments - 1;
				}

				const float localTimePoint = scaledTime - static_cast< float >(segmentIndex);

				Vector< dim_t, number_t > pointA, pointB, pointC;

				if ( m_closed )
				{
					/* For a closed curve, we loop over the points using the modulo.
					 * The segment 'i' is formed by (midpoint(Pi, Pi+1), Pi+1, midpoint(Pi+1, Pi+2))
					 * For simplicity, we can consider the segment i as (Pi, Pi+1, Pi+2) */
					pointA = m_points[segmentIndex % pointCount];
					pointB = m_points[(segmentIndex + 1) % pointCount];
					pointC = m_points[(segmentIndex + 2) % pointCount];
				}
				else
				{
					/* For an open curve, the segment 'i' is (Pi, Pi+1, Pi+2) */
					pointA = m_points[segmentIndex];
					pointB = m_points[segmentIndex + 1];
					pointC = m_points[segmentIndex + 2];
				}

				/* NOTE: This logic assumes a chain of quadratic Bézier curves where P_i, P_i+1, P_i+2 form a curve.
				 * Another interpretation (Catmull-Rom type) would use the points differently (e.g. P_i+1 is the control point).
				 * The above logic is simple and avoids crashes. */
				return Vector< dim_t, number_t >::quadraticBezierInterpolation(pointA, pointB, pointC, localTimePoint);
			}

			std::vector< Vector< dim_t, number_t > > m_points;
			bool m_closed{false};
	};
}
