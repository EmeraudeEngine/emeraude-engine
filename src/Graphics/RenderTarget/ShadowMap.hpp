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
#include "Vulkan/TextureInterface.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Processor.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/Framebuffer.hpp"
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
	 * @extends EmEn::Vulkan::TextureInterface This is a texture.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	template< typename view_matrices_t >
	requires (std::is_base_of_v< ViewMatricesInterface, view_matrices_t >)
	class ShadowMap final : public Vulkan::TextureInterface, public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ShadowMap"};

			/**
			 * @brief Constructs a shadow map.
			 * @param deviceName A reference to a string.
			 * @param resolution The shadow map resolution.
			 * @param viewDistance The max viewable distance in meters.
			 * @param isOrthographicProjection Set orthographic projection instead of perspective.
			 */
			ShadowMap (const std::string & deviceName, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices2DUBO >)
				: Abstract {
					deviceName,
					{0U, 0U, 0U, 0U, 32U, 0U, 1U},
					{resolution, resolution, 1},
					viewDistance,
					RenderTargetType::ShadowMap,
					Scenes::AVConsole::ConnexionType::Input,
					isOrthographicProjection,
					true
				}
			{

			}

			/**
			 * @brief Constructs a shadow cubemap.
			 * @param deviceName A reference to a string.
			 * @param resolution The shadow map resolution.
			 * @param viewDistance The max viewable distance in meters.
			 * @param isOrthographicProjection Set orthographic projection instead of perspective.
			 */
			ShadowMap (const std::string & deviceName, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept requires (std::is_same_v< view_matrices_t, ViewMatrices3DUBO >)
				: Abstract{
					deviceName,
					{0U, 0U, 0U, 0U, 32U, 0U, 1U},
					{resolution, resolution, 1},
					viewDistance,
					RenderTargetType::ShadowCubemap,
					Scenes::AVConsole::ConnexionType::Input,
					isOrthographicProjection,
					true
				}
			{

			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCreated() const noexcept */
			[[nodiscard]]
			bool
			isCreated () const noexcept override
			{
				/* NOTE: Extra checks. */
				if constexpr ( IsDebug )
				{
					if ( m_depthImage == nullptr || !m_depthImage->isCreated() )
					{
						return false;
					}

					if ( m_depthImageView == nullptr || !m_depthImageView->isCreated() )
					{
						return false;
					}

					if ( this->isCubemap() )
					{
						if ( m_depthCubeImageView == nullptr || !m_depthCubeImageView->isCreated() )
						{
							return false;
						}
					}
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

			/** @copydoc EmEn::Vulkan::TextureInterface::type() const noexcept */
			[[nodiscard]]
			Vulkan::TextureType
			type () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO > )
				{
					return Vulkan::TextureType::TextureCube;
				}
				else
				{
					return Vulkan::TextureType::Texture2D;
				}
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::dimensions() const noexcept */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO > )
				{
					return 3;
				}
				else
				{
					return 2;
				}
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCubemapTexture() const noexcept */
			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				if constexpr ( std::is_same_v< view_matrices_t, ViewMatrices3DUBO > )
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::image() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			image () const noexcept override
			{
				return m_depthImage;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::imageView() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			imageView () const noexcept override
			{
				/* NOTE: As a texture request, we give the right image view with cubemap. */
				if ( this->isCubemap() && m_depthCubeImageView != nullptr )
				{
					return m_depthCubeImageView;
				}

				return m_depthImageView;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::sampler() const noexcept */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Sampler >
			sampler () const noexcept override
			{
				return m_sampler;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::request3DTextureCoordinates() const noexcept */
			[[nodiscard]]
			bool
			request3DTextureCoordinates () const noexcept override
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

				this->setViewDistance(distanceOrFar);
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

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			Scenes::AVConsole::VideoType
			videoType () const noexcept override
			{
				return Scenes::AVConsole::VideoType::ShadowMap;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() const */
			[[nodiscard]]
			const Vulkan::Framebuffer *
			framebuffer () const noexcept override
			{
				return m_framebuffer.get();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isReadyForRendering() const */
			[[nodiscard]]
			bool
			isReadyForRendering () const noexcept override
			{
				return m_isReadyForRendering;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::capture() */
			[[nodiscard]]
			std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 >
			capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool /*keepAlpha*/, bool /*withDepthBuffer*/, bool /*withStencilBuffer*/) const noexcept override
			{
				std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > result{};

				/* Validate layer index for cubemaps and single-layer shadow maps. */
				const uint32_t maxLayers = this->isCubemap() ? 6 : 1;

				if ( layerIndex >= maxLayers )
				{
					if ( maxLayers == 1 && layerIndex > 0 )
					{
						TraceWarning{ClassId} << "Single-layer shadow map does not support layer " << layerIndex << ". Using layer 0 instead for shadow map '" << this->id() << "'.";

						layerIndex = 0;
					}
					else
					{
						TraceError{ClassId} << "Invalid layer index " << layerIndex << " (max: " << maxLayers - 1 << ") for shadow map '" << this->id() << "' !";

						return result;
					}
				}

				/* NOTE: ShadowMaps don't have color buffers - result[0] remains empty. */

				/* NOTE: Shadow maps are depth-only. We always capture depth regardless of withDepthBuffer flag.
				 * The depth data goes into result[1] to maintain consistency with the capture() interface. */
				if ( m_depthImage != nullptr && m_depthImage->isCreated() )
				{
					if ( !transferManager.downloadImage(*m_depthImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, result[1]) )
					{
						TraceWarning{ClassId} << "Failed to capture depth buffer for shadow map '" << this->id() << "' !";
					}
				}

				/* NOTE: Stencil is never used for shadow mapping, ignore withStencilBuffer flag. */

				return result;
			}

		private:

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
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

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_worldCoordinates;
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void
			updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override
			{
				m_worldCoordinates = worldCoordinates;
				m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onInputDeviceConnected() */
			void
			onInputDeviceConnected (EngineContext & engineContext, AbstractVirtualDevice & /*sourceDevice*/) noexcept override
			{
				m_viewMatrices.create(engineContext.graphicsRenderer, this->id());
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onInputDeviceDisconnected() */
			void
			onInputDeviceDisconnected (EngineContext & /*engineContext*/, AbstractVirtualDevice & /*sourceDevice*/) noexcept override
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

				/* The main framebuffer. */
				m_framebuffer.reset();

				/* The texture sampler. */
				m_sampler.reset();

				/* The depth/stencil buffers. */
				m_depthCubeImageView.reset();
				m_depthImageView.reset();
				m_depthImage.reset();
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
					TraceWarning{ClassId} << "Color bits requested for shadow map '" << this->id() << "', ignoring ...";
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
						/* NOTE: Here we use VK_IMAGE_VIEW_TYPE_2D_ARRAY instead
						 * of VK_IMAGE_VIEW_TYPE_CUBE when rendering to a cubemap for the multiview feature. */
						this->isCubemap() ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D,
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

					/* NOTE: Create a specific view for reading
					 * the cubemap in shaders according to the multiview feature. */
					if ( this->isCubemap() )
					{
						m_depthCubeImageView = std::make_shared< Vulkan::ImageView >(
							m_depthImage,
							VK_IMAGE_VIEW_TYPE_CUBE,
							VkImageSubresourceRange{
								.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
								.baseMipLevel = 0,
								.levelCount = m_depthImage->createInfo().mipLevels, /* Must be 1 */
								.baseArrayLayer = 0,
								.layerCount = 6
							}
						);
						m_depthCubeImageView->setIdentifier(ClassId, this->id(), "CubeImageView");

						if ( !m_depthCubeImageView->createOnHardware() )
						{
							TraceError{ClassId} << "Unable to create a cube image view (Depth buffer) for shadow map '" << this->id() << "' !";

							return false;
						}
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
				/* Create a new RenderPass for this shadow map render target. */
				auto renderPass = std::make_shared< Vulkan::RenderPass >(renderer.device(), 0);
				renderPass->setIdentifier(ClassId, this->id(), "RenderPass");

				/* Prepare a subpass for the render pass. */
				Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

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

					subPass.setDepthStencilAttachment(0, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
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

				/* Enable multiview for cubemap rendering (Vulkan 1.1+) */
				if ( this->isCubemap() )
				{
					renderPass->enableMultiview();
				}

				if ( !renderPass->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the render pass for shadow map '" << this->id() << "' !";

					return nullptr;
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

			std::shared_ptr< Vulkan::Image > m_depthImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::ImageView > m_depthCubeImageView;
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			view_matrices_t m_viewMatrices;
			Libs::Math::CartesianFrame< float > m_worldCoordinates{};
			bool m_isReadyForRendering{false};
	};
}
