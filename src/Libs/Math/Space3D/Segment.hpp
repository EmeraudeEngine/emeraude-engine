/*
 * src/Libs/Math/Space3D/Segment.hpp
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
#include <sstream>
#include <string>
#include <array>

/* Local inclusions. */
#include "Point.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Class for a segment in 3D space.
	 * @tparam precision_t The data precision. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class Segment final
	{
		public:

			/**
			 * @brief Constructs a default segment.
			 */
			constexpr Segment () noexcept = default;

			/**
			 * @brief Constructs a segment from origin to a point.
			 * @param endPoint A reference to a point.
			 */
			explicit
			constexpr
			Segment (const Point< precision_t > & endPoint) noexcept
				: m_points({{}, endPoint})
			{

			}

			/**
			 * @brief Constructs a segment from two points.
			 * @param startPoint A reference to a point.
			 * @param endPoint A reference to a point.
			 */
			constexpr
			Segment (const Point< precision_t > & startPoint, const Point< precision_t > & endPoint) noexcept
				: m_points({startPoint, endPoint})
			{

			}

			/**
			 * @brief Returns whether the segment is valid.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			isValid () const noexcept
			{
				return m_points[0] != m_points[1];
			}

			/**
			 * @brief Returns the point array.
			 * @return std::array< Point< precision_t >, 2 > &
			 */
			[[nodiscard]]
			std::array< Point< precision_t >, 2 > &
			points () noexcept
			{
				return m_points;
			}

			/**
			 * @brief Returns the point array.
			 * @return std::array< Point< precision_t >, 2 > &
			 */
			[[nodiscard]]
			const std::array< Point< precision_t >, 2 > &
			points () const noexcept
			{
				return m_points;
			}

			/**
			 * @brief Sets the starting point of the segment.
			 * @param point A reference to a point.
			 * @return void
			 */
			void
			setStart (const Point< precision_t > & point) noexcept
			{
				m_points[0] = point;
			}

			/**
			 * @brief Sets the ending point of the segment.
			 * @param point A reference to a point.
			 * @return void
			 */
			void
			setEnd (const Point< precision_t > & point) noexcept
			{
				m_points[1] = point;
			}

			/**
			 * @brief Returns the starting point of the segment.
			 * @return const Point< precision_t > &
			 */
			[[nodiscard]]
			const Point< precision_t > &
			startPoint () const noexcept
			{
				return m_points[0];
			}

			/**
			 * @brief Returns the starting point X value of the segment.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			startX () const noexcept
			{
				return m_points[0][X];
			}

			/**
			 * @brief Returns the starting point Y value of the segment.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			startY () const noexcept
			{
				return m_points[0][Y];
			}

			/**
			 * @brief Returns the starting point Z value of the segment.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			startZ () const noexcept
			{
				return m_points[0][Z];
			}

			/**
			 * @brief Returns the ending point of the segment.
			 * @return const Point< precision_t > &
			 */
			[[nodiscard]]
			const Point< precision_t > &
			endPoint () const noexcept
			{
				return m_points[1];
			}

			/**
			 * @brief Returns the ending point X value of the segment.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			endX () const noexcept
			{
				return m_points[1][X];
			}

			/**
			 * @brief Returns the ending point Y value of the segment.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			endY () const noexcept
			{
				return m_points[1][Y];
			}

			/**
			 * @brief Returns the ending point Z value of the segment.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			endZ () const noexcept
			{
				return m_points[1][Z];
			}

			/**
			 * @brief Reset the segment to null value.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_points[0].reset();
				m_points[1].reset();
			}

			/**
			 * @brief Returns the length of the segment.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			getLength () const noexcept
			{
				return Point< precision_t >::distance(m_points[0], m_points[1]);
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Segment & obj) noexcept
			{
				return out << "Segment (3D) data :\n"
					"Start point : " << obj.m_points[0] << "\n"
					"End point : " << obj.m_points[1] << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Segment & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			std::array< Point< precision_t >, 2 > m_points{};
	};
}
