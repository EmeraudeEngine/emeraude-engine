/*
 * src/Graphics/RenderTarget/ShadowMapCascaded.hpp
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

#pragma once

/* Local inclusions for inheritances. */
#include "Vulkan/TextureInterface.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "Libs/PixelFactory/Processor.hpp"
#include "Vulkan/Instance.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatricesCascadedUBO.hpp"

namespace EmEn::Graphics::RenderTarget
{
	/**
	 * @brief Cascaded shadow map render target for directional lights.
	 * @extends EmEn::Vulkan::TextureInterface This is a texture (2D array).
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 * @note Uses a 2D array texture where each layer corresponds to a cascade.
	 *       Supports up to 4 cascades (MaxCascadeCount).
	 */
	class ShadowMapCascaded final : public Vulkan::TextureInterface, public Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"ShadowMapCascaded"};

			/**
			 * @brief Constructs a cascaded shadow map.
			 * @param deviceName A reference to a string.
			 * @param resolution The shadow map resolution (same for all cascades).
			 * @param viewDistance The max viewable distance in meters.
			 * @param cascadeCount The number of cascades (1-4).
			 * @param lambda The split factor (0 = linear, 1 = logarithmic, 0.5 = balanced).
			 */
			ShadowMapCascaded (const std::string & deviceName, uint32_t resolution, float viewDistance, uint32_t cascadeCount = MaxCascadeCount, float lambda = DefaultCascadeLambda) noexcept
				: Abstract{
					deviceName,
					{0U, 0U, 0U, 0U, 32U, 0U, 1U},
					{resolution, resolution, 1},
					viewDistance,
					RenderTargetType::ShadowMap,
					Scenes::AVConsole::ConnexionType::Input,
					true, /* Orthographic projection for directional lights. */
					true
				},
				m_viewMatrices{cascadeCount, lambda},
				m_cascadeCount{std::clamp(cascadeCount, 1U, MaxCascadeCount)}
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

					if ( m_depthArrayImageView == nullptr || !m_depthArrayImageView->isCreated() )
					{
						return false;
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
				return Vulkan::TextureType::Texture2DArray;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::dimensions() const noexcept */
			[[nodiscard]]
			uint32_t
			dimensions () const noexcept override
			{
				return 2; /* 2D array is still 2D sampling. */
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCubemapTexture() const noexcept */
			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return false;
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
				return m_depthArrayImageView;
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
				return false; /* Uses 2D coordinates + layer index. */
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::updateViewRangesProperties() */
			void
			updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept override
			{
				const auto & extent = this->extent();
				const auto width = static_cast< float >(extent.width);
				const auto height = static_cast< float >(extent.height);

				/* CSM always uses orthographic projection. */
				m_viewMatrices.updateOrthographicViewProperties(width, height, fovOrNear, distanceOrFar);

				this->setViewDistance(distanceOrFar);
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::aspectRatio() */
			[[nodiscard]]
			float
			aspectRatio () const noexcept override
			{
				if ( this->extent().height == 0 )
				{
					return 0.0F;
				}

				return static_cast< float >(this->extent().width) / static_cast< float >(this->extent().height);
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCubemap() */
			[[nodiscard]]
			bool
			isCubemap () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCascadedShadowMap() */
			[[nodiscard]]
			bool
			isCascadedShadowMap () const noexcept override
			{
				return true;
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

			/**
			 * @brief Returns the cascaded view matrices.
			 * @return ViewMatricesCascadedUBO &
			 */
			[[nodiscard]]
			ViewMatricesCascadedUBO &
			cascadedViewMatrices () noexcept
			{
				return m_viewMatrices;
			}

			/**
			 * @brief Returns the cascaded view matrices (const).
			 * @return const ViewMatricesCascadedUBO &
			 */
			[[nodiscard]]
			const ViewMatricesCascadedUBO &
			cascadedViewMatrices () const noexcept
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

			/**
			 * @brief Returns the number of cascades.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			cascadeCount () const noexcept
			{
				return m_cascadeCount;
			}

			/**
			 * @brief Returns the image view for a specific cascade layer.
			 * @param cascadeIndex The cascade index (0 to cascadeCount-1).
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			cascadeImageView (size_t cascadeIndex) const noexcept
			{
				if ( cascadeIndex >= m_cascadeCount )
				{
					Tracer::error(ClassId, "Cascade index overflow !");

					cascadeIndex = 0;
				}

				return m_perCascadeImageViews[cascadeIndex];
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::capture() */
			[[nodiscard]]
			std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 >
			capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool /*keepAlpha*/, bool /*withDepthBuffer*/, bool /*withStencilBuffer*/) const noexcept override
			{
				std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > result{};

				/* Validate layer index. */
				if ( layerIndex >= m_cascadeCount )
				{
					TraceError{ClassId} << "Invalid layer index " << layerIndex << " (max: " << m_cascadeCount - 1 << ") for CSM '" << this->id() << "' !";

					return result;
				}

				/* NOTE: Shadow maps are depth-only. */
				if ( m_depthImage != nullptr && m_depthImage->isCreated() )
				{
					if ( !transferManager.downloadImage(*m_depthImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, result[1]) )
					{
						TraceWarning{ClassId} << "Failed to capture depth buffer for CSM '" << this->id() << "' !";
					}
				}

				return result;
			}

		private:

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void
			updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override
			{
				if ( !isOrthographicProjection )
				{
					TraceWarning{ClassId} << "CSM '" << this->id() << "' requires orthographic projection !";

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

				/* Create a sampler for the texture array. */
				m_sampler = renderer.getSampler("ShadowMapCascaded", [&] (Settings &, VkSamplerCreateInfo & createInfo) {
					createInfo.magFilter = VK_FILTER_LINEAR;
					createInfo.minFilter = VK_FILTER_LINEAR;
					createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					/* NOTE: Use CLAMP_TO_BORDER so that sampling outside the shadow map
					 * returns borderColor (white = no shadow) instead of edge pixels. */
					createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
					createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
					createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
					createInfo.compareEnable = VK_TRUE;
					createInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
					createInfo.minLod = 0.0F;
					createInfo.maxLod = 1.0F;
					createInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
				});

				if ( m_sampler == nullptr )
				{
					TraceError{ClassId} << "Unable to create a sampler for CSM '" << this->id() << "' !";

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

				for ( auto & view : m_perCascadeImageViews )
				{
					view.reset();
				}

				m_depthArrayImageView.reset();
				m_depthImage.reset();
			}

			/**
			 * @brief Creates the images and the image views.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			createImages (const Renderer & renderer) noexcept
			{
				const auto device = renderer.device();

				/* Validate depth bits. */
				if ( this->precisions().depthBits() == 0 )
				{
					TraceError{ClassId} << "No depth bits requested for CSM '" << this->id() << "' !";

					return false;
				}

				/* Create the depth image as a 2D array. */
				m_depthImage = std::make_shared< Vulkan::Image >(
					device,
					VK_IMAGE_TYPE_2D,
					Vulkan::Instance::findDepthStencilFormat(device, this->precisions()),
					this->extent(),
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
					0, /* No special flags needed for 2D array. */
					1,
					m_cascadeCount /* Array layers = cascade count. */
				);
				m_depthImage->setIdentifier(ClassId, this->id(), "Image");

				if ( !m_depthImage->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the depth image for CSM '" << this->id() << "' !";

					return false;
				}

				/* Set the expected final image layout. */
				m_depthImage->setCurrentImageLayout(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

				/* Create the array image view for shader sampling. */
				m_depthArrayImageView = std::make_shared< Vulkan::ImageView >(
					m_depthImage,
					VK_IMAGE_VIEW_TYPE_2D_ARRAY,
					VkImageSubresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = m_cascadeCount
					}
				);
				m_depthArrayImageView->setIdentifier(ClassId, this->id(), "ArrayImageView");

				if ( !m_depthArrayImageView->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the array image view for CSM '" << this->id() << "' !";

					return false;
				}

				/* Create per-cascade image views for rendering to individual layers. */
				for ( uint32_t i = 0; i < m_cascadeCount; ++i )
				{
					m_perCascadeImageViews[i] = std::make_shared< Vulkan::ImageView >(
						m_depthImage,
						VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
							.baseMipLevel = 0,
							.levelCount = 1,
							.baseArrayLayer = i,
							.layerCount = 1
						}
					);
					m_perCascadeImageViews[i]->setIdentifier(ClassId, this->id(), ("CascadeImageView" + std::to_string(i)).c_str());

					if ( !m_perCascadeImageViews[i]->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create the image view for cascade " << i << " of CSM '" << this->id() << "' !";

						return false;
					}
				}

				return true;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass >
			createRenderPass (Renderer & renderer) const noexcept override
			{
				/* Create a new RenderPass for this CSM render target.
				 * NOTE: We use multiview to render all cascades in a single pass. */
				auto renderPass = std::make_shared< Vulkan::RenderPass >(renderer.device(), 0);
				renderPass->setIdentifier(ClassId, this->id(), "RenderPass");

				/* Prepare a subpass for the render pass. */
				Vulkan::RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

				/* Depth attachment. */
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
					TraceError{ClassId} << "The depth image is not created for CSM '" << this->id() << "' !";

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

				/* Enable multiview for rendering to all cascade layers. */
				renderPass->enableMultiview(m_cascadeCount);

				if ( !renderPass->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the render pass for CSM '" << this->id() << "' !";

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
				/* Prepare the framebuffer with multiview.
				 * NOTE: When using multiview, framebuffer layers = 1.
				 * The render pass multiview extension handles rendering to multiple array layers. */
				const VkExtent2D extent2D{this->extent().width, this->extent().height};
				m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, extent2D, 1);
				m_framebuffer->setIdentifier(ClassId, this->id(), "Framebuffer");

				/* Attach the array image view. */
				if ( m_depthArrayImageView != nullptr )
				{
					m_framebuffer->addAttachment(m_depthArrayImageView->handle());
				}
				else
				{
					TraceError{ClassId} << "The depth array image view is not created for CSM '" << this->id() << "' !";

					return false;
				}

				if ( !m_framebuffer->createOnHardware() )
				{
					TraceError{ClassId} << "Unable to create the framebuffer for CSM '" << this->id() << "' !";

					return false;
				}

				return true;
			}

			std::shared_ptr< Vulkan::Image > m_depthImage;
			std::shared_ptr< Vulkan::ImageView > m_depthArrayImageView;
			std::array< std::shared_ptr< Vulkan::ImageView >, MaxCascadeCount > m_perCascadeImageViews{};
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			ViewMatricesCascadedUBO m_viewMatrices;
			Libs::Math::CartesianFrame< float > m_worldCoordinates{};
			uint32_t m_cascadeCount{MaxCascadeCount};
			bool m_isReadyForRendering{false};
	};
}
