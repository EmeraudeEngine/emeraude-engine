/*
 * src/Graphics/SceneRenderTarget.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>

/* Local inclusions for inheritances. */
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/ViewMatrices2DUBO.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	class Image;
	class ImageView;
	class Framebuffer;
	class RenderPass;
}

namespace EmEn::Graphics
{
	/**
	 * @brief Internal scene render target for HDR and MSAA rendering.
	 * @note This provides a framebuffer with configurable color format (8-bit or float16)
	 * and sample count, used when the post-processor is active, MSAA is enabled,
	 * or the engine runs in windowless mode.
	 * The resolved color and depth images are sampleable, allowing the post-processor
	 * to read them via blit or direct sampling.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	class SceneRenderTarget final : public RenderTarget::Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SceneRenderTarget"};

			/**
			 * @brief Constructs a scene render target.
			 * @param name A reference to a string for the name of the video device.
			 * @param width The width of the render target.
			 * @param height The height of the render target.
			 * @param colorFormat The Vulkan color format (e.g. VK_FORMAT_R16G16B16A16_SFLOAT for HDR).
			 * @param normalsFormat The Vulkan format for the normals MRT attachment. VK_FORMAT_UNDEFINED to skip.
			 * @param materialPropertiesFormat The Vulkan format for the material properties MRT attachment. VK_FORMAT_UNDEFINED to skip.
			 * @param depthFormat The Vulkan depth format (e.g. VK_FORMAT_D24_UNORM_S8_UINT).
			 * @param viewDistance The max viewable distance in meters.
			 */
			SceneRenderTarget (const std::string & name, uint32_t width, uint32_t height, VkFormat colorFormat, VkFormat normalsFormat, VkFormat materialPropertiesFormat, VkFormat depthFormat, float viewDistance) noexcept;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::setViewDistance() */
			void setViewDistance (float meters) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewDistance() */
			[[nodiscard]]
			float viewDistance () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::updateViewRangesProperties() */
			void updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::aspectRatio() */
			[[nodiscard]]
			float aspectRatio () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCubemap() */
			[[nodiscard]]
			bool isCubemap () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() */
			[[nodiscard]]
			const Vulkan::Framebuffer * framebuffer () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::postProcessFramebuffer() */
			[[nodiscard]]
			const Vulkan::Framebuffer * postProcessFramebuffer () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() const */
			[[nodiscard]]
			const ViewMatricesInterface & viewMatrices () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() */
			[[nodiscard]]
			ViewMatricesInterface & viewMatrices () noexcept override;

			/**
			 * @brief Sets the source view matrices to delegate to.
			 * @note The SceneRenderTarget shares the camera view of the main render target
			 * (swapchain or windowless view). This avoids creating a separate UBO.
			 * @param source A reference to the source view matrices interface.
			 */
			void
			setSourceViewMatrices (ViewMatricesInterface & source) noexcept
			{
				m_sourceViewMatrices = &source;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isReadyForRendering() */
			[[nodiscard]]
			bool isReadyForRendering () const noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			Scenes::AVConsole::VideoType videoType () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::capture() */
			bool capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool keepAlpha, bool withDepthBuffer, bool withStencilBuffer, std::array< Libs::PixelFactory::Pixmap< uint8_t >, 3 > & result) const noexcept override;

			/**
			 * @brief Returns the resolved color image for blit/sampling operations.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			colorImage () const noexcept
			{
				return m_colorImage;
			}

			/**
			 * @brief Returns the depth/stencil image for blit/sampling operations.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			depthStencilImage () const noexcept
			{
				return m_depthStencilImage;
			}

			/**
			 * @brief Returns the color format used by this render target.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat
			colorFormat () const noexcept
			{
				return m_colorFormat;
			}

			/**
			 * @brief Returns the depth format used by this render target.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat
			depthFormat () const noexcept
			{
				return m_depthFormat;
			}

			/**
			 * @brief Returns the normals format used by this render target.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat
			normalsFormat () const noexcept
			{
				return m_normalsFormat;
			}

			/**
			 * @brief Returns the normals image for blit/sampling operations.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			normalsImage () const noexcept
			{
				return m_normalsImage;
			}

			/**
			 * @brief Returns the material properties format used by this render target.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat
			materialPropertiesFormat () const noexcept
			{
				return m_materialPropertiesFormat;
			}

			/**
			 * @brief Returns the material properties image for blit/sampling operations.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			materialPropertiesImage () const noexcept
			{
				return m_materialPropertiesImage;
			}

		private:

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Libs::Math::CartesianFrame< float > getWorldCoordinates () const noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onInputDeviceConnected() */
			void onInputDeviceConnected (EngineContext & engineContext, AbstractVirtualDevice & sourceDevice) noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::onInputDeviceDisconnected() */
			void onInputDeviceDisconnected (EngineContext & engineContext, AbstractVirtualDevice & sourceDevice) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::writeCombinedImageSampler() */
			[[nodiscard]]
			bool writeCombinedImageSampler (const Vulkan::DescriptorSet & descriptorSet, uint32_t bindingIndex) const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass > createRenderPass (Renderer & renderer) const noexcept override;

			/**
			 * @brief Creates a post-process render pass with LOAD_OP_LOAD for TranslucentGB rendering.
			 * @param renderer A reference to the graphics renderer.
			 * @return std::shared_ptr< Vulkan::RenderPass >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass > createPostProcessRenderPass (Renderer & renderer) const noexcept;

			/**
			 * @brief Creates the post-process framebuffer from the render pass and image views.
			 * @param renderPass A reference to the render pass smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPostProcessFramebuffer (const std::shared_ptr< Vulkan::RenderPass > & renderPass) noexcept;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool onCreate (Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void onDestroy () noexcept override;

			/**
			 * @brief Creates the GPU images and image views.
			 * @param renderer A reference to the graphics renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createImages (const Renderer & renderer) noexcept;

			/**
			 * @brief Creates the framebuffer from the render pass and image views.
			 * @param renderPass A reference to the render pass smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createFramebuffer (const std::shared_ptr< Vulkan::RenderPass > & renderPass) noexcept;

			VkFormat m_colorFormat;
			VkFormat m_normalsFormat;
			VkFormat m_materialPropertiesFormat;
			VkFormat m_depthFormat;
			std::shared_ptr< Vulkan::Image > m_colorImage;
			std::shared_ptr< Vulkan::ImageView > m_colorImageView;
			std::shared_ptr< Vulkan::Image > m_normalsImage;
			std::shared_ptr< Vulkan::ImageView > m_normalsImageView;
			std::shared_ptr< Vulkan::Image > m_materialPropertiesImage;
			std::shared_ptr< Vulkan::ImageView > m_materialPropertiesImageView;
			std::shared_ptr< Vulkan::Image > m_depthStencilImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			std::shared_ptr< Vulkan::Framebuffer > m_postProcessFramebuffer;
			ViewMatricesInterface * m_sourceViewMatrices{nullptr};
			ViewMatrices2DUBO m_viewMatrices;
			Libs::Math::CartesianFrame< float > m_worldCoordinates{};
			bool m_isReadyForRendering{false};
	};
}
