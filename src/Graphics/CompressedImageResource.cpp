/*
 * src/Graphics/CompressedImageResource.cpp
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
 * https://github.com/EmeraudeEngine/emeraude-engine
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

#include "CompressedImageResource.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* Local inclusions. */
#include "IO/IO.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/Processor.hpp"
#include "PrimaryServices.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "TextureCompressor.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	uint32_t
	CompressedImageResource::maxDimension (Settings & settings) noexcept
	{
		return settings.getOrSetDefault< uint32_t >(GraphicsTextureMaxDimensionKey, DefaultGraphicsTextureMaxDimension);
	}

	bool
	CompressedImageResource::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		constexpr size_t DefaultSize{64};

		/* NOTE: The default payload is built the same way ImageResource builds its own, then
		 * compressed on the spot. It has to go through the encoder because there is no such
		 * thing as a procedural block-compressed pattern. 64x64 keeps that negligible. */
		PixelFactory::Pixmap< uint8_t > pixmap;

		if ( !pixmap.initialize(DefaultSize, DefaultSize, PixelFactory::ChannelMode::RGBA) )
		{
			Tracer::error(ClassId, "Unable to create the default pixmap !");

			return this->setLoadSuccess(false);
		}

		if constexpr ( IsDebug )
		{
			if ( !pixmap.fill(PixelFactory::Magenta) )
			{
				Tracer::error(ClassId, "Unable to fill the default pixmap !");

				return this->setLoadSuccess(false);
			}

			PixelFactory::Processor processor{pixmap};

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
			if ( !pixmap.perlinNoise(2.0F) )
			{
				Tracer::error(ClassId, "Unable to fill the default pixmap !");

				return this->setLoadSuccess(false);
			}
		}

		TextureCompressor::initialize();

		auto mip = TextureCompressor::compressSingle(pixmap, *this->serviceProvider().primaryServices().threadPool());

		if ( mip.data.empty() )
		{
			Tracer::error(ClassId, "Unable to compress the default pixmap !");

			return this->setLoadSuccess(false);
		}

		m_payload.mips.emplace_back(std::move(mip));
		m_payload.format = VK_FORMAT_BC7_UNORM_BLOCK;

		return this->setLoadSuccess(true);
	}

	bool
	CompressedImageResource::load (const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		std::vector< uint8_t > content;

		if ( !IO::fileGetContents(filepath, content) )
		{
			TraceError{ClassId} << "Unable to read the compressed image file '" << filepath << "' !";

			return this->setLoadSuccess(false);
		}

		const auto bytes = std::as_bytes(std::span{content});

		if ( !KTX2Decoder::isKTX2(bytes) )
		{
			TraceError{ClassId} << "The file '" << filepath << "' is not a KTX2 container !";

			return this->setLoadSuccess(false);
		}

		const KTX2Decoder::Options options{
			.maxDimension = maxDimension(this->serviceProvider().primaryServices().settings())
		};

		m_payload = KTX2Decoder::decodeCompressed(bytes, options, this->name());

		return this->setLoadSuccess(m_payload.isValid());
	}

	bool
	CompressedImageResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::error(ClassId, "This method can't be used !");

		return this->setLoadSuccess(false);
	}

	bool
	CompressedImageResource::load (std::span< const std::byte > bytes) noexcept
	{
		/* NOTE: Manual loading pair, not beginLoading()/setLoadSuccess() : this overload is meant to
		 * be called from a resource container creation function, which already owns the loading
		 * state machine. Same contract as ImageResource::load(Pixmap &&). */
		if ( !this->enableManualLoading() )
		{
			return false;
		}

		if ( !KTX2Decoder::isKTX2(bytes) )
		{
			TraceError{ClassId} << "The blob given for '" << this->name() << "' is not a KTX2 container !";

			return this->setManualLoadSuccess(false);
		}

		const KTX2Decoder::Options options{
			.maxDimension = maxDimension(this->serviceProvider().primaryServices().settings())
		};

		m_payload = KTX2Decoder::decodeCompressed(bytes, options, this->name());

		return this->setManualLoadSuccess(m_payload.isValid());
	}
}
