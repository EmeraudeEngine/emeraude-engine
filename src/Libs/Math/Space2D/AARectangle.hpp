/*
 * src/Libs/Math/Space2D/AARectangle.hpp
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
#include <ostream>
#include <sstream>
#include <string>
#include <array>

/* Local inclusions for usages. */
#include "Point.hpp"

namespace EmEn::Libs::Math::Space2D
{
	/**
	 * @brief Class for an axis-aligned rectangle in 2D space.
	 * @note The origin is top-left (Positive Y goes downward, Positive X goes rightward).
	 * @tparam precision_t The precision type. Default float.
	 */
	template< typename precision_t = float >
	requires (std::is_arithmetic_v< precision_t >)
	class AARectangle final
	{
		public:

			static constexpr auto OffsetX{0U};
			static constexpr auto OffsetY{1U};
			static constexpr auto Width{2U};
			static constexpr auto Height{3U};

			/**
			 * @brief Constructs a default rectangle.
			 */
			constexpr AARectangle () noexcept = default;

			/**
			 * @brief Constructs a rectangle with dimensions.
			 * @note Negative width or height values will result in a dimension of 0.
			 * @param width The width of the rectangle.
			 * @param height The height of the rectangle.
			 */
			constexpr
			AARectangle (precision_t width, precision_t height) noexcept
				: m_data({0, 0, width < 0 ? 0 : width, height < 0 ? 0 : height})
			{

			}

			/**
			 * @brief Constructs a rectangle with position and dimensions.
			 * @note Negative width or height values will result in a dimension of 0.
			 * @param positionX The left position of the rectangle.
			 * @param positionY The top position of the rectangle.
			 * @param width The width of the rectangle.
			 * @param height The height of the rectangle.
			 */
			constexpr
			AARectangle (precision_t positionX, precision_t positionY, precision_t width, precision_t height) noexcept
				: m_data({positionX, positionY, width < 0 ? 0 : width, height < 0 ? 0 : height})
			{

			}

			/**
			 * @brief Returns whether the area of another rectangle is bigger.
			 * @param operand A reference to another rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			operator> (const AARectangle< precision_t > & operand) const noexcept
			{
				return this->width() * this->height() > operand.width() * operand.height();
			}

			/**
			 * @brief Returns whether the area of another rectangle is bigger or equal.
			 * @param operand A reference to another rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			operator>= (const AARectangle< precision_t > & operand) const noexcept
			{
				return this->width() * this->height() >= operand.width() * operand.height();
			}

			/**
			 * @brief Returns whether the area of another rectangle is smaller.
			 * @param operand A reference to another rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			operator< (const AARectangle< precision_t > & operand) const noexcept
			{
				return this->width() * this->height() < operand.width() * operand.height();
			}

			/**
			 * @brief Returns whether the area of another rectangle is smaller or equal.
			 * @param operand A reference to another rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			operator<= (const AARectangle< precision_t > & operand) const noexcept
			{
				return this->width() * this->height() <= operand.width() * operand.height();
			}

			/**
			 * @brief Returns whether the area of another rectangle is equal.
			 * @param operand A reference to another rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			operator== (const AARectangle< precision_t > & operand) const noexcept
			{
				return m_data == operand.m_data;
			}

			/**
			 * @brief Returns whether the area of another rectangle is different.
			 * @param operand A reference to another rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			operator!= (const AARectangle< precision_t > & operand) const noexcept
			{
				return !(*this == operand);
			}

			/**
			 * @brief Returns whether the rectangle is a coherent surface.
			 * @return bool
			 */
			[[nodiscard]]
			constexpr
			bool
			isValid () const noexcept
			{
				return this->width() > 0 && this->height() > 0;
			}

			/**
			 * @brief Returns the point array.
			 * @note The layout is  [topLeft, bottomLeft, topRight, bottomRight].
			 * @return std::array< Point< precision_t >, 4 > &
			 */
			[[nodiscard]]
			std::array< Point< precision_t >, 4 >
			points () noexcept
			{
				return {
					this->topLeft(),
					this->bottomLeft(),
					this->topRight(),
					this->bottomRight()
				};
			}

			/**
			 * @brief Sets the left (X-) coordinate of the rectangle.
			 * @param value A X-axis coordinate.
			 * @return void
			 */
			void
			setLeft (precision_t value) noexcept
			{
				m_data[OffsetX] = value;
			}

			/**
			 * @brief Sets the right (X+) coordinate of the rectangle.
			 * @note This will modify the rectangle width.
			 * @warning The right coordinate must be greater than the left coordinate, otherwise the method will ignore the new value.
			 * @param value A X-axis coordinate.
			 * @return void
			 */
			void
			setRight (precision_t value) noexcept
			{
				if ( value > this->left() )
				{
					m_data[Width] = value - this->left();
				}
			}

			/**
			 * @brief Sets the top (Y-) coordinate of the rectangle.
			 * @param value A Y-axis coordinate.
			 * @return void
			 */
			void
			setTop (precision_t value) noexcept
			{
				m_data[OffsetY] = value;
			}

			/**
			 * @brief Sets the bottom (Y+) coordinate of the rectangle.
			 * @note This will modify the rectangle height.
			 * @warning The bottom coordinate must be greater than the top coordinate, otherwise the method will ignore the new value.
			 * @param value A Y-axis coordinate.
			 * @return void
			 */
			void
			setBottom (precision_t value) noexcept
			{
				if ( value > this->top() )
				{
					m_data[Height] = value - this->top();
				}
			}

			/**
			 * @brief Sets the top-left coordinate of the rectangle.
			 * @param position A reference to a vector.
			 * @return void
			 */
			void
			setPosition (const Point< precision_t > & position) noexcept
			{
				m_data[OffsetX] = position.x();
				m_data[OffsetY] = position.y();
			}

			/**
			 * @brief Sets the width of the rectangle from offset X.
			 * @warning The value must be positive, otherwise the method will ignore the new value.
			 * @param value An X-axis size.
			 * @return void
			 */
			void
			setWidth (precision_t value) noexcept
			{
				if ( value > 0 )
				{
					m_data[Width] = value;
				}
			}

			/**
			 * @brief Sets the height of the rectangle from offset Y.
			 * @warning The value must be positive, otherwise the method will ignore the new value.
			 * @param value A Y-axis size.
			 * @return void
			 */
			void
			setHeight (precision_t value) noexcept
			{
				if ( value > 0 )
				{
					m_data[Height] = value;
				}
			}

			/**
			 * @brief Moves the rectangle top-left coordinate by a distance in X and Y.
			 * @param x A distance on X axis.
			 * @param y A distance on Y axis.
			 * @return void
			 */
			void
			move (precision_t x, precision_t y) noexcept
			{
				m_data[OffsetX] += x;
				m_data[OffsetY] += y;
			}

			/**
			 * @brief Modifies the width with a value to add or remove.
			 * @note The result will always be positive or 0.
			 * @param value The width difference.
			 * @return void
			 */
			void
			modifyWidthBy (precision_t value) noexcept
			{
				m_data[Width] += value;

				if ( m_data[Width] < 0 )
				{
					m_data[Width] = 0;
				}
			}

			/**
			 * @brief Modifies the height with a value to add or remove.
			 * @note The result will always be positive or 0.
			 * @param value The height difference.
			 * @return void
			 */
			void
			modifyHeightBy (precision_t value) noexcept
			{
				m_data[Height] += value;

				if ( m_data[Height] < 0 )
				{
					m_data[Height] = 0;
				}
			}

			/**
			 * @brief Returns the left coordinate (X-) of the rectangle.
			 * @return data_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			left () const noexcept
			{
				return m_data[OffsetX];
			}

			/**
			 * @brief Returns the right coordinate (X+) of the rectangle.
			 * @return data_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			right () const noexcept
			{
				return this->left() + this->width();
			}

			/**
			 * @brief Returns the top coordinate (Y-) of the rectangle.
			 * @return data_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			top () const noexcept
			{
				return m_data[OffsetY];
			}

			/**
			 * @brief Returns the bottom coordinate (Y+) of the rectangle.
			 * @return data_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			bottom () const noexcept
			{
				return this->top() + this->height();
			}

			/**
			 * @brief Returns the width of the rectangle.
			 * @return data_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			width () const noexcept
			{
				return m_data[Width];
			}

			/**
			 * @brief Returns the height of the rectangle.
			 * @return data_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			height () const noexcept
			{
				return m_data[Height];
			}

			/**
			 * @brief Returns the top-left coordinate of the rectangle as a vector.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			topLeft () const noexcept
			{
				return {this->left(), this->top()};
			}

			/**
			 * @brief Returns the bottom-left coordinate of the rectangle as a vector.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			bottomLeft () const noexcept
			{
				return {this->left(), this->bottom()};
			}

			/**
			 * @brief Returns the top-right coordinate of the rectangle as a vector.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			topRight () const noexcept
			{
				return {this->right(), this->top()};
			}

			/**
			 * @brief Returns the bottom-right coordinate of the rectangle as a vector.
			 * @return Point< precision_t >
			 */
			[[nodiscard]]
			constexpr
			Point< precision_t >
			bottomRight () const noexcept
			{
				return {this->right(), this->bottom()};
			}

			/**
			 * @brief Resets the rectangle to origin 0,0 and dimension 1x1.
			 * @return void
			 */
			void
			reset () noexcept
			{
				m_data = {0, 0, 1, 1};
			}

			/**
			 * @brief Merges two rectangles to produce a new one.
			 * @param rectangle A reference to another rectangle.
			 * @return void
			 */
			void
			merge (const AARectangle< precision_t > & rectangle) noexcept
			{
				if ( !rectangle.isValid() )
				{
					return;
				}

				if ( !this->isValid() )
				{
					*this = rectangle;

					return;
				}

				const precision_t newLeft = std::min(this->left(), rectangle.left());
				const precision_t newTop = std::min(this->top(), rectangle.top());
				const precision_t newRight = std::max(this->right(), rectangle.right());
				const precision_t newBottom = std::max(this->bottom(), rectangle.bottom());

				m_data[OffsetX] = newLeft;
				m_data[OffsetY] = newTop;
				m_data[Width] = newRight - newLeft;
				m_data[Height] = newBottom - newTop;
			}

			/**
			 * @brief Reduces the sizes of the rectangle.
			 * @param bounds A reference to a rectangle.
			 * @return bool
			 */
			bool
			cropOnOverflow (const AARectangle< precision_t > & bounds) noexcept
			{
				if ( !this->isValid() || !bounds.isValid() )
				{
					return false;
				}

				const precision_t newLeft = std::max(bounds.left(), this->left());
				const precision_t newTop = std::max(bounds.top(), this->top());
				precision_t newRight = std::min(bounds.right(), this->right());
				precision_t newBottom = std::min(bounds.bottom(), this->bottom());

				if ( newRight < newLeft )
				{
					newRight = newLeft;
				}

				if ( newBottom < newTop )
				{
					newBottom = newTop;
				}

				const precision_t newWidth = newRight - newLeft;
				const precision_t newHeight = newBottom - newTop;

				const bool changed =
					m_data[OffsetX] != newLeft ||
					m_data[OffsetY] != newTop ||
					m_data[Width] != newWidth ||
					m_data[Height] != newHeight;

				if ( changed )
				{
					m_data[OffsetX] = newLeft;
					m_data[OffsetY] = newTop;
					m_data[Width] = newWidth;
					m_data[Height] = newHeight;
				}

				return changed;
			}

			/**
			 * @brief Reduces the sizes of the rectangle.
			 * @param width The final width.
			 * @param height The final height.
			 * @return bool
			 */
			bool
			cropOnOverflow (precision_t width, precision_t height) noexcept
			{
				return this->cropOnOverflow({0, 0, width, height});
			}

			/**
			 * @brief Returns the aspect ratio of the surface.
			 * @warning This function will return 0 for an invalid surface.
			 * @return data_t
			 */
			[[nodiscard]]
			precision_t
			aspectRatio () const noexcept
			{
				if ( !this->isValid() )
				{
					return 0;
				}

				return this->width() / this->height();
			}

			/**
			 * @brief Returns the intersection with another rectangle.
			 * @note If not intersection occurs, the rectangle will not change.
			 * @param rectangle A reference to another rectangle.
			 * @return bool
			 */
			bool
			intersect (const AARectangle< precision_t > & rectangle) noexcept
			{
				if ( !this->isValid() || !rectangle.isValid() || this->isOutside(rectangle) )
				{
					return false;
				}

				const precision_t interLeft = std::max(this->left(), rectangle.left());
				const precision_t interTop = std::max(this->top(), rectangle.top());
				const precision_t interRight = std::min(this->right(), rectangle.right());
				const precision_t interBottom = std::min(this->bottom(), rectangle.bottom());

				const precision_t interWidth = interRight - interLeft;
				const precision_t interHeight = interBottom - interTop;

				if ( interWidth > 0 && interHeight > 0 )
				{
					m_data[OffsetX] = interLeft;
					m_data[OffsetY] = interTop;
					m_data[Width] = interWidth;
					m_data[Height] = interHeight;

					return true;
				}

				return false;
			}

			/**
			 * @brief Returns whether the rectangle does not intersect another one.
			 * @param rectangle A reference to another rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOutside (const AARectangle< precision_t > & rectangle) const noexcept
			{
				return
					this->left() > rectangle.right() ||
					rectangle.left() > this->right() ||
					this->top() > rectangle.bottom() ||
					rectangle.top() > this->bottom();
			}

			/**
			 * @brief Returns whether the rectangle does not intersect with a rectangle where coordinates are 0,0.
			 * @param width The width of the rectangle. (x = 0)
			 * @param height The height of the rectangle. (y = 0)
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isOutside (precision_t width, precision_t height) const noexcept
			{
				return
					this->left() >= width ||
					this->right() <= 0 ||
					this->top() >= height ||
					this->bottom() <= 0;
			}

			/**
			 * @brief Returns whether the rectangle intersects another one.
			 * @param rectangle A reference to a rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInside (const AARectangle< precision_t > & rectangle) const noexcept
			{
				if ( this->left() < rectangle.left() )
				{
					return false;
				}

				if ( this->right() > rectangle.right() )
				{
					return false;
				}

				if ( this->top() < rectangle.top() )
				{
					return false;
				}

				if ( this->bottom() > rectangle.bottom() )
				{
					return false;
				}

				return true;
			}

			/**
			 * @brief Returns whether the rectangle intersects with a rectangle where coordinates are 0,0.
			 * @param width The width of the rectangle. (x = 0)
			 * @param height The height of the rectangle. (y = 0)
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isInside (precision_t width, precision_t height) const noexcept
			{
				return this->left() >= 0 && this->right() <= width && this->top() >= 0 && this->bottom() <= height;
			}

			/**
			 * @brief Returns whether the rectangle is intersecting a rectangle.
			 * @param rectangle A reference to a rectangle.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isIntersect (const AARectangle< precision_t > & rectangle) const noexcept
			{
				return !this->isOutside(rectangle);
			}

			/**
			 * @brief Returns whether the rectangle is intersecting a rectangle.
			 * @param width The width of the rectangle. (x = 0)
			 * @param height The height of the rectangle. (y = 0)
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isIntersect (precision_t width, precision_t height) const noexcept
			{
				return !(this->left() >= width  || this->right() <= 0 || this->top() >= height || this->bottom() <= 0);
			}

			/**
			 * @brief Returns the rectangle perimeter.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			getPerimeter () const noexcept
			{
				return (this->width() + this->height()) * 2;
			}

			/**
			 * @brief Returns the rectangle area.
			 * @return precision_t
			 */
			[[nodiscard]]
			constexpr
			precision_t
			getArea () const noexcept
			{
				return this->width() * this->height();
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const AARectangle & obj) noexcept
			{
				out <<
					"Axis-Aligned rectangle data :" "\n"
					"Position (top-left) : X " << obj.left() << ", Y " << obj.top() << "\n"
					"Position (bottom-right) : X " << obj.right() << ", Y " << obj.bottom() << "\n"
					"Dimensions : " << obj.width() << " X " << obj.height() << '\n';

				return out;
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const AARectangle & obj) noexcept
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			std::array< precision_t, 4 > m_data{0, 0, 1, 1};
	};
}
