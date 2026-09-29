/*
 * src/Graphics/SelectionDepthTarget.hpp
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

#pragma once

/* Project configuration. */
#include "emeraude_export.hpp"

/* STL inclusions. */
#include <cstdint>
#include <memory>

/* Local inclusions for inheritances. */
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/ViewMatrices2DUBO.hpp"

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
	 * @brief The "custom depth" of the selection: the depth of the SELECTED instances alone, through the main camera —
	 * Unreal's CustomDepth. The source of the selection outline (Graphics::SelectionOutline).
	 * @note Depth only (`D32_SFLOAT`, sampleable, left in SHADER_READ_ONLY_OPTIMAL by its render pass), the size of the
	 * scene. Drawn with the depth-only shadow-casting programs — skinning, alpha test, wind and heightfields come for
	 * free — which drop their bias and clamp for this target type (RenderTargetType::SelectionDepth) and never jitter:
	 * the outline stays still under the TAA.
	 * @note Its view matrices DELEGATE to the main camera (setSourceViewMatrices(), the scene target's scheme); the
	 * owned instance only exists for the base class's lifecycle.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	class EMEN_API SelectionDepthTarget final : public RenderTarget::Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"SelectionDepthTarget"};

			/** @brief The depth format: sampleable everywhere, and the exact depth the comparison wants. */
			static constexpr auto DepthFormat{VK_FORMAT_D32_SFLOAT};

			/**
			 * @brief Constructs a selection depth target.
			 * @param width The width, the scene's.
			 * @param height The height, the scene's.
			 * @param viewDistance The max viewable distance in meters.
			 */
			SelectionDepthTarget (uint32_t width, uint32_t height, float viewDistance) noexcept;

			/**
			 * @brief Sets the view matrices to delegate to: the main camera's.
			 * @param source A reference to the source view matrices interface.
			 * @return void
			 */
			void
			setSourceViewMatrices (ViewMatricesInterface & source) noexcept
			{
				m_sourceViewMatrices = &source;
			}

			/**
			 * @brief Returns the depth image.
			 * @return std::shared_ptr< Vulkan::Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::Image >
			depthImage () const noexcept
			{
				return m_depthImage;
			}

			/**
			 * @brief Returns the depth image view, for sampling.
			 * @return std::shared_ptr< Vulkan::ImageView >
			 */
			[[nodiscard]]
			std::shared_ptr< Vulkan::ImageView >
			depthImageView () const noexcept
			{
				return m_depthImageView;
			}

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
			bool
			isCubemap () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() */
			[[nodiscard]]
			const Vulkan::Framebuffer * framebuffer () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() const */
			[[nodiscard]]
			const ViewMatricesInterface & viewMatrices () const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() */
			[[nodiscard]]
			ViewMatricesInterface & viewMatrices () noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isReadyForRendering() */
			[[nodiscard]]
			bool
			isReadyForRendering () const noexcept override
			{
				return m_isReadyForRendering;
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::videoType() */
			[[nodiscard]]
			Scenes::AVConsole::VideoType
			videoType () const noexcept override
			{
				return Scenes::AVConsole::VideoType::View;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::capture() */
			bool capture (Vulkan::TransferManager & transferManager, uint32_t layerIndex, bool keepAlpha, bool withDepthBuffer, bool withStencilBuffer, std::array< Base::PixelFactory::Pixmap< uint8_t >, 3 > & result) const noexcept override;

		protected:

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::ownViewMatrices() */
			[[nodiscard]]
			ViewMatricesInterface &
			ownViewMatrices () noexcept override
			{
				return m_viewMatrices;
			}

		private:

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateNearestObjectDistance() */
			void
			updateNearestObjectDistance (float /*distance*/) noexcept override
			{
				/* The main camera's view decides: this target only borrows it. */
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return {};
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void
			updateDeviceFromCoordinates (const Base::Math::CartesianFrame< float > & /*worldCoordinates*/, const Base::Math::Vector< 3, float > & /*worldVelocity*/) noexcept override
			{
				/* The main camera's view decides: this target only borrows it. */
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::writeCombinedImageSampler() */
			[[nodiscard]]
			bool
			writeCombinedImageSampler (const Vulkan::DescriptorSet & /*descriptorSet*/, uint32_t /*bindingIndex*/) const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< Vulkan::RenderPass > createRenderPass (Renderer & renderer) const noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool onCreate (Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void onDestroy () noexcept override;

			std::shared_ptr< Vulkan::Image > m_depthImage;
			std::shared_ptr< Vulkan::ImageView > m_depthImageView;
			std::shared_ptr< Vulkan::Framebuffer > m_framebuffer;
			ViewMatricesInterface * m_sourceViewMatrices{nullptr};
			ViewMatrices2DUBO m_viewMatrices;
			bool m_isReadyForRendering{false};
	};
}
