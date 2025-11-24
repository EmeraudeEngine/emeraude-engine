/*
 * src/Graphics/ImageResource.cpp
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

#include "ImageResource.hpp"

/* Local inclusions. */
#include "Libs/PixelFactory/Color.hpp"
#include "Libs/PixelFactory/FileIO.hpp"
#include "Libs/PixelFactory/Processor.hpp"
#include "TextureResource/Abstract.hpp"
#include "Resources/Manager.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Libs;

	bool
	ImageResource::load (Resources::ServiceProvider & /*serviceProvider*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		constexpr size_t DefaultSize{64};

		if ( !m_pixmap.initialize(DefaultSize, DefaultSize, PixelFactory::ChannelMode::RGBA) )
		{
			Tracer::error(ClassId, "Unable to load the default pixmap !");

			return this->setLoadSuccess(false);
		}

		if constexpr ( IsDebug )
		{
			if ( !m_pixmap.fill(PixelFactory::Magenta) )
			{
				Tracer::error(ClassId, "Unable to fill the default pixmap !");

				return this->setLoadSuccess(false);
			}

			PixelFactory::Processor processor{m_pixmap};

			processor.drawSegment(
				Math::Vector< 2, int32_t >{0, 0},
				Math::Vector< 2, int32_t >{DefaultSize - 1, DefaultSize - 1},
				PixelFactory::Black
			);

			processor.drawSegment(
				Math::Vector< 2, int32_t >{DefaultSize - 1, 0},
				Math::Vector< 2, int32_t >{0, DefaultSize - 1},
				PixelFactory::Black
			);
		}
		else
		{
			if ( !m_pixmap.perlinNoise(2.0F) )
			{
				Tracer::error(ClassId, "Unable to fill the default pixmap !");

				return this->setLoadSuccess(false);
			}
		}

		return this->setLoadSuccess(true);
	}

	bool
	ImageResource::load (Resources::ServiceProvider & /*serviceProvider*/, const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !PixelFactory::FileIO::read(filepath, m_pixmap) )
		{
			TraceError{ClassId} << "Unable to load the image file '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		if ( !TextureResource::Abstract::validatePixmap(ClassId, this->name(), m_pixmap) )
		{
			TraceError{ClassId} << "Unable to use the pixmap from file '" << filepath << "' to create an image !";

			m_pixmap.clear();

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	ImageResource::load (Resources::ServiceProvider & /*serviceProvider*/, const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::error(ClassId, "This method can't be used !");

		return this->setLoadSuccess(false);
	}
}
