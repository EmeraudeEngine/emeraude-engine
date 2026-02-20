/*
 * src/Graphics/IntermediateRenderTarget.cpp
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

#include "IntermediateRenderTarget.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	using namespace Vulkan;

	bool
	IntermediateRenderTarget::create (Renderer & renderer, uint32_t width, uint32_t height, VkFormat format, const std::string & identifier) noexcept
	{
		if ( this->isCreated() )
		{
			return true;
		}

		m_width = width;
		m_height = height;
		m_format = format;

		const auto device = renderer.device();

		/* Create the color image with render target and sampled usage. */
		m_image = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			format,
			VkExtent3D{width, height, 1},
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		);
		m_image->setIdentifier(ClassId, identifier, "Image");

		if ( !m_image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create image for IRT '" << identifier << "' !";

			return false;
		}

		/* Transition to shader read layout (initial state for sampling). */
		{
			const auto & transferManager = renderer.transferManager();

			if ( !transferManager.transitionImageLayout(
				*m_image,
				VK_IMAGE_ASPECT_COLOR_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			) )
			{
				TraceError{ClassId} << "Unable to transition IRT '" << identifier << "' to shader read layout !";

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
		m_imageView->setIdentifier(ClassId, identifier, "ImageView");

		if ( !m_imageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create image view for IRT '" << identifier << "' !";

			return false;
		}

		/* Get or create the sampler: linear filtering, clamp-to-edge. */
		m_sampler = renderer.getSampler("IRT_" + identifier, [] (Settings &, VkSamplerCreateInfo & createInfo) {
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
			TraceError{ClassId} << "Unable to get sampler for IRT '" << identifier << "' !";

			return false;
		}

		/* Create the render pass. */
		if ( !this->createRenderPass(device) )
		{
			TraceError{ClassId} << "Unable to create render pass for IRT '" << identifier << "' !";

			return false;
		}

		/* Create the framebuffer. */
		if ( !this->createFramebuffer(device) )
		{
			TraceError{ClassId} << "Unable to create framebuffer for IRT '" << identifier << "' !";

			return false;
		}

		TraceSuccess{ClassId} << "IRT '" << identifier << "' created (" << width << "x" << height << ").";

		return true;
	}

	void
	IntermediateRenderTarget::destroy () noexcept
	{
		m_framebuffer.reset();
		m_renderPass.reset();
		m_sampler.reset();
		m_imageView.reset();
		m_image.reset();
		m_width = 0;
		m_height = 0;
		m_format = VK_FORMAT_UNDEFINED;
	}

	bool
	IntermediateRenderTarget::recreate (Renderer & renderer, uint32_t width, uint32_t height, VkFormat format, const std::string & identifier) noexcept
	{
		this->destroy();

		return this->create(renderer, width, height, format, identifier);
	}

	const Framebuffer &
	IntermediateRenderTarget::framebuffer () const noexcept
	{
		return *m_framebuffer;
	}

	const RenderPass &
	IntermediateRenderTarget::renderPass () const noexcept
	{
		return *m_renderPass;
	}

	void
	IntermediateRenderTarget::beginRenderPass (const CommandBuffer & commandBuffer) const noexcept
	{
		const VkRect2D renderArea{
			.offset = {0, 0},
			.extent = {m_width, m_height}
		};

		const std::array< VkClearValue, 1 > clearValues{
			VkClearValue{.color = {{0.0F, 0.0F, 0.0F, 0.0F}}}
		};

		commandBuffer.beginRenderPass(*m_framebuffer, renderArea, clearValues, VK_SUBPASS_CONTENTS_INLINE);
	}

	void
	IntermediateRenderTarget::endRenderPass (const CommandBuffer & commandBuffer) const noexcept
	{
		commandBuffer.endRenderPass();
	}

	bool
	IntermediateRenderTarget::createRenderPass (const std::shared_ptr< Device > & device) noexcept
	{
		auto renderPass = std::make_shared< RenderPass >(device);
		renderPass->setIdentifier(ClassId, "IRT", "RenderPass");

		/* Single color attachment: don't care about previous contents, store the result,
		 * final layout is shader read so it can be sampled immediately after. */
		renderPass->addAttachmentDescription(VkAttachmentDescription{
			.flags = 0,
			.format = m_format,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		});

		/* Single subpass with the color attachment. */
		RenderSubPass subPass;
		subPass.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		renderPass->addSubPass(subPass);

		/* External -> subpass 0 dependency. */
		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});

		/* Subpass 0 -> external dependency. */
		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		});

		if ( !renderPass->createOnHardware() )
		{
			return false;
		}

		m_renderPass = std::move(renderPass);

		return true;
	}

	bool
	IntermediateRenderTarget::createFramebuffer (const std::shared_ptr< Device > & /*device*/) noexcept
	{
		const VkExtent2D extent{m_width, m_height};

		m_framebuffer = std::make_unique< Framebuffer >(m_renderPass, extent);
		m_framebuffer->setIdentifier(ClassId, "IRT", "Framebuffer");
		m_framebuffer->addAttachment(m_imageView->handle());

		if ( !m_framebuffer->createOnHardware() )
		{
			m_framebuffer.reset();

			return false;
		}

		return true;
	}

	bool
	IntermediateRenderTarget::isCreated () const noexcept
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

		if ( m_framebuffer == nullptr || !m_framebuffer->isCreated() )
		{
			return false;
		}

		return true;
	}

	TextureType
	IntermediateRenderTarget::type () const noexcept
	{
		return TextureType::Texture2D;
	}

	uint32_t
	IntermediateRenderTarget::dimensions () const noexcept
	{
		return 2;
	}

	bool
	IntermediateRenderTarget::isCubemapTexture () const noexcept
	{
		return false;
	}

	std::shared_ptr< Image >
	IntermediateRenderTarget::image () const noexcept
	{
		return m_image;
	}

	std::shared_ptr< ImageView >
	IntermediateRenderTarget::imageView () const noexcept
	{
		return m_imageView;
	}

	std::shared_ptr< Sampler >
	IntermediateRenderTarget::sampler () const noexcept
	{
		return m_sampler;
	}

	bool
	IntermediateRenderTarget::request3DTextureCoordinates () const noexcept
	{
		return false;
	}
}
