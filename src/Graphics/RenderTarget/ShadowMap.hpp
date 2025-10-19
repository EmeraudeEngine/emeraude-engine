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
#include "Vulkan/Sampler.hpp"
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
			 * @param deviceName A reference to a string.
			 * @param resolution The shadow map resolution.
			 * @param isOrthographicProjection Set orthographic projection instead of perspective.
			 * @param debug Print the z-depth into the color buffer for debugging.
			 */
			ShadowMap (const std::string & deviceName, uint32_t resolution, bool isOrthographicProjection, bool debug) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices2DUBO >)
				: Abstract {
					deviceName,
					{debug ? 8U : 0U, debug ? 8U : 0U, debug ? 8U : 0U, debug ? 8U : 0U, 32U, 0U, 1U},
					{resolution, resolution, 1},
					RenderTargetType::ShadowMap,
					AVConsole::ConnexionType::Input,
					isOrthographicProjection,
					true
				}
			{

			}

			/**
			 * @brief Constructs a shadow cubemap.
			 * @param deviceName A reference to a string.
			 * @param resolution The shadow map resolution.
			 * @param isOrthographicProjection Set orthographic projection instead of perspective.
			 */
			ShadowMap (const std::string & deviceName, uint32_t resolution, bool isOrthographicProjection) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices3DUBO >)
				: Abstract{
					deviceName,
					{0U, 0U, 0U, 0U, 32U, 0U, 1U},
					{resolution, resolution, 1},
					RenderTargetType::ShadowCubemap,
					AVConsole::ConnexionType::Input,
					isOrthographicProjection,
					true
				}
			{

			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::updateViewRangesProperties() */
			void
			updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept override
			{
				const auto & extent = this->extent();
				const auto width = static_cast< float >(extent.width);
				const auto height = static_cast< float >(extent.width);

				if ( this->isOrthographicProjection() )
				{
					m_viewMatrices.updateOrthographicViewProperties(width, height, fovOrNear, distanceOrFar);
				}
				else
				{
					m_viewMatrices.updatePerspectiveViewProperties(width, height, fovOrNear, distanceOrFar);
				}
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

			/**
			 * @brief Returns the shadow map compare sampler.
			 * @return std::shared_ptr< Vulkan::Sampler >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			sampler () const noexcept
			{
				return m_sampler;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isReadyForRendering() const */
			[[nodiscard]]
			bool
			isReadyForRendering () const noexcept override
			{
				return m_isReadyForRendering;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isDebug() const */
			[[nodiscard]]
			bool
			isDebug () const noexcept override
			{
				return m_debugImage != nullptr;
			}

		private:

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void
			updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override
			{
				if ( this->isOrthographicProjection() != isOrthographicProjection )
				{
					TraceWarning{ClassId} << "The shadow map '" << this->id() << "' don't use the correct projection type !";

					return;
				}

				this->updateViewRangesProperties(fovOrNear, distanceOrFar);
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void
			updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override
			{
				m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onInputDeviceConnected() */
			void
			onInputDeviceConnected (AVConsole::AVManagers & AVManagers, AbstractVirtualDevice & /*sourceDevice*/) noexcept override
			{
				m_viewMatrices.create(AVManagers.graphicsRenderer, this->id());
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onInputDeviceDisconnected() */
			void
			onInputDeviceDisconnected (AVConsole::AVManagers & /*AVManagers*/, AbstractVirtualDevice & /*sourceDevice*/) noexcept override
			{
				m_viewMatrices.destroy();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool
			onCreate (Renderer & renderer) noexcept override
			{
				if ( !this->createImages(renderer) )
				{
					return false;
				}

				/* Create a sampler for the texture. */
				m_sampler = renderer.getSampler("ShadowMap", [] (Settings &, VkSamplerCreateInfo & createInfo) {
					//createInfo.flags = 0;
					createInfo.magFilter = VK_FILTER_LINEAR;
					createInfo.minFilter = VK_FILTER_LINEAR;
					createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
					//createInfo.mipLodBias = 0.0F;
					//createInfo.anisotropyEnable = VK_FALSE;
					//createInfo.maxAnisotropy = 1.0F;
					createInfo.compareEnable = VK_TRUE;
					createInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
					createInfo.minLod = 0.0F;
					createInfo.maxLod = 1.0F;
					createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
					//createInfo.unnormalizedCoordinates = VK_FALSE;
				});

				if ( m_sampler == nullptr )
				{
					TraceError{ClassId} << "Unable to create a sampler for the shadow map '" << this->id() << "' !";

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

				m_isReadyForRendering = true;

				return true;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void
			onDestroy () noexcept override
			{
				m_isReadyForRendering = false;

				m_framebuffer.reset();

				m_sampler.reset();

				m_depthImageView.reset();
				m_depthImage.reset();

				m_debugImageView.reset();
				m_debugImage.reset();
			}

			/**
			 * @brief Creates the images and the image views for each swap chain frame.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			createImages (const Renderer & renderer) noexcept
			{
				const auto device = renderer.device();

				/* The debug color buffer (optional). */
				if ( this->precisions().colorBits() > 0 )
				{
					TraceWarning{ClassId} << "Color bits requested for shadow map '" << this->id() << "', debugging enabled.";

					m_debugImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findColorFormat(device, this->precisions()),
						this->extent(),
						VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						this->isCubemap() ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
						1,
						this->isCubemap() ? 6 : 1
					);
					m_debugImage->setIdentifier(ClassId, this->id(), "Image");

					if ( !m_debugImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (DebugColor buffer) for shadow map '" << this->id() << "' !";

						return false;
					}

					m_debugImageView = std::make_shared< Vulkan::ImageView >(
						m_debugImage,
						this->isCubemap() ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
							.baseMipLevel = 0,
							.levelCount = m_debugImage->createInfo().mipLevels, /* Must be 1 */
							.baseArrayLayer = 0,
							.layerCount = m_debugImage->createInfo().arrayLayers /* Must be 1 or 6 (cubemap) */
						}
					);
					m_debugImageView->setIdentifier(ClassId, this->id(), "ImageView");

					if ( !m_debugImageView->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image view (DebugColor buffer) for shadow map '" << this->id() << "' !";

						return false;
					}
				}

				/* The depth buffer.
				 * NOTE: Stencil buffer is useless here! */
				if ( this->precisions().depthBits() > 0 )
				{
					m_depthImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findDepthStencilFormat(device, this->precisions()), /* Should be VK_FORMAT_D32_SFLOAT or VK_FORMAT_D16_UNORM */
						this->extent(),
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						this->isCubemap() ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0,
						1,
						this->isCubemap() ? 6 : 1
					);
					m_depthImage->setIdentifier(ClassId, this->id(), "Image");

					if ( !m_depthImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Depth buffer) for shadow map '" << this->id() << "' !";

						return false;
					}

					m_depthImageView = std::make_shared< Vulkan::ImageView >(
						m_depthImage,
						this->isCubemap() ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
							.baseMipLevel = 0,
							.levelCount = m_depthImage->createInfo().mipLevels, /* Must be 1 */
							.baseArrayLayer = 0,
							.layerCount = m_depthImage->createInfo().arrayLayers /* Must be 1 or 6 (cubemap) */
						}
					);
					m_depthImageView->setIdentifier(ClassId, this->id(), "ImageView");

					if ( !m_depthImageView->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image view (Depth buffer) for shadow map '" << this->id() << "' !";

						return false;
					}
				}
				else
				{
					TraceError{ClassId} << "No depth bits requested for shadow map '" << this->id() << "' !";

					return false;
				}

				if ( this->precisions().stencilBits() > 0 )
				{
					TraceWarning{ClassId} << "Stencil bits requested for shadow map '" << this->id() << "'. Ignoring ...";
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass >
			createRenderPass (Renderer & renderer) const noexcept override
			{
				/* FIXME: The identifier must reflect the enabled attachments !!! */
				auto renderPass = renderer.getRenderPass("ShadowRender", 0);

				if ( !renderPass->isCreated() )
				{
					/* Prepare a subpass for the render pass. */
					Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

					uint32_t attachmentIndex = 0;

					/* Color buffer. */
					if ( m_debugImage != nullptr )
					{
						renderPass->addAttachmentDescription(VkAttachmentDescription{
							.flags = 0,
							.format = m_debugImage->createInfo().format,
							.samples = VK_SAMPLE_COUNT_1_BIT,
							.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
							.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
							.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
							.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
							.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
							.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
						});

						subPass.addColorAttachment(attachmentIndex++, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
					}

					/* Depth/Stencil buffer. */
					if ( m_depthImage != nullptr )
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

						subPass.setDepthStencilAttachment(attachmentIndex, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
					}
					else
					{
						TraceError{ClassId} << "The depth image is not created for shadow map '" << this->id() << "' !";

						return nullptr;
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
				m_framebuffer->setIdentifier(ClassId, this->id(), "Framebuffer");

				/* Attach the debug color buffer, if present. */
				if ( m_debugImageView != nullptr )
				{
					m_framebuffer->addAttachment(m_debugImageView->handle());
				}
				else if constexpr ( IsDebug )
				{
					if ( this->precisions().colorBits() > 0 )
					{
						TraceError{ClassId} << "The color image view (Debug) is not created for shadow map '" << this->id() << "', but was requested !";

						return false;
					}
				}

				/* Attach the depth buffer. */
				if ( m_depthImageView != nullptr )
				{
					m_framebuffer->addAttachment(m_depthImageView->handle());
				}
				else
				{
					TraceError{ClassId} << "The depth image view is not created for shadow map '" << this->id() << "' !";

					return false;
				}

				if ( !m_framebuffer->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the framebuffer for shadow map '" << this->id() << "' !";

					return false;
				}

				return true;
			}

			std::shared_ptr< Vulkan::Image > m_debugImage;
			std::shared_ptr< Vulkan::ImageView > m_debugImageView;
			std::shared_ptr< Vulkan::Image > m_depthImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			view_matrices_t m_viewMatrices;
			bool m_isReadyForRendering{false};
	};
}
