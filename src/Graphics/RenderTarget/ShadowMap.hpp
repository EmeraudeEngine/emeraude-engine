/*
 * src/Graphics/RenderTarget/ShadowMap.hpp
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

#pragma once

/* Local inclusions for inheritances. */
#include "Abstract.hpp"

/* Local inclusions for usages. */
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/Image.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Instance.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Graphics/ViewMatrices3DUBO.hpp"

namespace EmEn::Graphics::RenderTarget
{
	constexpr auto Bias{0.5F};

	constexpr Libs::Math::Matrix< 4, float > ScaleBiasMatrix{{
		Bias, 0.0F, 0.0F, 0.0F,
		0.0F, Bias, 0.0F, 0.0F,
		0.0F, 0.0F, Bias, 0.0F,
		Bias, Bias, Bias, 1.0F
	}};

	/**
	 * @brief The shadow map template to handle 2D and cubemap render target.
	 * @tparam view_matrices_t The type of matrix interface.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	template< typename view_matrices_t >
	requires (std::is_base_of_v< ViewMatricesInterface, view_matrices_t >)
	class ShadowMap final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ShadowMap"};

			/**
			 * @brief Constructs a shadow map.
			 * @param name A reference to a string.
			 * @param resolution The shadow map resolution.
			 */
			ShadowMap (const std::string & name, uint32_t resolution) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices2DUBO >)
				: Abstract(name, {0, 0, 0, 0, 32, 0, 1}, {resolution, resolution, 1}, RenderTargetType::ShadowMap, AVConsole::ConnexionType::Input, true)
			{

			}

			/**
			 * @brief Constructs a shadow cubemap.
			 * @param name A reference to a string.
			 * @param resolution The shadow map resolution.
			 */
			ShadowMap (const std::string & name, uint32_t resolution) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices3DUBO >)
			: Abstract(name, {0, 0, 0, 0, 32, 0, 1}, {resolution, resolution, 1}, RenderTargetType::ShadowCubemap, AVConsole::ConnexionType::Input, true)
			{

			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::aspectRatio() */
			[[nodiscard]]
			float
			aspectRatio () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO >  )
				{
					return 1.0F;
				}
				else
				{
					if ( this->extent().height == 0 )
					{
						return 0.0F;
					}

					return static_cast< float >(this->extent().width) / static_cast< float >(this->extent().height);
				}
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCubemap() */
			[[nodiscard]]
			bool
			isCubemap () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO >  )
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() const */
			[[nodiscard]]
			const ViewMatricesInterface &
			viewMatrices () const noexcept override
			{
				return m_viewMatrices;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() */
			[[nodiscard]]
			ViewMatricesInterface &
			viewMatrices () noexcept override
			{
				return m_viewMatrices;
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			AVConsole::VideoType
			videoType () const noexcept override
			{
				return AVConsole::VideoType::ShadowMap;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() */
			[[nodiscard]]
			const Vulkan::Framebuffer *
			framebuffer () const noexcept override
			{
				return m_framebuffer.get();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::image() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			image () const noexcept override
			{
				return m_depthImage;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::imageView() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_depthImageView;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isValid() */
			[[nodiscard]]
			bool
			isValid () const noexcept override
			{
				if ( m_framebuffer == nullptr || !m_framebuffer->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Shadow) framebuffer is missing or not created !";

					return false;
				}

				if ( m_depthImage == nullptr || !m_depthImage->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Shadow) depth image is missing or not created !";

					return false;
				}

				if ( m_depthImageView == nullptr || !m_depthImageView->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (Shadow) depth image view is missing or not created !";

					return false;
				}

				return true;
			}

			/**
			 * @brief Returns the shadow map resolution from settings.
			 * @return uint32_t
			 */
			[[nodiscard]]
			static
			uint32_t
			getBaseResolution () noexcept
			{
				return Settings::instance()->get< uint32_t >(GraphicsShadowMappingBaseResolutionKey, DefaultGraphicsShadowMappingBaseResolution);
			}

		private:

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void
			updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override
			{
				m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateProperties() */
			void
			updateProperties (bool isPerspectiveProjection, float distance, float fovOrNear) noexcept override
			{
				const auto & extent = this->extent();

				if ( isPerspectiveProjection )
				{
					m_viewMatrices.updatePerspectiveViewProperties(static_cast< float >(extent.width), static_cast< float >(extent.height), distance, fovOrNear);
				}
				else
				{
					m_viewMatrices.updateOrthographicViewProperties(static_cast< float >(extent.width), static_cast< float >(extent.height), distance, fovOrNear);
				}
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onSourceConnected() */
			void
			onSourceConnected (AbstractVirtualDevice * sourceDevice) noexcept override
			{
				m_viewMatrices.create(*Renderer::instance(), this->id());
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onSourceDisconnected() */
			void
			onSourceDisconnected (AbstractVirtualDevice * sourceDevice) noexcept override
			{
				m_viewMatrices.destroy();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool
			onCreate (Renderer & renderer) noexcept override
			{
				if ( !this->createImages(renderer.device()) )
				{
					TraceError{ClassId} << "Unable to create image buffers for shadow map '" << this->id() << "' !";

					return false;
				}

				const auto renderPass = this->createRenderPass(renderer);

				if ( renderPass == nullptr )
				{
					return false;
				}

				return this->createFramebuffer(renderPass);
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void
			onDestroy () noexcept override
			{
				/* Depth buffer. */
				m_depthImageView.reset();
				m_depthImage.reset();

				/* Framebuffer. */
				m_framebuffer.reset();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass >
			createRenderPass (Renderer & renderer) const noexcept override
			{
				/* FIXME: The identifier must reflect the enabled attachments !!! */
				auto renderPass = renderer.getRenderPass(ShadowRender, 0);

				if ( !renderPass->isCreated() )
				{
					/* Prepare a sub-pass for the render pass. */
					Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

					/* Color buffer. */
					if ( this->precisions().colorBits() > 0 )
					{
						TraceWarning{ClassId} << "Color bits requested for shadow map '" << this->id() << "' !";
					}

					/* Depth/Stencil buffer. */
					if ( this->precisions().depthBits() > 0 || this->precisions().stencilBits() > 0 )
					{
						renderPass->addAttachmentDescription(VkAttachmentDescription{
							.flags = 0,
							.format = m_depthImage->createInfo().format,
							.samples = VK_SAMPLE_COUNT_1_BIT,
							.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
							.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
							.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
							.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
							.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
							.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
						});

						subPass.setDepthStencilAttachment(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
					}

					renderPass->addSubPass(subPass);

					renderPass->addSubPassDependency({
						.srcSubpass = VK_SUBPASS_EXTERNAL,
						.dstSubpass = 0,
						.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
						.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
						.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
					});

					renderPass->addSubPassDependency({
						.srcSubpass = 0,
						.dstSubpass = VK_SUBPASS_EXTERNAL,
						.srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
						.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
						.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
						.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
						.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
					});

					if ( !renderPass->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create the render pass for shadow map '" << this->id() << "' !";

						return nullptr;
					}
				}

				return renderPass;
			}

			/**
			 * @brief Creates the images and the image views for each swap chain frame.
			 * @param device A reference to the graphics device smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			createImages (const std::shared_ptr< Vulkan::Device > & device) noexcept
			{
				/* Color buffer. */
				if ( this->precisions().colorBits() > 0 )
				{
					TraceWarning{ClassId} << "Color bits requested for shadow map '" << this->id() << "' !";
				}

				/* Depth/Stencil buffer. */
				if ( this->precisions().depthBits() > 0 || this->precisions().stencilBits() > 0 )
				{
					m_depthImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findDepthStencilFormat(device, this->precisions()),
						this->extent(),
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED
					);
					m_depthImage->setIdentifier(this->id() + "-Depth-Image");

					if ( !m_depthImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Depth/Stencil buffer) for shadow map '" << this->id() << "' !";

						return false;
					}

					/* Create a view to exploit the depth part of the image. */
					if ( this->precisions().depthBits() > 0 )
					{
						m_depthImageView = std::make_shared< Vulkan::ImageView >(
							m_depthImage,
							VK_IMAGE_VIEW_TYPE_2D,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
								.baseMipLevel = 0,
								.levelCount = m_depthImage->createInfo().mipLevels,
								.baseArrayLayer = 0,
								.layerCount = m_depthImage->createInfo().arrayLayers
							}
						);
						m_depthImage->setIdentifier(this->id() + "-Depth-ImageView");

						if ( !m_depthImageView->createOnHardware() )
						{
							TraceError{ClassId} << "Unable to create an image view (Depth buffer) for shadow map '" << this->id() << "' !";

							return false;
						}
					}

					/* Create a view to exploit the stencil part of the image. */
					if ( this->precisions().stencilBits() > 0 )
					{
						TraceWarning{ClassId} << "Stencil bits requested for shadow map '" << this->id() << "' !";
					}
				}

				return true;
			}

			/**
			 * @brief Creates the framebuffer.
			 * @param renderPass A reference to the render pass smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			createFramebuffer (const std::shared_ptr< Vulkan::RenderPass > & renderPass) noexcept
			{
				/* Prepare the framebuffer. */
				m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, this->extent());
				m_framebuffer->setIdentifier(this->id() + "-Main-Framebuffer");

				/* Color buffer. */
				if ( this->precisions().colorBits() > 0 )
				{
					TraceWarning{ClassId} << "Color bits requested for shadow map '" << this->id() << "' !";
				}

				/* Depth buffer. */
				if ( this->precisions().depthBits() > 0 )
				{
					m_framebuffer->addAttachment(m_depthImageView->handle());
				}

				/* Stencil buffer. */
				if ( this->precisions().stencilBits() > 0 )
				{
					TraceWarning{ClassId} << "Stencil bits requested for shadow map '" << this->id() << "' !";
				}

				if ( !m_framebuffer->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the framebuffer for shadow map '" << this->id() << "' !";

					return false;
				}

				return true;
			}

			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			std::shared_ptr< Vulkan::Image > m_depthImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			view_matrices_t m_viewMatrices;
	};
}
