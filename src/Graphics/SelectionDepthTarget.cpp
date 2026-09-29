/*
 * src/Graphics/SelectionDepthTarget.cpp
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

#include "SelectionDepthTarget.hpp"

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/RenderPass.hpp"

namespace EmEn::Graphics
{
	SelectionDepthTarget::SelectionDepthTarget (uint32_t width, uint32_t height, float viewDistance) noexcept
		: Abstract{
			ClassId,
			FramebufferPrecisions{0, 0, 0, 0, 32, 0, 1},
			{width, height, 1U},
			viewDistance,
			RenderTargetType::SelectionDepth,
			Scenes::AVConsole::ConnexionType::Both,
			false,
			false
		}
	{

	}

	void
	SelectionDepthTarget::setViewDistance (float meters) noexcept
	{
		const auto & extent = this->extent();

		m_viewMatrices.updatePerspectiveViewProperties(static_cast< float >(extent.width), static_cast< float >(extent.height), m_viewMatrices.fieldOfView(), meters);
	}

	float
	SelectionDepthTarget::viewDistance () const noexcept
	{
		return this->viewMatrices().farPlane();
	}

	void
	SelectionDepthTarget::updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept
	{
		const auto & extent = this->extent();

		m_viewMatrices.updatePerspectiveViewProperties(static_cast< float >(extent.width), static_cast< float >(extent.height), fovOrNear, distanceOrFar);
	}

	float
	SelectionDepthTarget::aspectRatio () const noexcept
	{
		if ( this->extent().height == 0 )
		{
			return 0.0F;
		}

		return static_cast< float >(this->extent().width) / static_cast< float >(this->extent().height);
	}

	const Vulkan::Framebuffer *
	SelectionDepthTarget::framebuffer () const noexcept
	{
		return m_framebuffer.get();
	}

	const ViewMatricesInterface &
	SelectionDepthTarget::viewMatrices () const noexcept
	{
		if ( m_sourceViewMatrices != nullptr )
		{
			return *m_sourceViewMatrices;
		}

		return m_viewMatrices;
	}

	ViewMatricesInterface &
	SelectionDepthTarget::viewMatrices () noexcept
	{
		if ( m_sourceViewMatrices != nullptr )
		{
			return *m_sourceViewMatrices;
		}

		return m_viewMatrices;
	}

	bool
	SelectionDepthTarget::capture (Vulkan::TransferManager & /*transferManager*/, uint32_t /*layerIndex*/, bool /*keepAlpha*/, bool /*withDepthBuffer*/, bool /*withStencilBuffer*/, std::array< Base::PixelFactory::Pixmap< uint8_t >, 3 > & /*result*/) const noexcept
	{
		TraceWarning{ClassId} << "The selection depth target '" << this->id() << "' cannot be captured !";

		return false;
	}

	void
	SelectionDepthTarget::updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept
	{
		this->setOrthographicProjection(isOrthographicProjection);
		this->updateViewRangesProperties(fovOrNear, distanceOrFar);
	}

	std::shared_ptr< Vulkan::RenderPass >
	SelectionDepthTarget::createRenderPass (Renderer & renderer) const noexcept
	{
		auto renderPass = std::make_shared< Vulkan::RenderPass >(renderer.device(), 0);
		renderPass->setIdentifier(ClassId, this->id(), "RenderPass");

		Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

		/* The only attachment: cleared to the far plane, left SAMPLEABLE for the outline composite. */
		renderPass->addAttachmentDescription(VkAttachmentDescription{
			.flags = 0,
			.format = DepthFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		});

		subPass.setDepthStencilAttachment(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		renderPass->addSubPass(subPass);

		/* The previous frame's composite sampled this image: its reads end before the depth is written again. */
		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		});

		/* The composite samples the depth this pass wrote. */
		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = 0
		});

		if ( !renderPass->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the render pass for '" << this->id() << "' !";

			return nullptr;
		}

		return renderPass;
	}

	bool
	SelectionDepthTarget::onCreate (Renderer & renderer) noexcept
	{
		const auto device = renderer.device();

		m_depthImage = std::make_shared< Vulkan::Image >(
			device,
			VK_IMAGE_TYPE_2D,
			DepthFormat,
			this->extent(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		);
		m_depthImage->setIdentifier(ClassId, this->id(), "DepthImage");

		if ( !m_depthImage->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the depth image for '" << this->id() << "' !";

			return false;
		}

		m_depthImageView = std::make_shared< Vulkan::ImageView >(
			m_depthImage,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		);
		m_depthImageView->setIdentifier(ClassId, this->id(), "DepthImageView");

		if ( !m_depthImageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the depth image view for '" << this->id() << "' !";

			return false;
		}

		const auto renderPass = this->createRenderPass(renderer);

		if ( renderPass == nullptr )
		{
			return false;
		}

		m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, this->extent());
		m_framebuffer->setIdentifier(ClassId, this->id(), "Framebuffer");
		m_framebuffer->addAttachment(m_depthImageView->handle());

		if ( !m_framebuffer->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the framebuffer for '" << this->id() << "' !";

			return false;
		}

		m_isReadyForRendering = true;

		return true;
	}

	void
	SelectionDepthTarget::onDestroy () noexcept
	{
		m_isReadyForRendering = false;

		m_framebuffer.reset();
		m_depthImageView.reset();
		m_depthImage.reset();
	}
}
