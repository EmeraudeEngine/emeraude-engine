/*
 * src/Graphics/RenderTarget/View.hpp
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
#include "Graphics/RenderTarget/Abstract.hpp"

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
	/**
	 * @brief The render to view template.
	 * @tparam view_matrices_t The type of matrix interface.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	template< typename view_matrices_t >
	requires (std::is_base_of_v< ViewMatricesInterface, view_matrices_t >)
	class View final : public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"View"};

			/**
			 * @brief Constructs a render to view.
			 * @param name A reference to a string for the name of the video device.
			 * @param width The width of the view.
			 * @param height The height of the view.
			 * @param precisions A structure to describe framebuffer precisions.
			 */
			View (const std::string & name, uint32_t width, uint32_t height, const FramebufferPrecisions & precisions = {}) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices2DUBO >)
				: Abstract(name, precisions, {width, height, 1}, RenderTargetType::View, AVConsole::ConnexionType::Both, true)
			{

			}

			/**
			 * @brief Constructs a render to cubic view.
			 * @param name A reference to a string for the name of the video device.
			 * @param size The size of the cubemap.
			 * @param precisions A structure to describe framebuffer precisions.
			 */
			View (const std::string & name, uint32_t size, const FramebufferPrecisions & precisions = {}) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices3DUBO >)
				: Abstract(name, precisions, {size, size, 1}, RenderTargetType::CubicView, AVConsole::ConnexionType::Both, true)
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
				return AVConsole::VideoType::View;
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
				return m_colorImage;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::imageView() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				return m_colorImageView;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isValid() */
			[[nodiscard]]
			bool
			isValid () const noexcept override
			{
				if ( m_framebuffer == nullptr || !m_framebuffer->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (View) framebuffer is missing or not created !";

					return false;
				}

				if ( m_colorImage == nullptr || !m_colorImage->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (View) color image is missing or not created !";

					return false;
				}

				if ( m_colorImageView == nullptr || !m_colorImageView->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (View) color image view is missing or not created !";

					return false;
				}

				if ( m_depthStencilImage == nullptr || !m_depthStencilImage->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (View) depth/stencil image is missing or not created !";

					return false;
				}

				if ( m_depthImageView == nullptr || !m_depthImageView->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (View) depth image view is missing or not created !";

					return false;
				}

				if ( m_stencilImageView == nullptr || !m_stencilImageView->isCreated() )
				{
					TraceError{ClassId} << "The render target '" << this->id() << "' (View) stencil image view is missing or not created !";

					return false;
				}

				return true;
			}

			/**
			 * @brief Gives access to the main hardware depth stencil image of the render target.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			depthStencilImage () const noexcept
			{
				return m_depthStencilImage;
			}

			/**
			 * @brief Gives access to the main hardware depth image view object of the render target.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			depthImageView () const noexcept
			{
				return m_depthImageView;
			}

			/**
			 * @brief Gives access to the main hardware stencil image view object of the render target.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			stencilImageView () const noexcept
			{
				return m_stencilImageView;
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
			onSourceConnected (AbstractVirtualDevice * /*sourceDevice*/) noexcept override
			{
				m_viewMatrices.create(*Renderer::instance(), this->id());
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onSourceDisconnected() */
			void
			onSourceDisconnected (AbstractVirtualDevice * /*sourceDevice*/) noexcept override
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
					TraceError{ClassId} << "Unable to create image buffers for view '" << this->id() << "' !";

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
				/* Depth stencil buffer. */
				m_stencilImageView.reset();
				m_depthImageView.reset();
				m_depthStencilImage.reset();

				/* Color buffer */
				m_colorImageView.reset();
				m_colorImage.reset();

				/* Framebuffer */
				m_framebuffer.reset();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass >
			createRenderPass (Renderer & renderer) const noexcept override
			{
				/* FIXME: The identifier must reflect the enabled attachments !!! */
				auto renderPass = renderer.getRenderPass(ViewRender, 0);

				if ( !renderPass->isCreated() )
				{
					/* Prepare a sub-pass for the render pass. */
					Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

					/* Color buffer. */
					if ( this->precisions().colorBits() > 0 )
					{
						renderPass->addAttachmentDescription(VkAttachmentDescription{
							.flags = 0,
							.format = m_colorImage->createInfo().format,
							.samples = VK_SAMPLE_COUNT_1_BIT,
							.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
							.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
							.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
							.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
							.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
							.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
						});

						subPass.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					}

					/* Depth/Stencil buffer. */
					if ( this->precisions().depthBits() > 0 || this->precisions().stencilBits() > 0 )
					{
						renderPass->addAttachmentDescription(VkAttachmentDescription{
							.flags = 0,
							.format = m_depthStencilImage->createInfo().format,
							.samples = VK_SAMPLE_COUNT_1_BIT,
							.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
							.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
							.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
							.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
							.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
							.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
						});

						subPass.setDepthStencilAttachment(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
					}

					renderPass->addSubPass(subPass);

					//VkSubpassDependency subpassDependency{};
					//subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
					//subpassDependency.dstSubpass = 0;
					//subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
					//subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
					//subpassDependency.srcAccessMask = 0;
					//subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					//subpassDependency.dependencyFlags = 0;

					//renderPass->addSubpassDependency(subpassDependency);

					if ( !renderPass->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create the render pass for view '" << this->id() << "' !";

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
					/* Create the image for color buffer in video memory. */
					m_colorImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findColorFormat(device, this->precisions()),
						this->extent(),
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED,
						0,
						1,
						1,
						device->findSampleCount(this->precisions().samples())
					);
					m_colorImage->setIdentifier(this->id() + "-Color-Image");

					if ( !m_colorImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Color buffer) for view '" << this->id() << "' !";

						return false;
					}

					/* Create a view to exploit the image. */
					m_colorImageView = std::make_shared< Vulkan::ImageView >(
						m_colorImage,
						VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = 0,
							.levelCount = m_colorImage->createInfo().mipLevels,
							.baseArrayLayer = 0,
							.layerCount = m_colorImage->createInfo().arrayLayers
						}
					);
					m_colorImageView->setIdentifier(this->id() + "-Color-ImageView");

					if ( !m_colorImageView->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image view (Color buffer) for view '" << this->id() << "' !";

						return false;
					}
				}

				/* Depth/Stencil buffer. */
				if ( this->precisions().depthBits() > 0 || this->precisions().stencilBits() > 0 )
				{
					/* Create the image for depth/stencil buffer in video memory. */
					m_depthStencilImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findDepthStencilFormat(device, this->precisions()),
						this->extent(),
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
						VK_IMAGE_LAYOUT_UNDEFINED
					);
					m_depthStencilImage->setIdentifier(this->id() + "-DepthStencil-Image");

					if ( !m_depthStencilImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Depth/stencil buffer) for view '" << this->id() << "' !";

						return false;
					}

					/* Create a view to exploit the depth part of the image. */
					if ( this->precisions().depthBits() > 0 )
					{
						m_depthImageView = std::make_shared< Vulkan::ImageView >(
							m_depthStencilImage,
							VK_IMAGE_VIEW_TYPE_2D,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
								.baseMipLevel = 0,
								.levelCount = m_depthStencilImage->createInfo().mipLevels,
								.baseArrayLayer = 0,
								.layerCount = m_depthStencilImage->createInfo().arrayLayers
							}
						);
						m_depthImageView->setIdentifier(this->id() + "-Depth-ImageView");

						if ( !m_depthImageView->createOnHardware() )
						{
							TraceError{ClassId} << "Unable to create an image view (Depth buffer) for view '" << this->id() << "' !";

							return false;
						}
					}

					/* Create a view to exploit the stencil part of the image. */
					if ( this->precisions().stencilBits() > 0 )
					{
						m_stencilImageView = std::make_shared< Vulkan::ImageView >(
							m_depthStencilImage,
							VK_IMAGE_VIEW_TYPE_2D,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
								.baseMipLevel = 0,
								.levelCount = m_depthStencilImage->createInfo().mipLevels,
								.baseArrayLayer = 0,
								.layerCount = m_depthStencilImage->createInfo().arrayLayers
							}
						);
						m_stencilImageView->setIdentifier(this->id() + "-Stencil-ImageView");

						if ( !m_stencilImageView->createOnHardware() )
						{
							TraceError{ClassId} << "Unable to create an image view (Stencil buffer) for view '" << this->id() << "' !";

							return false;
						}
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
				m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, this->extent());
				m_framebuffer->setIdentifier(this->id() + "-Main-Framebuffer");

				/* Color buffer. */
				if ( this->precisions().colorBits() > 0 )
				{
					m_framebuffer->addAttachment(m_colorImageView->handle());
				}

				/* Depth buffer. */
				if ( this->precisions().depthBits() > 0 )
				{
					m_framebuffer->addAttachment(m_depthImageView->handle());
				}

				/* Stencil buffer. */
				if ( this->precisions().stencilBits() > 0 )
				{
					m_framebuffer->addAttachment(m_stencilImageView->handle());
				}

				if ( !m_framebuffer->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the framebuffer for view '" << this->id() << "' !";

					return false;
				}

				return true;
			}

			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			std::shared_ptr< Vulkan::Image > m_colorImage;
			std::shared_ptr< Vulkan::ImageView > m_colorImageView;
			std::shared_ptr< Vulkan::Image > m_depthStencilImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::ImageView > m_stencilImageView;
			view_matrices_t m_viewMatrices;
	};
}
