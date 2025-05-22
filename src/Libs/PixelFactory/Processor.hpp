/*
 * src/Libs/PixelFactory/Processor.hpp
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
#include <cstdint>
#include <cstring>
#include <cmath>
#include <limits>
#include <type_traits>

/* Local inclusions. */
#include "Libs/Math/Space2D/Intersections/LineLine.hpp"
#include "Libs/Math/Space2D/Segment.hpp"
#include "Libs/Math/Space2D/Circle.hpp"
#include "Pixmap.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief The pixmap processor is used to performs operations to a targeted pixmap.
	 * @TODO Use swap buffer to optimize some of operations.
	 * @note For further addition look with: https://github.com/podgorskiy/EnvMapTooL
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class Processor final
	{
		public:

			/**
			 * @brief Constructs a processor on a pixmap.
			 * @param target A reference to a target pixmap.
			 */
			explicit
			Processor (Pixmap< pixel_data_t, dimension_t > & target) noexcept
				: m_target(target)
			{

			}

			/**
			 * @brief Multiplies pixels value by a scalar.
			 * @tparam scale_t The type of scalar. Default float.
			 * @tparam color_data_t The color data type. Default float.
			 * @param scalar A scalar value.
			 * @return bool
			 */
			template< typename scale_t = float, typename color_data_t = float >
			bool
			scaleValue (scale_t scalar) noexcept requires (std::is_floating_point_v< scale_t >)
			{
				if ( !m_target.isValid() )
				{
					return false;
				}

				m_target.template forEachPixel< color_data_t >([scalar] (Color< color_data_t > & pixel) {
					pixel *= scalar;

					return true;
				});

				m_target.markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Multiplies pixels sub-channel value by a scalar.
			 * @tparam scale_t The type of scalar. Default float.
			 * @tparam color_data_t The color data type. Default float.
			 * @param scalar A scalar value.
			 * @param channel The channel.
			 * @return bool
			 */
			template< typename scale_t = float, typename color_data_t = float >
			bool
			scaleValue (scale_t scalar, Channel channel) noexcept requires (std::is_floating_point_v< scale_t >)
			{
				if ( !m_target.isValid() )
				{
					return false;
				}

				m_target.template forEachPixel< color_data_t >([scalar, channel] (Color< color_data_t > & pixel) {
					switch ( channel )
					{
						case Channel::Red :
							pixel.setRed(pixel.red() * scalar);
							break;

						case Channel::Green :
							pixel.setGreen(pixel.green() * scalar);
							break;

						case Channel::Blue :
							pixel.setBlue(pixel.blue() * scalar);
							break;

						case Channel::Alpha :
							pixel.setAlpha(pixel.alpha() * scalar);
							break;
					}

					return true;
				});

				m_target.markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Draws a line (Bresenham algorithm) between two points on the pixmap.
			 * @todo Check for "Cohen-Sutherland" or "Liang-Barsky" algorithms for speed efficiency.
			 * @tparam vector_data_t The vector data type. Default int32_t.
			 * @tparam color_data_t The color data type. Default float.
			 * @param pointA A reference to a vector for the start coordinates of the line.
			 * @param pointB A reference to a vector for the end coordinates of the line.
			 * @param color A reference to a color.
			 * @param mode The drawing mode. Default DrawPixelMode::Replace.
			 * @return bool
			 */
			template< typename vector_data_t = int32_t, typename color_data_t = float >
			bool
			drawSegment (Math::Vector< 2, vector_data_t > pointA, Math::Vector< 2, vector_data_t > pointB, const Color< color_data_t > & color, DrawPixelMode mode = DrawPixelMode::Replace) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !m_target.isValid() )
				{
					return false;
				}

				const auto width = static_cast< vector_data_t >(m_target.width());
				const auto height = static_cast< vector_data_t >(m_target.height());

				/* Checks if the segment is inside the Pixmap. */
				if
				(
					( pointA[Math::X] < 0 && pointB[Math::X] < 0 ) || ( pointA[Math::X] > width && pointB[Math::X] > width ) ||
					( pointA[Math::Y] < 0 && pointB[Math::Y] < 0 ) || ( pointA[Math::Y] > height && pointB[Math::Y] > height )
				)
				{
					return false;
				}

				if ( !this->clampSegmentFirstPoint(pointA, pointB) )
				{
					return false;
				}

				if ( !this->clampSegmentFirstPoint(pointB, pointA) )
				{
					return false;
				}

				const auto steep = std::abs(pointB[Math::Y] - pointA[Math::Y]) > std::abs(pointB[Math::X] - pointA[Math::X]);

				if ( steep )
				{
					pointA.swapAxis();
					pointB.swapAxis();
				}

				if ( pointA[Math::X] > pointB[Math::X] )
				{
					pointA.swap(pointB);
				}

				const auto deltaX = pointB[Math::X] - pointA[Math::X];
				const auto deltaY = std::abs(pointB[Math::Y] - pointA[Math::Y]);

				auto error = static_cast< float >(deltaX) / 2.0F;
				const auto yStep = (pointA[Math::Y] < pointB[Math::Y] ) ? 1 : -1;
				auto y = pointA[Math::Y];

				for ( auto x = pointA[Math::X]; x <= pointB[Math::X]; ++x )
				{
					if ( steep )
					{
						m_target.blendFreePixel(static_cast< dimension_t >(y), static_cast< dimension_t >(x), color, mode);
					}
					else
					{
						m_target.blendFreePixel(static_cast< dimension_t >(x), static_cast< dimension_t >(y), color, mode);
					}

					error -= deltaY;

					if ( error < 0 )
					{
						y += yStep;
						error += deltaX;
					}
				}

				return true;
			}

			/**
			 * @brief Draws a line between two points on the pixmap.
			 * @tparam segment_data_t The segment data type. Default int32_t.
			 * @tparam color_data_t The color data type. Default float.
			 * @param segment A reference to a segment.
			 * @param color A reference to a color.
			 * @param mode The drawing mode. Default DrawPixelMode::Replace.
			 * @return bool
			 */
			template< typename segment_data_t = int32_t, typename color_data_t = float >
			bool
			drawSegment (const Math::Space2D::Segment< segment_data_t > & segment, const Color< color_data_t > & color, DrawPixelMode mode = DrawPixelMode::Replace) noexcept requires (std::is_floating_point_v< segment_data_t >)
			{
				return this->drawSegment(segment.start(), segment.end(), color, mode);
			}

			/**
			 * @brief Draws a circle on the pixmap.
			 * @todo Check boundary before calling blendFreePixel() and use blendPixel().
			 * @tparam color_data_t The color data type. Default float.
			 * @param center The center of the circle.
			 * @param radius The radius of the circle.
			 * @param color A reference to a color.
			 * @param mode The drawing mode. Default DrawPixelMode::Replace.
			 * @return bool
			 */
			template< typename vector_data_t = int32_t, typename color_data_t = float >
			bool
			drawCircle (const Math::Vector< 2, vector_data_t > & center, vector_data_t radius, const Color< color_data_t > & color, DrawPixelMode mode = DrawPixelMode::Replace) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !m_target.isValid() )
				{
					return false;
				}

				vector_data_t x = 0;
				vector_data_t y = radius;
				vector_data_t delta = radius - 1;

				while ( y >= x )
				{
					const auto signedX = static_cast< int32_t >(x);
					const auto signedY = static_cast< int32_t >(y);

					/* Quadrant 1 */
					m_target.blendFreePixel(signedX + center.x(), signedY + center.y(), color, mode);
					m_target.blendFreePixel(signedY + center.x(), signedX + center.y(), color, mode);

					/* Quadrant 2 */
					m_target.blendFreePixel(-signedX + center.x(), signedY + center.y(), color, mode);
					m_target.blendFreePixel(-signedY + center.x(), signedX + center.y(), color, mode);

					/* Quadrant 3 */
					m_target.blendFreePixel(signedX + center.x(), -signedY + center.y(), color, mode);
					m_target.blendFreePixel(signedY + center.x(), -signedX + center.y(), color, mode);

					/* Quadrant 4 */
					m_target.blendFreePixel(-signedX + center.x(), -signedY + center.y(), color, mode);
					m_target.blendFreePixel(-signedY + center.x(), -signedX + center.y(), color, mode);

					if ( delta >= 2 * x )
					{
						delta -= (2 * x) - 1;

						++x;
					}
					else if ( delta <= 2 * (radius - y) )
					{
						delta += (2 * y) - 1;

						--y;
					}
					else
					{
						delta += 2 * (y - x - 1);

						--y;
						++x;
					}
				}

				return true;
			}

			/**
			 * @brief Draws a circle on the pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param circle A reference to a circle.
			 * @param color A reference to a color.
			 * @param mode The drawing mode. Default DrawPixelMode::Replace.
			 * @return bool
			 */
			template< typename color_data_t = float >
			bool
			drawCircle (const Math::Space2D::Circle< float > & circle, const Color< color_data_t > & color, DrawPixelMode mode = DrawPixelMode::Replace) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				return this->drawCircle(circle.position(), static_cast< size_t >(circle.radius()), color, mode);
			}

			/**
			 * @brief Draws a square on the pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param rectangle A reference to a rectangle.
			 * @param color A reference to a color.
			 * @param mode The drawing mode. Default DrawPixelMode::Replace.
			 * @return bool
			 */
			template< typename color_data_t = float >
			bool
			drawSquare (const Math::Space2D::AARectangle< int32_t > & rectangle, const Color< color_data_t > & color, DrawPixelMode mode = DrawPixelMode::Replace) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !this->checkPixmapClipping(m_target, rectangle) )
				{
					return false;
				}

				this->drawSegment(rectangle.bottomLeft(), rectangle.bottomRight(), color, mode);
				this->drawSegment(rectangle.bottomRight(), rectangle.topRight(), color, mode);
				this->drawSegment(rectangle.topLeft(), rectangle.topRight(), color, mode);
				this->drawSegment(rectangle.topLeft(), rectangle.bottomLeft(), color, mode);

				return true;
			}

			/**
			 * @brief Draws a cross on the pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param rectangle A reference to a rectangle.
			 * @param color A reference to a color.
			 * @param mode The drawing mode. Default DrawPixelMode::Replace.
			 * @return bool
			 */
			template< typename color_data_t = float >
			bool
			drawCross (const Math::Space2D::AARectangle< int32_t > & rectangle, const Color< color_data_t > & color, DrawPixelMode mode = DrawPixelMode::Replace) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !this->checkPixmapClipping(m_target, rectangle) )
				{
					return false;
				}

				this->drawSegment(rectangle.topLeft(), rectangle.bottomRight(), color, mode);
				this->drawSegment(rectangle.bottomLeft(), rectangle.topRight(), color, mode);

				return true;
			}

			/**
			 * @brief Draws a straight cross on the pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param rectangle A reference to a rectangle.
			 * @param color A reference to a color.
			 * @param mode The drawing mode. Default DrawPixelMode::Replace.
			 * @return bool
			 */
			template< typename color_data_t = float >
			bool
			drawStraightCross (const Math::Space2D::AARectangle< int32_t > & rectangle, const Color< color_data_t > & color, DrawPixelMode mode = DrawPixelMode::Replace) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !this->checkPixmapClipping(m_target, rectangle) )
				{
					return false;
				}

				const auto midX = rectangle.left() + (rectangle.width() / 2);
				const auto midY = rectangle.top() + (rectangle.height() / 2);

				this->drawSegment({midX, rectangle.top()}, {midX, rectangle.bottom()}, color, mode);
				this->drawSegment({rectangle.left(), midY}, {rectangle.right(), midY}, color, mode);

				return true;
			}

			/**
			 * @brief Moves pixels into a direction.
			 * @note Positive X value will move pixels to the left while positive Y value will move pixels upward.
			 * @param xDirection An integer to move pixels in X-Axis.
			 * @param yDirection An integer to move pixels in Y-Axis.
			 * @return bool
			 */
			bool
			move (int32_t xDirection, int32_t yDirection) noexcept
			{
				if ( !m_target.isValid() )
				{
					return false;
				}

				/* NOTE: No movement at all! */
				if ( xDirection == 0 && yDirection == 0 )
				{
					return false;
				}

				this->prepareSwapBuffer();

				/* NOTE: The shift is completely moving the pixmap outside the canvas in X-Axis or Y-axis. */
				if ( std::abs(xDirection) < m_target.width() && std::abs(yDirection) < m_target.height() )
				{
					const auto rowSize = m_target.pitch();
					const auto xShift = xDirection * static_cast< int32_t >(m_target.colorCount());

					/* NOTE: Defines chunk sizes to move. */
					const auto xLimit = rowSize - std::abs(xShift);
					const auto yLimit = m_target.height() - std::abs(yDirection);

					size_t src = 0;
					size_t dst = 0;

					for ( size_t rowIndex = 0; rowIndex < yLimit; rowIndex++ )
					{
						if ( yDirection > 0 )
						{
							src = rowIndex;
							dst = (rowIndex + yDirection) * rowSize;
						}
						else
						{
							src = rowIndex - yDirection;
							dst = rowIndex  * rowSize;
						}

						if ( xShift > 0 )
						{
							std::memcpy(m_swapBuffer.data() + dst + xShift, m_target.rowPointer(src), xLimit);
						}
						else
						{
							std::memcpy(m_swapBuffer.data() + dst, m_target.rowPointer(src) - xShift, xLimit);
						}
					}
				}

				m_target.markEverythingUpdated();

				return this->swapBuffers();
			}

			/**
			 * @brief Moves pixels into a direction.
			 * @note Positive X value will move pixels to the left while positive Y value will move pixels upward.
			 * @param direction A reference to a 2D vector.
			 * @return bool
			 */
			bool
			move (const Math::Vector< 2, int32_t > & direction) noexcept
			{
				return this->move(direction[Math::X], direction[Math::Y]);
			}

			/**
			 * @brief Shift the pixmap into a direction. Same as Processor::move() with pixel wrapping.
			 * @note Positive X value will move pixels to the left while positive Y value will move pixels upward.
			 * @param xDirection An integer to move pixels in X-Axis.
			 * @param yDirection An integer to move pixels in Y-Axis.
			 * @return bool
			 */
			bool
			shift (int32_t xDirection, int32_t yDirection) noexcept
			{
				if ( !m_target.isValid() )
				{
					return false;
				}

				/* NOTE: No movement at all! */
				if ( xDirection == 0 && yDirection == 0 )
				{
					return true;
				}

				/* NOTE: Remove the shifting round trip. */
				xDirection =
					xDirection < 0 ?
					xDirection % -m_target.width() :
					xDirection % m_target.width();
				yDirection =
					yDirection < 0 ?
					yDirection % -m_target.height() :
					yDirection % m_target.height();

				/* NOTE: Complete shift in both directions. */
				if ( xDirection == 0 && yDirection == 0 )
				{
					return true;
				}

				this->prepareSwapBuffer();

				const auto rowSize = m_target.pitch();
				const auto xShift = xDirection * m_target.colorCount();

				/* NOTE: Defines chunk sizes to move. */
				const auto rowCopySizeA = rowSize - xShift;
				const auto rowCopySizeB = rowSize - rowCopySizeA;

				size_t dstRowIndex = std::abs(yDirection);

				for ( size_t rowIndex = 0; rowIndex < m_target.height(); rowIndex++ )
				{
					auto dst = dstRowIndex * rowSize;

					std::memcpy(m_swapBuffer.data() + dst, m_target.rowPointer(rowIndex) + rowCopySizeA, rowCopySizeB);

					dst += rowCopySizeB;

					std::memcpy(m_swapBuffer.data() + dst, m_target.rowPointer(rowIndex), rowCopySizeA);

					/* NOTE: Next row on the destination and loop if requested. */
					dstRowIndex++;

					if ( dstRowIndex >= m_target.height() )
					{
						dstRowIndex = 0;
					}
				}

				m_target.markEverythingUpdated();

				return this->swapBuffers();
			}

			/**
			 * @brief Shift the pixmap into a direction. Same as Processor::move() with pixel wrapping.
			 * @note Positive X value will move pixels to the left while positive Y value will move pixels upward.
			 * @param direction A reference to a 2D vector.
			 * @return bool
			 */
			bool
			shift (const Math::Vector< 2, int32_t > & direction) noexcept
			{
				return this->shift(direction[Math::X], direction[Math::Y]);
			}

			/**
			 * @brief Moves pixels upward or downward for a console-like text area.
			 * @param distance The distance of moving in pixel.
			 * @return bool
			 */
			bool
			shiftTextArea (int32_t distance) noexcept
			{
				if ( !m_target.isValid() )
				{
					return false;
				}

				/* NOTE: Not moving at all! */
				if ( distance == 0 )
				{
					return true;
				}

				this->prepareSwapBuffer();

				const auto rowSize = m_target.pitch();
				const auto bufferSize = (m_target.height() - std::abs(distance)) * rowSize;

				if ( distance > 0 )
				{
					std::memcpy(m_swapBuffer.data() + (distance * rowSize), m_target.rowPointer(0UL), bufferSize);
				}
				else
				{
					std::memcpy(m_swapBuffer.data(), m_target.rowPointer(std::abs(distance)), bufferSize);
				}

				m_target.markEverythingUpdated();

				return this->swapBuffers();
			}

			/**
			 * @brief Copies a clipped area of the source pixmap to a clipped area of the target pixmap.
			 * @warning Pixmap involved must have the same color count.
			 * @param source A reference to a source pixmap.
			 * @param sourceClip A reference to a rectangle on the source pixmap.
			 * @param destinationClip A reference to a rectangle on the target pixmap.
			 * @return bool
			 */
			bool
			blit (const Pixmap< pixel_data_t, dimension_t > & source, const Math::Space2D::AARectangle< dimension_t > & sourceClip, const Math::Space2D::AARectangle< dimension_t > & destinationClip) const noexcept
			{
				if ( !Processor::checkPixmapClipping(source, sourceClip) && !Processor::checkPixmapClipping(m_target, destinationClip) )
				{
					return false;
				}

				/* NOTE: If both pixmap have the same channel mode, we can perform the blit operation! */
				if ( source.channelMode() != m_target.channelMode() )
				{
					return false;
				}

				const auto pixelSize = m_target.colorCount() * sizeof(pixel_data_t);
				const auto rowSize = destinationClip.width() * pixelSize;

				for ( size_t destinationY = 0; destinationY < destinationClip.height(); destinationY++ )
				{
					const auto sourceIndex = ((sourceClip.top() + destinationY) * source.width() + sourceClip.left()) * pixelSize;
					const auto destinationIndex = ((destinationClip.top() + destinationY) * m_target.width() + destinationClip.left()) * pixelSize;

					std::memcpy(m_target.data().data() + destinationIndex, source.data().data() + sourceIndex, rowSize);
				}

				m_target.markRectangleUpdated(destinationClip);

				return true;
			}

			/**
			 * @brief Copies the source pixmap to the target pixmap.
			 * @param source A reference to a source pixmap.
			 */
			bool
			blit (const Pixmap< pixel_data_t, dimension_t > & source) const noexcept
			{
				const auto sourceArea = source.area();

				return this->blit(source, sourceArea, sourceArea);
			}

			/**
			 * @brief Copies the source pixmap to a clipped area of the target pixmap.
			 * @warning Pixmap involved must have the same color count.
			 * @param source A reference to a source pixmap.
			 * @param destinationClip A reference to a rectangle on the target pixmap.
			 * @return bool
			 */
			bool
			blit (const Pixmap< pixel_data_t, dimension_t > & source, const Math::Space2D::AARectangle< dimension_t > & destinationClip) const noexcept
			{
				return this->blit(source, {source.width(), source.height()}, destinationClip);
			}

			/**
			 * @brief Copies raw data to a clipped area of the target pixmap.
			 * @warning Be sure to have enough data in the pointer.
			 * @param rawData A reference to a structure of raw data.
			 * @param sourceClip A reference to a rectangle on the source pixmap.
			 * @param destinationClip A reference to a rectangle on the target pixmap.
			 * @return bool
			 */
			bool
			blit (const RawPixmapData< pixel_data_t > & rawData, const Math::Space2D::AARectangle< dimension_t > & sourceClip, const Math::Space2D::AARectangle< dimension_t > & destinationClip) const noexcept
			{
				if ( sourceClip.isOutside(rawData.width, rawData.height) )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "The clipping area is outside the pixmap !" "\n";
					}

					return false;
				}

				if ( !Processor::checkPixmapClipping(m_target, destinationClip) )
				{
					return false;
				}

				/* NOTE: If both pixmap have the same channel mode, we can perform the blit operation! */
				if ( rawData.colorCount != m_target.colorCount() )
				{
					return false;
				}

				const auto pixelSize = m_target.colorCount() * sizeof(pixel_data_t);
				const auto rowSize = destinationClip.width() * pixelSize;

				for ( size_t destinationY = 0; destinationY < destinationClip.height(); destinationY++ )
				{
					const auto sourceIndex = ((sourceClip.top() + destinationY) * rawData.width + sourceClip.left()) * pixelSize;
					const auto destinationIndex = ((destinationClip.top() + destinationY) * m_target.width() + destinationClip.left()) * pixelSize;

					std::memcpy(m_target.data().data() + destinationIndex, rawData.pointer + sourceIndex, rowSize);
				}

				m_target.markRectangleUpdated(destinationClip);

				return true;
			}

			/**
			 * @brief Copies raw data to a clipped area of the target pixmap.
			 * @warning Be sure to have enough data in the pointer.
			 * @param rawData A reference to a structure of raw data.
			 * @param clip A reference to a rectangle on the target pixmap. This will be used as the source clip and the destination clip.
			 * @return bool
			 */
			bool
			blit (const RawPixmapData< pixel_data_t > & rawData, const Math::Space2D::AARectangle< dimension_t > & clip) const noexcept
			{
				return this->blit(rawData, clip, clip);
			}

			/**
			 * @brief Copies a clipped area of the source pixmap to a clipped area of the target pixmap.
			 * @param source A reference to a source pixmap.
			 * @param sourceClip A reference to a rectangle on the source pixmap.
			 * @param destinationClip A reference to a rectangle on the target pixmap.
			 * @param mode Set the copy technic. Default: Replace.
			 * @param opacity A global opacity. Default 1.0.
			 * @return bool
			 */
			bool
			copy (const Pixmap< pixel_data_t, dimension_t > & source, Math::Space2D::AARectangle< dimension_t > sourceClip, Math::Space2D::AARectangle< dimension_t > destinationClip, DrawPixelMode mode = DrawPixelMode::Replace, float opacity = 1.0F) const noexcept
			{
				/* NOTE: Check if we can replace the copy operation with a blit operation (much faster). */
				if (
					mode == DrawPixelMode::Replace &&
					source.channelMode() != m_target.channelMode() &&
					sourceClip.width() != destinationClip.width() &&
					sourceClip.height() != destinationClip.height()
				)
				{
					return this->blit(source, sourceClip, destinationClip);
				}

				if ( !Processor::checkPixmapClipping(source, sourceClip) && !Processor::checkPixmapClipping(m_target, destinationClip) )
				{
					return false;
				}

				for ( size_t y = 0; y < destinationClip.height(); y++ )
				{
					const auto sourceY = sourceClip.top() + y;
					const auto destinationY = destinationClip.top() + y;

					for ( size_t x = 0; x < destinationClip.width(); x++ )
					{
						const auto sourceX = sourceClip.left() + x;
						const auto destinationX = destinationClip.left() + x;

						m_target.blendPixel(destinationX, destinationY, source.pixel(sourceX, sourceY), mode, opacity);
					}
				}

				m_target.markRectangleUpdated(destinationClip);

				return true;
			}

			/**
			 * @brief Copies the source pixmap to a clipped area of the target pixmap.
			 * @note The source pixmap will be used from the top-left corner.
			 * @param source A reference to a source pixmap.
			 * @param mode Set the copy technic. Default: Replace.
			 * @param opacity A global opacity. Default 1.0.
			 * @return bool
			 */
			bool
			copy (const Pixmap< pixel_data_t, dimension_t > & source, DrawPixelMode mode = DrawPixelMode::Replace, float opacity = 1.0F) const noexcept
			{
				const auto sourceClip = source.template rectangle< dimension_t >();

				return this->copy(source, sourceClip, sourceClip, mode, opacity);
			}

			/**
			 * @brief Copies the source pixmap to a clipped area of the target pixmap.
			 * @note The source pixmap will be used from the top-left corner.
			 * @param source A reference to a source pixmap.
			 * @param destinationClip A reference to a rectangle on the target pixmap.
			 * @param mode Set the copy technic. Default: Replace.
			 * @param opacity A global opacity. Default 1.0.
			 * @return bool
			 */
			bool
			copy (const Pixmap< pixel_data_t, dimension_t > & source, const Math::Space2D::AARectangle< dimension_t > & destinationClip, DrawPixelMode mode = DrawPixelMode::Replace, float opacity = 1.0F) const noexcept
			{
				return this->copy(source, source.area(), destinationClip, mode, opacity);
			}

			/**
			 * @brief Copies the source pixmap to a clipped area of the target pixmap.
			 * @note The source pixmap will be used from the top-left corner.
			 * @param source A reference to a source pixmap.
			 * @param xPosition The position in X where to execute the copy.
			 * @param yPosition The position in Y where to execute the copy.
			 * @param mode Set the copy technic. Default: Replace.
			 * @param opacity A global opacity. Default 1.0.
			 * @return bool
			 */
			bool
			copy (const Pixmap< pixel_data_t, dimension_t > & source, int32_t xPosition, int32_t yPosition, DrawPixelMode mode = DrawPixelMode::Replace, float opacity = 1.0F) const noexcept
			{
				Math::Space2D::AARectangle< dimension_t > sourceClip;
				Math::Space2D::AARectangle< dimension_t > destinationClip;

				if ( xPosition < 0 )
				{
					const auto shiftSize = static_cast< size_t >(std::abs(xPosition));

					if ( shiftSize >= source.width() )
					{
						if constexpr ( IsDebug )
						{
							std::cerr << "The source pixmap do not lie in target pixmap on X axis !" "\n";
						}

						return false;
					}

					const auto newWidth = source.width() - shiftSize;

					sourceClip.setLeft(shiftSize);
					sourceClip.setWidth(newWidth);

					destinationClip.setWidth(newWidth);
				}
				else
				{
					sourceClip.setWidth(source.width());

					destinationClip.setLeft(static_cast< size_t >(xPosition));
					destinationClip.setWidth(source.width());
				}

				if ( yPosition < 0 )
				{
					const auto shiftSize = static_cast< size_t >(std::abs(yPosition));

					if ( shiftSize >= source.height() )
					{
						if constexpr ( IsDebug )
						{
							std::cerr << "The source pixmap do not lie in target pixmap on Y axis !" "\n";
						}

						return false;
					}

					const auto newHeight = source.height() - shiftSize;

					sourceClip.setTop(shiftSize);
					sourceClip.setHeight(newHeight);

					destinationClip.setHeight(newHeight);
				}
				else
				{
					sourceClip.setHeight(source.height());

					destinationClip.setTop(static_cast< size_t >(yPosition));
					destinationClip.setHeight(source.height());
				}

				return this->copy(source, sourceClip, destinationClip, mode, opacity);
			}

			/**
			 * @brief Copies the source pixmap to a clipped area of the target pixmap.
			 * @note The source pixmap will be used from the top-left corner.
			 * @param source A reference to a source pixmap.
			 * @param position A reference to a vector.
			 * @param mode Set the copy technic. Default: Replace.
			 * @param opacity A global opacity. Default 1.0.
			 * @return bool
			 */
			bool
			copy (const Pixmap< pixel_data_t, dimension_t > & source, const Math::Vector< 2, int32_t > & position, DrawPixelMode mode = DrawPixelMode::Replace, float opacity = 1.0F) const noexcept
			{
				return this->copy(source, source.area(), position.x(), position.y(), mode, opacity);
			}

			/**
			 * @brief Copies a colored area to the target pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param color A reference to a color.
			 * @param clip A reference to a rectangle.
			 * @param mode Set the copy technic. Default: Replace.
			 * @param opacity A global opacity. Default 1.0.
			 * @return bool
			 */
			template< typename color_data_t = float >
			bool
			copy (const Color< color_data_t > & color, const Math::Space2D::AARectangle< dimension_t > & clip, DrawPixelMode mode = DrawPixelMode::Replace, float opacity = 1.0F) const noexcept
			{
				if ( !Processor::checkPixmapClipping(m_target, clip) )
				{
					return false;
				}

				for ( dimension_t y = 0; y < clip.height(); ++y )
				{
					const auto destinationY = clip.top() + y;

					for ( dimension_t x = 0; x < clip.width(); ++x )
					{
						const auto destinationX = clip.left() + x;

						m_target.blendPixel(destinationX, destinationY, color, mode, opacity);
					}
				}

				m_target.markRectangleUpdated(clip);

				return true;
			}

			/**
			 * @brief Copies with a stencil mask a clipped area of the source pixmap to a clipped area of the target pixmap.
			 * @note The source pixmap will be used from the top-left corner.
			 * @warning The mask pixmap must be grayscale.
			 * @param source A reference to a source pixmap.
			 * @param sourceClip A reference to a rectangle on the source pixmap.
			 * @param destinationClip A reference to a rectangle on the target pixmap.
			 * @param mask A reference to a pixmap to act as the stencil mask.
			 * @param mode Set the copy technic. Default: Replace.
			 * @return bool
			 */
			bool
			stencil (const Pixmap< pixel_data_t, dimension_t > & source, Math::Space2D::AARectangle< dimension_t > sourceClip, Math::Space2D::AARectangle< dimension_t > destinationClip, const Pixmap< pixel_data_t, dimension_t > & mask, DrawPixelMode mode = DrawPixelMode::Replace) const noexcept
			{
				if ( !mask.isValid() || !mask.isGrayScale() || (!Processor::checkPixmapClipping(source, sourceClip) && !Processor::checkPixmapClipping(m_target, destinationClip)) )
				{
					return false;
				}

				const auto rowPixelCount = destinationClip.width();

				for ( size_t row = 0; row < destinationClip.height(); row++ )
				{
					const auto sourceIndex = (((sourceClip.top() + row) * source.width()) + sourceClip.left());
					const auto destinationIndex = (((destinationClip.top() + row) * m_target.width()) + destinationClip.left());

					/* FIXME: Check the value of the mask. */
					for ( size_t pixel = 0; pixel < rowPixelCount; ++pixel )
					{
						m_target.blendPixel(destinationIndex + pixel, row, source.pixel(sourceIndex + pixel, row), mode);
					}
				}

				m_target.markRectangleUpdated(destinationClip);

				return true;
			}

			/**
			 * @brief Copies with a stencil mask the source pixmap to a clipped area of the target pixmap.
			 * @note The source pixmap will be used from the top-left corner.
			 * @warning The mask pixmap must be grayscale.
			 * @param source A reference to a source pixmap.
			 * @param clip A reference to a rectangle on the target pixmap.
			 * @param mask A reference to a pixmap to act as the stencil mask.
			 * @param mode Set the copy technic. Default: Replace.
			 * @param opacity A global opacity. Default 1.0.
			 * @return bool
			 */
			bool
			stencil (const Pixmap< pixel_data_t, dimension_t > & source, const Math::Space2D::AARectangle< dimension_t > & clip, const Pixmap< pixel_data_t, dimension_t > & mask, DrawPixelMode mode = DrawPixelMode::Replace, float opacity = 1.0F) const noexcept
			{
				return this->stencil(source, {source.width(), source.height()}, clip, mask, mode, opacity);
			}

			/**
			 * @brief Copies a colored area with a stencil mask to the target pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param color A reference to a color.
			 * @param clip A reference to a rectangle.
			 * @param mode Set the copy technic. Default: Replace.
			 * @param mask A reference to a pixmap to act as the stencil mask.
			 * @param opacity A global opacity. Default 1.0.
			 * @return bool
			 */
			template< typename color_data_t = float >
			bool
			stencil (const Color< color_data_t > & /*color*/, const Math::Space2D::AARectangle< dimension_t > & /*clip*/, const Pixmap< pixel_data_t, dimension_t > & /*mask*/, DrawPixelMode /*mode = DrawPixelMode::Replace*/, float /*opacity = 1.0F*/) const noexcept
			{
				// TODO ...

				return false;
			}

			/**
			 * @brief Resizes a pixmap.
			 * @param source A reference to a pixmap.
			 * @param width The new width.
			 * @param height The new height.
			 * @param destination A writable reference to a pixmap.
			 * @param filteringMode The filtering mode to apply during the resize. Default linear.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			resize (const Pixmap< pixel_data_t, dimension_t > & source, dimension_t width, dimension_t height, Pixmap< pixel_data_t, dimension_t > & destination, FilteringMode filteringMode = FilteringMode::Linear) noexcept
			{
				if ( !source.isValid() )
				{
					return false;
				}

				if ( width == source.width() && height == source.height() )
				{
					destination = source;

					return true;
				}

				if ( !destination.initialize(width, height, source.channelMode()) )
				{
					return false;
				}

				switch ( filteringMode )
				{
					case FilteringMode::Cubic :
						resizeCubic(source, width, height, destination);
						break;

					case FilteringMode::Linear :
						resizeLinear(source, width, height, destination);
						break;

					case FilteringMode::Nearest :
						resizeNearest(source, width, height, destination);
						break;
				}

				return true;
			}

			/**
			 * @brief Resizes a pixmap using a ratio.
			 * @param source A reference to a pixmap.
			 * @param ratio The scale ratio.
			 * @param destination A reference to a writable pixmap.
			 * @param filteringMode The filtering mode to apply during the resize. Default linear.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			resize (const Pixmap< pixel_data_t, dimension_t > & source, float ratio, Pixmap< pixel_data_t, dimension_t > & destination, FilteringMode filteringMode = FilteringMode::Linear) noexcept
			{
				return Processor::resize(
					source,
					static_cast< size_t >(source.width() * ratio),
					static_cast< size_t >(source.height() * ratio),
					destination,
					filteringMode
				);
			}

			/**
			 * @brief Returns a resized pixmap.
			 * @param source A reference to a pixmap.
			 * @param width The new width.
			 * @param height The new height.
			 * @param filteringMode The filtering mode to apply during the resize. Default linear.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			resize (const Pixmap< pixel_data_t, dimension_t > & source, dimension_t width, dimension_t height, FilteringMode filteringMode = FilteringMode::Linear) noexcept
			{
				if ( width == source.width() && height == source.height() )
				{
					return source;
				}

				Pixmap< pixel_data_t, dimension_t > output;

				if ( !Processor::resize(source, width, height, output, filteringMode) )
				{
					return {};
				}

				return output;
			}

			/**
			 * @brief Returns a resized pixmap using a ratio.
			 * @param source A reference to a pixmap.
			 * @param ratio The scale ratio.
			 * @param filteringMode The filtering mode to apply during the resize. Default linear.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			resize (const Pixmap< pixel_data_t, dimension_t > & source, float ratio, FilteringMode filteringMode = FilteringMode::Linear) noexcept
			{
				return Processor::resize(
					source,
					static_cast< size_t >(source.width() * ratio),
					static_cast< size_t >(source.height() * ratio),
					filteringMode
				);
			}

			/**
			 * @brief Crops a pixmap.
			 * @param source A reference to a pixmap.
			 * @param rectangle A reference to a rectangle.
			 * @param destination A writable reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			crop (const Pixmap< pixel_data_t, dimension_t > & source, const Math::Space2D::AARectangle< dimension_t > & rectangle, Pixmap< pixel_data_t, dimension_t > & destination) noexcept
			{
				if ( !source.isValid() || !rectangle.isValid() || !rectangle.isIntersect(source.width(), source.height()) )
				{
					return false;
				}

				if ( !destination.initialize(rectangle.width(), rectangle.height(), source.channelMode()) )
				{
					return false;
				}

				const auto rowBytes = destination.pitch();
				const auto pixelBytes = source.colorCount() * sizeof(pixel_data_t);

				for ( size_t rowIndex = 0; rowIndex < rectangle.height(); rowIndex++ )
				{
					const auto baseY = rowIndex + rectangle.top();
					const auto destinationIndex = rowIndex * rowBytes;
					const auto sourceIndex = (baseY * source.width() + rectangle.left()) * pixelBytes;

					std::memcpy(destination.data().data() + destinationIndex, source.data().data() + sourceIndex, rowBytes);
				}

				return true;
			}

			/**
			 * @brief Returns a cropped pixmap.
			 * @param source A reference to a pixmap.
			 * @param rectangle A reference to a rectangle.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			crop (const Pixmap< pixel_data_t, dimension_t > & source, const Math::Space2D::AARectangle< dimension_t > & rectangle) noexcept
			{
				Pixmap< pixel_data_t, dimension_t > output;

				if ( !Processor::crop(source, rectangle, output) )
				{
					return {};
				}

				return output;
			}

			/**
			 * @brief Extends a pixmap with black borders.
			 * @param source A reference to a pixmap.
			 * @param destination A writable reference to a pixmap.
			 * @param borders A reference to an array of dimensions for borders [right, top, left, bottom].
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			extend (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & destination, const std::array< dimension_t, 4 > & borders) noexcept
			{
				const auto newWidth = source.width() + borders[0] + borders[2];
				const auto newHeight = source.height() + borders[1] + borders[3];

				if ( !destination.initialize(newWidth, newHeight, source.channelMode()) )
				{
					return false;
				}

				Processor processor{destination};

				if ( !processor.blit(source, {borders[0], borders[1], source.width(), source.height()}) )
				{
					return false;
				}

				return destination;
			}

			/**
			 * @brief Extends a pixmap with selectable color borders.
			 * @tparam color_data_t The color data type. Default float.
			 * @param source A reference to a pixmap.
			 * @param destination A writable reference to a pixmap.
			 * @param borders A reference to an array of dimensions for borders [right, top, left, bottom].
			 * @param color A reference to a color.
			 * @return bool
			 */
			template < typename color_data_t = float >
			[[nodiscard]]
			static
			bool
			extend (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & destination, const std::array< dimension_t, 4 > & borders, const Color< color_data_t > & color) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				const auto newWidth = source.width() + borders[0] + borders[2];
				const auto newHeight = source.height() + borders[1] + borders[3];

				if ( !destination.initialize(newWidth, newHeight, source.channelMode()) )
				{
					return false;
				}

				Processor processor{destination};

				if ( !processor.blit(source, {borders[0], borders[1], source.width(), source.height()}) )
				{
					return false;
				}

				/* Top */
				if ( borders[1] > 0 )
				{
					if ( !processor.copy(color, {0, 0, destination.width(), borders[1]}) )
					{
						return false;
					}
				}

				/* Bottom */
				if ( borders[3] > 0 )
				{
					if ( !processor.copy(color, {0, destination.height() - borders[3], destination.width(), borders[3]}) )
					{
						return false;
					}
				}

				/* Right */
				if ( borders[0] > 0 )
				{
					if ( !processor.copy(color, {0, borders[1], borders[0], source.height()}) )
					{
						return false;
					}
				}

				/* Left */
				if ( borders[2] > 0 )
				{
					if ( !processor.copy(color, {destination.width() - borders[2], borders[1], borders[2], source.height()}) )
					{
						return false;
					}
				}

				return true;
			}

			/**
			 * @brief Returns an extended pixmap with black borders.
			 * @param source A reference to a pixmap.
			 * @param borders A reference to an array of dimensions for borders [right, top, left, bottom].
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			extend (const Pixmap< pixel_data_t, dimension_t > & source, const std::array< dimension_t, 4 > & borders) noexcept
			{
				Pixmap< pixel_data_t, dimension_t > target;

				if ( !Processor::extend(source, target, borders) )
				{
					return {};
				}

				return target;
			}

			/**
			 * @brief Returns an extended pixmap with selectable color borders.
			 * @tparam color_data_t The color data type. Default float.
			 * @param source A reference to a pixmap.
			 * @param borders A reference to an array of dimensions for borders [right, top, left, bottom].
			 * @param color A reference to a color.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			template < typename color_data_t = float >
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			extend (const Pixmap< pixel_data_t, dimension_t > & source, const std::array< dimension_t, 4 > & borders, const Color< color_data_t > & color) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				Pixmap< pixel_data_t, dimension_t > target;

				if ( !Processor::extend(source, target, borders, color) )
				{
					return {};
				}

				return target;
			}

			/**
			 * @brief Mirrors the pixmap on X axis, Y axis or both axes.
			 * @param source A reference to a pixmap.
			 * @param destination A writable reference to a pixmap.
			 * @param mode The mirror mode desired.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			mirror (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & destination, MirrorMode mode) noexcept
			{
				if ( !source.isValid() )
				{
					return false;
				}

				if ( !destination.initialize(source.width(), source.height(), source.channelMode()) )
				{
					return false;
				}

				switch ( mode )
				{
					case MirrorMode::X :
						Processor::mirrorX(source, destination);

						return true;

					case MirrorMode::Y :
						return Processor::mirrorY(source, destination);

					case MirrorMode::Both :
						return Processor::mirrorBoth(source, destination);
				}

				return false;
			}

			/**
			 * @brief Returns a mirrored pixmap on X axis, Y axis or both axes.
			 * @param source A reference to a pixmap.
			 * @param mode The mirror mode desired.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			mirror (const Pixmap< pixel_data_t, dimension_t > & source, MirrorMode mode) noexcept
			{
				Pixmap< pixel_data_t, dimension_t > output;

				if ( !Processor::mirror(source, output, mode) )
				{
					return {};
				}

				return output;
			}

			/**
			 * @brief Orthogonally rotates a pixmap with an angle of +90°.
			 * @param source A reference to a pixmap.
			 * @param destination A writable reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			rotateQuarterTurn (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & destination) noexcept
			{
				if ( !source.isValid() )
				{
					return false;
				}
				
				const auto & srcData = source.data();
				const auto colorCount = source.colorCount();
				const auto pixelCount = source.pixelCount();

				if ( !destination.initialize(source.height(), source.width(), source.channelMode()) )
				{
					return false;
				}

				auto & dstData = destination.data();

				/* NOTE: We start from the first row of the source
				 * to copy to the last column of the target (top to bottom). */
				size_t coordX = destination.width() - 1;
				size_t coordY = 0;

				/* NOTE: Per-pixel loop. */
				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					const auto sourceIndex = pixelIndex * colorCount;
					const auto targetIndex = (coordY * destination.width() + coordX) * colorCount;

					switch ( colorCount )
					{
						case 4 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							dstData[targetIndex+2] = srcData[sourceIndex+2];
							dstData[targetIndex+3] = srcData[sourceIndex+3];
							break;

						case 3 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							dstData[targetIndex+2] = srcData[sourceIndex+2];
							break;

						case 2 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							break;

						case 1 :
							dstData[targetIndex] = srcData[sourceIndex];
							break;

						default:
							break;
					}

					/* NOTE: Check the next target pixel coordinates. */
					if ( coordY >= destination.height() - 1 )
					{
						--coordX;
						coordY = 0;
					}
					else
					{
						++coordY;
					}
				}

				return true;
			}

			/**
			 * @brief Returns an orthogonally rotated pixmap with an angle of +90°.
			 * @param source A reference to a pixmap.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			rotateQuarterTurn (const Pixmap< pixel_data_t, dimension_t > & source) noexcept
			{
				Pixmap< pixel_data_t, dimension_t > output;

				if ( !Processor::rotateQuarterTurn(source, output) )
				{
					return {};
				}

				return output;
			}

			/**
			 * @brief Orthogonally rotates a pixmap with an angle of +180°.
			 * @param source A reference to a pixmap.
			 * @param destination A writable reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			rotateHalfTurn (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & destination) noexcept
			{
				if ( !source.isValid() )
				{
					return false;
				}

				const auto & srcData = source.data();
				const auto colorCount = source.colorCount();
				const auto pixelCount = source.pixelCount();

				if ( !destination.initialize(source.width(), source.height(), source.channelMode()) )
				{
					return false;
				}

				auto & dstData = destination.data();

				/* NOTE: We start from the first row of the source
				 * to copy to the last row of the target (right to left). */
				size_t coordX = destination.width() - 1;
				size_t coordY = destination.height() - 1;

				/* NOTE: Per-pixel loop. */
				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					const auto sourceIndex = pixelIndex * colorCount;
					const auto targetIndex = (coordY * destination.width() + coordX) * colorCount;

					switch ( colorCount )
					{
						case 4 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							dstData[targetIndex+2] = srcData[sourceIndex+2];
							dstData[targetIndex+3] = srcData[sourceIndex+3];
							break;

						case 3 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							dstData[targetIndex+2] = srcData[sourceIndex+2];
							break;

						case 2 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							break;

						case 1 :
							dstData[targetIndex] = srcData[sourceIndex];
							break;

						default:
							break;
					}

					/* NOTE: Check the next target pixel coordinates. */
					if ( coordX == 0 )
					{
						coordX = destination.width() - 1;
						--coordY;
					}
					else
					{
						--coordX;
					}
				}

				return true;
			}

			/**
			 * @brief Returns an orthogonally rotated pixmap with an angle of +180°.
			 * @param source A reference to a pixmap.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			rotateHalfTurn (const Pixmap< pixel_data_t, dimension_t > & source) noexcept
			{
				Pixmap< pixel_data_t, dimension_t > output;

				if ( !Processor::rotateHalfTurn(source, output) )
				{
					return {};
				}

				return output;
			}

			/**
			 * @brief Orthogonally rotates a pixmap with an angle of +270°.
			 * @param source A reference to a pixmap.
			 * @param destination A writable reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			rotateThreeQuarterTurn (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & destination) noexcept
			{
				if ( !source.isValid() )
				{
					return false;
				}

				const auto & srcData = source.data();
				const auto colorCount = source.colorCount();
				const auto pixelCount = source.pixelCount();

				if ( !destination.initialize(source.height(), source.width(), source.channelMode()) )
				{
					return false;
				}

				auto & dstData = destination.data();

				/* NOTE: We start from the first row of the source
				 * to copy to the first column of the target (bottom to top). */
				size_t coordX = 0;
				size_t coordY = destination.height() - 1;

				/* NOTE: Per-pixel loop. */
				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					const auto sourceIndex = pixelIndex * colorCount;
					const auto targetIndex = (coordY * destination.width() + coordX) * colorCount;

					switch ( colorCount )
					{
						case 4 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							dstData[targetIndex+2] = srcData[sourceIndex+2];
							dstData[targetIndex+3] = srcData[sourceIndex+3];
							break;

						case 3 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							dstData[targetIndex+2] = srcData[sourceIndex+2];
							break;

						case 2 :
							dstData[targetIndex] = srcData[sourceIndex];
							dstData[targetIndex+1] = srcData[sourceIndex+1];
							break;

						case 1 :
							dstData[targetIndex] = srcData[sourceIndex];
							break;

						default:
							break;
					}

					/* NOTE: Check the next target pixel coordinates. */
					if ( coordY == 0 )
					{
						++coordX;
						coordY = destination.height() - 1;
					}
					else
					{
						--coordY;
					}
				}

				return true;
			}

			/**
			 * @brief Returns an orthogonally rotated pixmap with an angle of +270°.
			 * @param source A reference to a pixmap.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			rotateThreeQuarterTurn (const Pixmap< pixel_data_t, dimension_t > & source) noexcept
			{
				Pixmap< pixel_data_t, dimension_t > output;

				if ( !Processor::rotateThreeQuarterTurn(source, output) )
				{
					return {};
				}

				return output;
			}

			/**
			 * @brief Inverses the colors.
			 * @note This leaves the alpha channel untouched.
			 * @param source A reference to a pixmap.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			inverseColors (const Pixmap< pixel_data_t, dimension_t > & source) noexcept
			{
				if ( !source.isValid() )
				{
					return {};
				}

				if ( source.channelMode() == ChannelMode::Grayscale || source.channelMode() == ChannelMode::GrayscaleAlpha )
				{
					return source;
				}

				Pixmap< pixel_data_t, dimension_t > output{source.width(), source.height(), source.channelMode()};

				const auto pixelCount = source.pixelCount();

				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					const auto * pixelSrc = source.pixelPointer(pixelIndex);
					auto * pixelDst = output.pixelPointer(pixelIndex);

					pixelDst[0] = std::numeric_limits< pixel_data_t >::max() - pixelSrc[0];
					pixelDst[1] = std::numeric_limits< pixel_data_t >::max() - pixelSrc[1];
					pixelDst[2] = std::numeric_limits< pixel_data_t >::max() - pixelSrc[2];
				}

				return output;
			}

			/**
			 * @brief This reverses the order of color components in each pixel. E.g., RGB -> BGR. RGBA -> BGRA.
			 * @note If swapAlpha is active, ARGB -> BGRA.
			 * @param source A reference to a pixmap.
			 * @param swapAlpha Set to swap the alpha channel too.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			swapChannels (const Pixmap< pixel_data_t, dimension_t > & source, bool swapAlpha = false) noexcept
			{
				if ( !source.isValid() )
				{
					return {};
				}

				if ( source.channelMode() == ChannelMode::Grayscale || source.channelMode() == ChannelMode::GrayscaleAlpha )
				{
					return source;
				}

				Pixmap< pixel_data_t, dimension_t > output{source.width(), source.height(), source.channelMode()};

				const auto elementCount = source.elementCount();
				const auto stride = source.colorCount();

				const auto & dataSrc = source.data();
				auto & dataDst = output.data();

				switch ( source.channelMode() )
				{
					case ChannelMode::RGB :
						for ( size_t index = 0; index < elementCount; index += stride )
						{
							/* RGB -> BGR */
							dataDst[index] = dataSrc[index+2];
							dataDst[index+1] = dataSrc[index+1];
							dataDst[index+2] = dataSrc[index];
						}
						break;

					case ChannelMode::RGBA :
						if ( swapAlpha )
						{
							for ( size_t index = 0; index < elementCount; index += stride )
							{
								/* RGBA -> ABGR */
								dataDst[index] = dataSrc[index+3];
								dataDst[index+1] = dataSrc[index+2];
								dataDst[index+2] = dataSrc[index+1];
								dataDst[index+3] = dataSrc[index];
							}
						}
						else
						{
							for ( size_t index = 0; index < elementCount; index += stride )
							{
								/* RGB -> BGR */
								dataDst[index] = dataSrc[index+2];
								dataDst[index+1] = dataSrc[index+1];
								dataDst[index+2] = dataSrc[index];

								/* Keeping alpha. */
								dataDst[index+3] = dataSrc[index+3];
							}
						}
						break;

					case ChannelMode::Grayscale :
					case ChannelMode::GrayscaleAlpha :
						/* NOTE: should never happen */
						return source;

					default:
						return {};
				}

				return output;
			}

			/**
			 * @brief Returns a pixmap with an alpha channel.
			 * @note Will return false if the pixmap already have an alpha channel.
			 * @param source A reference to a pixmap.
			 * @param destination A writable reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			addAlphaChannel (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & destination) noexcept
			{
				if ( !source.isValid() )
				{
					return false;
				}

				if ( source.channelMode() == ChannelMode::GrayscaleAlpha || source.channelMode() == ChannelMode::RGBA )
				{
					return false;
				}

				const auto pixelCount = source.template pixelCount< size_t >();

				if ( source.channelMode() == ChannelMode::Grayscale )
				{
					destination.initialize(source.width(), source.height(), ChannelMode::GrayscaleAlpha);

					for ( size_t index = 0; index < pixelCount; index++ )
					{
						const auto srcData = source.pixelPointer(index);
						auto dstData = destination.pixelPointer(index);

						dstData[0] = srcData[0];
						dstData[1] = Pixmap< pixel_data_t, dimension_t >::one();
					}
				}
				else
				{
					destination.initialize(source.width(), source.height(), ChannelMode::RGBA);

					for ( size_t index = 0; index < pixelCount; index++ )
					{
						const auto srcData = source.pixelPointer(index);
						auto dstData = destination.pixelPointer(index);

						dstData[0] = srcData[0];
						dstData[1] = srcData[1];
						dstData[2] = srcData[2];
						dstData[3] = Pixmap< pixel_data_t, dimension_t >::one();
					}
				}

				return true;
			}

			/**
			 * @brief Returns a pixmap with an alpha channel.
			 * @param source A reference to a pixmap.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			addAlphaChannel (const Pixmap< pixel_data_t, dimension_t > & source) noexcept
			{
				if ( source.channelMode() == ChannelMode::GrayscaleAlpha || source.channelMode() == ChannelMode::RGBA )
				{
					return source;
				}

				Pixmap< pixel_data_t, dimension_t > output;

				if ( !Processor::addAlphaChannel(source, output) )
				{
					return {};
				}

				return output;
			}

			/**
			 * @brief Returns a pixmap without the alpha channel.
			 * @note Will return false if the pixmap don't have an alpha channel.
			 * @param source A reference to a pixmap.
			 * @param destination A writable reference to a pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			removeAlphaChannel (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & destination) noexcept
			{
				if ( !source.isValid() )
				{
					return false;
				}

				if ( source.channelMode() == ChannelMode::Grayscale || source.channelMode() == ChannelMode::RGB )
				{
					return false;
				}

				const auto pixelCount = source.template pixelCount< size_t >();

				if ( source.channelMode() == ChannelMode::GrayscaleAlpha )
				{
					destination.initialize(source.width(), source.height(), ChannelMode::Grayscale);

					for ( size_t index = 0; index < pixelCount; index++ )
					{
						const auto srcData = source.pixelPointer(index);
						auto dstData = destination.pixelPointer(index);

						dstData[0] = srcData[0];
					}
				}
				else
				{
					destination.initialize(source.width(), source.height(), ChannelMode::RGB);

					for ( size_t index = 0; index < pixelCount; index++ )
					{
						const auto srcData = source.pixelPointer(index);
						auto dstData = destination.pixelPointer(index);

						dstData[0] = srcData[0];
						dstData[1] = srcData[1];
						dstData[2] = srcData[2];
					}
				}

				return true;
			}

			/**
			 * @brief Returns a pixmap without the alpha channel.
			 * @param source A reference to a pixmap.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			removeAlphaChannel (const Pixmap< pixel_data_t, dimension_t > & source) noexcept
			{
				if ( source.channelMode() == ChannelMode::Grayscale || source.channelMode() == ChannelMode::RGB )
				{
					return source;
				}

				Pixmap< pixel_data_t, dimension_t > output;

				if ( !Processor::removeAlphaChannel(source, output) )
				{
					return {};
				}

				return output;
			}

			/**
			 * @brief Returns one channel of the pixmap as a new grayscale pixmap.
			 * @param source A reference to a pixmap.
			 * @param channel The desired channel to extract.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			extractChannel (const Pixmap< pixel_data_t, dimension_t > & source, Channel channel) noexcept
			{
				if ( !source.isValid() )
				{
					return {};
				}

				size_t channelIndex = 0;

				switch ( source.channelMode() )
				{
					case ChannelMode::Grayscale :
						if ( channel == Channel::Alpha )
						{
							return {source.width(), source.height(), ChannelMode::Grayscale, White};
						}

						return source;

					case ChannelMode::GrayscaleAlpha :
						if ( channel == Channel::Alpha )
						{
							channelIndex = 1UL;
						}
						break;

					case ChannelMode::RGB :
						if ( channel == Channel::Alpha )
						{
							return {source.width(), source.height(), ChannelMode::Grayscale, White};
						}

						channelIndex = static_cast< size_t >(channel);
						break;

					case ChannelMode::RGBA :
						channelIndex = static_cast< size_t >(channel);
						break;

					default:
						break;
				}

				Pixmap< pixel_data_t, dimension_t > output{source.width(), source.height(), ChannelMode::Grayscale};

				const auto pixelCount = source.pixelCount();

				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					*output.pixelPointer(pixelIndex) = *(source.pixelPointer(pixelIndex) + channelIndex);
				}

				return output;
			}

			/**
			 * @brief Returns a pixmap in grayscale mode.
			 * @param source A reference to a pixmap.
			 * @param mode The conversion mode. Default LumaRec709.
			 * @param option Additional parameter for specific mode. Default 0.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			toGrayscale (const Pixmap< pixel_data_t, dimension_t > & source, GrayscaleConversionMode mode = GrayscaleConversionMode::Average, int option = 0) noexcept
			{
				if ( !source.isValid() )
				{
					return {};
				}

				if ( source.channelMode() == ChannelMode::Grayscale || source.channelMode() == ChannelMode::GrayscaleAlpha )
				{
					return source;
				}

				Pixmap< pixel_data_t, dimension_t > output{source.width(), source.height(), ChannelMode::Grayscale};

				const auto pixelCount = source.pixelCount();

				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					if constexpr ( std::is_floating_point_v< pixel_data_t > )
					{
						*output.pixelPointer(pixelIndex) = source.pixel(pixelIndex).luminance(mode, option);
					}
					else
					{
						*output.pixelPointer(pixelIndex) = source.pixel(pixelIndex).template luminanceInteger< pixel_data_t >(mode, option);
					}
				}

				return output;
			}

			/**
			 * @brief Returns a pixmap in RGB mode.
			 * @param source A reference to a pixmap.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			toRGB (const Pixmap< pixel_data_t, dimension_t > & source) noexcept
			{
				if ( !source.isValid() )
				{
					return {};
				}

				if ( source.channelMode() == ChannelMode::RGB )
				{
					return source;
				}

				Pixmap< pixel_data_t, dimension_t > output{source.width(), source.height(), ChannelMode::RGB};

				const auto pixelCount = source.pixelCount();

				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					output.setPixel(pixelIndex, source.pixel(pixelIndex));
				}

				return output;
			}

			/**
			 * @brief Returns a pixmap in RGBA mode.
			 * @param source A reference to a pixmap.
			 * @param opacity Alpha default value. Default 1.0.
			 * @return Pixmap< pixel_data_t, dimension_t >
			 */
			[[nodiscard]]
			static
			Pixmap< pixel_data_t, dimension_t >
			toRGBA (const Pixmap< pixel_data_t, dimension_t > & source, float opacity = 1.0F) noexcept
			{
				if ( !source.isValid() )
				{
					return {};
				}

				if ( source.channelMode() == ChannelMode::RGBA )
				{
					return source;
				}

				Pixmap< pixel_data_t, dimension_t > output{source.width(), source.height(), ChannelMode::RGBA};

				const auto pixelCount = source.pixelCount();

				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					auto color = source.pixel(pixelIndex);
					color.setAlpha(opacity);

					output.setPixel(pixelIndex, color);
				}

				return output;
			}

		private:

			/**
			 * @brief Checks the clipping area within a pixmap.
			 * @tparam rectangle_data_t The rectangle integer type. Default uint32_t.
			 * @param pixmap A reference to a pixmap.
			 * @param clip A reference to a rectangle.
			 * @return bool
			 */
			template< typename rectangle_data_t = uint32_t >
			[[nodiscard]]
			static
			bool
			checkPixmapClipping (const Pixmap< pixel_data_t, dimension_t > & pixmap, const Math::Space2D::AARectangle< rectangle_data_t > & clip) noexcept
			{
				if ( !pixmap.isValid() )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "Target pixmap is invalid !" "\n";
					}

					return false;
				}

				if ( !clip.isValid() )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "The clipping area is invalid !" "\n";
					}

					return false;
				}

				if ( clip.isOutside(pixmap.width(), pixmap.height()) )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "The clipping area is outside the pixmap !" "\n";
					}

					return false;
				}

				return true;
			}

			/**
			 * @brief Checks the clipping area within a pixmap and clamps it on overlapping.
			 * @tparam rectangle_data_t The rectangle integer type. Default uint32_t.
			 * @param pixmap A reference to a pixmap.
			 * @param clip A reference to a rectangle.
			 * @return bool
			 */
			template< typename rectangle_data_t = uint32_t >
			[[nodiscard]]
			static
			bool
			checkPixmapClipping (const Pixmap< pixel_data_t, dimension_t > & pixmap, Math::Space2D::AARectangle< rectangle_data_t > & clip) noexcept
			{
				if ( !pixmap.isValid() )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "Target pixmap is invalid !" "\n";
					}

					return false;
				}

				if ( !clip.isValid() )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "The clipping area is invalid !" "\n";
					}

					return false;
				}

				if ( clip.isOutside(pixmap.width(), pixmap.height()) )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "The clipping area is outside the pixmap !" "\n";
					}

					return false;
				}

				clip.cropOnOverflow(pixmap.width(), pixmap.height());

				return true;
			}

			/**
			 * @brief Clamps the first point of a segment into the pixmap boundaries.
			 * @todo This method can be simplified by using findIntersection() using segments instead of lines.
			 * @param pointA A reference to the first point of the segment.
			 * @param pointB A reference to the second point of the segment.
			 * @return bool
			 */
			bool
			clampSegmentFirstPoint (Math::Vector< 2, int32_t > & pointA, const Math::Vector< 2, int32_t > & pointB) const noexcept
			{
				const auto width = static_cast< int32_t >(m_target.width());
				const auto height = static_cast< int32_t >(m_target.height());

				/* NOTE: If the first point is outside the pixmap from left side or bottom. */
				if ( pointA[Math::X] < 0 || pointA[Math::Y] < 0 )
				{
					Math::Vector< 2, float > intersectionPoint;

					auto intersect = Math::Space2D::isIntersecting(
						0.0F, 0.0F,
						0.0F, height - 1.0F,
						static_cast< float >(pointA[Math::X]), static_cast< float >(pointA[Math::Y]),
						static_cast< float >(pointB[Math::X]), static_cast< float >(pointB[Math::Y]),
						intersectionPoint
					);

					if ( intersect )
					{
						const auto x = static_cast< int32_t >(std::roundf(intersectionPoint[Math::X]));
						const auto y = static_cast< int32_t >(std::roundf(intersectionPoint[Math::Y]));

						if ( x >= 0 && y >= 0 && x < width && y < height )
						{
							pointA[Math::X] = x;
							pointA[Math::Y] = y;

							return true;
						}
					}

					intersect = Math::Space2D::isIntersecting(
						0.0F, 0.0F,
						width - 1.0F, 0.0F,
						static_cast< float >(pointA[Math::X]), static_cast< float >(pointA[Math::Y]),
						static_cast< float >(pointB[Math::X]), static_cast< float >(pointB[Math::Y]),
						intersectionPoint
					);

					if ( intersect )
					{
						const auto x = static_cast< int32_t >(std::roundf(intersectionPoint[Math::X]));
						const auto y = static_cast< int32_t >(std::roundf(intersectionPoint[Math::Y]));

						if ( x >= 0 && y >= 0 && x < width && y < height )
						{
							pointA[Math::X] = x;
							pointA[Math::Y] = y;

							return true;
						}
					}

					return false;
				}

				/* NOTE: If the first point is outside the pixmap from right side or top. */
				if ( pointA[Math::X] > width || pointA[Math::Y] > height )
				{
					Math::Vector< 2, float > intersectionPoint;

					auto intersect = Math::Space2D::isIntersecting(
						0.0F, height - 1.0F,
						width - 1.0F, height - 1.0F,
						static_cast< float >(pointA[Math::X]), static_cast< float >(pointA[Math::Y]),
						static_cast< float >(pointB[Math::X]), static_cast< float >(pointB[Math::Y]),
						intersectionPoint
					);

					if ( intersect )
					{
						const auto x = static_cast< int32_t >(std::roundf(intersectionPoint[Math::X]));
						const auto y = static_cast< int32_t >(std::roundf(intersectionPoint[Math::Y]));

						if ( x >= 0 && y >= 0 && x < width && y < height )
						{
							pointA[Math::X] = x;
							pointA[Math::Y] = y;

							return true;
						}
					}

					intersect = Math::Space2D::isIntersecting(
						width - 1.0F, 0.0F,
						width - 1.0F, height - 1.0F,
						static_cast< float >(pointA[Math::X]), static_cast< float >(pointA[Math::Y]),
						static_cast< float >(pointB[Math::X]), static_cast< float >(pointB[Math::Y]),
						intersectionPoint
					);

					if ( intersect )
					{
						const auto x = static_cast< int32_t >(std::roundf(intersectionPoint[Math::X]));
						const auto y = static_cast< int32_t >(std::roundf(intersectionPoint[Math::Y]));

						if ( x >= 0 && y >= 0 && x < width && y < height )
						{
							pointA[Math::X] = x;
							pointA[Math::Y] = y;

							return true;
						}
					}

					return false;
				}

				return true;
			}

			/**
			 * @brief Prepares the swap buffer for copy operations.
			 * @return void
			 */
			void
			prepareSwapBuffer () noexcept
			{
				const auto size = m_target.bytes();

				if ( m_swapBuffer.size() != size )
				{
					m_swapBuffer.resize(size, 0);
				}
			}

			/**
			 * @brief Swaps between processor buffer data and pixmap data to save results.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			swapBuffers () noexcept
			{
				if ( m_swapBuffer.empty() )
				{
					return false;
				}

				if ( m_swapBuffer.size() != m_target.bytes() )
				{
					return false;
				}

				m_swapBuffer.swap(m_target.data());

				return true;
			}

			/**
			 * @brief Performs the nearest version of Processor::resize().
			 * @todo Check for SIMD/parralelization
			 * @param source A reference to a pixmap.
			 * @param width The new width.
			 * @param height The new height.
			 * @param target A writable reference to a pixmap.
			 * @return void
			 */
			static
			void
			resizeNearest (const Pixmap< pixel_data_t, dimension_t > & source, dimension_t width, dimension_t height, Pixmap< pixel_data_t, dimension_t > & target) noexcept
			{
				const auto colorCount = source.colorCount();
				const auto xRatio = static_cast< float >(source.width() - 1) / static_cast< float >(width);
				const auto yRatio = static_cast< float >(source.height() - 1) / static_cast< float >(height);

				const auto & sourceData = source.data();
				auto & targetData = target.data();

				size_t dstIndex = 0;

				/* Y (row) on the destination */
				for ( size_t destinationY = 0; destinationY < height; destinationY++ )
				{
					const auto sourceY = static_cast< size_t >(std::round(yRatio * static_cast< float >(destinationY)));
					const auto sourceRowIndex = sourceY * source.width() * colorCount;

					/* X (column) on the destination */
					for ( size_t destinationX = 0; destinationX < width; destinationX++ )
					{
						const auto sourceX = static_cast< size_t >(std::round(xRatio * static_cast< float >(destinationX)));
						const auto sourceIndex = sourceRowIndex + (sourceX * colorCount);

						switch ( source.channelMode() )
						{
							case ChannelMode::Grayscale :
								targetData[dstIndex++] = sourceData[sourceIndex];
								break;

							case ChannelMode::GrayscaleAlpha :
								targetData[dstIndex++] = sourceData[sourceIndex];
								targetData[dstIndex++] = sourceData[sourceIndex+1];
								break;

							case ChannelMode::RGB :
								targetData[dstIndex++] = sourceData[sourceIndex];
								targetData[dstIndex++] = sourceData[sourceIndex+1];
								targetData[dstIndex++] = sourceData[sourceIndex+2];
								break;

							case ChannelMode::RGBA :
								targetData[dstIndex++] = sourceData[sourceIndex];
								targetData[dstIndex++] = sourceData[sourceIndex+1];
								targetData[dstIndex++] = sourceData[sourceIndex+2];
								targetData[dstIndex++] = sourceData[sourceIndex+3];
								break;

							default:
								break;
						}
					}
				}
			}

			/**
			 * @brief Performs the linear version of Processor::resize().
			 * @todo Check for SIMD/parralelization
			 * @param source A reference to a pixmap.
			 * @param width The new width.
			 * @param height The new height.
			 * @param target A writable reference to a pixmap.
			 * @return void
			 */
			static
			void
			resizeLinear (const Pixmap< pixel_data_t, dimension_t > & source, dimension_t width, dimension_t height, Pixmap< pixel_data_t, dimension_t > & target) noexcept
			{
				const auto colorCount = source.colorCount();
				const auto xRatio = static_cast< float >(source.width() - 1) / static_cast< float >(width);
				const auto yRatio = static_cast< float >(source.height() - 1) / static_cast< float >(height);

				const auto & sourceData = source.data();
				auto & targetData = target.data();

				size_t dstIndex = 0;

				/* Y (row) on the destination */
				for ( size_t y = 0; y < height; y++ )
				{
					const auto realY = yRatio * static_cast< float >(y);

					const auto yFloor = static_cast< size_t >(std::floor(realY));
					const auto yCeil = static_cast< size_t >(std::ceil(realY));

					/* Source row of pixels */
					const auto srcRowA = yFloor * source.width();
					const auto srcRowB = yCeil * source.width();

					const auto yFactor = realY - static_cast< float >(yFloor);

					/* X (column) on the destination */
					for ( size_t x = 0; x < width; x++ )
					{
						const auto realX = static_cast< float >(x) * xRatio;

						const auto xFloor = static_cast< size_t >(std::floor(realX));
						const auto xCeil = static_cast< size_t >(std::ceil(realX));

						/* Source pixel */
						const auto srcIndexA = (srcRowA + xFloor) * colorCount;
						const auto srcIndexB = (srcRowA + xCeil) * colorCount;
						const auto srcIndexC = (srcRowB + xFloor) * colorCount;
						const auto srcIndexD = (srcRowB + xCeil) * colorCount;

						const auto xFactor = realX - static_cast< float >(xFloor);

						switch ( source.channelMode() )
						{
							case ChannelMode::Grayscale :
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA], sourceData[srcIndexB], sourceData[srcIndexC], sourceData[srcIndexD], xFactor, yFactor);
								break;

							case ChannelMode::GrayscaleAlpha :
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA], sourceData[srcIndexB], sourceData[srcIndexC], sourceData[srcIndexD], xFactor, yFactor);
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA+1], sourceData[srcIndexB+1], sourceData[srcIndexC+1], sourceData[srcIndexD+1], xFactor, yFactor);
								break;

							case ChannelMode::RGB :
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA], sourceData[srcIndexB], sourceData[srcIndexC], sourceData[srcIndexD], xFactor, yFactor);
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA+1], sourceData[srcIndexB+1], sourceData[srcIndexC+1], sourceData[srcIndexD+1], xFactor, yFactor);
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA+2], sourceData[srcIndexB+2], sourceData[srcIndexC+2], sourceData[srcIndexD+2], xFactor, yFactor);
								break;

							case ChannelMode::RGBA :
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA], sourceData[srcIndexB], sourceData[srcIndexC], sourceData[srcIndexD], xFactor, yFactor);
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA+1], sourceData[srcIndexB+1], sourceData[srcIndexC+1], sourceData[srcIndexD+1], xFactor, yFactor);
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA+2], sourceData[srcIndexB+2], sourceData[srcIndexC+2], sourceData[srcIndexD+2], xFactor, yFactor);
								targetData[dstIndex++] = Processor::bilinearFiltering(sourceData[srcIndexA+3], sourceData[srcIndexB+3], sourceData[srcIndexC+3], sourceData[srcIndexD+3], xFactor, yFactor);
								break;

							default:
								break;
						}
					}
				}
			}

			/**
			 * @brief Used during cubic resize.
			 * @param value The value to clamp
			 * @return data_t
			 */
			[[nodiscard]]
			static
			pixel_data_t
			clampValue (float value) noexcept
			{
				if constexpr ( std::is_floating_point_v< pixel_data_t > )
				{
					return std::max(0.0F, std::min(1.0F, value));
				}
				else
				{
					constexpr auto maxValue = std::numeric_limits< float >::max();

					return static_cast< pixel_data_t >(std::max(0.0F, std::min(maxValue, (value * maxValue) + 0.5F)));
				}
			}

			/**
			 * @brief Performs the cubic version of Processor::resize().
			 * @todo Check for SIMD/parralelization
			 * @param source A reference to a pixmap.
			 * @param width The new width.
			 * @param height The new height.
			 * @param target A writable reference to a pixmap.
			 * @return void
			 */
			static
			void
			resizeCubic (const Pixmap< pixel_data_t, dimension_t > & source, dimension_t width, dimension_t height, Pixmap< pixel_data_t, dimension_t > & target) noexcept
			{
				const auto colorCount = source.colorCount();
				const auto xRatio = static_cast< float >(source.width() - 1) / static_cast< float >(width);
				const auto yRatio = static_cast< float >(source.height() - 1) / static_cast< float >(height);

				//const auto & sourceData = source.data();
				auto & targetData = target.data();

				size_t dstIndex = 0;

				for ( size_t y = 0; y < height; ++y )
				{
					const float realY = yRatio * static_cast< float >(y);
					const auto yFloor = static_cast< int32_t >(std::floor(realY));
					const float tY = realY - static_cast< float >(yFloor);

					for ( size_t x = 0; x < width; ++x )
					{
						const float realX = xRatio * static_cast< float >(x);
						const auto xFloor = static_cast< int32_t >(std::floor(realX));
						const float tX = realX - static_cast< float >(xFloor);

						std::array< float, 4 > interpolatedValues{0.0F, 0.0F, 0.0F, 0.0F};

						const auto numChannels = static_cast< size_t >(colorCount);

						for ( size_t channel = 0; channel < numChannels; ++channel )
						{
							std::array< float, 4 > intermediate{0.0F, 0.0F, 0.0F, 0.0F};

							for ( int j = 0; j < 4; ++j )
							{
								const int currentY = yFloor - 1 + j;

								std::array< float, 4 > pixel{0.0F, 0.0F, 0.0F, 0.0F};

								for ( int i = 0; i < 4; ++i )
								{
									const int currentX = xFloor - 1 + i;
									const auto color = source.safePixel(currentX, currentY);

									switch ( source.channelMode() )
									{
										case ChannelMode::Grayscale:
											pixel[i] = color.red();
											break;

										case ChannelMode::GrayscaleAlpha:
											pixel[i] = channel == 0 ? color.red() : color.alpha();
											break;

										case ChannelMode::RGB:
											if ( channel == 0 )
											{
												pixel[i] = color.red();
											}
											else if ( channel == 1 )
											{
												pixel[i] = color.green();
											}
											else
											{
												pixel[i] = color.blue();
											}
											break;

										case ChannelMode::RGBA:
											if ( channel == 0 )
											{
												pixel[i] = color.red();
											}
											else if ( channel == 1 )
											{
												pixel[i] = color.green();
											}
											else if ( channel == 2 )
											{
												pixel[i] = color.blue();
											}
											else
											{
												pixel[i] = color.alpha();
											}
											break;

										default:
											break;
									}
								}

								intermediate[j] = Math::cubicInterpolationCatmullRom(pixel[0], pixel[1], pixel[2], pixel[3], tX);
							}

							interpolatedValues[channel] = Math::cubicInterpolationCatmullRom(intermediate[0], intermediate[1], intermediate[2], intermediate[3], tY);
						}

						switch ( source.channelMode() )
						{
							case ChannelMode::Grayscale :
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[0]);
								break;

							case ChannelMode::GrayscaleAlpha :
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[0]);
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[1]);
								break;

							case ChannelMode::RGB :
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[0]);
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[1]);
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[2]);
								break;

							case ChannelMode::RGBA :
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[0]);
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[1]);
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[2]);
								targetData[dstIndex++] = Processor::clampValue(interpolatedValues[3]);
								break;

							default:
								break;
						}
					}
				}
			}

			/**
			 * @brief Used during linear resize.
			 * @param bottomLeft
			 * @param bottomRight
			 * @param topLeft
			 * @param topRight
			 * @param xFactor
			 * @param yFactor
			 * @return precision_t
			 */
			[[nodiscard]]
			static
			pixel_data_t
			bilinearFiltering (pixel_data_t bottomLeft, pixel_data_t bottomRight, pixel_data_t topLeft, pixel_data_t topRight, float xFactor, float yFactor) noexcept
			{
				return static_cast< pixel_data_t >(Math::linearInterpolation(
					Math::linearInterpolation(static_cast< float >(bottomLeft), static_cast< float >(bottomRight), xFactor),
					Math::linearInterpolation(static_cast< float >(topLeft), static_cast< float >(topRight), xFactor),
					yFactor
				));
			}

			/**
			 * @brief Mirrors the pixmap in X-Axis.
			 * @param source A reference to the input pixmap.
			 * @param output A reference to the output pixmap.
			 * @return void
			 */
			static
			void
			mirrorX (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & output) noexcept
			{
				const auto rowElementCount = source.pitch();
				const auto rowCount = source.height();

				/* Copy rows in reversed order to a new pixmap. */
				for ( size_t rowIndex = 0; rowIndex < rowCount; rowIndex++ )
				{
					const auto invertedRow = rowCount - (rowIndex + 1);

					std::memcpy(output.data().data() + (invertedRow * rowElementCount), source.data().data() + (rowIndex * rowElementCount), rowElementCount);
				}
			}

			/**
			 * @brief Mirrors the pixmap in Y-Axis.
			 * @todo Can be optimize with bloc copy (first attempt failed).
			 * @param source A reference to the input pixmap.
			 * @param output A reference to the output pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			mirrorY (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & output) noexcept
			{
				const auto rowCount = source.height();
				const auto width = source.width();
				const auto stride = source.colorCount();

				auto & data = output.data();
				const auto & baseData = source.data();

				switch ( source.channelMode() )
				{
					case ChannelMode::Grayscale :
						for ( auto row = 0U; row < rowCount; row++ )
						{
							const auto rowOffset = row * width;

							for ( size_t rowPixel = 0; rowPixel < width; rowPixel++ )
							{
								const auto pixel = rowOffset + rowPixel;
								const auto invertedPixel = rowOffset + (width - (rowPixel + 1));

								data[pixel] = baseData[invertedPixel];
							}
						}
						break;

					case ChannelMode::GrayscaleAlpha :
						for ( auto row = 0U; row < rowCount; row++ )
						{
							const auto rowOffset = row * width * stride;

							for ( size_t rowPixel = 0; rowPixel < width; rowPixel++ )
							{
								const auto pixel = rowOffset + (rowPixel * stride);
								const auto invertedPixel = rowOffset + ((width - (rowPixel + 1)) * stride);

								data[pixel] = baseData[invertedPixel];
								data[pixel+1] = baseData[invertedPixel+1];
							}
						}
						break;

					case ChannelMode::RGB :
						for ( auto row = 0U; row < rowCount; row++ )
						{
							const auto rowOffset = row * width * stride;

							for ( size_t rowPixel = 0; rowPixel < width; rowPixel++ )
							{
								const auto pixel = rowOffset + (rowPixel * stride);
								const auto invertedPixel = rowOffset + ((width - (rowPixel + 1)) * stride);

								data[pixel] = baseData[invertedPixel];
								data[pixel+1] = baseData[invertedPixel+1];
								data[pixel+2] = baseData[invertedPixel+2];
							}
						}
						break;

					case ChannelMode::RGBA :
						for ( auto row = 0U; row < rowCount; row++ )
						{
							const auto rowOffset = row * width * stride;

							for ( size_t rowPixel = 0; rowPixel < width; rowPixel++ )
							{
								const auto pixel = rowOffset + (rowPixel * stride);
								const auto invertedPixel = rowOffset + ((width - (rowPixel + 1)) * stride);

								data[pixel] = baseData[invertedPixel];
								data[pixel+1] = baseData[invertedPixel+1];
								data[pixel+2] = baseData[invertedPixel+2];
								data[pixel+3] = baseData[invertedPixel+3];
							}
						}
						break;

					default:
						return false;
				}

				return true;
			}

			/**
			 * @brief Mirrors the pixmap in both X-Axis and Y-Axis.
			 * @param source A reference to the input pixmap.
			 * @param output A reference to the output pixmap.
			 * @return bool
			 */
			[[nodiscard]]
			static
			bool
			mirrorBoth (const Pixmap< pixel_data_t, dimension_t > & source, Pixmap< pixel_data_t, dimension_t > & output) noexcept
			{
				const auto componentCount = source.elementCount();
				const auto stride = source.colorCount();

				auto & data = output.data();
				const auto & baseData = source.data();

				switch ( source.channelMode() )
				{
					case ChannelMode::Grayscale :
						for ( size_t component = 0; component < componentCount; component++ )
						{
							const auto invertedComponent = componentCount - (component + 1);

							data[component] = baseData[invertedComponent];
						}
						break;

					case ChannelMode::GrayscaleAlpha :
						for ( size_t component = 0; component < componentCount; component += stride )
						{
							const auto invertedComponent = componentCount - (component + stride);

							data[component] = baseData[invertedComponent];
							data[component+1] = baseData[invertedComponent+1];
						}
						break;

					case ChannelMode::RGB :
						for ( size_t component = 0; component < componentCount; component += stride )
						{
							const auto invertedComponent = componentCount - (component + stride);

							data[component] = baseData[invertedComponent];
							data[component+1] = baseData[invertedComponent+1];
							data[component+2] = baseData[invertedComponent+2];
						}
						break;

					case ChannelMode::RGBA :
						for ( size_t component = 0; component < componentCount; component += stride )
						{
							const auto invertedComponent = componentCount - (component + stride);

							data[component] = baseData[invertedComponent];
							data[component+1] = baseData[invertedComponent+1];
							data[component+2] = baseData[invertedComponent+2];
							data[component+3] = baseData[invertedComponent+3];
						}
						break;

					default:
						return false;
				}

				return true;
			}

			Pixmap< pixel_data_t, dimension_t > & m_target;
			std::vector< pixel_data_t > m_swapBuffer;
	};
}
