/*
 * src/Graphics/RenderTarget/ShadowMap.hpp
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
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Graphics/ViewMatrices3DUBO.hpp"
#include "Graphics/ViewMatricesCascadedUBO.hpp"

namespace EmEn::Graphics::RenderTarget
{
	constexpr auto Bias{0.5F};

	/* NOTE: This matrix transforms from clip space to texture coordinates.
	 * For x and y: [-1, 1] → [0, 1] using scale 0.5 and bias 0.5.
	 * For z: identity since Vulkan projection already outputs [0, 1].
	 *
	 * Column-major storage (OpenGL/Vulkan convention):
	 * The matrix performs: x' = 0.5*x + 0.5, y' = 0.5*y + 0.5, z' = z, w' = w
	 * Translation (0.5, 0.5, 0) goes in column 3. */
	constexpr Libs::Math::Matrix< 4, float > ScaleBiasMatrix{{
		Bias, 0.0F, 0.0F, 0.0F,  /* Column 0: scale X */
		0.0F, Bias, 0.0F, 0.0F,  /* Column 1: scale Y */
		0.0F, 0.0F, 1.0F, 0.0F,  /* Column 2: Z passthrough */
		Bias, Bias, 0.0F, 1.0F   /* Column 3: translation + w=1 */
	}};

	/* Type trait helpers for view matrix type detection. */
	template< typename T >
	static constexpr bool Is2DViewMatrix = std::is_same_v< T, ViewMatrices2DUBO >;

	template< typename T >
	static constexpr bool IsCubemapViewMatrix = std::is_same_v< T, ViewMatrices3DUBO >;

	template< typename T >
	static constexpr bool IsCascadedViewMatrix = std::is_same_v< T, ViewMatricesCascadedUBO >;

	/**
	 * @brief The shadow map template to handle 2D, cubemap and cascaded render targets.
	 * @tparam view_matrices_t The type of matrix interface (ViewMatrices2DUBO, ViewMatrices3DUBO, or ViewMatricesCascadedUBO).
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
			 * @brief Constructs a 2D shadow map.
			 * @param deviceName A reference to a string.
			 * @param resolution The shadow map resolution.
			 * @param viewDistance The max viewable distance in meters.
			 * @param isOrthographicProjection Set orthographic projection instead of perspective.
			 */
			ShadowMap (const std::string & deviceName, uint32_t resolution, float viewDistance, bool isOrthographicProjection) noexcept
				requires (Is2DViewMatrix< view_matrices_t >)
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
			 */
			ShadowMap (const std::string & deviceName, uint32_t resolution, float viewDistance) noexcept
				requires (IsCubemapViewMatrix< view_matrices_t >)
				: Abstract{
					deviceName,
					{0U, 0U, 0U, 0U, 32U, 0U, 1U},
					{resolution, resolution, 1},
					viewDistance,
					RenderTargetType::ShadowCubemap,
					Scenes::AVConsole::ConnexionType::Input,
					false,
					true
				}
			{

			}

			/**
			 * @brief Constructs a cascaded shadow map.
			 * @param deviceName A reference to a string.
			 * @param resolution The shadow map resolution (same for all cascades).
			 * @param viewDistance The max viewable distance in meters.
			 * @param cascadeCount The number of cascades (1-4).
			 * @param lambda The split factor (0 = linear, 1 = logarithmic, 0.5 = balanced).
			 */
			ShadowMap (const std::string & deviceName, uint32_t resolution, float viewDistance, uint32_t cascadeCount = MaxCascadeCount, float lambda = DefaultCascadeLambda) noexcept
				requires (IsCascadedViewMatrix< view_matrices_t >)
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

					if ( m_depthImageView == nullptr || !m_depthImageView->isCreated() )
					{
						return false;
					}

					if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
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

				return m_isReadyForRendering;
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::type() const noexcept */
			[[nodiscard]]
			Vulkan::TextureType
			type () const noexcept override
			{
				if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
				{
					return Vulkan::TextureType::TextureCube;
				}
				else if constexpr ( IsCascadedViewMatrix< view_matrices_t > )
				{
					return Vulkan::TextureType::Texture2DArray;
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
				if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
				{
					return 3;
				}
				else
				{
					return 2; /* 2D and 2D array are still 2D sampling. */
				}
			}

			/** @copydoc EmEn::Vulkan::TextureInterface::isCubemapTexture() const noexcept */
			[[nodiscard]]
			bool
			isCubemapTexture () const noexcept override
			{
				return IsCubemapViewMatrix< view_matrices_t >;
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
				/* NOTE: As a texture request, we give the right image view for sampling.
				 * - 2D: Use m_depthImageView (VK_IMAGE_VIEW_TYPE_2D)
				 * - Cubemap: Use m_depthCubeImageView (VK_IMAGE_VIEW_TYPE_CUBE)
				 * - Cascade: Use m_depthImageView (VK_IMAGE_VIEW_TYPE_2D_ARRAY) */
				if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
				{
					return m_depthCubeImageView;
				}
				else
				{
					return m_depthImageView;
				}
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
				return IsCubemapViewMatrix< view_matrices_t >;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::setViewDistance() */
			void
			setViewDistance (float meters) noexcept override
			{
				const auto & extent = this->extent();
				const auto width = static_cast< float >(extent.width);
				const auto height = static_cast< float >(extent.height);

				if ( this->isOrthographicProjection() )
				{
					m_viewMatrices.updateOrthographicViewProperties(width, height, m_viewMatrices.nearPlane(), meters);
				}
				else
				{
					m_viewMatrices.updatePerspectiveViewProperties(width, height, m_viewMatrices.fieldOfView(), meters);
				}
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewDistance() */
			[[nodiscard]]
			float
			viewDistance () const noexcept override
			{
				return m_viewMatrices.farPlane();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::updateViewRangesProperties() */
			void
			updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept override
			{
				const auto & extent = this->extent();
				const auto width = static_cast< float >(extent.width);
				const auto height = static_cast< float >(extent.height);

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
				if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
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
				return IsCubemapViewMatrix< view_matrices_t >;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCascadedShadowMap() */
			[[nodiscard]]
			bool
			isCascadedShadowMap () const noexcept override
			{
				return IsCascadedViewMatrix< view_matrices_t >;
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

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::writeCombinedImageSampler(const Vulkan::DescriptorSet &, uint32_t) const */
			[[nodiscard]]
			bool
			writeCombinedImageSampler (const Vulkan::DescriptorSet & descriptorSet, uint32_t bindingIndex) const noexcept override
			{
				if ( !this->isReadyForRendering() )
				{
					TraceError{ClassId} << "The shadow map is not ready for rendering!";

					return false;
				}

				return descriptorSet.writeCombinedImageSampler(bindingIndex, *this->image(), *this->imageView(), *this->sampler());
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::capture() */
			bool
			capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool /*keepAlpha*/, bool /*withDepthBuffer*/, bool /*withStencilBuffer*/, std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > & result) const noexcept override
			{
				/* Validate layer index. */
				uint32_t maxLayers = 1;

				if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
				{
					maxLayers = 6;
				}
				else if constexpr ( IsCascadedViewMatrix< view_matrices_t > )
				{
					maxLayers = m_cascadeCount;
				}

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

						return false;
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

				return true;
			}

			/* Cascade-specific methods (only available for cascaded shadow maps). */

			/**
			 * @brief Returns the number of cascades.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			cascadeCount () const noexcept
				requires (IsCascadedViewMatrix< view_matrices_t >)
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
				requires (IsCascadedViewMatrix< view_matrices_t >)
			{
				if ( cascadeIndex >= m_cascadeCount )
				{
					Tracer::error(ClassId, "Cascade index overflow !");

					cascadeIndex = 0;
				}

				return m_perCascadeImageViews[cascadeIndex];
			}

			/**
			 * @brief Returns the cascaded view matrices.
			 * @return ViewMatricesCascadedUBO &
			 */
			[[nodiscard]]
			ViewMatricesCascadedUBO &
			cascadedViewMatrices () noexcept
				requires (IsCascadedViewMatrix< view_matrices_t >)
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
				requires (IsCascadedViewMatrix< view_matrices_t >)
			{
				return m_viewMatrices;
			}

		private:

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void
			updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override
			{
				if constexpr ( IsCascadedViewMatrix< view_matrices_t > )
				{
					if ( !isOrthographicProjection )
					{
						TraceWarning{ClassId} << "CSM '" << this->id() << "' requires orthographic projection !";

						return;
					}
				}
				else
				{
					if ( this->isOrthographicProjection() != isOrthographicProjection )
					{
						TraceWarning{ClassId} << "The shadow map '" << this->id() << "' don't use the correct projection type !";

						return;
					}
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
				m_sampler = renderer.getSampler("ShadowMap", [this] (Settings &, VkSamplerCreateInfo & createInfo) {
					//createInfo.flags = 0;
					createInfo.magFilter = VK_FILTER_LINEAR;
					createInfo.minFilter = VK_FILTER_LINEAR;
					createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					/* NOTE: Use CLAMP_TO_BORDER so that sampling outside the shadow map
					 * returns borderColor (white = no shadow) instead of edge pixels. */
					createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
					createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
					createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
					//createInfo.mipLodBias = 0.0F;
					//createInfo.anisotropyEnable = VK_FALSE;
					//createInfo.maxAnisotropy = 1.0F;
					//createInfo.maxAnisotropy = 1.0F;
					/* NOTE: Cubemap doesn't use compare (manual sampling). */
					if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
					{
						createInfo.compareEnable = VK_FALSE;
						createInfo.compareOp = VK_COMPARE_OP_ALWAYS;
					}
					else
					{
						createInfo.compareEnable = VK_TRUE;
						createInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
					}
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

				/* Per-cascade image views (cascade only). */
				if constexpr ( IsCascadedViewMatrix< view_matrices_t > )
				{
					for ( auto & view : m_perCascadeImageViews )
					{
						view.reset();
					}
				}

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
					/* Calculate layer count based on shadow map type. */
					uint32_t layerCount = 1;
					VkImageCreateFlags flags = 0;

					if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
					{
						layerCount = 6;
						flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
					}
					else if constexpr ( IsCascadedViewMatrix< view_matrices_t > )
					{
						layerCount = m_cascadeCount;
					}

					m_depthImage = std::make_shared< Vulkan::Image >(
						device,
						VK_IMAGE_TYPE_2D,
						Vulkan::Instance::findDepthStencilFormat(device, this->precisions()), /* Should be VK_FORMAT_D32_SFLOAT or VK_FORMAT_D16_UNORM */
						this->extent(),
						VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
						flags,
						1,
						layerCount
					);
					m_depthImage->setIdentifier(ClassId, this->id(), "Image");

					if ( !m_depthImage->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image (Depth buffer) for shadow map '" << this->id() << "' !";

						return false;
					}

					/* NOTE: Perform an initial layout transition from UNDEFINED to DEPTH_STENCIL_READ_ONLY_OPTIMAL.
					 * This allows the shadow map to be used immediately in descriptors/samplers.
					 * The RenderPass will transition to DEPTH_STENCIL_ATTACHMENT_OPTIMAL when rendering,
					 * then back to DEPTH_STENCIL_READ_ONLY_OPTIMAL when done. */
					if ( !renderer.transferManager().transitionImageLayout(*m_depthImage, VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) )
					{
						TraceError{ClassId} << "Unable to transition depth image to DEPTH_STENCIL_READ_ONLY_OPTIMAL for shadow map '" << this->id() << "' !";

						return false;
					}

					/* Create the main image view for rendering.
					 * - 2D: VK_IMAGE_VIEW_TYPE_2D
					 * - Cubemap: VK_IMAGE_VIEW_TYPE_2D_ARRAY (for multiview rendering)
					 * - Cascade: VK_IMAGE_VIEW_TYPE_2D_ARRAY (for multiview rendering and sampling) */
					VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;

					if constexpr ( IsCubemapViewMatrix< view_matrices_t > || IsCascadedViewMatrix< view_matrices_t > )
					{
						viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
					}

					m_depthImageView = std::make_shared< Vulkan::ImageView >(
						m_depthImage,
						viewType,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
							.baseMipLevel = 0,
							.levelCount = m_depthImage->createInfo().mipLevels, /* Must be 1 */
							.baseArrayLayer = 0,
							.layerCount = layerCount
						}
					);
					m_depthImageView->setIdentifier(ClassId, this->id(), "ImageView");

					if ( !m_depthImageView->createOnHardware() )
					{
						TraceError{ClassId} << "Unable to create an image view (Depth buffer) for shadow map '" << this->id() << "' !";

						return false;
					}

					/* NOTE: Create a specific view for reading the cubemap in shaders (cubemap only). */
					if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
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

					/* Create per-cascade image views for rendering to individual layers (cascade only). */
					if constexpr ( IsCascadedViewMatrix< view_matrices_t > )
					{
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
								TraceError{ClassId} << "Unable to create the image view for cascade " << i << " of shadow map '" << this->id() << "' !";

								return false;
							}
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
					/* The image starts in DEPTH_STENCIL_READ_ONLY_OPTIMAL (transitioned at creation),
					 * transitions to DEPTH_STENCIL_ATTACHMENT_OPTIMAL during rendering,
					 * then back to DEPTH_STENCIL_READ_ONLY_OPTIMAL when done. */
					renderPass->addAttachmentDescription(VkAttachmentDescription{
						.flags = 0,
						.format = m_depthImage->createInfo().format,
						.samples = VK_SAMPLE_COUNT_1_BIT,
						.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
						.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
						.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
						.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
						.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
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

				/* Enable multiview for cubemap and cascade rendering (Vulkan 1.1+). */
				if constexpr ( IsCubemapViewMatrix< view_matrices_t > )
				{
					renderPass->enableMultiview();
				}
				else if constexpr ( IsCascadedViewMatrix< view_matrices_t > )
				{
					renderPass->enableMultiview(m_cascadeCount);
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
				/* Prepare the framebuffer.
				 * NOTE: When using multiview, framebuffer layers = 1.
				 * The render pass multiview extension handles rendering to multiple array layers. */
				if constexpr ( IsCubemapViewMatrix< view_matrices_t > || IsCascadedViewMatrix< view_matrices_t > )
				{
					const VkExtent2D extent2D{this->extent().width, this->extent().height};
					m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, extent2D, 1);
				}
				else
				{
					m_framebuffer = std::make_shared< Vulkan::Framebuffer >(renderPass, this->extent());
				}
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
			std::shared_ptr< Vulkan::ImageView > m_depthImageView; /* NOTE: In 2D mode, this is used for rendering AND sampling. In cubemap/cascade mode, this is used for rendering. */
			std::shared_ptr< Vulkan::ImageView > m_depthCubeImageView; /* NOTE: In cubemap mode, this is used for sampling. */
			std::array< std::shared_ptr< Vulkan::ImageView >, MaxCascadeCount > m_perCascadeImageViews{}; /* NOTE: In cascade mode, used for per-cascade rendering. */
			std::shared_ptr< Vulkan::Sampler > m_sampler;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			view_matrices_t m_viewMatrices;
			Libs::Math::CartesianFrame< float > m_worldCoordinates{};
			uint32_t m_cascadeCount{MaxCascadeCount};
			bool m_isReadyForRendering{false};
	};
}
