/*
 * src/Graphics/ImposterAtlas.cpp
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

#include "ImposterAtlas.hpp"

/* STL inclusions. */
#include <algorithm>
#include <array>
#include <bit>

/* Local inclusions. */
#include "PixelFactory/FileIO.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Sampler.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "Vulkan/TextureInterface.hpp"
#include "Vulkan/TransferManager.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	namespace
	{
		/**
		 * @brief One texture of the atlas, as a material samples it.
		 */
		class AtlasTexture final : public TextureInterface
		{
			public:

				AtlasTexture (std::shared_ptr< Image > image, std::shared_ptr< ImageView > imageView, std::shared_ptr< Sampler > sampler) noexcept
					: m_image{std::move(image)},
					m_imageView{std::move(imageView)},
					m_sampler{std::move(sampler)}
				{

				}

				[[nodiscard]]
				bool
				isCreated () const noexcept override
				{
					return m_image != nullptr && m_image->isCreated() && m_imageView != nullptr && m_imageView->isCreated() && m_sampler != nullptr && m_sampler->isCreated();
				}

				[[nodiscard]]
				TextureType
				type () const noexcept override
				{
					return TextureType::Texture2D;
				}

				[[nodiscard]]
				uint32_t
				dimensions () const noexcept override
				{
					return 2;
				}

				[[nodiscard]]
				bool
				isCubemapTexture () const noexcept override
				{
					return false;
				}

				[[nodiscard]]
				std::shared_ptr< Image >
				image () const noexcept override
				{
					return m_image;
				}

				[[nodiscard]]
				std::shared_ptr< ImageView >
				imageView () const noexcept override
				{
					return m_imageView;
				}

				[[nodiscard]]
				std::shared_ptr< Sampler >
				sampler () const noexcept override
				{
					return m_sampler;
				}

				[[nodiscard]]
				bool
				request3DTextureCoordinates () const noexcept override
				{
					return false;
				}

			private:

				std::shared_ptr< Image > m_image;
				std::shared_ptr< ImageView > m_imageView;
				std::shared_ptr< Sampler > m_sampler;
		};

		/**
		 * @brief Creates one cleared, mipmapped atlas image and its view.
		 */
		[[nodiscard]]
		bool
		createAtlasImage (Renderer & renderer, VkFormat format, uint32_t size, uint32_t mipLevels, const std::string & identifier, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView) noexcept
		{
			image = std::make_shared< Image >(
				renderer.device(),
				VK_IMAGE_TYPE_2D,
				format,
				VkExtent3D{size, size, 1},
				VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
				0,
				mipLevels
			);
			image->setIdentifier(ImposterAtlas::ClassId, identifier, "Image");

			if ( !image->createOnHardware() )
			{
				TraceError{ImposterAtlas::ClassId} << "Unable to create the image of '" << identifier << "' !";

				return false;
			}

			imageView = std::make_shared< ImageView >(
				image,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = mipLevels,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			);
			imageView->setIdentifier(ImposterAtlas::ClassId, identifier, "ImageView");

			if ( !imageView->createOnHardware() )
			{
				TraceError{ImposterAtlas::ClassId} << "Unable to create the image view of '" << identifier << "' !";

				return false;
			}

			/* Cleared, every mip: coverage 0 until the bake, so an imposter drawn early discards everything. */
			auto & transferManager = renderer.transferManager();

			if ( !transferManager.transitionImageLayout(*image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) ||
				 !transferManager.clearColorImage(*image, VkClearColorValue{.float32 = {0.0F, 0.0F, 0.0F, 0.0F}}) ||
				 !transferManager.transitionImageLayout(*image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) )
			{
				TraceError{ImposterAtlas::ClassId} << "Unable to clear the image of '" << identifier << "' !";

				return false;
			}

			return true;
		}
	}

	ImposterAtlas::ImposterAtlas (std::string name, uint32_t gridSize, uint32_t cellSize, uint32_t mipLevels) noexcept
		: m_name{std::move(name)},
		m_gridSize{std::max(gridSize, 2U)},
		m_cellSize{std::max(cellSize, 1U)},
		m_mipLevels{std::clamp(mipLevels, 1U, static_cast< uint32_t >(std::bit_width(std::max(cellSize, 1U))))}
	{

	}

	ImposterAtlas::~ImposterAtlas () = default;

	bool
	ImposterAtlas::create (Renderer & renderer) noexcept
	{
		/* Trilinear, clamped, NO anisotropy: an anisotropic footprint reaches across a cell border into the
		 * neighbouring view. The LOD range stops at the last mip the atlas carries. */
		const auto maxLod = static_cast< float >(m_mipLevels - 1);

		const auto sampler = renderer.getSampler(m_mipLevels == DefaultMipLevels ? "ImposterAtlas" : "ImposterAtlas" + std::to_string(m_mipLevels), [maxLod] (Settings & /*settings*/, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.anisotropyEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = maxLod;
		});

		if ( sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get the sampler of the imposter atlas '" << m_name << "' !";

			return false;
		}

		std::shared_ptr< ImageView > albedoView;
		std::shared_ptr< ImageView > normalView;

		if ( !createAtlasImage(renderer, AlbedoFormat, this->size(), m_mipLevels, m_name + "/Albedo", m_albedoImage, albedoView) ||
			 !createAtlasImage(renderer, NormalFormat, this->size(), m_mipLevels, m_name + "/Normal", m_normalImage, normalView) )
		{
			return false;
		}

		m_albedoTexture = std::make_shared< AtlasTexture >(m_albedoImage, albedoView, sampler);
		m_normalTexture = std::make_shared< AtlasTexture >(m_normalImage, normalView, sampler);

		return true;
	}

	bool
	ImposterAtlas::isCreated () const noexcept
	{
		return m_albedoTexture != nullptr && m_albedoTexture->isCreated() && m_normalTexture != nullptr && m_normalTexture->isCreated();
	}

	void
	ImposterAtlas::recordCopyAndMips (const CommandBuffer & commandBuffer, const Image & source, Image & destination) const noexcept
	{
		const auto size = static_cast< int32_t >(this->size());

		/* The whole chain leaves the shaders for the transfer. */
		{
			const Sync::ImageMemoryBarrier barrier{destination, VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};

			commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		/* Mip 0: the bake itself, one to one. */
		{
			const VkImageBlit region{
				.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
				.srcOffsets = {{0, 0, 0}, {size, size, 1}},
				.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
				.dstOffsets = {{0, 0, 0}, {size, size, 1}}
			};

			commandBuffer.blitImage(source, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region, VK_FILTER_NEAREST);
		}

		/* The chain, each level a box filter of the previous one. ⚠️ An sRGB blit filters in LINEAR space (the
		 * format conversion happens on read and on write), which is what a premultiplied albedo average needs. */
		auto levelSize = size;

		for ( uint32_t level = 1; level < m_mipLevels; ++level )
		{
			{
				Sync::ImageMemoryBarrier barrier{destination, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL};
				barrier.targetMipLevel(level - 1);

				commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			}

			const auto nextSize = std::max(levelSize / 2, 1);

			const VkImageBlit region{
				.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, 1},
				.srcOffsets = {{0, 0, 0}, {levelSize, levelSize, 1}},
				.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, level, 0, 1},
				.dstOffsets = {{0, 0, 0}, {nextSize, nextSize, 1}}
			};

			commandBuffer.blitImage(destination, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, destination, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region, VK_FILTER_LINEAR);

			levelSize = nextSize;
		}

		/* Back to the shaders: every level but the last was read as a blit source, the last only written. */
		if ( m_mipLevels > 1 )
		{
			Sync::ImageMemoryBarrier barrier{destination, VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
			barrier.targetMipLevel(0, m_mipLevels - 1);

			commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		{
			Sync::ImageMemoryBarrier barrier{destination, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
			barrier.targetMipLevel(m_mipLevels - 1);

			commandBuffer.pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		destination.setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	}

	void
	ImposterAtlas::recordBake (const CommandBuffer & commandBuffer, const Image & albedoSource, const Image & normalSource) noexcept
	{
		if ( !this->isCreated() )
		{
			TraceError{ClassId} << "The imposter atlas '" << m_name << "' is not created, the bake is dropped !";

			return;
		}

		this->recordCopyAndMips(commandBuffer, albedoSource, *m_albedoImage);
		this->recordCopyAndMips(commandBuffer, normalSource, *m_normalImage);

		/* Recorded, not executed: every later command of the queue — the frame's main pass included — reads the
		 * atlas after these barriers. */
		m_baked.store(true, std::memory_order_release);
	}

	bool
	ImposterAtlas::writeAlbedo (TransferManager & transferManager, const std::filesystem::path & filepath) const noexcept
	{
		if ( m_albedoImage == nullptr )
		{
			return false;
		}

		Base::PixelFactory::Pixmap< uint8_t > pixmap;

		if ( !transferManager.downloadImage(*m_albedoImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, pixmap) )
		{
			TraceError{ClassId} << "Unable to download the albedo of the imposter atlas '" << m_name << "' !";

			return false;
		}

		return Base::PixelFactory::FileIO::write(pixmap, filepath, true);
	}
}
