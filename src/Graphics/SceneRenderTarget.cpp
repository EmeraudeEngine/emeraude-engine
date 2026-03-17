/*
 * src/Graphics/SceneRenderTarget.cpp
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

#include "SceneRenderTarget.hpp"

/* Local inclusions. */
#include "Libs/PixelFactory/Processor.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/RenderPass.hpp"
#include "Graphics/Renderer.hpp"
#include "Tracer.hpp"

namespace EmEn::Graphics
{
	SceneRenderTarget::SceneRenderTarget (const std::string & name, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat normalsFormat, VkFormat materialPropertiesFormat, VkFormat depthFormat, float viewDistance) noexcept
		: Abstract{
			name,
			FramebufferPrecisions{8, 8, 8, 8, 24, 0, 1},
			{width, height, 1U},
			viewDistance,
			RenderTargetType::View,
			Scenes::AVConsole::ConnexionType::Both,
			false,
			true
		},
		m_colorFormat{colorFormat},
		m_normalsFormat{normalsFormat},
		m_materialPropertiesFormat{materialPropertiesFormat},
		m_depthFormat{depthFormat}
	{

	}

	void
	SceneRenderTarget::setViewDistance (float meters) noexcept
	{
		const auto & ext = this->extent();
		const auto width = static_cast< float >(ext.width);
		const auto height = static_cast< float >(ext.height);

		if ( this->isOrthographicProjection() )
		{
			m_viewMatrices.updateOrthographicViewProperties(width, height, m_viewMatrices.nearPlane(), meters);
		}
		else
		{
			m_viewMatrices.updatePerspectiveViewProperties(width, height, m_viewMatrices.fieldOfView(), meters);
		}
	}

	float
	SceneRenderTarget::viewDistance () const noexcept
	{
		if ( m_sourceViewMatrices != nullptr )
		{
			return m_sourceViewMatrices->farPlane();
		}

		return m_viewMatrices.farPlane();
	}

	void
	SceneRenderTarget::updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept
	{
		const auto & ext = this->extent();
		const auto width = static_cast< float >(ext.width);
		const auto height = static_cast< float >(ext.height);

		if ( this->isOrthographicProjection() )
		{
			m_viewMatrices.updateOrthographicViewProperties(width, height, fovOrNear, distanceOrFar);
		}
		else
		{
			m_viewMatrices.updatePerspectiveViewProperties(width, height, fovOrNear, distanceOrFar);
		}
	}

	float
	SceneRenderTarget::aspectRatio () const noexcept
	{
		if ( this->extent().height == 0 )
		{
			return 0.0F;
		}

		return static_cast< float >(this->extent().width) / static_cast< float >(this->extent().height);
	}

	bool
	SceneRenderTarget::isCubemap () const noexcept
	{
		return false;
	}

	const Vulkan::Framebuffer *
	SceneRenderTarget::framebuffer () const noexcept
	{
		return m_framebuffer.get();
	}

	const Vulkan::Framebuffer *
	SceneRenderTarget::postProcessFramebuffer () const noexcept
	{
		return m_postProcessFramebuffer.get();
	}

	const ViewMatricesInterface &
	SceneRenderTarget::viewMatrices () const noexcept
	{
		if ( m_sourceViewMatrices != nullptr )
		{
			return *m_sourceViewMatrices;
		}

		return m_viewMatrices;
	}

	ViewMatricesInterface &
	SceneRenderTarget::viewMatrices () noexcept
	{
		if ( m_sourceViewMatrices != nullptr )
		{
			return *m_sourceViewMatrices;
		}

		return m_viewMatrices;
	}

	bool
	SceneRenderTarget::isReadyForRendering () const noexcept
	{
		return m_isReadyForRendering;
	}

	Scenes::AVConsole::VideoType
	SceneRenderTarget::videoType () const noexcept
	{
		return Scenes::AVConsole::VideoType::View;
	}

	bool
	SceneRenderTarget::capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool keepAlpha, bool withDepthBuffer, bool /*withStencilBuffer*/, std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > & result) const noexcept
	{
		if ( layerIndex > 0 )
		{
			TraceWarning{ClassId} << "SceneRenderTarget does not support layered images. Layer " << layerIndex << " requested, using layer 0 instead for '" << this->id() << "'.";
		}

		if ( m_colorImage != nullptr && m_colorImage->isCreated() )
		{
			if ( !transferManager.downloadImage(*m_colorImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_COLOR_BIT, result[0]) )
			{
				TraceError{ClassId} << "Failed to capture color buffer for '" << this->id() << "' !";
				return false;
			}

			if ( !keepAlpha )
			{
				result[0] = Libs::PixelFactory::Processor< uint8_t >::toRGB(result[0]);
			}
		}

		if ( withDepthBuffer && m_depthStencilImage != nullptr && m_depthStencilImage->isCreated() )
		{
			if ( !transferManager.downloadImage(*m_depthStencilImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, result[1]) )
			{
				TraceWarning{ClassId} << "Failed to capture depth buffer for '" << this->id() << "' !";
			}
		}

		return true;
	}

	void
	SceneRenderTarget::updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept
	{
		this->setOrthographicProjection(isOrthographicProjection);
		this->updateViewRangesProperties(fovOrNear, distanceOrFar);
	}

	Libs::Math::CartesianFrame< float >
	SceneRenderTarget::getWorldCoordinates () const noexcept
	{
		return m_worldCoordinates;
	}

	void
	SceneRenderTarget::updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept
	{
		m_worldCoordinates = worldCoordinates;
		m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
	}

	void
	SceneRenderTarget::onInputDeviceConnected (EngineContext & engineContext, AbstractVirtualDevice & /*sourceDevice*/) noexcept
	{
		m_viewMatrices.create(engineContext.graphicsRenderer, this->id());
	}

	void
	SceneRenderTarget::onInputDeviceDisconnected (EngineContext & /*engineContext*/, AbstractVirtualDevice & /*sourceDevice*/) noexcept
	{
		m_viewMatrices.destroy();
	}

	bool
	SceneRenderTarget::writeCombinedImageSampler (const Vulkan::DescriptorSet & /*descriptorSet*/, uint32_t /*bindingIndex*/) const noexcept
	{
		return false;
	}

	std::shared_ptr< Vulkan::RenderPass >
	SceneRenderTarget::createRenderPass (Renderer & renderer) const noexcept
	{
		auto renderPass = std::make_shared< Vulkan::RenderPass >(renderer.device(), 0);
		renderPass->setIdentifier(ClassId, this->id(), "RenderPass");

		Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

		/* Attachment 0: Color buffer.
		 * The finalLayout is COLOR_ATTACHMENT_OPTIMAL because this is an internal render target,
		 * not a swapchain image. The post-processor will transition it to TRANSFER_SRC when needed. */
		renderPass->addAttachmentDescription(VkAttachmentDescription{
			.flags = 0,
			.format = m_colorFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		});

		subPass.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		uint32_t nextAttachment = 1;

		/* Attachment 1: Normals buffer (MRT). */
		if ( m_normalsFormat != VK_FORMAT_UNDEFINED )
		{
			renderPass->addAttachmentDescription(VkAttachmentDescription{
				.flags = 0,
				.format = m_normalsFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			});

			subPass.addColorAttachment(nextAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			++nextAttachment;
		}

		/* Attachment N: Material properties buffer (MRT). */
		if ( m_materialPropertiesFormat != VK_FORMAT_UNDEFINED )
		{
			renderPass->addAttachmentDescription(VkAttachmentDescription{
				.flags = 0,
				.format = m_materialPropertiesFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			});

			subPass.addColorAttachment(nextAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			++nextAttachment;
		}

		/* Attachment N: Depth buffer. */
		if ( m_depthFormat != VK_FORMAT_UNDEFINED )
		{
			renderPass->addAttachmentDescription(VkAttachmentDescription{
				.flags = 0,
				.format = m_depthFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			});

			subPass.setDepthStencilAttachment(nextAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}

		renderPass->addSubPass(subPass);

		if ( !renderPass->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the render pass for '" << this->id() << "' !";
			return nullptr;
		}

		return renderPass;
	}

	std::shared_ptr< Vulkan::RenderPass >
	SceneRenderTarget::createPostProcessRenderPass (Renderer & renderer) const noexcept
	{
		auto renderPass = std::make_shared< Vulkan::RenderPass >(renderer.device(), 0);
		renderPass->setIdentifier(ClassId, this->id(), "PostProcessRenderPass");

		Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

		/* Attachment 0: Color buffer with LOAD_OP_LOAD to preserve scene content.
		 * Used for TranslucentGB rendering after the grab pass blit. */
		renderPass->addAttachmentDescription(VkAttachmentDescription{
			.flags = 0,
			.format = m_colorFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		});

		subPass.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		uint32_t nextAttachment = 1;

		/* Attachment 1: Normals buffer with LOAD_OP_LOAD to preserve normals. */
		if ( m_normalsFormat != VK_FORMAT_UNDEFINED )
		{
			renderPass->addAttachmentDescription(VkAttachmentDescription{
				.flags = 0,
				.format = m_normalsFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			});

			subPass.addColorAttachment(nextAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			++nextAttachment;
		}

		/* Attachment N: Material properties buffer with LOAD_OP_LOAD to preserve properties. */
		if ( m_materialPropertiesFormat != VK_FORMAT_UNDEFINED )
		{
			renderPass->addAttachmentDescription(VkAttachmentDescription{
				.flags = 0,
				.format = m_materialPropertiesFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			});

			subPass.addColorAttachment(nextAttachment, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			++nextAttachment;
		}

		/* Attachment N: Depth buffer with LOAD_OP_LOAD for depth testing. */
		if ( m_depthFormat != VK_FORMAT_UNDEFINED )
		{
			renderPass->addAttachmentDescription(VkAttachmentDescription{
				.flags = 0,
				.format = m_depthFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			});

			subPass.setDepthStencilAttachment(nextAttachment, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
		}

		renderPass->addSubPass(subPass);

		/* Subpass dependencies for synchronization with external
		 * transfer operations (grab pass blit before, post-processor blit after). */
		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		});

		renderPass->addSubPassDependency(VkSubpassDependency{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		});

		if ( !renderPass->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the post-process render pass for '" << this->id() << "' !";
			return nullptr;
		}

		return renderPass;
	}

	bool
	SceneRenderTarget::createPostProcessFramebuffer (const std::shared_ptr< Vulkan::RenderPass > & renderPass) noexcept
	{
		m_postProcessFramebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, this->extent());
		m_postProcessFramebuffer->setIdentifier(ClassId, this->id(), "PostProcessFramebuffer");

		m_postProcessFramebuffer->addAttachment(m_colorImageView->handle());

		if ( m_normalsImageView != nullptr )
		{
			m_postProcessFramebuffer->addAttachment(m_normalsImageView->handle());
		}

		if ( m_materialPropertiesImageView != nullptr )
		{
			m_postProcessFramebuffer->addAttachment(m_materialPropertiesImageView->handle());
		}

		if ( m_depthImageView != nullptr )
		{
			m_postProcessFramebuffer->addAttachment(m_depthImageView->handle());
		}

		if ( !m_postProcessFramebuffer->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the post-process framebuffer for '" << this->id() << "' !";
			return false;
		}

		return true;
	}

	bool
	SceneRenderTarget::onCreate (Renderer & renderer) noexcept
	{
		if ( !this->createImages(renderer) )
		{
			return false;
		}

		const auto renderPass = this->createRenderPass(renderer);

		if ( renderPass == nullptr )
		{
			return false;
		}

		if ( !this->createFramebuffer(renderPass) )
		{
			return false;
		}

		const auto postProcessRP = this->createPostProcessRenderPass(renderer);

		if ( postProcessRP == nullptr )
		{
			return false;
		}

		if ( !this->createPostProcessFramebuffer(postProcessRP) )
		{
			return false;
		}

		m_isReadyForRendering = true;

		return true;
	}

	void
	SceneRenderTarget::onDestroy () noexcept
	{
		m_isReadyForRendering = false;

		m_postProcessFramebuffer.reset();
		m_framebuffer.reset();
		m_depthImageView.reset();
		m_depthStencilImage.reset();
		m_materialPropertiesImageView.reset();
		m_materialPropertiesImage.reset();
		m_normalsImageView.reset();
		m_normalsImage.reset();
		m_colorImageView.reset();
		m_colorImage.reset();
	}

	bool
	SceneRenderTarget::createImages (const Renderer & renderer) noexcept
	{
		const auto device = renderer.device();

		/* Color image: sampleable + color attachment + transfer source (for blit to grab pass). */
		m_colorImage = std::make_shared< Vulkan::Image >(
			device,
			VK_IMAGE_TYPE_2D,
			m_colorFormat,
			this->extent(),
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		);
		m_colorImage->setIdentifier(ClassId, this->id(), "ColorImage");

		if ( !m_colorImage->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the color image for '" << this->id() << "' !";
			return false;
		}

		m_colorImageView = std::make_shared< Vulkan::ImageView >(
			m_colorImage,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		);
		m_colorImageView->setIdentifier(ClassId, this->id(), "ColorImageView");

		if ( !m_colorImageView->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the color image view for '" << this->id() << "' !";
			return false;
		}

		/* Normals image: color attachment + transfer source + sampleable (for SSR/SSAO). */
		if ( m_normalsFormat != VK_FORMAT_UNDEFINED )
		{
			m_normalsImage = std::make_shared< Vulkan::Image >(
				device,
				VK_IMAGE_TYPE_2D,
				m_normalsFormat,
				this->extent(),
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			);
			m_normalsImage->setIdentifier(ClassId, this->id(), "NormalsImage");

			if ( !m_normalsImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the normals image for '" << this->id() << "' !";
				return false;
			}

			m_normalsImageView = std::make_shared< Vulkan::ImageView >(
				m_normalsImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			);
			m_normalsImageView->setIdentifier(ClassId, this->id(), "NormalsImageView");

			if ( !m_normalsImageView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the normals image view for '" << this->id() << "' !";
				return false;
			}
		}

		/* Material properties image: color attachment + transfer source + sampleable (for post-process effects). */
		if ( m_materialPropertiesFormat != VK_FORMAT_UNDEFINED )
		{
			m_materialPropertiesImage = std::make_shared< Vulkan::Image >(
				device,
				VK_IMAGE_TYPE_2D,
				m_materialPropertiesFormat,
				this->extent(),
				VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			);
			m_materialPropertiesImage->setIdentifier(ClassId, this->id(), "MaterialPropertiesImage");

			if ( !m_materialPropertiesImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the material properties image for '" << this->id() << "' !";
				return false;
			}

			m_materialPropertiesImageView = std::make_shared< Vulkan::ImageView >(
				m_materialPropertiesImage,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
					.baseMipLevel = 0,
					.levelCount = 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			);
			m_materialPropertiesImageView->setIdentifier(ClassId, this->id(), "MaterialPropertiesImageView");

			if ( !m_materialPropertiesImageView->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the material properties image view for '" << this->id() << "' !";
				return false;
			}
		}

		/* Depth image: depth attachment + transfer source (for depth blit to grab pass). */
		if ( m_depthFormat != VK_FORMAT_UNDEFINED )
		{
			m_depthStencilImage = std::make_shared< Vulkan::Image >(
				device,
				VK_IMAGE_TYPE_2D,
				m_depthFormat,
				this->extent(),
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
			);
			m_depthStencilImage->setIdentifier(ClassId, this->id(), "DepthImage");

			if ( !m_depthStencilImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create the depth image for '" << this->id() << "' !";
				return false;
			}

			m_depthImageView = std::make_shared< Vulkan::ImageView >(
				m_depthStencilImage,
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
		}

		return true;
	}

	bool
	SceneRenderTarget::createFramebuffer (const std::shared_ptr< Vulkan::RenderPass > & renderPass) noexcept
	{
		m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, this->extent());
		m_framebuffer->setIdentifier(ClassId, this->id(), "Framebuffer");

		m_framebuffer->addAttachment(m_colorImageView->handle());

		if ( m_normalsImageView != nullptr )
		{
			m_framebuffer->addAttachment(m_normalsImageView->handle());
		}

		if ( m_materialPropertiesImageView != nullptr )
		{
			m_framebuffer->addAttachment(m_materialPropertiesImageView->handle());
		}

		if ( m_depthImageView != nullptr )
		{
			m_framebuffer->addAttachment(m_depthImageView->handle());
		}

		if ( !m_framebuffer->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create the framebuffer for '" << this->id() << "' !";
			return false;
		}

		return true;
	}
}
