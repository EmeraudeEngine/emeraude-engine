/*
 * src/Libs/PixelFactory/Pixmap.hpp
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

/* Emeraude-Engine configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstring>
#include <cassert>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <limits>
#include <type_traits>
#include <iostream>

/* Local inclusions for usage. */
#include "Libs/Algorithms/PerlinNoise.hpp"
#include "Libs/Math/Space2D/AARectangle.hpp"
#include "Color.hpp"
#include "Gradient.hpp"
#include "Types.hpp"

namespace EmEn::Libs::PixelFactory
{
	/** @brief 72dpi resolution. */
	static constexpr auto Resolution72DPI{2835UL};
	/** @brief 96dpi resolution. */
	static constexpr auto Resolution96DPI{3780UL};

	/**
	 * @brief Defines a structure to store pixels to make an image.
	 * @tparam pixel_data_t The pixel component type for the pixmap depth precision. Default uint8_t.
	 * @tparam dimension_t The type of unsigned integer used for pixmap dimension. Default uint32_t.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	class Pixmap
	{
		public:

			/**
			 * @brief The image compression format enumeration.
			 */
			enum class Format : uint8_t
			{
				None,
				Targa, /* No compression */
				PNG, /* Loss-less compression */
				Jpeg /* Destructive compression */
			};

			/**
			 * @brief Constructs a default pixmap.
			 * @note The channel mode will be set to RGB.
			 */
			Pixmap () noexcept = default;

			/**
			 * @brief Constructs a new pixmap.
			 * @param width The initial width of the pixmap.
			 * @param height The initial height of the pixmap.
			 * @param channelMode The desired color count. Default RGB.
			 */
			Pixmap (dimension_t width, dimension_t height, ChannelMode channelMode = ChannelMode::RGB)
				: m_width(width),
				m_height(height),
				m_channelMode(channelMode)
			{
				m_data.resize(this->elementCount());

				if constexpr ( IsDebug )
				{
					if ( !this->initAlphaChannel() )
					{
						std::cerr << "Unable to check alpha channel initialization !" "\n";
					}
				}
				else
				{
					this->initAlphaChannel();
				}
			}

			/**
			 * @brief Constructs a new pixmap initialized with a designed color.
			 * @tparam color_data_t The color data type. Default float.
			 * @param width The initial width of the pixmap.
			 * @param height The initial height of the pixmap.
			 * @param channelMode The desired color count.
			 * @param color A reference to a color.
			 */
			template< typename color_data_t = float >
			Pixmap (dimension_t width, dimension_t height, ChannelMode channelMode, const Color< color_data_t > & color) requires (std::is_floating_point_v< color_data_t >)
				: m_width(width),
				m_height(height),
				m_channelMode(channelMode)
			{
				m_data.resize(this->elementCount());

				if constexpr ( IsDebug )
				{
					if ( !this->fill(color) )
					{
						std::cerr << "Unable to initialize color !" "\n";
					}
				}
				else
				{
					this->fill(color);
				}
			}

			/**
			 * @brief Initializes the pixmap data.
			 * @param width The initial width of the pixmap.
			 * @param height The initial height of the pixmap.
			 * @param channelMode The desired color count. Default RGB.
			 * @return bool
			 */
			bool
			initialize (dimension_t width, dimension_t height, ChannelMode channelMode = ChannelMode::RGB)
			{
				if ( width == 0 || height == 0 )
				{
					if constexpr ( IsDebug )
					{
						std::cerr << "Invalid pixmap dimensions !" "\n";
					}

					return false;
				}

				m_width = width;
				m_height = height;
				m_channelMode = channelMode;
				m_data.resize(this->elementCount());

				return this->initAlphaChannel();
			}

			/**
			 * @brief Clears the pixmap data.
			 * @note Width and height will be reset to zero and the channel mode to RGB.
			 * @return void
			 */
			void
			clear () noexcept
			{
				m_width = 0;
				m_height = 0;
				m_channelMode = ChannelMode::RGB;
				m_data.clear();
			}

			/**
			 * @brief Returns the pixmap width in pixels.
			 * @return dimension_t
			 */
			[[nodiscard]]
			dimension_t
			width () const noexcept
			{
				return m_width;
			}

			/**
			 * @brief Returns the pixmap half-width in pixels.
			 * @return dimension_t
			 */
			[[nodiscard]]
			dimension_t
			halfWidth () const noexcept
			{
				return static_cast< dimension_t >(std::floor(static_cast< float >(this->width()) * 0.5F));
			}

			/**
			 * @brief Returns the pixmap height in pixels.
			 * @return dimension_t
			 */
			[[nodiscard]]
			dimension_t
			height () const noexcept
			{
				return m_height;
			}

			/**
			 * @brief Returns the pixmap half-height in pixels.
			 * @return dimension_t
			 */
			[[nodiscard]]
			dimension_t
			halfHeight () const noexcept
			{
				return static_cast< dimension_t >(std::floor(static_cast< float >(this->height()) * 0.5F));
			}

			/**
			 * @brief Returns the pixmap ratio.
			 * @tparam output_t The output data type. Default float.
			 * @return float
			 */
			template< typename output_t >
			[[nodiscard]]
			output_t
			ratio () const noexcept
			{
				if ( this->width() == 0 || this->height() == 0 )
				{
					return static_cast< output_t >(0);
				}

				if ( this->width() == this->height() )
				{
					return static_cast< output_t >(1);
				}

				return static_cast< output_t >(this->width()) / static_cast< output_t >(this->height());
			}

			/**
			 * @brief Returns whether the data is non-empty and dimension valid.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isValid () const noexcept
			{
				if ( this->width() == 0 || this->height() == 0 )
				{
					return false;
				}

				return !m_data.empty();
			}

			/**
			 * @brief Returns the channel mode.
			 * @return ChannelMode
			 */
			[[nodiscard]]
			ChannelMode
			channelMode () const noexcept
			{
				return m_channelMode;
			}

			/**
			 * @brief Returns the pixel count of the pixmap.
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			pixelCount () const noexcept requires (std::is_unsigned_v< output_t >)
			{
				return static_cast< output_t >(this->width() * this->height());
			}

			/**
			 * @brief Returns the color count by pixel.
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			colorCount () const noexcept
			{
				return static_cast< output_t >(this->channelMode());
			}

			/**
			 * @brief Returns the element count of the pixmap (WIDTH * HEIGHT * COLOR COUNT).
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			elementCount () const noexcept
			{
				return this->pixelCount() * this->colorCount();
			}

			/**
			 * @brief Returns the size of the Pixmap in bytes (WIDTH * HEIGHT * COLOR COUNT * PRECISION).
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			bytes () const noexcept
			{
				return this->elementCount() * static_cast< output_t >(sizeof(pixel_data_t));
			}

			/**
			 * @brief Returns the size of a pixel in bytes (COLOR COUNT * PRECISION).
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			bytePerPixel () const noexcept
			{
				return this->colorCount() * static_cast< output_t >(sizeof(pixel_data_t));
			}

			/**
			 * @brief Returns the size of a pixel in bits (COLOR COUNT * PRECISION * 8).
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			bitPerPixel () const noexcept
			{
				return this->bytePerPixel() * static_cast< output_t >(8);
			}

			/**
			 * @brief Returns the size in bytes for one row of pixel (WIDTH * COLOR COUNT * PRECISION).
			 * @tparam output_t The type of output data. Default uint32_t.
			 * @return output_t
			 */
			template< typename output_t = uint32_t >
			[[nodiscard]]
			output_t
			pitch () const noexcept
			{
				return this->width() * this->colorCount() * static_cast< output_t >(sizeof(pixel_data_t));
			}

			/**
			 * @brief Returns whether the pixmap is a square.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isSquare () const noexcept
			{
				return this->width() == this->height();
			}

			/**
			 * @brief Returns a rectangle corresponding to the pixmap.
			 * @note Offset parameter is added to the size of the pixmap.
			 * @tparam rectangle_data_t The type of rectangle data. Default int32_t.
			 * @param offsetX Set an offset in X to the rectangle. Default 0.
			 * @param offsetY Set an offset in Y to the rectangle. Default 0.
			 * @return Math::Space2D::AARectangle< rectangle_data_t >
			 */
			template< typename rectangle_data_t = int32_t >
			[[nodiscard]]
			Math::Space2D::AARectangle< rectangle_data_t >
			rectangle (rectangle_data_t offsetX = 0, rectangle_data_t offsetY = 0) const noexcept
			{
				return {offsetX, offsetY, static_cast< rectangle_data_t >(this->width()), static_cast< rectangle_data_t >(this->height())};
			}

			/**
			 * @brief Returns whether the pixmap dimensions are a power of two.
			 * @note Useful for GPU concerns.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isPowerOfTwo () const noexcept
			{
				return std::has_single_bit(this->width()) && std::has_single_bit(this->height());
			}

			/**
			 * @brief Returns read-only access to raw data of the pixmap.
			 * @return const std::vector< pixel_data_t > &
			 */
			[[nodiscard]]
			const std::vector< pixel_data_t > &
			data () const noexcept
			{
				return m_data;
			}

			/**
			 * @brief Returns a write access to raw data of the pixmap.
			 * @return std::vector< pixel_data_t > &
			 */
			[[nodiscard]]
			std::vector< pixel_data_t > &
			data () noexcept
			{
				return m_data;
			}

			/**
			 * @brief Returns a read-only pointer to the first element of a row.
			 * @warning On row overflow, the method will return the last one.
			 * @param rowIndex The index of the row.
			 * @return const pixel_data_t *
			 */
			[[nodiscard]]
			const pixel_data_t *
			rowPointer (size_t rowIndex) const noexcept
			{
				assert(rowIndex < this->height());

				return m_data.data() + (rowIndex * this->width() * this->colorCount());
			}

			/**
			 * @brief Returns the pointer to the first element of a row.
			 * @warning On row overflow, the method will return the last one.
			 * @param rowIndex The index of the row.
			 * @return pixel_data_t *
			 */
			[[nodiscard]]
			pixel_data_t *
			rowPointer (size_t rowIndex) noexcept
			{
				assert(rowIndex < this->height());

				return m_data.data() + (rowIndex * this->width() * this->colorCount());
			}

			/**
			 * @brief Returns a raw pixel pointer by its buffer index.
			 * @warning On pixel overflow, the method will return the last one.
			 * @param pixelIndex The index of the pixel.
			 * @return const pixel_data_t *
			 */
			[[nodiscard]]
			const pixel_data_t *
			pixelPointer (size_t pixelIndex) const noexcept
			{
				assert(pixelIndex < this->pixelCount());

				return m_data.data() + (pixelIndex * this->colorCount());
			}

			/**
			 * @brief Returns a raw pixel pointer by its buffer index.
			 * @param pixelIndex The index of the pixel.
			 * @return const pixel_data_t *
			 */
			[[nodiscard]]
			pixel_data_t *
			pixelPointer (size_t pixelIndex) noexcept
			{
				assert(pixelIndex < this->pixelCount());

				return m_data.data() + (pixelIndex * this->colorCount());
			}

			/**
			 * @brief Returns a raw pixel pointer by its image coordinates.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @return const pixel_data_t *
			 */
			[[nodiscard]]
			const pixel_data_t *
			pixelPointer (dimension_t coordX, dimension_t coordY) const noexcept
			{
				return m_data.data() + (this->pixelIndex(coordX, coordY) * this->colorCount());
			}

			/**
			 * @brief Returns a raw pixel pointer by its image coordinates.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @return const pixel_data_t *
			 */
			[[nodiscard]]
			pixel_data_t *
			pixelPointer (dimension_t coordX, dimension_t coordY) noexcept
			{
				return m_data.data() + (this->pixelIndex(coordX, coordY) * this->colorCount());
			}

			/**
			 * @brief Returns the X coordinates in the image from the array index.
			 * @param pixelIndex The pixel index.
			 * @return dimension_t
			 */
			[[nodiscard]]
			dimension_t
			x (size_t pixelIndex) const noexcept
			{
				assert(pixelIndex < this->pixelCount());

				return pixelIndex % m_width;
			}

			/**
			 * @brief Returns the Y coordinates in the image from the array index.
			 * @param pixelIndex The pixel index.
			 * @return dimension_t
			 */
			[[nodiscard]]
			dimension_t
			y (size_t pixelIndex) const noexcept
			{
				assert(pixelIndex < this->pixelCount());

				return pixelIndex / m_width;
			}

			/**
			 * @brief Returns the U texture2Ds coordinates in the image from the array index.
			 * @warning On pixel overflow, the method will return the last one.
			 * @tparam output_t The output data type. Default float.
			 * @param pixelIndex The pixel index.
			 * @return float
			 */
			template< typename output_t = float >
			[[nodiscard]]
			output_t
			u (size_t pixelIndex) const noexcept requires (std::is_floating_point_v< output_t >)
			{
				return static_cast< output_t >(this->x(pixelIndex)) / static_cast< output_t >(m_width - 1);
			}

			/**
			 * @brief Returns the V texture2Ds coordinates in the image from the array index.
			 * @warning On pixel overflow, the method will return the last one.
			 * @tparam output_t The output data type. Default float.
			 * @param pixelIndex The pixel index.
			 * @return float
			 */
			template< typename output_t = float >
			[[nodiscard]]
			float
			v (size_t pixelIndex) const noexcept requires (std::is_floating_point_v< output_t >)
			{
				return static_cast< output_t >(this->y(pixelIndex)) / static_cast< output_t >(m_height - 1);
			}

			/**
			 * @brief Returns the pixel buffer index by its image coordinates.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			pixelIndex (dimension_t coordX, dimension_t coordY) const noexcept
			{
				assert(coordX < m_width && coordY < m_height);

				return (coordY * m_width) + coordX;
			}

			/**
			 * @brief Sets a color to a pixel.
			 * @warning Do not use this method in a loop.
			 * @tparam color_data_t The color data type. Default float.
			 * @param pixelIndex The index of the pixel.
			 * @param color A reference to the color of the pixel.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			setPixel (size_t pixelIndex, const Color< color_data_t > & color) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				assert(pixelIndex < this->pixelCount());

				const auto dataOffset = pixelIndex * this->colorCount();

				if constexpr ( std::is_floating_point_v< pixel_data_t > )
				{
					switch ( m_channelMode )
					{
						case ChannelMode::Grayscale :
							m_data[dataOffset] = color.gray();
							break;

						case ChannelMode::GrayscaleAlpha :
							m_data[dataOffset] = color.gray();
							m_data[dataOffset + 1] = color.alpha();
							break;

						case ChannelMode::RGB :
							m_data[dataOffset] = color.red();
							m_data[dataOffset + 1] = color.green();
							m_data[dataOffset + 2] = color.blue();
							break;

						case ChannelMode::RGBA :
							m_data[dataOffset] = color.red();
							m_data[dataOffset + 1] = color.green();
							m_data[dataOffset + 2] = color.blue();
							m_data[dataOffset + 3] = color.alpha();
							break;
					}
				}
				else
				{
					switch ( m_channelMode )
					{
						case ChannelMode::Grayscale :
							m_data[dataOffset] = color.template grayInteger< pixel_data_t >();
							break;

						case ChannelMode::GrayscaleAlpha :
							m_data[dataOffset] = color.template grayInteger< pixel_data_t >();
							m_data[dataOffset + 1] = color.template alphaInteger< pixel_data_t >();
							break;

						case ChannelMode::RGB :
							m_data[dataOffset] = color.template redInteger< pixel_data_t >();
							m_data[dataOffset + 1] = color.template greenInteger< pixel_data_t >();
							m_data[dataOffset + 2] = color.template blueInteger< pixel_data_t >();
							break;

						case ChannelMode::RGBA :
							m_data[dataOffset] = color.template redInteger< pixel_data_t >();
							m_data[dataOffset + 1] = color.template greenInteger< pixel_data_t >();
							m_data[dataOffset + 2] = color.template blueInteger< pixel_data_t >();
							m_data[dataOffset + 3] = color.template alphaInteger< pixel_data_t >();
							break;
					}
				}

				if ( m_flags[UpdatedRegionMarkerEnabled] )
				{
					this->markPixelUpdated(
						static_cast< dimension_t >(pixelIndex) % m_width,
						static_cast< dimension_t >(pixelIndex) / m_width
					);
				}
			}

			/**
			 * @brief Sets a color to a pixel using image coordinates.
			 * @warning Do not use this method in a loop.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param color A reference to the color of the pixel.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			setPixel (dimension_t coordX, dimension_t coordY, const Color< color_data_t > & color) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				this->setPixel(this->pixelIndex(coordX, coordY), color);
			}

			/**
			 * @brief Sets a color to a pixel using image coordinates. It will be ignored if it falls outside the pixmap.
			 * @warning Do not use this method in a loop.
			 * @note The change will be ignored if the pixel coordinates are outside the pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param color A reference to the color of the pixel.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			setFreePixel (int32_t coordX, int32_t coordY, const Color< color_data_t > & color) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if (
					coordX >= 0 && coordX < static_cast< int32_t >(this->width()) &&
					coordY >= 0 && coordY < static_cast< int32_t >(this->height())
				)
				{
					const auto pixelIndex = this->pixelIndex(static_cast< dimension_t >(coordX), static_cast< dimension_t >(coordY));

					this->setPixel(pixelIndex, color);
				}
			}

			/**
			 * @brief Mixes a color to an existing pixel with a scalar value.
			 * @tparam color_data_t The color data type. Default float.
			 * @param pixelIndex The index of the pixel.
			 * @param color A reference to a color.
			 * @param mix The mix scale.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			mixPixel (size_t pixelIndex, const Color< color_data_t > & color, float mix) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				const auto previousColor = this->pixel(pixelIndex);

				this->setPixel(pixelIndex, Color< color_data_t >::mix(previousColor, color, mix));
			}

			/**
			 * @brief Mixes a color to an existing pixel with a scalar value.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param color A reference to a color.
			 * @param mix The blending technics.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			mixPixel (dimension_t coordX, dimension_t coordY, const Color< color_data_t > & color, float mix) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				const auto previousColor = this->pixel(coordX, coordY);

				this->setPixel(coordX, coordY, Color< color_data_t >::mix(previousColor, color, mix));
			}

			/**
			 * @brief Mixes a color to an existing pixel with a scalar value. It will be ignored if it falls outside the pixmap.
			 * @warning Do not use this method, use mixPixel() instead.
			 * @note The change will be ignored if the pixel coordinates are outside the pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param color A reference to a color.
			 * @param mix The blending technics.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			mixFreePixel (int32_t coordX, int32_t coordY, const Color< color_data_t > & color, float mix) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if (
					coordX >= 0 && coordX < static_cast< int32_t >(this->width()) &&
					coordY >= 0 && coordY < static_cast< int32_t >(this->height())
				)
				{
                    const auto pixelIndex = this->pixelIndex(static_cast< dimension_t >(coordX), static_cast< dimension_t >(coordY));

                    const auto previousColor = this->pixel(pixelIndex);

					this->setPixel(pixelIndex, Color< color_data_t >::mix(previousColor, color, mix));
				}
			}

			/**
			 * @brief Blends a color to an existing pixel.
			 * @tparam color_data_t The color data type. Default float.
			 * @param pixelIndex The index of the pixel.
			 * @param color A reference to a color.
			 * @param mode The blending technics.
			 * @param opacity A global opacity (Ignored with Replace). Default 1.0.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			blendPixel (size_t pixelIndex, const Color< color_data_t > & color, DrawPixelMode mode, float opacity = 1.0F) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( mode == DrawPixelMode::Replace )
				{
					this->setPixel(pixelIndex, color);
				}
				else
				{
					const auto previousColor = this->pixel(pixelIndex);

					this->setPixel(pixelIndex, Color< color_data_t >::blend(previousColor, color, mode, opacity));
				}
			}

			/**
			 * @brief Blends a color to an existing pixel.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param color A reference to a color.
			 * @param mode The blending technics.
			 * @param opacity A global opacity (Ignored with Replace). Default 1.0.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			blendPixel (dimension_t coordX, dimension_t coordY, const Color< color_data_t > & color, DrawPixelMode mode, float opacity = 1.0F) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( mode == DrawPixelMode::Replace )
				{
					this->setPixel(coordX, coordY, color);
				}
				else
				{
					const auto pixelIndex = this->pixelIndex(coordX, coordY);

					const auto previousColor = this->pixel(pixelIndex);

					this->setPixel(pixelIndex, Color< color_data_t >::blend(previousColor, color, mode, opacity));
				}
			}

			/**
			 * @brief Blends a color to an existing pixel. It will be ignored if it falls outside the pixmap.
			 * @warning Do not use this method, use blendPixel() instead.
			 * @note The change will be ignored if the pixel coordinates are outside the pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param color A reference to a color.
			 * @param mode The blending technics.
			 * @param opacity A global opacity (Ignored with Replace). Default 1.0.
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			blendFreePixel (int32_t coordX, int32_t coordY, const Color< color_data_t > & color, DrawPixelMode mode, float opacity = 1.0F) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if (
					coordX >= 0 && coordX < static_cast< int32_t >(this->width()) &&
					coordY >= 0 && coordY < static_cast< int32_t >(this->height())
				)
				{
					if ( mode == DrawPixelMode::Replace )
					{
						this->setPixel(static_cast< dimension_t >(coordX), static_cast< dimension_t >(coordY), color);
					}
					else
					{
						const auto pixelIndex = this->pixelIndex(static_cast< dimension_t >(coordX), static_cast< dimension_t >(coordY));

						const auto previousColor = this->pixel(pixelIndex);

						this->setPixel(pixelIndex, Color< color_data_t >::blend(previousColor, color, mode, opacity));
					}
				}
			}

			/**
			 * @brief Returns a pixel color at buffer index.
			 * @tparam color_data_t The color data type. Default float.
			 * @param pixelIndex The index of the pixel.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float >
			[[nodiscard]]
			Color< color_data_t >
			pixel (size_t pixelIndex) const noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				assert(pixelIndex < this->pixelCount());

				const auto dataOffset = pixelIndex * this->colorCount();

				if constexpr ( std::is_floating_point_v< pixel_data_t > )
				{
					switch ( m_channelMode )
					{
						case ChannelMode::Grayscale :
							return {m_data[dataOffset], m_data[dataOffset], m_data[dataOffset], 1};

						case ChannelMode::GrayscaleAlpha :
							return {m_data[dataOffset], m_data[dataOffset], m_data[dataOffset], m_data[dataOffset+1]};

						case ChannelMode::RGB :
							return {m_data[dataOffset], m_data[dataOffset+1], m_data[dataOffset+2], 1};

						case ChannelMode::RGBA :
							return {m_data[dataOffset], m_data[dataOffset+1], m_data[dataOffset+2], m_data[dataOffset+3]};
					}
				}
				else
				{
					switch ( m_channelMode )
					{
						case ChannelMode::Grayscale :
							return ColorFromInteger(m_data[dataOffset], m_data[dataOffset], m_data[dataOffset], Pixmap::one());

						case ChannelMode::GrayscaleAlpha :
							return ColorFromInteger(m_data[dataOffset], m_data[dataOffset], m_data[dataOffset], m_data[dataOffset+1]);

						case ChannelMode::RGB :
							return ColorFromInteger(m_data[dataOffset], m_data[dataOffset+1], m_data[dataOffset+2], Pixmap::one());

						case ChannelMode::RGBA :
							return ColorFromInteger(m_data[dataOffset], m_data[dataOffset+1], m_data[dataOffset+2], m_data[dataOffset+3]);
					}
				}

				return {};
			}

			/**
			 * @brief Returns a pixel color by its image coordinates.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float >
			[[nodiscard]]
			Color< color_data_t >
			pixel (dimension_t coordX, dimension_t coordY) const noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				return this->pixel(this->pixelIndex(coordX, coordY));
			}

			/**
			 * @brief Returns a pixel color at buffer index.
			 * @note This version returns a default color on overflow.
			 * @tparam color_data_t The color data type. Default float.
			 * @param pixelIndex The index of the pixel.
			 * @param defaultColor Returns the color for out of bound pixel. Default black.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float >
			[[nodiscard]]
			Color< color_data_t >
			safePixel (size_t pixelIndex, const Color< color_data_t > & defaultColor = Black) const noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( pixelIndex >= this->pixelCount() )
				{
					return defaultColor;
				}

				return this->pixel(pixelIndex);
			}

			/**
			 * @brief Returns a pixel color by its image coordinates.
			 * @note This version returns a default color on overflow.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param defaultColor Returns the color for out of bound pixel. Default black.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float >
			[[nodiscard]]
			Color< color_data_t >
			safePixel (dimension_t coordX, dimension_t coordY, const Color< color_data_t > & defaultColor = Black) const noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( coordX >= m_width || coordY >= m_height)
				{
					return defaultColor;
				}

				return this->pixel(this->pixelIndex(coordX, coordY));
			}

			/**
			 * @brief Returns the closest pixel to boundary at coordinates.
			 * @tparam color_data_t The color data type. Default float.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float >
			[[nodiscard]]
			Color< color_data_t >
			closestPixel (int32_t coordX, int32_t coordY) const noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( coordX < 0 )
				{
					coordX = 0;
				}
				else if ( coordX >= m_width )
				{
					coordX = m_width - 1;
				}

				if ( coordY < 0 )
				{
					coordY = 0;
				}
				else if ( coordY >= m_height )
				{
					coordY = m_height - 1;
				}

				return this->pixel(static_cast< dimension_t >(coordX), static_cast< dimension_t >(coordY));
			}

			/**
			 * @brief Returns a sampled pixel using the nearest algorithm.
			 * @note Will return a black color when outside the pixmap and UVWrappingEnabled turned off.
			 * @tparam color_data_t The color data type. Default float.
			 * @tparam uv_t The UV coordinates data type. Default float.
			 * @param u The X coordinate of the pixel.
			 * @param v The Y coordinate of the pixel.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float, typename uv_t = float >
			[[nodiscard]]
			Color< color_data_t >
			nearestSample (uv_t u, uv_t v) const noexcept requires (std::is_floating_point_v< color_data_t > && std::is_floating_point_v< uv_t >)
			{
				constexpr uv_t Zero{0};
				constexpr uv_t One{1};

				if ( m_flags[UVWrappingEnabled] )
				{
					if ( u < Zero || u > One )
					{
						u = std::fmod(std::abs(u), One);
					}

					if ( v < Zero || v > One )
					{
						v = std::fmod(std::abs(v), One);
					}
				}
				else if ( u < Zero || u > One || v < Zero || v > One )
				{
					return Black;
				}

				/* Prepare variable for X axis. */
				const auto realX = static_cast< uv_t >(m_width - 1) * u;

				/* Prepare variable for Y axis. */
				const auto realY = static_cast< uv_t >(m_height - 1) * v;

				return this->pixel(
					static_cast< dimension_t >(std::round(realX)),
					static_cast< dimension_t >(std::round(realY))
				);
			}

			/**
			 * @brief Returns a sampled pixel using the linear algorithm.
			 * @note Will return a black color when outside the pixmap and UVWrappingEnabled turned off.
			 * @tparam color_data_t The color data type. Default float.
			 * @tparam uv_t The UV coordinates data type. Default float.
			 * @param u The X coordinate of the pixel.
			 * @param v The Y coordinate of the pixel.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float, typename uv_t = float >
			[[nodiscard]]
			Color< color_data_t >
			linearSample (uv_t u, uv_t v) const noexcept requires (std::is_floating_point_v< color_data_t > && std::is_floating_point_v< uv_t >)
			{
				constexpr uv_t Zero{0};
				constexpr uv_t One{1};

				if ( m_flags[UVWrappingEnabled] )
				{
					if ( u < Zero || u > One )
					{
						u = std::fmod(std::abs(u), One);
					}

					if ( v < Zero || v > One )
					{
						v = std::fmod(std::abs(v), One);
					}
				}
				else if ( u < Zero || u > One || v < Zero || v > One )
				{
					return Black;
				}

				/* Prepare variable for X axis. */
				const auto realX = static_cast< uv_t >(m_width - 1) * u;
				const auto loX = static_cast< dimension_t >(std::floor(realX));
				const auto hiX = static_cast< dimension_t >(std::ceil(realX));

				/* Prepare variable for Y axis. */
				const auto realY = static_cast< uv_t >(m_height - 1) * v;
				const auto loY = static_cast< dimension_t >(std::floor(realY));
				const auto hiY = static_cast< dimension_t >(std::ceil(realY));

				return Color< color_data_t >::bilinearInterpolation(
					this->pixel(loX, loY),
					this->pixel(hiX, loY),
					this->pixel(loX, hiY),
					this->pixel(hiX, hiY),
					realX - static_cast< uv_t >(loX),
					realY - static_cast< uv_t >(loY)
				);
			}

			/**
			 * @brief Returns a sampled pixel using the cosine algorithm.
			 * @note Will return a black color when outside the pixmap and UVWrappingEnabled turned off.
			 * @tparam color_data_t The color data type. Default float.
			 * @tparam uv_t The UV coordinates data type. Default float.
			 * @param u The X coordinate of the pixel.
			 * @param v The Y coordinate of the pixel.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float, typename uv_t = float >
			[[nodiscard]]
			Color< color_data_t >
			cosineSample (uv_t u, uv_t v) const noexcept requires (std::is_floating_point_v< color_data_t > && std::is_floating_point_v< uv_t >)
			{
				constexpr uv_t Zero{0};
				constexpr uv_t One{1};

				if ( m_flags[UVWrappingEnabled] )
				{
					if ( u < Zero || u > One )
					{
						u = std::fmod(std::abs(u), One);
					}

					if ( v < Zero || v > One )
					{
						v = std::fmod(std::abs(v), One);
					}
				}
				else if ( u < Zero || u > One || v < Zero || v > One )
				{
					return Black;
				}

				/* Prepare variable for X axis. */
				const auto realX = static_cast< uv_t >(m_width - 1) * u;
				const auto loX = static_cast< dimension_t >(std::floor(realX));
				const auto hiX = static_cast< dimension_t >(std::ceil(realX));

				/* Prepare variable for Y axis. */
				const auto realY = static_cast< uv_t >(m_height - 1) * v;
				const auto loY = static_cast< dimension_t >(std::floor(realY));
				const auto hiY = static_cast< dimension_t >(std::ceil(realY));

				return Color< color_data_t >::biCosineInterpolation(
					this->pixel(loX, loY),
					this->pixel(hiX, loY),
					this->pixel(loX, hiY),
					this->pixel(hiX, hiY),
					realX - static_cast< uv_t >(loX),
					realY - static_cast< uv_t >(loY)
				);
			}

			/**
			 * @brief Returns a sampled pixel using the cubic algorithm.
			 * @note Will return a black color when outside the pixmap and UVWrappingEnabled turned off.
			 * @tparam color_data_t The color data type. Default float.
			 * @tparam uv_t The UV coordinates data type. Default float.
			 * @param u The X coordinate of the pixel.
			 * @param v The Y coordinate of the pixel.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float, typename uv_t = float >
			[[nodiscard]]
			Color< color_data_t >
			cubicSample (float u, float v) const noexcept requires (std::is_floating_point_v< color_data_t > && std::is_floating_point_v< uv_t >)
			{
				constexpr uv_t Zero{0};
				constexpr uv_t One{1};

				if ( m_flags[UVWrappingEnabled] )
				{
					if ( u < Zero || u > One )
					{
						u = std::fmod(std::abs(u), One);
					}

					if ( v < Zero || v > One )
					{
						v = std::fmod(std::abs(v), One);
					}
				}
				else if ( u < Zero || u > One || v < Zero || v > One )
				{
					return Black;
				}

				/* Prepare variable for X axis */
				const auto realX = static_cast< uv_t >(m_width - 1) * u;
				const auto loX = static_cast< dimension_t >(std::floor(realX));
				const auto factorX = realX - static_cast< uv_t >(loX);

				/* Prepare variable for Y axis */
				const auto realY = static_cast< uv_t >(m_height - 1) * v;
				const auto loY = static_cast< dimension_t >(std::floor(realY));
				const auto factorY = realY - static_cast< uv_t >(loY);

				/* Gets involved pixels */
				const auto p00 = this->safePixel(loX - 1, loY - 1);
				const auto p10 = this->safePixel(loX + 0, loY - 1);
				const auto p20 = this->safePixel(loX + 1, loY - 1);
				const auto p30 = this->safePixel(loX + 2, loY - 1);

				const auto p01 = this->safePixel(loX - 1, loY + 0);
				const auto p11 = this->safePixel(loX + 0, loY + 0);
				const auto p21 = this->safePixel(loX + 1, loY + 0);
				const auto p31 = this->safePixel(loX + 2, loY + 0);

				const auto p02 = this->safePixel(loX - 1, loY + 1);
				const auto p12 = this->safePixel(loX + 0, loY + 1);
				const auto p22 = this->safePixel(loX + 1, loY + 1);
				const auto p32 = this->safePixel(loX + 2, loY + 1);

				const auto p03 = this->safePixel(loX - 1, loY + 2);
				const auto p13 = this->safePixel(loX + 0, loY + 2);
				const auto p23 = this->safePixel(loX + 1, loY + 2);
				const auto p33 = this->safePixel(loX + 2, loY + 2);

				/* Red component */
				const auto red = Math::cubicInterpolation(
					Math::cubicInterpolation(p00.red(), p10.red(), p20.red(), p30.red(), factorX),
					Math::cubicInterpolation(p01.red(), p11.red(), p21.red(), p31.red(), factorX),
					Math::cubicInterpolation(p02.red(), p12.red(), p22.red(), p32.red(), factorX),
					Math::cubicInterpolation(p03.red(), p13.red(), p23.red(), p33.red(), factorX),
					factorY
				);

				/* Green component */
				const auto green = Math::cubicInterpolation(
					Math::cubicInterpolation(p00.green(), p10.green(), p20.green(), p30.green(), factorX),
					Math::cubicInterpolation(p01.green(), p11.green(), p21.green(), p31.green(), factorX),
					Math::cubicInterpolation(p02.green(), p12.green(), p22.green(), p32.green(), factorX),
					Math::cubicInterpolation(p03.green(), p13.green(), p23.green(), p33.green(), factorX),
					factorY
				);

				/* Blue component */
				const auto blue = Math::cubicInterpolation(
					Math::cubicInterpolation(p00.blue(), p10.blue(), p20.blue(), p30.blue(), factorX),
					Math::cubicInterpolation(p01.blue(), p11.blue(), p21.blue(), p31.blue(), factorX),
					Math::cubicInterpolation(p02.blue(), p12.blue(), p22.blue(), p32.blue(), factorX),
					Math::cubicInterpolation(p03.blue(), p13.blue(), p23.blue(), p33.blue(), factorX),
					factorY
				);

				/* Alpha component */
				const auto alpha = Math::cubicInterpolation(
					Math::cubicInterpolation(p00.alpha(), p10.alpha(), p20.alpha(), p30.alpha(), factorX),
					Math::cubicInterpolation(p01.alpha(), p11.alpha(), p21.alpha(), p31.alpha(), factorX),
					Math::cubicInterpolation(p02.alpha(), p12.alpha(), p22.alpha(), p32.alpha(), factorX),
					Math::cubicInterpolation(p03.alpha(), p13.alpha(), p23.alpha(), p33.alpha(), factorX),
					factorY
				);

				return {red, green, blue, alpha};
			}

			/**
			 * @brief Sets a pixel sub-element.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param channel The pixmap channel targeted.
			 * @param value The new value.
			 */
			void
			setPixelElement (dimension_t coordX, dimension_t coordY, Channel channel, pixel_data_t value) noexcept
			{
				const auto dataOffset = (this->pixelIndex(coordX, coordY) * this->colorCount< size_t >()) + static_cast< size_t >(channel);

				m_data[dataOffset] = value;

				if ( m_flags[UpdatedRegionMarkerEnabled] )
				{
					this->markPixelUpdated(coordX, coordY);
				}
			}

			/**
			 * @brief Returns a pixel sub-element.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @param channel The pixmap channel targeted.
			 * @return pixel_data_t
			 */
			[[nodiscard]]
			pixel_data_t
			pixelElement (dimension_t coordX, dimension_t coordY, Channel channel) const noexcept
			{
				const auto dataOffset = (this->pixelIndex(coordX, coordY) * this->colorCount< size_t >()) + static_cast< size_t >(channel);

				return m_data[dataOffset];
			}

			/**
			 * @brief Returns whether the pixmap is grayscale.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isGrayScale () const noexcept
			{
				return m_channelMode == ChannelMode::Grayscale;
			}

			/**
			 * @brief Returns whether the pixmap has an alpha channel.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			hasAlphaChannel () const noexcept
			{
				return m_channelMode == ChannelMode::GrayscaleAlpha || m_channelMode == ChannelMode::RGBA;
			}

			/**
			 * @brief Returns the average color of the pixmap.
			 * @tparam color_data_t The color data type. Default float.
			 * @return Color< color_data_t >
			 */
			template< typename color_data_t = float >
			[[nodiscard]]
			Color< color_data_t >
			averageColor () const noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !this->isValid() )
				{
					return {};
				}

				const auto limit = this->pixelCount();
				const auto stride = this->colorCount();
				const auto dataLimit = m_data.size();

				switch ( m_channelMode )
				{
					case ChannelMode::Grayscale :
					case ChannelMode::GrayscaleAlpha :
						if constexpr ( std::is_floating_point_v< pixel_data_t > )
						{
							pixel_data_t maxValue = 0;

							for ( size_t index = 0; index < dataLimit; index += stride )
							{
								maxValue += m_data[index];
							}

							const auto value = static_cast< color_data_t >(maxValue / limit);

							return {value, value, value};
						}
						else
						{
							uint32_t maxValue = 0;

							for ( size_t index = 0; index < dataLimit; index += stride )
							{
								maxValue += m_data[index];
							}

							const auto value = static_cast< pixel_data_t >(std::floor(maxValue / limit));

							return ColorFromInteger(value, value, value);
						}

					case ChannelMode::RGB :
					case ChannelMode::RGBA :
						if constexpr ( std::is_floating_point_v< pixel_data_t > )
						{
							pixel_data_t maxR = 0;
							pixel_data_t maxG = 0;
							pixel_data_t maxB = 0;

							for ( size_t index = 0; index < dataLimit; index += stride )
							{
								maxR += m_data[index];
								maxG += m_data[index+1];
								maxB += m_data[index+2];
							}

							return {
								static_cast< color_data_t >(maxR / limit),
								static_cast< color_data_t >(maxG / limit),
								static_cast< color_data_t >(maxB / limit)
							};
						}
						else
						{
							uint32_t maxR = 0;
							uint32_t maxG = 0;
							uint32_t maxB = 0;

							for ( size_t index = 0; index < dataLimit; index += stride )
							{
								maxR += m_data[index];
								maxG += m_data[index+1];
								maxB += m_data[index+2];
							}

							return ColorFromInteger(
								static_cast< pixel_data_t >(std::floor(maxR / limit)),
								static_cast< pixel_data_t >(std::floor(maxG / limit)),
								static_cast< pixel_data_t >(std::floor(maxB / limit))
							);
						}

					default:
						return {};
				}
			}

			/**
			 * @brief Enables the updated region marker.
			 * @note This feature will update a rectangle where changes are made in the pixmap. Use Pixmap::updatedRegion() to get this region.
			 * @param state The state.
			 */
			void
			enableUpdatedRegionMarker (bool state) noexcept
			{
				m_flags[UpdatedRegionMarkerEnabled] = state;
			}

			/**
			 * @brief Returns whether the updated region marker is enabled.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isUpdatedRegionMarkerEnabled () const noexcept
			{
				return m_flags[UpdatedRegionMarkerEnabled];
			}

			/**
			 * @brief Returns the region of the pixmap where changes have been made since the last marker reset.
			 * @note Remember to call Pixmap::resetUpdatedRegionMarker() when you are done with the rectangle for the next update.
			 * @return const Math::Space2D::AARectangle< dimension_t > &
			 */
			[[nodiscard]]
			const Math::Space2D::AARectangle< dimension_t > &
			updatedRegion () const noexcept
			{
				return m_lastUpdatedRegion;
			}

			/**
			 * @brief Updates the updated region marker from one pixel.
			 * @param coordX The X coordinate of the pixel.
			 * @param coordY The Y coordinate of the pixel.
			 * @return void
			 */
			void
			markPixelUpdated (dimension_t coordX, dimension_t coordY) noexcept
			{
				if ( m_lastUpdatedRegion.isValid() )
				{
					if ( coordX < m_lastUpdatedRegion.left() )
					{
						m_lastUpdatedRegion.setLeft(coordX);
						m_lastUpdatedRegion.modifyWidthBy(m_lastUpdatedRegion.left() - coordX);
					}

					if ( coordX > m_lastUpdatedRegion.right() )
					{
						m_lastUpdatedRegion.setWidth((coordX - m_lastUpdatedRegion.left()) + 1);
					}

					if ( coordY < m_lastUpdatedRegion.top() )
					{
						m_lastUpdatedRegion.setTop(coordY);
						m_lastUpdatedRegion.modifyHeightBy(m_lastUpdatedRegion.top() - coordY);
					}

					if ( coordY > m_lastUpdatedRegion.bottom() )
					{
						m_lastUpdatedRegion.setHeight((coordY - m_lastUpdatedRegion.top()) + 1);
					}
				}
				else
				{
					m_lastUpdatedRegion = {coordX, coordY, 1, 1};
				}
			}

			/**
			 * @brief Updates the updated region marker from a rectangle.
			 * @param rectangle A reference to a rectangle.
			 * @return void
			 */
			void
			markRectangleUpdated (const Math::Space2D::AARectangle< dimension_t > & rectangle) noexcept
			{
				if ( m_flags[UpdatedRegionMarkerEnabled] )
				{
					m_lastUpdatedRegion.merge(rectangle);
				}
			}

			/**
			 * @brief Updates whole the updated region marker.
			 * @return void
			 */
			void
			markEverythingUpdated () noexcept
			{
				if ( m_flags[UpdatedRegionMarkerEnabled] )
				{
					m_lastUpdatedRegion.setLeft(0);
					m_lastUpdatedRegion.setRight(0);
					m_lastUpdatedRegion.setWidth(m_width);
					m_lastUpdatedRegion.setHeight(m_height);
				}
			}

			/**
			 * @brief Reset the updated region marker.
			 * @return void
			 */
			void
			resetUpdatedRegionMarker () noexcept
			{
				m_lastUpdatedRegion.reset();
			}

			/**
			 * @brief Adds alpha channel.
			 * @todo Rework it to avoid whole copy of the pixel buffer.
			 * @param alphaValue The initial alpha value.
			 * @param modifyUpdatedRegion Set the updated region. Default true.
			 * @return bool
			 */
			bool
			addAlphaChannel (pixel_data_t alphaValue, bool modifyUpdatedRegion = true)
			{
				if ( !this->isValid() )
				{
					return false;
				}

				switch ( m_channelMode )
				{
					case ChannelMode::GrayscaleAlpha :
					case ChannelMode::RGBA :
						return true;

					case ChannelMode::Grayscale :
					{
						const auto copy = m_data;

						if ( !this->initialize(m_width, m_height, ChannelMode::GrayscaleAlpha) )
						{
							return false;
						}

						const auto limit = this->pixelCount();

						size_t dstIndex = 0;

						for ( size_t index = 0; index < limit; index++ )
						{
							m_data[dstIndex++] = copy[index];
							m_data[dstIndex++] = alphaValue;
						}
					}

					case ChannelMode::RGB :
					{
						const auto copy = m_data;

						if ( !this->initialize(m_width, m_height, ChannelMode::RGBA) )
						{
							return false;
						}

						const auto limit = this->pixelCount();

						size_t srcIndex = 0;
						size_t dstIndex = 0;

						for ( size_t index = 0; index < limit; index++ )
						{
							m_data[dstIndex++] = copy[srcIndex++];
							m_data[dstIndex++] = copy[srcIndex++];
							m_data[dstIndex++] = copy[srcIndex++];
							m_data[dstIndex++] = alphaValue;
						}
					}
				}

				if ( modifyUpdatedRegion )
				{
					this->markEverythingUpdated();
				}

				return true;
			}

			/**
			 * @brief Fills the pixmap with a single value.
			 * @note This will ignore the alpha value.
			 * @param value The value.
			 * @return bool
			 */
			bool
			fill (pixel_data_t value) noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				switch ( m_channelMode )
				{
					case ChannelMode::Grayscale :
					case ChannelMode::RGB :
						std::memset(m_data.data(), value, m_data.size() * sizeof(pixel_data_t));
						break;

					case ChannelMode::GrayscaleAlpha :
					{
						const auto limit = this->elementCount();

						for ( size_t index = 0; index < limit; index += 2 )
						{
							m_data[index] = value;
						}
					}
						break;

					case ChannelMode::RGBA :
					{
						const auto limit = this->elementCount();

						for ( size_t index = 0; index < limit; index += 4 )
						{
							m_data[index] = value;
							m_data[index+1] = value;
							m_data[index+2] = value;
						}
					}
						break;
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap with a sequence of data.
			 * @note This will ignore the alpha value.
			 * @param data A raw pointer to data.
			 * @param size The size of the data.
			 * @return bool
			 */
			bool
			fill (const pixel_data_t * data, size_t size) noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				switch ( m_channelMode )
				{
					case ChannelMode::Grayscale :
					case ChannelMode::RGB :
						if ( size >= m_data.size() )
						{
							std::memcpy(m_data.data(), data, m_data.size());
						}
						else
						{
							const auto loopLimit = std::ceil(m_data.size() / size);
							const auto remain = m_data.size() % size;

							for ( size_t loopIndex = 0; loopIndex < loopLimit; loopIndex++ )
							{
								const auto shift = loopIndex * size;

								std::memcpy(m_data.data() + shift, data + shift, loopIndex + 1 == loopLimit ? remain : size);
							}
						}
						break;

					case ChannelMode::GrayscaleAlpha :
					{
						const auto limit = this->elementCount();

						size_t offset = 0;

						for ( size_t index = 0; index < limit; index += 2 )
						{
							m_data[index] = data[offset++];

							if ( offset >= size )
							{
								offset = 0;
							}
						}
					}
						break;

					case ChannelMode::RGBA :
					{
						const auto limit = this->elementCount();

						size_t offset = 0;

						for ( size_t index = 0; index < limit; index += 4 )
						{
							m_data[index] = data[offset++];

							if ( offset >= size )
							{
								offset = 0;
							}

							m_data[index+1] = data[offset++];

							if ( offset >= size )
							{
								offset = 0;
							}

							m_data[index+2] = data[offset++];

							if ( offset >= size )
							{
								offset = 0;
							}
						}
					}
						break;
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap with a sequence of data.
			 * @note This will ignore the alpha value.
			 * @param vector A reference to a vector.
			 * @return bool
			 */
			bool
			fill (const std::vector< pixel_data_t > & vector) noexcept
			{
				return this->fill(vector.data(), vector.size());
			}

			/**
			 * @brief Fills the pixmap with a color.
			 * @warning This method is slow.
			 * @tparam color_data_t The color data type. Default float.
			 * @param color A reference to a color.
			 * @return bool
			 */
			template< typename color_data_t = float >
			bool
			fill (const Color< color_data_t > & color) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !this->isValid() )
				{
					return false;
				}

				const auto limit = this->pixelCount();

				for ( size_t index = 0; index < limit; index++ )
				{
					this->setPixel(index, color);
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap with a pattern.
			 * @note This may be a naive approach. But this library is not intended to be Photoshop.
			 * @param pattern A reference to a pixmap.
			 * @return bool
			 */
			bool
			fill (const Pixmap & pattern) noexcept
			{
				if ( !this->isValid() || !pattern.isValid() )
				{
					return false;
				}

				if ( this->colorCount() != pattern.colorCount() )
				{
					return false;
				}

				size_t patternRowIndex = 0;

				for ( size_t rowIndex = 0; rowIndex < m_height; rowIndex++ )
				{
					auto * rowData = this->rowPointer(rowIndex);
					auto * patternRowData = pattern.rowPointer(patternRowIndex);

					auto bytesLeft = this->pitch();

					while ( bytesLeft > 0 )
					{
						const auto chunkBytes = pattern.pitch() > bytesLeft ? bytesLeft : pattern.pitch();

						std::memcpy(rowData, patternRowData, chunkBytes);

						rowData += chunkBytes;
						bytesLeft -= chunkBytes;
					}

					/* NOTE: Advance in the pattern rows and reset at the bottom of the pattern image. */
					patternRowIndex++;

					if ( patternRowIndex >= pattern.height() )
					{
						patternRowIndex = 0;
					}
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap with a horizontal gradient.
			 * @tparam scale_data_t The scale data type. Default float.
			 * @tparam color_data_t The color data type. Default float.
			 * @param gradient A reference to a gradient.
			 * @return bool
			 */
			template< typename scale_data_t = float, typename color_data_t = float >
			bool
			fillHorizontal (const Gradient< scale_data_t, color_data_t > & gradient) noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				for ( dimension_t rowIndex = 0; rowIndex < m_height; ++rowIndex )
				{
					const auto position = static_cast< scale_data_t >(rowIndex) / static_cast< scale_data_t >(m_height);

					const auto color = gradient.colorAt(position);

					for ( dimension_t colIndex = 0; colIndex < m_width; ++colIndex )
					{
						this->setPixel(colIndex, rowIndex, color);
					}
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap with a vertical gradient.
			 * @tparam scale_data_t The scale data type. Default float.
			 * @tparam color_data_t The color data type. Default float.
			 * @param gradient A reference to a gradient.
			 * @return bool
			 */
			template< typename scale_data_t = float, typename color_data_t = float >
			bool
			fillVertical (const Gradient< scale_data_t, color_data_t > & gradient) noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				for ( dimension_t colIndex = 0; colIndex < m_width; ++colIndex )
				{
					const auto position = static_cast< scale_data_t >(colIndex) / static_cast< scale_data_t >(m_width);

					const auto color = gradient.colorAt(position);

					for ( dimension_t rowIndex = 0; rowIndex < m_height; ++rowIndex )
					{
						this->setPixel(colIndex, rowIndex, color);
					}
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap channel with a single value.
			 * @param channel The targeted channel of the pixmap.
			 * @param value The value.
			 * @return bool
			 */
			bool
			fillChannel (Channel channel, pixel_data_t value) noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				const auto channelIndex = static_cast< size_t >(channel);

				if ( channelIndex >= this->colorCount() )
				{
					return false;
				}

				if ( m_channelMode == ChannelMode::Grayscale )
				{
					std::memset(m_data.data(), value, m_data.size() * sizeof(pixel_data_t));

					return true;
				}

				const auto limit = this->elementCount();
				const auto stride = this->colorCount();

				for ( size_t index = 0; index < limit; index += stride )
				{
					m_data[index + channelIndex] = value;
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap channel with a sequence of data.
			 * @param channel The targeted channel of the pixmap.
			 * @param data A raw pointer to data.
			 * @param size The size of the data.
			 * @return bool
			 */
			bool
			fillChannel (Channel channel, const pixel_data_t * data, size_t size) noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				const auto channelIndex = static_cast< size_t >(channel);

				if ( channelIndex >= this->colorCount() )
				{
					return false;
				}

				if ( m_channelMode == ChannelMode::Grayscale )
				{
					return this->fill(data, size);
				}

				const auto limit = this->elementCount();
				const auto stride = this->colorCount();

				size_t offset = 0;

				for ( size_t index = 0; index < limit; index += stride )
				{
					m_data[index + channelIndex] = data[offset++];

					if ( offset >= size )
					{
						offset = 0;
					}
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap channel with a sequence of data.
			 * @param channel The targeted channel of the pixmap.
			 * @param vector A reference to a vector.
			 * @return bool
			 */
			bool
			fillChannel (Channel channel, const std::vector< pixel_data_t > & vector) noexcept
			{
				return this->fillChannel(channel, vector.data(), vector.size());
			}

			/**
			 * @brief Fills the pixmap channel with a pattern.
			 * @note The pattern pixmap will be used as grayscale.
			 * @param channel The targeted channel of the pixmap.
			 * @param pattern A reference to a pixmap.
			 * @param mode The conversion mode. Default LumaRec709.
			 * @param option Additional parameter for specific mode. Default 0.
			 * @return bool
			 */
			bool
			fillChannel (Channel channel, const Pixmap & pattern, GrayscaleConversionMode mode = GrayscaleConversionMode::LumaRec709, int option = 0) noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				const auto channelIndex = static_cast< size_t >(channel);

				if ( channelIndex >= this->colorCount() )
				{
					return false;
				}

				dimension_t patternRowIndex = 0;
				dimension_t patternColIndex = 0;

				for ( dimension_t rowIndex = 0; rowIndex < m_height; ++rowIndex )
				{
					for ( dimension_t colIndex = 0; colIndex < m_width; ++colIndex )
					{
						if constexpr ( std::is_floating_point_v< pixel_data_t > )
						{
							this->setPixelElement(
								colIndex,
								rowIndex,
								channel,
								pattern.pixel(patternColIndex, patternRowIndex).luminance(mode, option)
							);
						}
						else
						{
							this->setPixelElement(
								colIndex,
								rowIndex,
								channel,
								pattern.pixel(patternColIndex, patternRowIndex).luminanceInteger(mode, option)
							);
						}

						/* NOTE: Advance in the pattern columns and reset at the right of the pattern image. */
						++patternColIndex;

						if ( patternColIndex >= pattern.width() )
						{
							patternColIndex = 0;
						}
					}

					/* NOTE: Advance in the pattern rows and reset at the bottom of the pattern image. */
					++patternRowIndex;
					patternColIndex = 0;

					if ( patternRowIndex >= pattern.height() )
					{
						patternRowIndex = 0;
					}
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap channel with a horizontal gradient.
			 * @note The gradient will be used as grayscale.
			 * @tparam scale_data_t The scale data type. Default float.
			 * @tparam color_data_t The color data type. Default float.
			 * @param channel The targeted channel of the pixmap.
			 * @param gradient A reference to a gradient.
			 * @param mode The conversion mode. Default LumaRec709.
			 * @param option Additional parameter for specific mode. Default 0.
			 * @return bool
			 */
			template< typename scale_data_t = float, typename color_data_t = float >
			bool
			fillHorizontalChannel (Channel channel, const Gradient< color_data_t, color_data_t > & gradient, GrayscaleConversionMode mode = GrayscaleConversionMode::LumaRec709, int option = 0) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !this->isValid() )
				{
					return false;
				}

				const auto channelIndex = static_cast< size_t >(channel);

				if ( channelIndex >= this->colorCount() )
				{
					return false;
				}

				for ( dimension_t rowIndex = 0; rowIndex < m_height; ++rowIndex )
				{
					const auto position = static_cast< scale_data_t >(rowIndex) / static_cast< scale_data_t >(m_height);

					pixel_data_t value;

					if constexpr ( std::is_floating_point_v< pixel_data_t > )
					{
						value = gradient.colorAt(position).luminance(mode, option);
					}
					else
					{
						value = gradient.colorAt(position).luminanceInteger(mode, option);
					}

					for ( dimension_t colIndex = 0; colIndex < m_width; ++colIndex )
					{
						this->setPixelElement(colIndex, rowIndex, channel, value);
					}
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Fills the pixmap channel with a vertical gradient.
			 * @note The gradient will be used as grayscale.
			 * @tparam scale_data_t The scale data type. Default float.
			 * @tparam color_data_t The color data type. Default float.
			 * @param channel The targeted channel of the pixmap.
			 * @param gradient A reference to a gradient.
			 * @param mode The conversion mode. Default LumaRec709.
			 * @param option Additional parameter for specific mode. Default 0.
			 * @return bool
			 */
			template< typename scale_data_t = float, typename color_data_t = float >
			bool
			fillVerticalChannel (Channel channel, const Gradient< color_data_t, color_data_t > & gradient, GrayscaleConversionMode mode = GrayscaleConversionMode::LumaRec709, int option = 0) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				if ( !this->isValid() )
				{
					return false;
				}

				const auto channelIndex = static_cast< size_t >(channel);

				if ( channelIndex >= this->colorCount() )
				{
					return false;
				}

				for ( dimension_t colIndex = 0; colIndex < m_width; ++colIndex )
				{
					const auto position = static_cast< scale_data_t >(colIndex) / static_cast< scale_data_t >(m_width);

					pixel_data_t value;

					if constexpr ( std::is_floating_point_v< pixel_data_t > )
					{
						value = gradient.colorAt(position).luminance(mode, option);
					}
					else
					{
						value = gradient.colorAt(position).luminanceInteger(mode, option);
					}

					for ( dimension_t rowIndex = 0; rowIndex < m_height; ++rowIndex )
					{
						this->setPixelElement(colIndex, rowIndex, channel, value);
					}
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Sets data to zero.
			 * @return void
			 */
			void
			zeroFill () noexcept
			{
				if ( !this->isValid() )
				{
					return;
				}

				/* NOTE : C++ std::fill(myArray, myArray+N, 0); */
				//std::fill(m_data.begin(), m_data.end(), 0);
				std::memset(m_data.data(), 0, m_data.size() * sizeof(pixel_data_t));

				this->markEverythingUpdated();
			}

			/**
			 * @brief Fills the pixmap with a basic noise.
			 * @return bool
			 */
			bool
			noise () noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				size_t bufferSize = 0;

				switch ( m_channelMode )
				{
					case ChannelMode::Grayscale :
					case ChannelMode::GrayscaleAlpha :
						bufferSize = m_width * m_height;
						break;

					case ChannelMode::RGB :
					case ChannelMode::RGBA :
						bufferSize = m_width * m_height * 3;
						break;

					default:
						return false;
				}

				if constexpr ( std::is_floating_point_v< pixel_data_t > )
				{
					Randomizer< pixel_data_t > randomizer;

					const auto pixelBuffer = randomizer.vector(
					   bufferSize,
					   static_cast< pixel_data_t >(0),
					   static_cast< pixel_data_t >(1)
					);

					return this->fill(pixelBuffer);
				}

				if constexpr ( std::is_integral_v< pixel_data_t > )
				{
					Randomizer< pixel_data_t > randomizer;

					const auto pixelBuffer = randomizer.vector(
					   bufferSize,
					   static_cast< pixel_data_t >(0),
					   std::numeric_limits< pixel_data_t >::max()
					);

					return this->fill(pixelBuffer);
				}

				return false;
			}

			/**
			 * @brief Fills the pixmap with a perlin noise.
			 * @param scale The perlin noise scale factor.
			 * @param grayNoise Generate only gray. Default false.
			 * @return bool
			 */
			bool
			perlinNoise (float scale, bool grayNoise = false) noexcept
			{
				if ( !this->isValid() )
				{
					return false;
				}

				Randomizer< uint32_t > randomizer;

				const auto limit = this->pixelCount();
				const auto stride = this->colorCount();

				switch ( m_channelMode )
				{
					case ChannelMode::Grayscale :
					case ChannelMode::GrayscaleAlpha :
					{
						Algorithms::PerlinNoise< float > generator{randomizer.value(0U, std::numeric_limits< uint32_t >::max())};

						for ( size_t index = 0; index < limit; ++index )
						{
							const auto offset = index * stride;

							m_data[offset] = generator.generate8bits(this->u(index) * scale, this->v(index) * scale);
						}
					}
						break;

					case ChannelMode::RGB :
					case ChannelMode::RGBA :
						if ( grayNoise )
						{
							Algorithms::PerlinNoise< float > generator{randomizer.value(0U, std::numeric_limits< uint32_t >::max())};

							for ( size_t index = 0; index < limit; ++index )
							{
								const auto offset = index * stride;
								const auto value = generator.generate8bits(this->u(index) * scale, this->v(index) * scale);

								m_data[offset] = value;
								m_data[offset+1] = value;
								m_data[offset+2] = value;
							}
						}
						else
						{
							Algorithms::PerlinNoise< float > redGenerator{randomizer.value(0U, std::numeric_limits< uint32_t >::max())};
							Algorithms::PerlinNoise< float > greenGenerator{randomizer.value(0U, std::numeric_limits< uint32_t >::max())};
							Algorithms::PerlinNoise< float > blueGenerator{randomizer.value(0U, std::numeric_limits< uint32_t >::max())};

							for ( size_t index = 0; index < limit; ++index )
							{
								const auto offset = index * stride;
								const auto uCoord = this->u(index) * scale;
								const auto vCoord = this->v(index) * scale;

								m_data[offset] = redGenerator.generate8bits(uCoord, vCoord);
								m_data[offset+1] = greenGenerator.generate8bits(uCoord, vCoord);
								m_data[offset+2] = blueGenerator.generate8bits(uCoord, vCoord);
							}
						}
						break;
				}

				this->markEverythingUpdated();

				return true;
			}

			/**
			 * @brief Applies a function on every pixel in rows order (Single loop).
			 * @note Returning false will skip the pixel.
			 * @warning This method is slow.
			 * @param updatePixel A reference to a function to modify the pixel. Signature: bool (Color & pixel)
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			forEachPixel (const std::function< bool (Color< color_data_t > &) > & updatePixel) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				const auto pixelCount = static_cast< size_t >(this->pixelCount());

				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					auto pixelColor = this->pixel(pixelIndex);

					if ( !updatePixel(pixelColor) )
					{
						continue;
					}

					this->setPixel(pixelIndex, pixelColor);
				}
			}

			/**
			 * @brief Applies a function on every pixel in rows order (Single loop).
			 * @note Returning false will skip the pixel.
			 * @warning This method is slow.
			 * @tparam color_data_t The color data type. Default float.
			 * @param updatePixel A reference to a function to modify the pixel. Signature: bool (Color & pixel, dimension_t coordX, dimension_t coordY)
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			forEachPixelRowMajor (const std::function< bool (Color< color_data_t > &, dimension_t, dimension_t) > & updatePixel) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				const auto pixelCount = static_cast< size_t >(this->pixelCount());

				for ( size_t pixelIndex = 0; pixelIndex < pixelCount; pixelIndex++ )
				{
					auto pixelColor = this->pixel(pixelIndex);

					if ( !updatePixel(pixelColor, this->x(pixelIndex), this->y(pixelIndex)) )
					{
						continue;
					}

					this->setPixel(pixelIndex, pixelColor);
				}
			}

			/**
			 * @brief Applies a function on every pixel in column order (X^Y loops).
			 * @note Returning false will skip the pixel.
			 * @warning This method is slow.
			 * @tparam color_data_t The color data type. Default float.
			 * @param updatePixel A reference to a function to modify the pixel. Signature: bool (Color & pixel, dimension_t coordX, dimension_t coordY)
			 * @return void
			 */
			template< typename color_data_t = float >
			void
			forEachPixelColMajor (const std::function< bool (Color< color_data_t > &, dimension_t, dimension_t) > & updatePixel) noexcept requires (std::is_floating_point_v< color_data_t >)
			{
				for ( dimension_t coordX = 0; coordX < m_width; ++coordX )
				{
					for ( dimension_t coordY = 0; coordY < m_height; ++coordY )
					{
						const auto pixelIndex = this->pixelIndex(coordX, coordY);

						auto pixelColor = this->pixel(pixelIndex);

						if ( !updatePixel(pixelColor, coordX, coordY) )
						{
							continue;
						}

						this->setPixel(pixelIndex, pixelColor);
					}
				}
			}

			/**
			 * @brief Returns the zero value for a pixmap component.
			 * @return pixel_data_t
			 */
			[[nodiscard]]
			static
			constexpr
			pixel_data_t
			zero () noexcept
			{
				return static_cast< pixel_data_t >(0);
			}

			/**
			 * @brief Returns the one value for a pixmap component.
			 * @return pixel_data_t
			 */
			[[nodiscard]]
			static
			constexpr
			pixel_data_t
			one () noexcept
			{
				if constexpr ( std::is_floating_point_v< pixel_data_t > )
				{
					return static_cast< pixel_data_t >(1);
				}
				else
				{
					return std::numeric_limits< pixel_data_t >::max();
				}
			}

			/**
			 * @brief STL streams printable object.
			 * @param out A reference to the stream output.
			 * @param obj A reference to the object to print.
			 * @return std::ostream &
			 */
			friend
			std::ostream &
			operator<< (std::ostream & out, const Pixmap & obj)
			{
				out <<
					"Pixmap data :\n"
					"Width : " << obj.width() << "\n"
					"Height : " << obj.height() << '\n';

				switch ( obj.colorCount() )
				{
					case 1 :
						out << "Channels count : 1 (Grayscale)" "\n";
						break;

					case 2 :
						out << "Channels count : 2 (GrayscaleAlpha)" "\n";
						break;

					case 3 :
						out << "Channels count : 3 (RGB)" "\n";
						break;

					case 4 :
						out << "Channels count : 4 (RGBA)" "\n";
						break;

					default:
						out << "Channels count : ERROR !" "\n";
						break;
				}

				out <<
					"Pixel count : " << obj.pixelCount() << "\n"
					"Pixel size : " << obj.bytePerPixel() << " bytes (" << obj.bitPerPixel() << " bit)\n"
					"Element count : " << obj.elementCount() << "\n"
					"Data size : " << obj.bytes() << " bytes" "\n";

				return out;
			}

			/**
			 * @brief Stringifies the object.
			 * @param obj A reference to the object to print.
			 * @return std::string
			 */
			friend
			std::string
			to_string (const Pixmap & obj)
			{
				std::stringstream output;

				output << obj;

				return output.str();
			}

		private:

			/**
			 * @brief Clamps a pixel index to a valid position in the pixmap.
			 * @param pixelIndex A pixel index.
			 * @return void
			 */
			void
			clampPixelIndex (size_t & pixelIndex) const noexcept
			{
				const auto pixelCount = this->pixelCount();

				static_assert(pixelIndex >= pixelCount, "Pixel index overflow !");

				if ( pixelIndex >= pixelCount )
				{
					pixelIndex = pixelCount - 1;
				}
			}

			/**
			 * @brief Clamps a pixel index to a valid position in the pixmap.
			 * @param coordX The pixel position in X.
			 * @param coordY The pixel position in Y.
			 * @return void
			 */
			void
			clampPixelCoords (dimension_t & coordX, dimension_t & coordY) const noexcept
			{
				if ( coordX >= m_width )
				{
					coordX = m_width - 1;
				}

				if ( coordY >= m_height )
				{
					coordY = m_height - 1;
				}
			}

			/**
			 * @brief Takes care of initialize the alpha channel to one (opaque).
			 * @return bool
			 */
			bool
			initAlphaChannel () noexcept
			{
				switch ( m_channelMode )
				{
					case ChannelMode::RGBA :
						return this->fillChannel(Channel::Alpha, Pixmap::one());

					case ChannelMode::GrayscaleAlpha :
						return this->fillChannel(Channel::Green, Pixmap::one());

					default:
						return true;
				}
			}

			/* Flag names. */
			static constexpr auto UVWrappingEnabled{0UL};
			static constexpr auto UpdatedRegionMarkerEnabled{1UL};

			dimension_t m_width{0};
			dimension_t m_height{0};
			ChannelMode m_channelMode{ChannelMode::RGB};
			std::vector< pixel_data_t > m_data{};
			Math::Space2D::AARectangle< dimension_t > m_lastUpdatedRegion;
			std::array< bool, 8 > m_flags{
				true/*UVWrappingEnabled*/,
				true/*UpdatedRegionMarkerEnabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};

	/**
	 * @brief Converts a pixmap from data to another.
	 * @tparam input_pixel_data_t The pixel data type of the source pixmap.
	 * @tparam output_pixel_data_t The pixel data type of the target pixmap.
	 * @param input A reference to input pixmap.
	 * @return Pixmap< output_pixel_data_t >
	 */
	template< typename input_pixel_data_t, typename output_pixel_data_t >
	[[nodiscard]]
	Pixmap< output_pixel_data_t >
	dataConversion (const Pixmap< input_pixel_data_t > & input) noexcept requires (std::is_arithmetic_v< input_pixel_data_t >, std::is_arithmetic_v< output_pixel_data_t >)
	{
		Pixmap< output_pixel_data_t > output{input.width(), input.height(), input.channelMode()};

		const auto & inputData = input.data();
		auto & outputData = output.data();

		for ( size_t index = 0; index < inputData.size(); index++ )
		{
			if constexpr ( std::is_floating_point_v< input_pixel_data_t > )
			{
				/* float -> float */
				if constexpr ( std::is_floating_point_v< output_pixel_data_t > )
				{
					outputData[index] = static_cast< output_pixel_data_t >(inputData[index]);
				}
				/* float -> integer */
				else
				{
					outputData[index] = static_cast< output_pixel_data_t >(std::round(inputData[index] * std::numeric_limits< output_pixel_data_t >::max()));
				}
			}
			else
			{
				/* integer -> float */
				if constexpr ( std::is_floating_point_v< output_pixel_data_t > )
				{
					outputData[index] = static_cast< output_pixel_data_t >(inputData[index]) / static_cast< output_pixel_data_t >(std::numeric_limits< input_pixel_data_t >::max());
				}
				/* integer -> integer */
				else
				{
					const auto ratio = static_cast< float >(std::numeric_limits< output_pixel_data_t >::max()) / static_cast< float >(std::numeric_limits< input_pixel_data_t >::max());

					outputData[index] = static_cast< output_pixel_data_t >(std::round(ratio * inputData[index]));
				}
			}
		}

		return output;
	}
}
