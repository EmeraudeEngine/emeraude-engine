/*
 * src/Graphics/TextureResource/Texture2D.cpp
 * This file is part of Emeraude-Engine
 *
 * Copyright (C) 2010-2026 - SÃ©bastien LÃ©on Claude Christian BÃ©melmans "LondNoir" <londnoir@gmail.com>
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

#include "Texture2D.hpp"

/* STL inclusions. */
#include <algorithm>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/KTX2Decoder.hpp"
#include "Graphics/MipChain.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/TextureCache.hpp"
#include "Graphics/TextureCompressor.hpp"
#include "Graphics/TextureResource/Abstract.hpp"
#include "Logging/Severity.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/Pixmap.hpp"
#include "PixelFactory/Processor.hpp"
#include "PixelFactory/Types.hpp"
#include "PrimaryServices.hpp"
#include "Resources/Container.hpp"
#include "SettingKeys.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/PhysicalDevice.hpp"
#include "Vulkan/Sampler.hpp"

namespace EmEn::Graphics::TextureResource
{
	using namespace Base;
	using namespace Vulkan;

	bool
	Texture2D::isCreated () const noexcept
	{
		if ( m_image == nullptr || !m_image->isCreated() )
		{
			return false;
		}

		if ( m_imageView == nullptr || !m_imageView->isCreated() )
		{
			return false;
		}

		if ( m_sampler == nullptr || !m_sampler->isCreated() )
		{
			return false;
		}

		return true;
	}

	bool
	Texture2D::createTexture (Renderer & renderer) noexcept
	{
		if ( m_compressedData != nullptr )
		{
			if ( !this->createFromCompressedData(renderer) )
			{
				return false;
			}
		}
		else if ( !this->createFromPixelData(renderer) )
		{
			return false;
		}

		/* Create a Vulkan image view. */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = m_image->createInfo().mipLevels,
				.baseArrayLayer = 0,
				.layerCount = m_image->createInfo().arrayLayers
			},
			/* NOTE: A one-channel image (BC4) broadcasts its R: the shaders read RGBA as from the source (D8). */
			TextureCompressor::componentMapping(m_image->createInfo().format)
		);
		m_imageView->setIdentifier(ClassId, this->name(), "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create an image view !");

			return false;
		}

		/* Get a Vulkan sampler.
		 * NOTE: The identifier is a DEBUG LABEL, not the cache key — Renderer::getSampler() keys on
		 * the create-info content, so two textures with different addressing can no longer steal
		 * each other's sampler and identical ones still share. Encoding the wrap modes in the name
		 * is therefore no longer load-bearing; it is kept because it makes the Vulkan object labels
		 * readable in a capture (a "Texture2D-cr" sampler tells you which asset asked for it). */
		std::string samplerIdentifier{"Texture2D"};

		if ( !this->usesDefaultWrapModes() )
		{
			samplerIdentifier += '-';
			samplerIdentifier += wrapModeCode(this->wrapModeU());
			samplerIdentifier += wrapModeCode(this->wrapModeV());
		}

		m_sampler = renderer.getSampler(samplerIdentifier, [wrapU = this->wrapModeU(), wrapV = this->wrapModeV()] (Settings & settings, VkSamplerCreateInfo & createInfo) {
			const auto magFilter = settings.getOrSetDefault< std::string >(GraphicsTextureMagFilteringKey, DefaultGraphicsTextureFiltering);
			const auto minFilter = settings.getOrSetDefault< std::string >(GraphicsTextureMinFilteringKey, DefaultGraphicsTextureFiltering);
			const auto mipmapMode = settings.getOrSetDefault< std::string >(GraphicsTextureMipFilteringKey, DefaultGraphicsTextureFiltering);
			const auto mipLevels = settings.getOrSetDefault< float >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels);
			const auto anisotropyLevels = settings.getOrSetDefault< float >(GraphicsTextureAnisotropyLevelsKey, DefaultGraphicsTextureAnisotropy);

			//createInfo.flags = 0;
			createInfo.magFilter = magFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			createInfo.minFilter = minFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			createInfo.mipmapMode = mipmapMode == "linear" ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
			/* The asset's addressing, not a global default: a texture authored with a border and
			 * declared CLAMP_TO_EDGE must show that border once instead of tiling it. W follows U
			 * — a 2D sampler never addresses it, and leaving it at repeat would be misleading. */
			createInfo.addressModeU = toVulkanAddressMode(wrapU);
			createInfo.addressModeV = toVulkanAddressMode(wrapV);
			createInfo.addressModeW = toVulkanAddressMode(wrapU);
			//createInfo.mipLodBias = 0.0F;
			createInfo.anisotropyEnable = anisotropyLevels > 1.0F ? VK_TRUE : VK_FALSE;
			createInfo.maxAnisotropy = anisotropyLevels;
			//createInfo.compareEnable = VK_FALSE;
			//createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			//createInfo.minLod = 0.0F;
			createInfo.maxLod = mipLevels > 0.0F ? mipLevels : VK_LOD_CLAMP_NONE;
			//createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			//createInfo.unnormalizedCoordinates = VK_FALSE;
		});

		if ( m_sampler == nullptr )
		{
			Tracer::error(ClassId, "Unable to get a sampler !");

			return false;
		}

		return true;
	}

	bool
	Texture2D::supportsGreyScaleColour (Renderer & renderer) noexcept
	{
		constexpr VkFormatFeatureFlags Needed = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT | VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT;

		const auto properties = renderer.device()->physicalDevice()->getFormatProperties(VK_FORMAT_R8_SRGB);

		return (properties.optimalTilingFeatures & Needed) == Needed;
	}

	uint32_t
	Texture2D::skippedMipLevels (Settings & settings) const noexcept
	{
		if ( this->isFullResolution() )
		{
			return 0;
		}

		const auto * key = GraphicsTextureDataSkippedMipsKey;

		if ( this->isNormalMap() )
		{
			key = GraphicsTextureNormalSkippedMipsKey;
		}
		else if ( this->isSRGB() )
		{
			key = GraphicsTextureAlbedoSkippedMipsKey;
		}

		return settings.getOrSetDefault< uint32_t >(key, DefaultGraphicsTextureSkippedMips);
	}

	bool
	Texture2D::createFromCompressedData (Renderer & renderer) noexcept
	{
		const auto & mips = m_compressedData->mips();

		if ( mips.empty() )
		{
			TraceError{ClassId} << "The compressed image '" << m_compressedData->name() << "' holds no mip level !";

			return false;
		}

		/* NOTE: Unlike the pixel path, there is nothing to compress here : the mip chain comes
		 * off the disk already in its final GPU layout. No bc7enc pass, no TextureCache round-trip,
		 * and no uncompressed copy is ever materialised. */
		if ( renderer.device()->physicalDevice()->featuresVK10().textureCompressionBC != VK_TRUE )
		{
			TraceError{ClassId} << "The device has no block-compression support, the texture '" << this->name() << "' cannot be created ! The image source should have been decoded to pixels instead.";

			return false;
		}

		/* NOTE: A block cannot be rewritten without decoding it, so a normal map needing its green
		 * channel flipped must be fixed at authoring time. Say so rather than letting the caller
		 * believe the flag was honored. */
		if ( this->isFlipNormalMapYEnabled() )
		{
			TraceWarning{ClassId} << "The texture '" << this->name() << "' requests a normal map Y flip, which a block-compressed image cannot honor. Flip it in the asset instead.";
		}

		/* NOTE: The compressed image stores the linear format ; the colour space is the texture's
		 * call, not the container's, and the blocks are identical either way. */
		const auto format = this->isSRGB() ? KTX2Decoder::sRGBFormat(m_compressedData->format()) : m_compressedData->format();

		/* D5: the top levels this texture's kind skips — free here, the chain ships ready-made (at least one level kept). */
		const auto first = std::min(static_cast< size_t >(this->skippedMipLevels(renderer.primaryServices().settings())), mips.size() - 1);
		const auto & top = mips[first];

		m_image = std::make_shared< Vulkan::Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			format,
			VkExtent3D{top.width, top.height, 1U},
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			0,
			static_cast< uint32_t >(mips.size() - first)
		);
		m_image->setIdentifier(ClassId, this->name(), "Image");

		std::vector< Vulkan::Image::MipLevelData > mipDescs;
		mipDescs.reserve(mips.size() - first);

		for ( const auto & mip : std::span{mips}.subspan(first) )
		{
			mipDescs.push_back({mip.data.data(), static_cast< uint32_t >(mip.data.size()), mip.width, mip.height});
		}

		if ( !m_image->createFromMipChain(renderer.transferManager(), mipDescs) )
		{
			TraceError{ClassId} << "Unable to upload the compressed texture '" << this->name() << "' !";

			m_image.reset();

			return false;
		}

		return true;
	}

	bool
	Texture2D::createFromPixelData (Renderer & renderer) noexcept
	{
		/* NOTE: The upload reads the pixels: they stay resident until it ends, a released copy
		 * is reloaded first (docs/subsystems/resources/03 § CPU Copies). */
		const auto localDataLease = m_localData->acquireLocalData();

		if ( !localDataLease.isValid() )
		{
			TraceError{ClassId} << "The pixels of texture '" << this->name() << "' could not come back (their released CPU copy failed to reload) !";

			return false;
		}

		/* NOTE: FlipNormalMapY (a Direct3D-style map, +Y down, into the Khronos convention) flips a COPY for this
		 * upload: the store image is shared by every texture made from it, and flipping it in place made two flagged
		 * textures cancel each other and an unflagged one read flipped pixels (owner decision 2026-10-12). */
		std::optional< Base::PixelFactory::Pixmap< uint8_t > > flippedPixels;

		if ( this->isFlipNormalMapYEnabled() )
		{
			flippedPixels.emplace(m_localData->data());
			flippedPixels->flipNormalMapY();
		}

		const auto & pixels = flippedPixels.has_value() ? *flippedPixels : m_localData->data();

		if ( !this->validateTexture(pixels) )
		{
			return false;
		}

		auto & settings = renderer.primaryServices().settings();

		const auto fullChain = Image::getMIPLevels(m_localData->width(), m_localData->height());
		const auto settingLevels = settings.getOrSetDefault< uint32_t >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels);
		const auto mipLevels = std::min(fullChain, settingLevels);
		/* D5: the top levels this texture's kind skips (at least one level kept). */
		const auto skipped = std::min(this->skippedMipLevels(settings), fullChain - 1);

		/* Block-compressed when the device supports BC and Core/Graphics/Texture/BlockCompression allows it. */
		const auto useBlockCompression = renderer.device()->physicalDevice()->featuresVK10().textureCompressionBC == VK_TRUE && settings.getOrSetDefault< bool >(GraphicsTextureBlockCompressionKey, DefaultGraphicsTextureBlockCompression);
		bool imageCreated = false;

		if ( useBlockCompression )
		{
			/* Compressed path: the format follows the texture's kind (graphics § 39, D8 — BC5 for a normal map, BC4 for an
			 * opaque grey map, BC7 otherwise); the texture cache service owns it end to end -- disk lookup, compression on
			 * a miss, and storing the result. */
			const auto blockFormat = TextureCompressor::selectFormat(pixels, this->isSRGB(), this->isNormalMap());
			/* NOTE: The chain is compressed (and cached) from the full resolution, the skipped levels included: changing
			 * the footprint settings re-encodes nothing, the upload starts lower. */
			const auto compressedMips = renderer.textureCache().getOrCompress(this->name(), pixels, std::min(fullChain, mipLevels + skipped), blockFormat, this->isSRGB());

			if ( !compressedMips.empty() )
			{
				const auto first = std::min(static_cast< size_t >(skipped), compressedMips.size() - 1);
				const auto & top = compressedMips[first];

				m_image = std::make_shared< Vulkan::Image >(
					renderer.device(),
					VK_IMAGE_TYPE_2D,
					TextureCompressor::vulkanFormat(blockFormat, this->isSRGB()),
					VkExtent3D{top.width, top.height, 1U},
					VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					0,
					static_cast< uint32_t >(compressedMips.size() - first)
				);
				m_image->setIdentifier(ClassId, this->name(), "Image");

				/* Build MipLevelData descriptors for the upload. */
				std::vector< Vulkan::Image::MipLevelData > mipDescs;
				mipDescs.reserve(compressedMips.size() - first);

				for ( const auto & mip : std::span{compressedMips}.subspan(first) )
				{
					mipDescs.push_back({mip.data.data(), static_cast< uint32_t >(mip.data.size()), mip.width, mip.height});
				}

				if ( m_image->createFromMipChain(renderer.transferManager(), mipDescs) )
				{
					imageCreated = true;
				}
				else
				{
					TraceWarning{ClassId} << to_cstring(blockFormat) << " upload failed for '" << this->name() << "', falling back to uncompressed.";

					m_image.reset();
				}
			}
			else
			{
				TraceWarning{ClassId} << to_cstring(blockFormat) << " compression failed for '" << this->name() << "', falling back to uncompressed.";
			}
		}

		if ( !imageCreated )
		{
			/* Uncompressed path (fallback or no BC support): the compressed path's chain, built on the CPU
			 * (MipChain::build(): the Kaiser, an sRGB colour in linear light, the dilated holes, the held coverage, the unit
			 * normals) and uploaded as it is, nothing blitted — the skipped levels (D5) dropped. */
			const auto chainLevels = std::min(fullChain, mipLevels + skipped);
			const auto greyColourSupported = !this->isSRGB() || supportsGreyScaleColour(renderer);
			/* NOTE: An image grey and opaque by its CONTENT but stored in RGB(A) (a palette PNG, a grey RGB file) goes up as
			 * one channel, as the BC path chooses BC4 by content (TextureCompressor::isOpaqueGrey(); owner, 2026-10-21) —
			 * a colour only where the device supports R8_SRGB. */
			std::optional< Base::PixelFactory::Pixmap< uint8_t > > greyPixels;

			if ( pixels.colorCount() == 4 && !this->isNormalMap() && greyColourSupported && TextureCompressor::isOpaqueGrey(pixels) )
			{
				greyPixels.emplace(Base::PixelFactory::Processor< uint8_t >::extractChannel(pixels, Base::PixelFactory::Channel::Red));
			}

			const auto & chainBase = greyPixels.has_value() ? *greyPixels : pixels;
			/* NOTE: A one-channel colour (a grey-scale sRGB image) needs R8_SRGB, whose sampling is optional in Vulkan:
			 * without it every level is uploaded as its RGBA expansion (graphics § 39, D8 step 2). */
			const auto expandGreyColour = chainBase.colorCount() == 1 && !greyColourSupported;
			const MipChain::Treatment treatment{
				.sRGB = this->isSRGB(),
				.normalMap = this->isNormalMap(),
				.alpha = !this->isNormalMap()
			};

			std::vector< Base::PixelFactory::Pixmap< uint8_t > > levels;
			levels.reserve(chainLevels - skipped);

			const auto chainBuilt = MipChain::build(chainBase, chainLevels, treatment, [&levels, skipped, expandGreyColour] (uint32_t level, const Base::PixelFactory::Pixmap< uint8_t > & pixmap) {
				if ( level >= skipped )
				{
					levels.emplace_back(expandGreyColour ? Base::PixelFactory::Processor< uint8_t >::toRGBA(pixmap) : pixmap);
				}

				return true;
			});

			if ( !chainBuilt || levels.empty() )
			{
				TraceError{ClassId} << "Unable to build the mip chain of the texture '" << this->name() << "' !";

				return false;
			}

			std::vector< Vulkan::Image::MipLevelData > mipDescs;
			mipDescs.reserve(levels.size());

			for ( const auto & level : levels )
			{
				/* NOTE: A level of 4 GiB or more (a 32768² RGBA base) does not fit the upload descriptor. */
				if ( !std::in_range< uint32_t >(level.data().size()) )
				{
					TraceError{ClassId} << "The texture '" << this->name() << "' is too large to upload (" << level.width() << "x" << level.height() << ") !";

					return false;
				}

				mipDescs.push_back({level.data().data(), static_cast< uint32_t >(level.data().size()), level.width(), level.height()});
			}

			const auto & top = levels.front();

			m_image = std::make_shared< Vulkan::Image >(
				renderer.device(),
				VK_IMAGE_TYPE_2D,
				Image::getFormat< uint8_t >(top.colorCount(), this->isSRGB()),
				VkExtent3D{top.width(), top.height(), 1U},
				VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				0,
				static_cast< uint32_t >(levels.size())
			);
			m_image->setIdentifier(ClassId, this->name(), "Image");

			if ( !m_image->createFromMipChain(renderer.transferManager(), mipDescs) )
			{
				Tracer::error(ClassId, "Unable to create an image !");

				m_image.reset();

				return false;
			}
		}
		/* The upload consumed the pixels: their CPU copy may be released. */
		m_localData->markLocalDataReleasable();

		return true;
	}

	bool
	Texture2D::destroyTexture () noexcept
	{
		if ( m_image != nullptr )
		{
			m_image->destroyFromHardware();
			m_image.reset();
		}

		if ( m_imageView != nullptr )
		{
			m_imageView->destroyFromHardware();
			m_imageView.reset();
		}

		/* NOTE: The sampler comes from the renderer's shared sampler cache (Renderer::getSampler) and is
		 * shared by every texture of this kind. Only release our reference here — destroying it from
		 * hardware would kill it for all other textures still using it (dangling bindless descriptors,
		 * "vkDestroySampler ... in use"). The cache owns it and destroys it at renderer shutdown. */
		m_sampler.reset();

		return true;
	}

	bool
	Texture2D::isGrayScale () const noexcept
	{
		/* NOTE: A block-compressed source cannot answer this without decoding, which is exactly
		 * what it exists to avoid. Answering "not grayscale" is the conservative reply : it costs
		 * a full-colour path where a grayscale one might have done, never the reverse. */
		if ( !this->isLoaded() || m_localData == nullptr )
		{
			return false;
		}

		return m_localData->isGrayScale();
	}

	PixelFactory::Color< float >
	Texture2D::averageColor () const noexcept
	{
		/* NOTE: Same reasoning as isGrayScale(). */
		if ( !this->isLoaded() || m_localData == nullptr )
		{
			return PixelFactory::Black;
		}

		return m_localData->averageColor();
	}

	bool
	Texture2D::isBinaryAlphaMask () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return false;
		}

		/* NOTE: A block-compressed source cannot be inspected, but a bake records the answer in its container
		 * (graphics § 40, L6); one that does not record it answers "not a mask", the conservative reply: it leaves the
		 * material blended, which is what the asset declared. */
		if ( m_compressedData != nullptr )
		{
			return m_compressedData->isBinaryAlphaMask();
		}

		if ( m_localData == nullptr )
		{
			return false;
		}

		return m_localData->isBinaryAlphaMask();
	}

	bool
	Texture2D::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_localData = this->serviceProvider().container< ImageResource >()->getDefaultResource();

		if ( !this->addDependency(m_localData) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	Texture2D::load (const std::filesystem::path & filepath) noexcept
	{
		return this->load(this->serviceProvider().container< ImageResource >()->getResource(
			ResourceTrait::getResourceNameFromFilepath(filepath, "Images"),
			true)
		);
	}

	bool
	Texture2D::load (const Json::Value & /*data*/) noexcept
	{
		/* NOTE: This resource has no local store,
		 * so this method won't be called from a resource container! */
		Tracer::error(ClassId, "This type of resource is not intended to be loaded this way !");

		return false;
	}

	bool
	Texture2D::load (const std::shared_ptr< ImageResource > & imageResource) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( imageResource == nullptr )
		{
			Tracer::error(ClassId, "The image resource is an empty smart pointer !");

			return this->setLoadSuccess(false);
		}

		m_localData = imageResource;

		if ( !this->addDependency(m_localData) )
		{
			TraceError{ClassId} << "Unable to add the image '" << imageResource->name() << "' as dependency !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	Texture2D::load (const std::shared_ptr< CompressedImageResource > & compressedImageResource) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( compressedImageResource == nullptr )
		{
			Tracer::error(ClassId, "The compressed image resource is an empty smart pointer !");

			return this->setLoadSuccess(false);
		}

		m_compressedData = compressedImageResource;

		if ( !this->addDependency(m_compressedData) )
		{
			TraceError{ClassId} << "Unable to add the compressed image '" << compressedImageResource->name() << "' as dependency !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	std::shared_ptr< ImageResource >
	Texture2D::localData () noexcept
	{
		return m_localData;
	}

	std::shared_ptr< CompressedImageResource >
	Texture2D::compressedData () noexcept
	{
		return m_compressedData;
	}

	bool
	Texture2D::onDependencyFailed (const ResourceTrait & dependency) noexcept
	{
		if ( !this->takeDefaultData(m_localData, dependency) )
		{
			return false;
		}

		/* The compressed path fell: the default IMAGE replaces it, the upload takes the uncompressed path. */
		m_compressedData.reset();

		return true;
	}
}
