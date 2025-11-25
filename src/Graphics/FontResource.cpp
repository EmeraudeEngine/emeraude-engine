/*
 * src/Graphics/FontResource.cpp
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

#include "FontResource.hpp"

/* STL inclusions. */
#include <bitset>

/* Local inclusions. */
#include "Libs/PixelFactory/DefaultFont.hpp"
#include "Resources/Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;
	using namespace Libs::PixelFactory;

	bool
	FontResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		constexpr auto BitmapSize{256};

		const std::bitset< BitmapSize * BitmapSize > bitmap{DefaultFont};

		Pixmap< uint8_t > charsMap{BitmapSize, BitmapSize, ChannelMode::Grayscale};

		for ( size_t index = 0; index < bitmap.size(); index++ )
		{
			*charsMap.pixelPointer(index) = bitmap[index] ? 255 : 0;
		}

		if ( !m_font.parsePixmap(charsMap, 16U, false) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !m_font.parsePixmap(charsMap, 24U, false) )
		{
			return this->setLoadSuccess(false);
		}

		if ( !m_font.parsePixmap(charsMap, 32U, false) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	FontResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		return this->setLoadSuccess(m_font.readFile(filepath, 16U, true));
	}

	bool
	FontResource::load (Resources::AbstractServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::warning(ClassId, "FIXME: This function is not available yet !");

		return this->setLoadSuccess(false);
	}
}
