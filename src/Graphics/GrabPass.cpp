/*
 * src/Graphics/GrabPass.cpp
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

#include "GrabPass.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Sync/ImageMemoryBarrier.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	bool
	GrabPass::create (Renderer & renderer, uint32_t width, uint32_t height, VkFormat format) noexcept
	{
		if ( this->isCreated() )
		{
			return true;
		}

		const auto device = renderer.device();

		/* Create the grab pass image with transfer destination and sampled usage. */
		m_image = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			format,
			VkExtent3D{width, height, 1},
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		);
		m_image->setIdentifier(ClassId, "Color", "Image");

		if ( !m_image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the grab pass image !";

			return false;
		}

		/* Transition to shader read layout. */
		{
			const auto & transferManager = renderer.transferManager();

			if ( !transferManager.transitionImageLayout(
				*m_image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			) )
			{
				TraceError{ClassId} << "Unable to transition grab pass image to shader read layout !";

				return false;
			}
		}

		m_image->setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		/* Create the image view. */
		m_imageView = std::make_shared< ImageView >(
			m_image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		);
		m_imageView->setIdentifier(ClassId, "Color", "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the grab pass image view !";

			return false;
		}

		/* Get or create the grab pass sampler: linear filtering, clamp-to-edge. */
		m_sampler = renderer.getSampler("GrabPass", [] (Settings &, VkSamplerCreateInfo & createInfo) {
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			createInfo.compareEnable = VK_FALSE;
			createInfo.minLod = 0.0F;
			createInfo.maxLod = 1.0F;
		});

		if ( m_sampler == nullptr )
		{
			TraceError{ClassId} << "Unable to get the sampler for grab pass !";

			return false;
		}

		TraceSuccess{ClassId} << "Grab pass texture created (" << width << "x" << height << ").";

		return true;
	}

	void
	GrabPass::destroy () noexcept
	{
		m_sampler.reset();
		m_imageView.reset();
		m_image.reset();
	}

	bool
	GrabPass::recreate (Renderer & renderer, uint32_t width, uint32_t height, VkFormat format) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height, format);
	}

	void
	GrabPass::recordBlit (const CommandBuffer & commandBuffer, const Image & srcColorImage) const noexcept
	{
		if ( !this->isCreated() )
		{
			return;
		}

		/* 1. Barrier: swapchain image COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL */
		{
			const Sync::ImageMemoryBarrier srcBarrier{
				srcColorImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				srcBarrier,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT
			);
		}

		/* 2. Barrier: grab texture SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL */
		{
			const Sync::ImageMemoryBarrier dstBarrier{
				*m_image,
				VK_ACCESS_SHADER_READ_BIT,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				dstBarrier,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT
			);
		}

		/* 3. Blit: swapchain -> grab texture */
		commandBuffer.blitImage(
			srcColorImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			*m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
		);

		/* 4. Barrier: grab texture TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL */
		{
			const Sync::ImageMemoryBarrier dstBarrier{
				*m_image,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				dstBarrier,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
			);
		}

		/* 5. Barrier: swapchain image TRANSFER_SRC_OPTIMAL -> COLOR_ATTACHMENT_OPTIMAL
		 * (ready for the post-process render pass). */
		{
			const Sync::ImageMemoryBarrier srcBarrier{
				srcColorImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			commandBuffer.pipelineBarrier(
				srcBarrier,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			);
		}
	}

	bool
	GrabPass::isCreated () const noexcept
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

	TextureType
	GrabPass::type () const noexcept
	{
		return TextureType::Texture2D;
	}

	uint32_t
	GrabPass::dimensions () const noexcept
	{
		return 2;
	}

	bool
	GrabPass::isCubemapTexture () const noexcept
	{
		return false;
	}

	std::shared_ptr< Image >
	GrabPass::image () const noexcept
	{
		return m_image;
	}

	std::shared_ptr< ImageView >
	GrabPass::imageView () const noexcept
	{
		return m_imageView;
	}

	std::shared_ptr< Sampler >
	GrabPass::sampler () const noexcept
	{
		return m_sampler;
	}

	bool
	GrabPass::request3DTextureCoordinates () const noexcept
	{
		return false;
	}
}
