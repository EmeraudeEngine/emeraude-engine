/*
 * src/Graphics/ImageResource.cpp
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

#include "ImageResource.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <ranges>

/* Local inclusions. */
#include "AlphaCoverage.hpp"
#include "BlobTrait.hpp"
#include "IO/IO.hpp"
#include "Math/Vector.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/FileIO.hpp"
#include "PixelFactory/Processor.hpp"
#include "PixelFactory/StreamIO.hpp"
#include "Resources/BaseInformation.hpp"
#include "StaticVector.hpp"
#include "TextureResource/Abstract.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Base;

	namespace
	{
		/**
		 * @brief Returns whether a file is a GPU texture container (KTX2, Basis Universal, DDS), by its extension.
		 * @param filepath The file.
		 * @return bool
		 */
		[[nodiscard]]
		bool
		isGPUContainer (const std::filesystem::path & filepath) noexcept
		{
			constexpr std::array< std::string_view, 3 > Extensions{".ktx2", ".basis", ".dds"};

			auto extension = IO::toU8String(filepath.extension());

			std::ranges::transform(extension, extension.begin(), [] (char character) {
				return character >= 'A' && character <= 'Z' ? static_cast< char >(character - 'A' + 'a') : character;
			});

			return std::ranges::find(Extensions, extension) != Extensions.end();
		}
	}

	bool
	ImageResource::load () noexcept
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
	ImageResource::load (const std::filesystem::path & filepath) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		/* NOTE: Images holds classic images only; a GPU container belongs to the GPUTextures store (graphics § 40, T8). */
		if ( isGPUContainer(filepath) )
		{
			TraceError{ClassId} << "The image '" << this->name() << "' is a GPU container (" << filepath.extension() << "): it belongs to the GPUTextures store, not to Images (graphics § 40).";

			return this->setLoadSuccess(false);
		}

		if ( !this->readLocalData(filepath) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	ImageResource::load (const Json::Value & /*data*/) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		Tracer::error(ClassId, "This method can't be used !");

		return this->setLoadSuccess(false);
	}

	bool
	ImageResource::isBinaryAlphaMask () const noexcept
	{
		if ( m_metadataExtracted )
		{
			return m_binaryAlphaMask;
		}

		return AlphaCoverage::isBinaryMask(m_pixmap);
	}

	void
	ImageResource::extractMetadata () noexcept
	{
		if ( m_metadataExtracted )
		{
			return;
		}

		m_width = m_pixmap.width();
		m_height = m_pixmap.height();
		m_grayScale = m_pixmap.isGrayScale();
		m_averageColor = m_pixmap.averageColor();
		m_binaryAlphaMask = AlphaCoverage::isBinaryMask(m_pixmap);
		m_metadataExtracted = true;
	}

	bool
	ImageResource::readLocalData (const std::filesystem::path & filepath) noexcept
	{
		PixelFactory::Pixmap< uint8_t > pixmap;

		if ( !PixelFactory::FileIO::read(filepath, pixmap) )
		{
			TraceError{ClassId} << "Unable to load the image file '" << filepath << "' !";

			return false;
		}

		/* NOTE: A grey-scale FILE (one channel, no alpha) keeps its one channel (graphics § 39, D8 step 2 — owner,
		 * 2026-10-21): its CPU copy is a quarter of the RGBA one, and every consumer reads it as the RGBA expansion did
		 * (R = G = B, A = 1: PackRecipe::fillChannel(), the block encoders, the R8 view's broadcast swizzle). Any other
		 * mode — grey + alpha included — is expanded to RGBA as before. Only this path: the scene loaders decode RGBA
		 * themselves, and validatePixmap() also serves the cubemap faces. */
		if ( pixmap.channelMode() == PixelFactory::ChannelMode::Grayscale )
		{
			if ( !pixmap.isValid() )
			{
				TraceError{ClassId} << "The grey-scale image from file '" << filepath << "' is invalid !";

				return false;
			}
		}
		else if ( !TextureResource::Abstract::validatePixmap(ClassId, this->name(), pixmap) )
		{
			TraceError{ClassId} << "Unable to use the pixmap from file '" << filepath << "' to create an image !";

			return false;
		}

		m_pixmap = std::move(pixmap);

		return true;
	}

	bool
	ImageResource::reloadLocalDataFromSource (const Resources::BaseInformation & source) noexcept
	{
		/* NOTE: Only a file source: this type has no JSON definition. */
		if ( source.sourceType() != Resources::SourceType::LocalData )
		{
			return false;
		}

		const auto filepath = source.dataString();

		return filepath.has_value() && this->readLocalData(IO::u8path(*filepath));
	}

	bool
	EncodedSource::decode (PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept
	{
		/* NOTE: The scene loaders' own decoding: forced to 4 channels, no flip, no premultiplication. */
		constexpr PixelFactory::ReadOptions options{
			.targetChannelMode = PixelFactory::TargetChannelMode::RGBA
		};

		switch ( m_kind )
		{
			case Kind::File :
				return PixelFactory::FileIO::read(m_filepath, pixmap, options);

			case Kind::FileRange :
			{
				std::vector< std::byte > bytes;

				return IO::fileGetRange(m_filepath, m_offset, m_length, bytes) && PixelFactory::StreamIO::read(bytes, m_format, pixmap, options);
			}

			case Kind::Embedded :
			{
				std::vector< std::byte > bytes;

				if ( m_reader == nullptr || !m_reader(m_filepath, static_cast< uint32_t >(m_offset), bytes) )
				{
					return false;
				}

				/* NOTE: The model file changed since the load: refuse rather than decode another image. */
				if ( bytes.size() != m_length )
				{
					return false;
				}

				return PixelFactory::StreamIO::read(bytes, m_format, pixmap, options);
			}
		}

		return false;
	}

	std::string
	EncodedSource::describe () const noexcept
	{
		switch ( m_kind )
		{
			case Kind::File :
				return "file " + Base::IO::toU8String(m_filepath);

			case Kind::FileRange :
				return "bytes [" + std::to_string(m_offset) + ", +" + std::to_string(m_length) + ") of " + Base::IO::toU8String(m_filepath);

			case Kind::Embedded :
				return "embedded image #" + std::to_string(m_offset) + " of " + Base::IO::toU8String(m_filepath);
		}

		return {};
	}

	namespace
	{
		/**
		 * @brief Returns the swizzle letter of a channel, for a packed image's name.
		 * @param channel The channel.
		 * @return char
		 */
		[[nodiscard]]
		char
		swizzle (PixelFactory::Channel channel) noexcept
		{
			switch ( channel )
			{
				case PixelFactory::Channel::Red : return 'r';
				case PixelFactory::Channel::Green : return 'g';
				case PixelFactory::Channel::Blue : return 'b';
				case PixelFactory::Channel::Alpha : return 'a';
			}

			return '?';
		}

		/**
		 * @brief Fills one output channel of a packed image from its source: a constant, a copy, or an upscaled copy.
		 * @param pixmap The packed RGBA pixmap, at its final size.
		 * @param output The output channel.
		 * @param source What it reads (its image leased by the caller).
		 * @return bool
		 */
		[[nodiscard]]
		bool
		fillPackedChannel (PixelFactory::Pixmap< uint8_t > & pixmap, PixelFactory::Channel output, const PackRecipe::ChannelSource & source) noexcept
		{
			return PackRecipe::fillChannel(pixmap, output, source.image != nullptr ? &source.image->data() : nullptr, source.channel, source.constant);
		}
	}

	bool
	PackRecipe::fillChannel (PixelFactory::Pixmap< uint8_t > & pixmap, PixelFactory::Channel output, const PixelFactory::Pixmap< uint8_t > * source, PixelFactory::Channel sourceChannel, uint8_t constant) noexcept
	{
		if ( source == nullptr )
		{
			return pixmap.fillChannel(output, constant);
		}

		/* NOTE: A one-channel (grey-scale) source reads as its RGBA expansion did: G and B are its grey, A is opaque. */
		if ( source->colorCount() == 1 && sourceChannel != PixelFactory::Channel::Red )
		{
			if ( sourceChannel == PixelFactory::Channel::Alpha )
			{
				return pixmap.fillChannel(output, uint8_t{255});
			}

			sourceChannel = PixelFactory::Channel::Red;
		}

		if ( source->width() == pixmap.width() && source->height() == pixmap.height() )
		{
			return pixmap.copyChannel(output, *source, sourceChannel);
		}

		/* NOTE: A smaller source is upscaled to the packed size: a TEXTURE resample (texel centres aligned, the
		 * edges wrapping like a repeating texture, Catmull-Rom), never resize(), which maps the corners and reads
		 * Black past the border — it shifted a 512² mask by half a texel and darkened every tile edge. */
		PixelFactory::Pixmap< uint8_t > upscaled;

		return PixelFactory::Processor< uint8_t >::resampleTexture(*source, pixmap.width(), pixmap.height(), upscaled, true) && pixmap.copyChannel(output, upscaled, sourceChannel);
	}

	PackRecipe::ChannelSource &
	PackRecipe::channelSource (PixelFactory::Channel output) noexcept
	{
		switch ( output )
		{
			case PixelFactory::Channel::Red : return std::get< 0 >(m_channels);
			case PixelFactory::Channel::Green : return std::get< 1 >(m_channels);
			case PixelFactory::Channel::Blue : return std::get< 2 >(m_channels);
			case PixelFactory::Channel::Alpha : return std::get< 3 >(m_channels);
		}

		/* NOTE: Unreachable for a valid Channel: a corrupted value lands on the alpha. */
		return std::get< 3 >(m_channels);
	}

	bool
	PackRecipe::hasSource () const noexcept
	{
		return std::ranges::any_of(m_channels, [] (const ChannelSource & source) {
			return source.image != nullptr;
		});
	}

	std::string
	PackRecipe::name (std::string_view layout) const noexcept
	{
		std::string name{NamePrefix};
		name += layout;

		char separator = '/';

		for ( const auto & source : m_channels )
		{
			name += separator;
			separator = '|';

			if ( source.image == nullptr )
			{
				name += '#';
				name += std::to_string(source.constant);
			}
			else
			{
				name += source.image->name();
				name += '.';
				name += swizzle(source.channel);
			}
		}

		return name;
	}

	bool
	PackRecipe::leaseSources (StaticVector< Resources::ResourceTrait::LocalDataLease, ChannelCount > & leases, uint32_t & width, uint32_t & height) const noexcept
	{
		width = 0;
		height = 0;

		for ( const auto & source : m_channels )
		{
			if ( source.image == nullptr )
			{
				continue;
			}

			auto lease = source.image->acquireLocalData();

			if ( !lease.isValid() || !source.image->data().isValid() )
			{
				TraceError{ImageResource::ClassId} << "The packed image source '" << source.image->name() << "' has no pixels !";

				return false;
			}

			/* NOTE: In capacity: one lease per output channel at most. */
			leases.push_back(std::move(lease));

			/* NOTE: D12, the largest source decides the size. */
			width = std::max(width, source.image->data().width());
			height = std::max(height, source.image->data().height());
		}

		return width > 0 && height > 0;
	}

	bool
	PackRecipe::compose (PixelFactory::Pixmap< uint8_t > & pixmap) const noexcept
	{
		/* NOTE: Every source is leased for the whole composition: a released copy comes back here (load-time thread). */
		StaticVector< Resources::ResourceTrait::LocalDataLease, ChannelCount > leases;
		uint32_t width = 0;
		uint32_t height = 0;

		if ( !this->leaseSources(leases, width, height) || !pixmap.initialize(width, height, PixelFactory::ChannelMode::RGBA) )
		{
			Tracer::error(ImageResource::ClassId, "Unable to initialize the packed image !");

			return false;
		}

		uint32_t outputIndex = 0;

		for ( const auto & source : m_channels )
		{
			if ( !fillPackedChannel(pixmap, static_cast< PixelFactory::Channel >(outputIndex++), source) )
			{
				TraceError{ImageResource::ClassId} << "Unable to fill a channel of the packed image from '" << (source.image != nullptr ? source.image->name() : std::string{"a constant"}) << "' !";

				return false;
			}
		}

		/* NOTE: The packed image holds what the sources gave: their copies may go (they come back for a recomposition). */
		for ( const auto & source : m_channels )
		{
			if ( source.image != nullptr )
			{
				source.image->markLocalDataReleasable();
			}
		}

		return true;
	}

	bool
	ImageResource::load (PackRecipe recipe) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( !recipe.hasSource() )
		{
			TraceError{ClassId} << "The packed image '" << this->name() << "' has no source image !";

			return this->setLoadSuccess(false);
		}

		for ( const auto & source : recipe.channels() )
		{
			if ( source.image != nullptr && !this->addDependency(source.image) )
			{
				TraceError{ClassId} << "Unable to add the image '" << source.image->name() << "' as a dependency of the packed image '" << this->name() << "' !";

				return this->setLoadSuccess(false);
			}
		}

		m_packRecipe = std::move(recipe);

		return this->setLoadSuccess(true);
	}

	bool
	ImageResource::onDependenciesLoaded () noexcept
	{
		if ( !m_packRecipe.has_value() )
		{
			return true;
		}

		PixelFactory::Pixmap< uint8_t > pixmap;

		if ( !m_packRecipe->compose(pixmap) )
		{
			TraceError{ClassId} << "Unable to compose the packed image '" << this->name() << "' !";

			return false;
		}

		m_pixmap = std::move(pixmap);

		return true;
	}

	bool
	ImageResource::reloadLocalDataFromOwnSource () noexcept
	{
		if ( m_packRecipe.has_value() )
		{
			PixelFactory::Pixmap< uint8_t > pixmap;

			if ( !m_packRecipe->compose(pixmap) )
			{
				TraceError{ClassId} << "The packed image '" << this->name() << "' cannot be composed again from its sources !";

				return false;
			}

			m_pixmap = std::move(pixmap);

			return true;
		}

		if ( !m_encodedSource.has_value() )
		{
			return false;
		}

		PixelFactory::Pixmap< uint8_t > pixmap;

		if ( !m_encodedSource->decode(pixmap) || !pixmap.isValid() )
		{
			TraceError{ClassId} << "The image '" << this->name() << "' cannot be read back from its " << m_encodedSource->describe() << " !";

			return false;
		}

		m_pixmap = std::move(pixmap);

		return true;
	}
}
