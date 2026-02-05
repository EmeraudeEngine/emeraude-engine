/*
 * src/PlatformSpecific/VideoCaptureDevice.cpp
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

/* Local inclusions. */
#include "VideoCaptureDevice.hpp"

/* STL inclusions. */
#include <algorithm>

namespace EmEn::PlatformSpecific
{
	uint32_t
	VideoCaptureDevice::width () const noexcept
	{
		return m_width;
	}

	uint32_t
	VideoCaptureDevice::height () const noexcept
	{
		return m_height;
	}

	void
	VideoCaptureDevice::convertYUYVtoRGBA (const uint8_t * yuyvData, size_t yuyvSize, std::vector< uint8_t > & rgbaOutput, uint32_t width, uint32_t height) noexcept
	{
		const size_t pixelCount = static_cast< size_t >(width) * height;

		rgbaOutput.resize(pixelCount * 4);

		/* YUYV packs 2 pixels in 4 bytes: Y0 U Y1 V */
		const size_t expectedYUYVSize = pixelCount * 2;

		if ( yuyvSize < expectedYUYVSize )
		{
			return;
		}

		for ( size_t i = 0; i < pixelCount; i += 2 )
		{
			const size_t yuyvIndex = i * 2;

			const auto y0 = static_cast< int >(yuyvData[yuyvIndex + 0]);
			const auto u  = static_cast< int >(yuyvData[yuyvIndex + 1]);
			const auto y1 = static_cast< int >(yuyvData[yuyvIndex + 2]);
			const auto v  = static_cast< int >(yuyvData[yuyvIndex + 3]);

			/* BT.601 conversion */
			const int c0 = y0 - 16;
			const int c1 = y1 - 16;
			const int d = u - 128;
			const int e = v - 128;

			/* Pixel 0 */
			const size_t rgbaIndex0 = i * 4;
			rgbaOutput[rgbaIndex0 + 0] = static_cast< uint8_t >(std::clamp((298 * c0 + 409 * e + 128) >> 8, 0, 255));
			rgbaOutput[rgbaIndex0 + 1] = static_cast< uint8_t >(std::clamp((298 * c0 - 100 * d - 208 * e + 128) >> 8, 0, 255));
			rgbaOutput[rgbaIndex0 + 2] = static_cast< uint8_t >(std::clamp((298 * c0 + 516 * d + 128) >> 8, 0, 255));
			rgbaOutput[rgbaIndex0 + 3] = 255;

			/* Pixel 1 */
			const size_t rgbaIndex1 = (i + 1) * 4;
			rgbaOutput[rgbaIndex1 + 0] = static_cast< uint8_t >(std::clamp((298 * c1 + 409 * e + 128) >> 8, 0, 255));
			rgbaOutput[rgbaIndex1 + 1] = static_cast< uint8_t >(std::clamp((298 * c1 - 100 * d - 208 * e + 128) >> 8, 0, 255));
			rgbaOutput[rgbaIndex1 + 2] = static_cast< uint8_t >(std::clamp((298 * c1 + 516 * d + 128) >> 8, 0, 255));
			rgbaOutput[rgbaIndex1 + 3] = 255;
		}
	}
}
