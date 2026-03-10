/*
 * src/Libs/PixelFactory/IOCommon.hpp
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
#include <cstdint>
#include <type_traits>

/* Local inclusions for usages. */
#include "Pixmap.hpp"
#include "Processor.hpp"
#include "Types.hpp"

namespace EmEn::Libs::PixelFactory
{
	/**
	 * @brief Applies read post-processing options to a decoded pixmap.
	 * @note This handles Y-axis inversion, channel conversion, and alpha premultiplication.
	 * Called by FileIO and StreamIO after format-specific decoding.
	 * @tparam pixel_data_t The pixel component type. Default uint8_t.
	 * @tparam dimension_t The dimension type. Default uint32_t.
	 * @param pixmap A reference to the pixmap to post-process.
	 * @param options The read options specifying which transformations to apply.
	 * @return bool True if post-processing succeeded.
	 */
	template< typename pixel_data_t = uint8_t, typename dimension_t = uint32_t >
	requires (std::is_arithmetic_v< pixel_data_t > && std::is_unsigned_v< dimension_t >)
	[[nodiscard]]
	bool
	applyReadOptions (Pixmap< pixel_data_t, dimension_t > & pixmap, const ReadOptions & options) noexcept
	{
		if ( !pixmap.isValid() )
		{
			return false;
		}

		/* Y-axis inversion (flip vertically for OpenGL convention). */
		if ( options.invertYAxis )
		{
			pixmap = Processor< pixel_data_t, dimension_t >::mirror(pixmap, MirrorMode::X);
		}

		/* Channel conversion. */
		if ( options.targetChannelMode != TargetChannelMode::KeepOriginal )
		{
			switch ( options.targetChannelMode )
			{
				case TargetChannelMode::Grayscale :
					if ( pixmap.channelMode() != ChannelMode::Grayscale )
					{
						pixmap = Processor< pixel_data_t, dimension_t >::toGrayscale(pixmap);
					}
					break;

				case TargetChannelMode::GrayscaleAlpha :
					if ( pixmap.channelMode() == ChannelMode::RGB || pixmap.channelMode() == ChannelMode::RGBA )
					{
						pixmap = Processor< pixel_data_t, dimension_t >::toGrayscale(pixmap);
					}
					if ( pixmap.channelMode() == ChannelMode::Grayscale )
					{
						pixmap = Processor< pixel_data_t, dimension_t >::addAlphaChannel(pixmap);
					}
					break;

				case TargetChannelMode::RGB :
					if ( pixmap.channelMode() != ChannelMode::RGB )
					{
						pixmap = Processor< pixel_data_t, dimension_t >::toRGB(pixmap);
					}
					break;

				case TargetChannelMode::RGBA :
					if ( pixmap.channelMode() != ChannelMode::RGBA )
					{
						pixmap = Processor< pixel_data_t, dimension_t >::toRGBA(pixmap);
					}
					break;

				default:
					break;
			}
		}

		/* Premultiply alpha. */
		if ( options.premultiplyAlpha &&
			(pixmap.channelMode() == ChannelMode::RGBA || pixmap.channelMode() == ChannelMode::GrayscaleAlpha) )
		{
			auto & data = pixmap.data();
			const auto colorCount = pixmap.template colorCount< size_t >();
			const auto alphaIndex = colorCount - 1;
			const auto totalPixels = static_cast< size_t >(pixmap.width()) * static_cast< size_t >(pixmap.height());

			for ( size_t pixelIndex = 0; pixelIndex < totalPixels; ++pixelIndex )
			{
				const auto offset = pixelIndex * colorCount;
				const auto alpha = data[offset + alphaIndex];

				for ( size_t channel = 0; channel < alphaIndex; ++channel )
				{
					if constexpr ( std::is_floating_point_v< pixel_data_t > )
					{
						data[offset + channel] = static_cast< pixel_data_t >(data[offset + channel] * alpha);
					}
					else
					{
						data[offset + channel] = static_cast< pixel_data_t >(
							static_cast< uint32_t >(data[offset + channel]) * static_cast< uint32_t >(alpha) / 255U
						);
					}
				}
			}
		}

		return true;
	}
}
