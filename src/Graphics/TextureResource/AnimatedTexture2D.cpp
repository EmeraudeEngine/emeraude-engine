/*
 * src/Graphics/TextureResource/AnimatedTexture2D.cpp
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

#include "AnimatedTexture2D.hpp"

/* STL inclusions. */
#include <algorithm>
#include <functional>
#include <span>
#include <string>
#include <vector>

/* Third-party inclusions. */
#include "volk.h"

/* Local inclusions. */
#include "BlobTrait.hpp"
#include "Graphics/MovieResource.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/TextureCache.hpp"
#include "Graphics/TextureCompressor.hpp"
#include "Graphics/TextureResource/Texture2D.hpp"
#include "Logging/Severity.hpp"
#include "PixelFactory/Color.hpp"
#include "PixelFactory/Pixmap.hpp"
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
	AnimatedTexture2D::isCreated () const noexcept
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
	AnimatedTexture2D::createCompressedImage (Renderer & renderer, uint32_t mipLevels) noexcept
	{
		const auto frameCount = m_localData->frameCount();

		/* NOTE: One format for every layer of the array (graphics § 39, D8): BC5 for a normal map; BC4 only when EVERY
		 * frame is an opaque grey map; BC7 otherwise. */
		auto blockFormat = BlockFormat::BC7;

		if ( !m_localData->frames().empty() )
		{
			blockFormat = TextureCompressor::selectFormat(m_localData->frames().front().pixmap(), this->isSRGB(), this->isNormalMap());

			if ( blockFormat == BlockFormat::BC4 && !std::ranges::all_of(m_localData->frames(), [] (const auto & frame) { return TextureCompressor::isOpaqueGrey(frame.pixmap()); }) )
			{
				blockFormat = BlockFormat::BC7;
			}
		}

		/* NOTE: Each frame through the texture cache (its pixels are its key): compressed once, read back afterwards. */
		std::vector< std::vector< CompressedMipLevel > > frames;
		frames.reserve(frameCount);

		for ( const auto & frame : m_localData->frames() )
		{
			auto mips = renderer.textureCache().getOrCompress(this->name() + "#layer" + std::to_string(frames.size()), frame.pixmap(), mipLevels, blockFormat);

			if ( mips.empty() || (!frames.empty() && mips.size() != frames.front().size()) )
			{
				TraceWarning{ClassId} << to_cstring(blockFormat) << " compression failed for frame " << frames.size() << " of '" << this->name() << "', falling back to uncompressed.";

				return false;
			}

			frames.push_back(std::move(mips));
		}

		if ( frames.empty() )
		{
			return false;
		}

		/* NOTE: Layer-major: the chain of frame 0, then frame 1's… (Vulkan::Image::createFromCompressed()). */
		std::vector< Vulkan::Image::CompressedMip > mipDescs;
		mipDescs.reserve(frames.size() * frames.front().size());

		for ( const auto & frameMips : frames )
		{
			for ( const auto & mip : frameMips )
			{
				mipDescs.push_back({mip.data.data(), static_cast< uint32_t >(mip.data.size()), mip.width, mip.height});
			}
		}

		m_image = std::make_shared< Vulkan::Image >(
			renderer.device(),
			VK_IMAGE_TYPE_2D,
			TextureCompressor::vulkanFormat(blockFormat, this->isSRGB()),
			VkExtent3D{m_localData->width(), m_localData->height(), 1U},
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			0,
			static_cast< uint32_t >(frames.front().size()),
			frameCount
		);
		m_image->setIdentifier(ClassId, this->name(), "Image");

		if ( !m_image->createFromCompressed(renderer.transferManager(), mipDescs, frameCount) )
		{
			TraceWarning{ClassId} << to_cstring(blockFormat) << " upload failed for '" << this->name() << "', falling back to uncompressed.";

			m_image.reset();

			return false;
		}

		return true;
	}

	bool
	AnimatedTexture2D::createTexture (Renderer & renderer) noexcept
	{
		/* NOTE: The upload reads the images the movie's frames show: they stay resident until it ends, a released copy
		 * is reloaded first (docs/subsystems/resources/03 § CPU Copies). */
		std::vector< Resources::ResourceTrait::LocalDataLease > localDataLeases;

		if ( !m_localData->leaseFrameImages(localDataLeases) )
		{
			TraceError{ClassId} << "The images of animated texture '" << this->name() << "' are not resident (their CPU copy was released) !";

			return false;
		}

		for ( const auto & frame : m_localData->frames() )
		{
			if ( !this->validateTexture(frame.pixmap()) )
			{
				return false;
			}
		}

		/* ⚠️⚠️ A FULL mip chain, as Texture2D builds (2026-09-25): created with a single level, the animated water
		 * normal map of `water-world`/`terrain` was sampled at full resolution at every distance — the sea shimmered
		 * as noise past a few dozen metres, worse once the owner tiled it once per metre. The upload blits the chain
		 * of EVERY layer (ImageTransferOperation), which is why the image is also a transfer source. */
		auto & settings = renderer.primaryServices().settings();

		const auto mipLevels = std::min(
			Image::getMIPLevels(m_localData->width(), m_localData->height()),
			settings.getOrSetDefault< uint32_t >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels)
		);

		/* NOTE: Block-compressed on every BC-capable GPU, like Texture2D (graphics § 39); RGBA8 otherwise, or when a frame fails. */
		const auto useBlockCompression = renderer.device()->physicalDevice()->featuresVK10().textureCompressionBC == VK_TRUE;

		if ( !useBlockCompression || !this->createCompressedImage(renderer, mipLevels) )
		{
			/* NOTE: One-channel colour frames (grey-scale sRGB images) need R8_SRGB, optional in Vulkan: refused rather
			 * than shown wrong (graphics § 39, D8 step 2 — the BC path above takes them on every BC-capable GPU). */
			if ( this->isSRGB() && m_localData->data(0).colorCount() == 1 && !Texture2D::supportsGreyScaleColour(renderer) )
			{
				TraceError{ClassId} << "The animated texture '" << this->name() << "' has grey-scale colour frames and the device supports neither block compression nor R8_SRGB !";

				return false;
			}

			/* Create a Vulkan image. */
			m_image = std::make_shared< Vulkan::Image >(
				renderer.device(),
				VK_IMAGE_TYPE_2D,
				Image::getFormat< uint8_t >(m_localData->data(0).colorCount(), this->isSRGB()),
				VkExtent3D{m_localData->width(), m_localData->height(), 1},
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
				0,
				mipLevels,
				m_localData->frameCount()
			);
			m_image->setIdentifier(ClassId, this->name(), "Image");

			if ( !m_image->create(renderer.transferManager(), m_localData) )
			{
				Tracer::error(ClassId, "Unable to create an image !");

				m_image.reset();

				return false;
			}
		}

		/* Create a Vulkan image view. */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_2D_ARRAY,
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

		/* Pre-create one VK_IMAGE_VIEW_TYPE_2D view per layer so the RT bindless
		 * `sampler2D[]` descriptor can sample a single frame at a time. The main
		 * imageView (above) is a 2D_ARRAY view used by the rasterization path. */
		const auto layerCount = m_image->createInfo().arrayLayers;
		m_frame2DViews.reserve(layerCount);

		for ( uint32_t layer = 0; layer < layerCount; ++layer )
		{
			auto frameView = std::make_shared< ImageView >(
				m_image,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = m_image->createInfo().mipLevels,
					.baseArrayLayer = layer,
					.layerCount = 1
				},
				TextureCompressor::componentMapping(m_image->createInfo().format)
			);
			frameView->setIdentifier(ClassId, this->name() + "_Frame" + std::to_string(layer), "ImageView");

			if ( !frameView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create per-frame 2D image view (layer " << layer << ") for '" << this->name() << "' !";

				return false;
			}

			m_frame2DViews.emplace_back(std::move(frameView));
		}

		/* Get a Vulkan sampler. */
		m_sampler = renderer.getSampler("AnimatedTexture2D", [] (Settings & samplerSettings, VkSamplerCreateInfo & createInfo) {
			const auto magFilter = samplerSettings.getOrSetDefault< std::string >(GraphicsTextureMagFilteringKey, DefaultGraphicsTextureFiltering);
			const auto minFilter = samplerSettings.getOrSetDefault< std::string >(GraphicsTextureMinFilteringKey, DefaultGraphicsTextureFiltering);
			const auto mipmapMode = samplerSettings.getOrSetDefault< std::string >(GraphicsTextureMipFilteringKey, DefaultGraphicsTextureFiltering);
			const auto maxLodLevels = samplerSettings.getOrSetDefault< float >(GraphicsTextureMipMappingLevelsKey, DefaultGraphicsTextureMipMappingLevels);
			const auto anisotropyLevels = samplerSettings.getOrSetDefault< float >(GraphicsTextureAnisotropyLevelsKey, DefaultGraphicsTextureAnisotropy);

			//createInfo.flags = 0;
			createInfo.magFilter = magFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			createInfo.minFilter = minFilter == "linear" ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
			createInfo.mipmapMode = mipmapMode == "linear" ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
			//createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			//createInfo.mipLodBias = 0.0F;
			createInfo.anisotropyEnable = anisotropyLevels > 1.0F ? VK_TRUE : VK_FALSE;
			createInfo.maxAnisotropy = anisotropyLevels;
			//createInfo.compareEnable = VK_FALSE;
			//createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
			//createInfo.minLod = 0.0F;
			createInfo.maxLod = maxLodLevels > 0.0F ? maxLodLevels : VK_LOD_CLAMP_NONE;
			//createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			//createInfo.unnormalizedCoordinates = VK_FALSE;
		});

		if ( m_sampler == nullptr )
		{
			Tracer::error(ClassId, "Unable to get a sampler !");

			return false;
		}

		/* The upload consumed the images: their CPU copy may be released. */
		m_localData->markFrameImagesReleasable();

		return true;
	}

	bool
	AnimatedTexture2D::destroyTexture () noexcept
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

		for ( auto & frameView : m_frame2DViews )
		{
			if ( frameView != nullptr )
			{
				frameView->destroyFromHardware();
			}
		}
		m_frame2DViews.clear();

		/* NOTE: The sampler comes from the renderer's shared sampler cache (Renderer::getSampler) and is
		 * shared by every texture of this kind. Only release our reference here — destroying it from
		 * hardware would kill it for all other textures still using it (dangling bindless descriptors,
		 * "vkDestroySampler ... in use"). The cache owns it and destroys it at renderer shutdown. */
		m_sampler.reset();

		return true;
	}

	bool
	AnimatedTexture2D::isGrayScale () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return false;
		}

		return m_localData->isGrayScale();
	}

	PixelFactory::Color< float >
	AnimatedTexture2D::averageColor () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return PixelFactory::Black;
		}

		return m_localData->averageColor();
	}

	uint32_t
	AnimatedTexture2D::frameCount () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->frameCount();
	}

	uint32_t
	AnimatedTexture2D::duration () const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->duration();
	}

	uint32_t
	AnimatedTexture2D::frameIndexAt (uint32_t sceneTime) const noexcept
	{
		if ( !this->isLoaded() )
		{
			return 0;
		}

		return m_localData->frameIndexAt(sceneTime);
	}

	bool
	AnimatedTexture2D::load () noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		m_localData = this->serviceProvider().container< MovieResource >()->getDefaultResource();

		if ( !this->addDependency(m_localData) )
		{
			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	AnimatedTexture2D::load (const std::filesystem::path & filepath) noexcept
	{
		/* Looking for a movie resource by extracting the resource name from the filepath.
		 * NOTE: The loading process is synchronous here. */
		const auto movieResource = this->serviceProvider().container< MovieResource >()->getResource(
			ResourceTrait::getResourceNameFromFilepath(filepath, "Movies"),
			false
		);

		return this->load(movieResource);
	}

	bool
	AnimatedTexture2D::load (const Json::Value & /*data*/) noexcept
	{
		/* NOTE: This resource has no local store,
		 * so this method won't be called from a resource container! */
		Tracer::warning(ClassId, "This type of resource is not intended to be loaded this way !");

		return false;
	}

	bool
	AnimatedTexture2D::load (const std::shared_ptr< MovieResource > & movieResource) noexcept
	{
		if ( !this->beginLoading() )
		{
			return false;
		}

		if ( movieResource == nullptr )
		{
			Tracer::error(ClassId, "The movie resource is an empty smart pointer !");

			return this->setLoadSuccess(false);
		}

		m_localData = movieResource;

		if ( !this->addDependency(m_localData) )
		{
			TraceError{ClassId} << "Unable to add the movie '" << movieResource->name() << "' as dependency !";

			return this->setLoadSuccess(false);
		}

		return this->setLoadSuccess(true);
	}

	bool
	AnimatedTexture2D::onDependencyFailed (const ResourceTrait & dependency) noexcept
	{
		return this->takeDefaultData(m_localData, dependency);
	}
}
