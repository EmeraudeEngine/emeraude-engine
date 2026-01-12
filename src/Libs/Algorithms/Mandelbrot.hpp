/*
 * src/Libs/Algorithms/Mandelbrot.hpp
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
#include <functional>
#include <type_traits>

/* Local inclusions. */
#include "Libs/PixelFactory/Pixmap.hpp"

namespace EmEn::Libs::Algorithms
{
	/**
	 * @brief The Mandelbrot algorithm class.
	 * @tparam number_t The type of number. Default float.
	 */
	template< typename number_t = float, typename precision_t = uint8_t >
	requires (std::is_floating_point_v< number_t > && std::is_arithmetic_v< precision_t >)
	class Mandelbrot final
	{
		public:

			using colorize_fn = std::function< void (PixelFactory::Pixmap< precision_t > &, uint32_t, uint32_t, int32_t, int32_t) >;

			/**
			 * @brief Constructs the mandelbrot algorithm.
			 * @param width The width of the pixmap.
			 * @param height The height of the pixmap.
			 * @param channels The number of color. Default RBG.
			 */
			Mandelbrot (size_t width, size_t height, PixelFactory::ChannelMode channels = PixelFactory::ChannelMode::RGB) noexcept
				: m_pixmap(width, height, channels)
			{

			}

			/**
			 * @brief Returns the generated pixmap.
			 * @return const PixelFactory::Pixmap< precision_t > &
			 */
			[[nodiscard]]
			const PixelFactory::Pixmap< precision_t > &
			pixmap () const noexcept
			{
				return m_pixmap;
			}

			/**
			 * @brief
			 * @param function
			 * @return void
			 */
			void
			setColorizationMethod (const colorize_fn & function)
			{
				m_colorizationFunction = function;
			}

			/**
			 * @brief Executes the generation.
			 * @param minReal
			 * @param maxReal
			 * @param minImaginary
			 * @param maxImaginary
			 * @param maxIterations
			 * @return void
			 */
			void
			execute (number_t minReal = -1.5, number_t maxReal = 0.7, number_t minImaginary = -1.0, number_t maxImaginary = 1.0, uint32_t maxIterations = 1000)
			{
				for ( size_t y = 0; y < m_pixmap.height(); y++ )
				{
					const auto ci = Mandelbrot::map(y, m_pixmap.height(), minImaginary, maxImaginary);

					for ( size_t x = 0; x < m_pixmap.width(); x++ )
					{
						const auto cr = Mandelbrot::map(x, m_pixmap.width(), minReal, maxReal);

						m_colorizationFunction(m_pixmap, x, y, Mandelbrot::findMandelbrot(cr, ci, maxIterations), maxIterations);
					}
				}
			}

		private:

			/**
			 * @brief
			 * @param cr
			 * @param ci
			 * @param maxIterations
			 * @return int32_t
			 */
			static
			int32_t
			findMandelbrot (number_t cr, number_t ci, int32_t maxIterations) noexcept
			{
				int32_t index = 0;

				number_t zr = 0;
				number_t zi = 0;

				while ( index < maxIterations && zr * zr + zi * zi < static_cast< number_t >(4.0) )
				{
					const auto tmp = zr * zr - zi * zi + cr;

					zi = static_cast< number_t >(2.0) * zr * zi + ci;

					zr = tmp;

					index++;
				}

				return index;
			}

			/**
			 * @brief
			 * @param offset
			 * @param dimension
			 * @param min
			 * @param max
			 * @return number_t
			 */
			static
			number_t
			map (uint32_t offset, uint32_t dimension, number_t min, number_t max) noexcept
			{
				const auto range = max - min;

				return static_cast< number_t >(offset) * (range / static_cast< number_t >(dimension)) + min;
			}

			PixelFactory::Pixmap< precision_t > m_pixmap;
			colorize_fn m_colorizationFunction{[] (PixelFactory::Pixmap< precision_t > & pixmap, uint32_t x, uint32_t y, int32_t value, int32_t /*iteration*/) {
				const auto cmp = static_cast< uint8_t >(value % 256);

				pixmap.setPixel(x, y, PixelFactory::ColorFromInteger(cmp, cmp, cmp));
			}};
	};
}
