/*
 * src/Libs/Math/Space3D/AACuboid.hpp
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
#include <cstddef>
#include <cmath>
#include <limits>
#include <ostream>
#include <sstream>
#include <string>
#include <algorithm>

/* Local inclusions for usages. */
#include "Point.hpp"

namespace EmEn::Libs::Math::Space3D
{
	/**
	 * @brief Class for an axis-aligned cuboid in 3D space.
	 * @tparam precision_t The precision type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_floating_point_v< precision_t >)
	class AACuboid
	{
		public:

			/**
			 * @brief Construct a default cuboid.
			 */
			constexpr AACuboid () noexcept = default;

			/**
			 * @brief Constructs a uniform axis aligned bounding box.
			 * @param value The maximal value in every direction.
			 */
			explicit
			constexpr
			AACuboid (precision_t value) noexcept
				: m_maximum(value, value, value),
				m_minimum(-value, -value, -value)
			{

			}

			/**
			 * @brief Constructs an axis aligned bounding box.
			 * @param maximum The highest positive XYZ coordinates of the box.
			 * @param minimum The lowest negative XYZ coordinates of the box.
			 */
			constexpr
			AACuboid (const Point< precision_t > & maximum, const Point< precision_t > & minimum) noexcept
			{
				this->set(maximum, minimum);
			}

			/**
			 * @brief Checks if the box volume consistency.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			isValid () const noexcept
			{
				if ( m_maximum[X] <= m_minimum[X] )
				{
					return false;
				}

				if ( m_maximum[Y] <= m_minimum[Y] )
				{
					return false;
				}

				if ( m_maximum[Z] <= m_minimum[Z] )
				{
					return false;
				}

				return true;
			}

			/**
			 * @brief Returns the point array.
			 * @note The layout is [bottomSouthEast, bottomNorthEast, bottomSouthWest, bottomNorthWest, topSouthEast, topNorthEast, topSouthWest, topNorthWest].
			 * @return std::array< Point< precision_t >, 4 > &
			 */
			[[nodiscard]]
			std::array< Point< precision_t >, 8 >
			points () noexcept
			{
				return {
					this->bottomSouthEast(),
					this->bottomNorthEast(),
					this->bottomSouthWest(),
					this->bottomNorthWest(),
					this->topSouthEast(),
					this->topNorthEast(),
					this->topSouthWest(),
					this->topNorthWest()
				};
			}

			/**
			 * @brief Sets the absolute volume of the box from its center.
			 * @param value The maximal value in every direction.
			 * @return void
			 */
			void
			set (precision_t value) noexcept
			{
				m_maximum[X] = value;
				m_maximum[Y] = value;
				m_maximum[Z] = value;

				m_minimum[X] = -value;
				m_minimum[Y] = -value;
				m_minimum[Z] = -value;
			}

			/**
			 * @brief Sets the volume of the box.
			 * @param maximum The highest positive XYZ coordinates of the box.
			 * @param minimum The lowest negative XYZ coordinates of the box.
			 * @return void
			 */
			void
			set (const Point< precision_t > & maximum, const Point< precision_t > & minimum) noexcept
			{
				if ( maximum[X] >= minimum[X] )
				{
					m_maximum[X] = maximum[X];
					m_minimum[X] = minimum[X];
				}
				else
				{
					m_maximum[X] = minimum[X];
					m_minimum[X] = maximum[X];
				}

				if ( maximum[Y] >= minimum[Y] )
				{
					m_maximum[Y] = maximum[Y];
					m_minimum[Y] = minimum[Y];
				}
				else
				{
					m_maximum[Y] = minimum[Y];
					m_minimum[Y] = maximum[Y];
				}

				if ( maximum[Z] >= minimum[Z] )
				{
					m_maximum[Z] = maximum[Z];
					m_minimum[Z] = minimum[Z];
				}
				else
				{
					m_maximum[Z] = minimum[Z];
					m_minimum[Z] = maximum[Z];
				}
			}

			/**
			 * @brief Returns the highest positive XYZ coordinates of the box.
			 * @return const Point< precision_t > &
			 */
			[[nodiscard]]
			const Point< precision_t > &
			maximum () const noexcept
			{
				return m_maximum;
			}

			/**
			 * @brief Returns the highest positive coordinates of the box on one axis.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			maximum (size_t index) const noexcept
			{
				return m_maximum[index];
			}

			/**
			 * @brief Returns the lowest negative XYZ coordinates of the box.
			 * @return const Point< precision_t > &
			 */
			[[nodiscard]]
			const Point< precision_t > &
			minimum () const noexcept
			{
				return m_minimum;
			}

			/**
			 * @brief Returns the highest lowest negative coordinates of the box on one axis.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			minimum (size_t index) const noexcept
			{
				return m_minimum[index];
			}

			/**
			 * @brief Returns the width of the box (X axis).
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			width () const noexcept
			{
				return m_maximum[X] - m_minimum[X];
			}

			/**
			 * @brief Returns the height of the box (Y axis).
			 * @return v
			 */
			[[nodiscard]]
			precision_t
			height () const noexcept
			{
				return m_maximum[Y] - m_minimum[Y];
			}

			/**
			 * @brief Returns the depth of the box (Z axis).
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			depth () const noexcept
			{
				return m_maximum[Z] - m_minimum[Z];
			}

			/**
			 * @brief Returns the farthest corner from the center of the box.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			farthestPoint () const noexcept
			{
				precision_t distance = 0;

				if ( m_maximum[X] > distance )
				{
					distance = m_maximum[X];
				}

				if ( m_maximum[Y] > distance )
				{
					distance = m_maximum[Y];
				}

				if ( m_maximum[Z] > distance )
				{
					distance = m_maximum[Z];
				}

				if ( std::abs(m_minimum[X]) > distance )
				{
					distance = std::abs(m_minimum[X]);
				}

				if ( std::abs(m_minimum[Y]) > distance )
				{
					distance = std::abs(m_minimum[Y]);
				}

				if ( std::abs(m_minimum[Z]) > distance )
				{
					distance = std::abs(m_minimum[Z]);
				}

				return distance;
			}

			/**
			 * @brief Returns the highest length of every axis.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			highestLength () const noexcept
			{
				auto length = this->width();

				const auto height = this->height();

				if ( height > length )
				{
					length = height;
				}

				const auto depth = this->depth();

				if ( depth > length )
				{
					length = depth;
				}

				return length;
			}

			/**
			 * @brief Returns the point at X+, Y- and Z+.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			bottomSouthEast () const noexcept
			{
				return {m_maximum[X], m_maximum[Y], m_maximum[Z]};
			}

			/**
			 * @brief Returns the point at X+, Y- and Z-.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			bottomNorthEast () const noexcept
			{
				return {m_maximum[X], m_maximum[Y], m_minimum[Z]};
			}

			/**
			 * @brief Returns the point at X-, Y- and Z+.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			bottomSouthWest () const noexcept
			{
				return {m_minimum[X], m_maximum[Y], m_maximum[Z]};
			}

			/**
			 * @brief Returns the point at X-, Y- and Z-.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			bottomNorthWest () const noexcept
			{
				return {m_minimum[X], m_maximum[Y], m_minimum[Z]};
			}

			/**
			 * @brief Returns the point at X+, Y+ and Z+.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			topSouthEast () const noexcept
			{
				return {m_maximum[X], m_minimum[Y], m_maximum[Z]};
			}

			/**
			 * @brief Returns the point at X+, Y+ and Z-.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			topNorthEast () const noexcept
			{
				return {m_maximum[X], m_minimum[Y], m_minimum[Z]};
			}

			/**
			 * @brief Returns the point at X-, Y+ and Z+.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			topSouthWest () const noexcept
			{
				return {m_minimum[X], m_minimum[Y], m_maximum[Z]};
			}

			/**
			 * @brief Returns the point at X-, Y+ and Z-.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			topNorthWest () const noexcept
			{
				return {m_minimum[X], m_minimum[Y], m_minimum[Z]};
			}

			/**
			 * @brief Returns the center of the bounding box.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			centroid () const noexcept
			{
				return (m_maximum + m_minimum) * static_cast< precision_t >(0.5);
			}

			/**
			 * @brief Reset the cuboid to null value.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_maximum[X] = std::numeric_limits< precision_t >::lowest();
				m_maximum[Y] = std::numeric_limits< precision_t >::lowest();
				m_maximum[Z] = std::numeric_limits< precision_t >::lowest();

				m_minimum[X] = std::numeric_limits< precision_t >::max();
				m_minimum[Y] = std::numeric_limits< precision_t >::max();
				m_minimum[Z] = std::numeric_limits< precision_t >::max();
			}

			/**
			 * @brief Extends the volume of this box with another one.
			 * @param other A reference to a cuboid.
			 * @return void
			 */
			void
			merge (const AACuboid & other) noexcept
			{
				/* If the other box is the same or is invalid, we skip it. */
				if ( this == &other || !other.isValid() )
				{
					return;
				}

				/* If THIS box is invalid, we simply
				 * copy the other over it. */
				if ( !this->isValid() )
				{
					m_maximum = other.m_maximum;
					m_minimum = other.m_minimum;

					return;
				}

				/* Compare and processLogics every axis boundary. */
				if ( other.m_maximum[X] > m_maximum[X] )
				{
					m_maximum[X] = other.m_maximum[X];
				}

				if ( other.m_minimum[X] < m_minimum[X] )
				{
					m_minimum[X] = other.m_minimum[X];
				}

				if ( other.m_maximum[Y] > m_maximum[Y] )
				{
					m_maximum[Y] = other.m_maximum[Y];
				}

				if ( other.m_minimum[Y] < m_minimum[Y] )
				{
					m_minimum[Y] = other.m_minimum[Y];
				}

				if ( other.m_maximum[Z] > m_maximum[Z] )
				{
					m_maximum[Z] = other.m_maximum[Z];
				}

				if ( other.m_minimum[Z] < m_minimum[Z] )
				{
					m_minimum[Z] = other.m_minimum[Z];
				}
			}

			/**
			 * @brief Extends the volume with a point.
			 * @param point A reference to a vector.
			 * @return void
			 */
			void
			merge (const Point< precision_t > & point) noexcept
			{
				this->mergeX(point[X]);

				this->mergeY(point[Y]);

				this->mergeZ(point[Z]);
			}

			/**
			 * @brief Extends the volume with a point.
			 * @param point A reference to a vector.
			 * @return void
			 */
			void
			merge (const Vector< 4, precision_t > & point) noexcept
			{
				this->mergeX(point[X]);

				this->mergeY(point[Y]);

				this->mergeZ(point[Z]);
			}

			/**
			 * @brief Merge a value on X.
			 * @param value The distance in X.
			 * @return void
			 */
			void
			mergeX (precision_t value) noexcept
			{
				if ( value > m_maximum[X] )
				{
					m_maximum[X] = value;
				}

				if ( value < m_minimum[X] )
				{
					m_minimum[X] = value;
				}
			}

			/**
			 * @brief Merge a value on Y.
			 * @param value The distance in Y.
			 * @return void
			 */
			void
			mergeY (precision_t value) noexcept
			{
				if ( value > m_maximum[Y] )
				{
					m_maximum[Y] = value;
				}

				if ( value < m_minimum[Y] )
				{
					m_minimum[Y] = value;
				}
			}

			/**
			 * @brief Merge a value on Z.
			 * @param value The distance in Z.
			 * @return void
			 */
			void
			mergeZ (precision_t value) noexcept
			{
				if ( value > m_maximum[Z] )
				{
					m_maximum[Z] = value;
				}

				if ( value < m_minimum[Z] )
				{
					m_minimum[Z] = value;
				}
			}

			/**
			 * @brief Returns the cuboid volume.
			 * @return precision_t
			 */
			[[nodiscard]]
			precision_t
			getVolume () const noexcept
			{
				return this->width() * this->height() * this->depth();
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const AACuboid & obj)
			{
				return out <<
					"Axis-Aligned AACuboid volume data :\n"
					"Maximum : " << obj.m_maximum << "\n"
					"Minimum : " << obj.m_minimum << '\n';
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const AACuboid & obj)
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			Point< precision_t > m_maximum{std::numeric_limits< precision_t >::lowest(), std::numeric_limits< precision_t >::lowest(), std::numeric_limits< precision_t >::lowest()};
			Point< precision_t > m_minimum{std::numeric_limits< precision_t >::max(), std::numeric_limits< precision_t >::max(), std::numeric_limits< precision_t >::max()};
	};
}
