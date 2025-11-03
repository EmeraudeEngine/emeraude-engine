/*
 * src/Vulkan/SwapChain.hpp
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

/* STL inclusions. */
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <optional>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "Libs/StaticVector.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Types.hpp"
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Graphics/ViewMatrices3DUBO.hpp"
#include "Window.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The vulkan swap-chain class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	class SwapChain final : public AbstractDeviceDependentObject, public Graphics::RenderTarget::Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanSwapChain"};

			/**
			 * @brief Constructs a swap-chain.
			 * @param device A reference to a smart pointer of the device in use with the swap-chain.
			 * @param renderer A reference to the graphics renderer.
			 * @param settings A reference to the settings.
			 */
			SwapChain (const std::shared_ptr< Device > & device, Graphics::Renderer & renderer, Settings & settings) noexcept;

			/**
			 * @brief Destructs the swap-chain.
			 */
			~SwapChain () override
			{
				this->destroyFromHardware();
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() noexcept */
			bool
			createOnHardware () noexcept override
			{
				if ( !this->createRenderTarget(m_renderer) )
				{
					return false;
				}

				this->setCreated();

				return true;
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() noexcept */
			bool
			destroyFromHardware () noexcept override
			{
				if ( !this->destroyRenderTarget() )
				{
					return false;
				}

				this->setDestroyed();

				return true;
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::videoType() const noexcept */
			[[nodiscard]]
			AVConsole::VideoType
			videoType () const noexcept override
			{
				return AVConsole::VideoType::View;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::updateViewRangesProperties() noexcept */
			void updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::aspectRatio() const noexcept */
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

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCubemap() const noexcept */
			[[nodiscard]]
			bool
			isCubemap () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() const noexcept */
			[[nodiscard]]
			const Framebuffer *
			framebuffer () const noexcept override
			{
				return m_frames[m_acquiredImageIndex].framebuffer.get();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() const noexcept */
			[[nodiscard]]
			const Graphics::ViewMatrices2DUBO &
			viewMatrices () const noexcept override
			{
				return m_viewMatrices;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() noexcept */
			[[nodiscard]]
			Graphics::ViewMatrices2DUBO &
			viewMatrices () noexcept override
			{
				return m_viewMatrices;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isReadyForRendering() const */
			[[nodiscard]]
			bool
			isReadyForRendering () const noexcept override
			{
				return this->isCreated() && m_status == Status::Ready;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isDebug() const */
			[[nodiscard]]
			bool
			isDebug () const noexcept override
			{
				return m_showInformation;
			}

			/**
			 * @brief Returns the swap-chain vulkan handle.
			 * @return VkSwapchainKHR
			 */
			[[nodiscard]]
			VkSwapchainKHR
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the swap-chain createInfo.
			 * @return const VkSwapchainCreateInfoKHR &
			 */
			[[nodiscard]]
			const VkSwapchainCreateInfoKHR &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the number of samples in use.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			sampleCount () const noexcept
			{
				return m_sampleCount;
			}

			/**
			 * @brief Returns whether the multisampling is enabled and effective.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMultisamplingEnabled () const noexcept
			{
				return this->sampleCount() > 1;
			}

			/**
			 * @brief Returns the number of images in the swap-chain.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			imageCount () const noexcept
			{
				return m_imageCount;
			}

			/**
			 * @brief Returns the current status of the swap-chain.
			 * @return Status
			 */
			[[nodiscard]]
			Status
			status () const noexcept
			{
				return m_status;
			}

			/**
			 * @brief Recreates the swap-chain when a resize occurs or properties change.
			 * @return bool
			 */
			[[nodiscard]]
			bool refresh () noexcept;

			/**
			 * @brief Acquires the next image index available in the swap-chain.
			 * @param imageAvailableSemaphore A pointer to the previous frame semaphore.
			 * @param timeout The timeout to acquire an image.
			 * @return std::optional< uint32_t >
			 */
			[[nodiscard]]
			std::optional< uint32_t > acquireNextImage (const Sync::Semaphore * imageAvailableSemaphore, uint64_t timeout) noexcept;

			/**
			 * @brief Presents an rendered image.
			 * @param imageIndex The current rendering image index.
			 * @param queue The graphics queue to present the image.
			 * @param renderFinishedSemaphore A semaphore handle to wait the signal for a finished render.
			 * @return void
			 */
			void present (const uint32_t & imageIndex, const Queue * queue, VkSemaphore renderFinishedSemaphore) noexcept;

		private:

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onInputDeviceConnected() */
			void onInputDeviceConnected (AVConsole::AVManagers & managers, AbstractVirtualDevice & sourceDevice) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onInputDeviceDisconnected() */
			void onInputDeviceDisconnected (AVConsole::AVManagers & managers, AbstractVirtualDevice & sourceDevice) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool onCreate (Graphics::Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void onDestroy () noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< RenderPass > createRenderPass (Graphics::Renderer & renderer) const noexcept override;

			/**
			 * @brief Returns the minimum image count desired in the swap-chain.
			 * @param capabilities
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t selectImageCount (const VkSurfaceCapabilitiesKHR & capabilities) noexcept;

			/**
			 * @brief Creates the base swap-chain object.
			 * @param window A reference to the window.
			 * @param oldSwapChain A handle to the previous swap-chain. Default none.
			 * @return bool
			 */
			[[nodiscard]]
			bool createBaseSwapChain (const Window & window, VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE) noexcept;

			/**
			 * @brief Destroys the base swap-chain object.
			 * @return void
			 */
			void destroyBaseSwapChain () noexcept;

			/**
			 * @brief Updates the view aspect ratio of the swap-chain.
			 * @note This version can be used alone to refresh the aspect ratio when a window resize occurs.
			 * @return void
			 */
			void updateViewProperties () noexcept;

			/**
			 * @brief Returns the best surface format.
			 * @return VkSurfaceFormatKHR
			 */
			[[nodiscard]]
			VkSurfaceFormatKHR chooseSurfaceFormat () const noexcept;

			/**
			 * @brief Returns the best present mode.
			 * @return VkPresentModeKHR
			 */
			[[nodiscard]]
			VkPresentModeKHR choosePresentMode () const noexcept;

			/**
			 * @brief Returns the dimensions of the swap-chain.
			 * @param capabilities A reference to the surface capabilities structure.
			 * @return VkExtent2D
			 */
			[[nodiscard]]
			VkExtent2D chooseSwapExtent (const VkSurfaceCapabilitiesKHR & capabilities) const noexcept;

			/**
			 * @brief Prepares data to complete the swap-chain framebuffer.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareFrameData () noexcept;

			/**
			 * @brief Returns images created by the swap-chain.
			 * @return std::vector< VkImage >
			 */
			[[nodiscard]]
			std::vector< VkImage > retrieveSwapChainImages () noexcept;

			/**
			 * @brief Creates a color buffer.
			 * @param swapChainImage A reference to a swap-chain image.
			 * @param image A reference to an image smart pointer.
			 * @param imageView A reference to an image view smart pointer.
			 * @param identifier A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			bool createColorBuffer (const VkImage & swapChainImage, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & identifier) const noexcept;

			/**
			 * @brief Creates a depth+stencil buffer.
			 * @param device A reference to a graphics device smart pointer.
			 * @param image A reference to an image smart pointer.
			 * @param depthImageView A reference to an image view smart pointer.
			 * @param stencilImageView A reference to an image view smart pointer.
			 * @param identifier A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDepthStencilBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & depthImageView, std::shared_ptr< ImageView > & stencilImageView, const std::string & identifier) noexcept override;

			/**
			 * @brief Creates the images and the image views for each swap-chain frame.
			 * @return bool
			 */
			[[nodiscard]]
			bool createImageArray () noexcept;

			/**
			 * @brief Creates the framebuffer swap-chain frame.
			 * @param renderPass A reference to the render pass smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createFramebufferArray (const std::shared_ptr< RenderPass > & renderPass) noexcept;

			/**
			 * @brief Creates the swap-chain framebuffer.
			 * @return bool
			 */
			bool createFramebuffer () noexcept;

			/**
			 * @brief Resets the swap-chain framebuffer.
			 * @return void
			 */
			void resetFramebuffer () noexcept;

			/**
			 * @brief Destroys the swap-chain framebuffer.
			 * @return void
			 */
			void destroyFramebuffer () noexcept;

			/**
			 * @brief swap-chain frame structure.
			 */
			struct Frame
			{
				/* Framebuffer configuration holder. */
				std::unique_ptr< Framebuffer > framebuffer;
				/* Color buffer */
				std::shared_ptr< Image > colorImage;
				std::shared_ptr< ImageView > colorImageView;
				/* Depth+Stencil buffers. */
				std::shared_ptr< Image > depthStencilImage;
				std::shared_ptr< ImageView > depthImageView;
				std::shared_ptr< ImageView > stencilImageView;
			};

			Graphics::Renderer & m_renderer;
			VkSwapchainKHR m_handle{VK_NULL_HANDLE};
			VkSwapchainCreateInfoKHR m_createInfo{};
			Status m_status{Status::Uninitialized};
			uint32_t m_sampleCount{1};
			uint32_t m_imageCount{0};
			uint32_t m_acquiredImageIndex{0};
			Libs::StaticVector< Frame, 5 > m_frames;
			Graphics::ViewMatrices2DUBO m_viewMatrices;
			float m_fovOrNear{0.0F};
			float m_distanceOrFar{0.0F};
			bool m_showInformation{false};
			bool m_isPerspectiveProjection{false};
			bool m_tripleBufferingEnabled{false};
			bool m_VSyncEnabled{false};
	};
}
