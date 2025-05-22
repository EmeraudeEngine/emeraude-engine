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
#include <array>
#include <vector>
#include <string>
#include <memory>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "AVConsole/AbstractVirtualDevice.hpp"
#include "AVConsole/Types.hpp"
#include "Graphics/Renderer.hpp"
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Libs/Math/CartesianFrame.hpp"
#include "Settings.hpp"

/* Forward declarations. */
namespace EmEn::Vulkan
{
	namespace Sync
	{
		class Semaphore;
		class Fence;
	}

	class RenderPass;
	class Framebuffer;
	class CommandBuffer;
}

namespace EmEn::Vulkan
{
	/**
	 * @brief Swap chain frame structure.
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
		/* Synchronization. */
		std::unique_ptr< Sync::Semaphore > imageAvailableSemaphore;
		std::unique_ptr< Sync::Semaphore > renderFinishedSemaphore;
		std::unique_ptr< Sync::Fence > inFlightFence;
	};

	/**
	 * @brief The vulkan swap chain class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	class SwapChain final : public AbstractDeviceDependentObject, public Graphics::RenderTarget::Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanSwapChain"};

			/** @brief The swap-chain status enumeration. */
			enum class Status: uint8_t
			{
				Uninitialized,
				Ready,
				Degraded,
				UnderConstruction,
				Failure
			};

			/**
			 * @brief Constructs a swap chain.
			 * @param device A reference to a smart pointer of the device in use with the swap chain.
			 * @param settings A reference to the settings.
			 * @param window A reference to the window.
			 */
			SwapChain (const std::shared_ptr< Device > & device, Settings & settings, Window & window) noexcept;

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::createOnHardware() */
			bool
			createOnHardware () noexcept override
			{
				if ( !this->create(*Graphics::Renderer::instance()) )
				{
					return false;
				}

				this->setCreated();

				return true;
			}

			/** @copydoc EmEn::Vulkan::AbstractDeviceDependentObject::destroyFromHardware() */
			bool
			destroyFromHardware () noexcept override
			{
				if ( !this->destroy() )
				{
					return false;
				}

				this->setDestroyed();

				return true;
			}

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::videoType() const */
			[[nodiscard]]
			AVConsole::VideoType
			videoType () const noexcept override
			{
				return AVConsole::VideoType::View;
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

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isCubemap() const */
			[[nodiscard]]
			bool
			isCubemap () const noexcept override
			{
				return false;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::framebuffer() const */
			[[nodiscard]]
			const Framebuffer *
			framebuffer () const noexcept override
			{
				return m_frames[m_currentFrame].framebuffer.get();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::image() const */
			[[nodiscard]]
			std::shared_ptr< Image >
			image () const noexcept override
			{
				return m_frames[m_currentFrame].colorImage;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::imageView() const */
			[[nodiscard]]
			std::shared_ptr< ImageView >
			imageView () const noexcept override
			{
				return m_frames[m_currentFrame].colorImageView;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() const */
			[[nodiscard]]
			const Graphics::ViewMatrices2DUBO &
			viewMatrices () const noexcept override
			{
				return m_viewMatrices;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewMatrices() */
			[[nodiscard]]
			Graphics::ViewMatrices2DUBO &
			viewMatrices () noexcept override
			{
				return m_viewMatrices;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::isValid() const */
			[[nodiscard]]
			bool
			isValid () const noexcept override
			{
				/* FIXME: Add a better check ! */
				return m_handle != VK_NULL_HANDLE;
			}

			/**
			 * @brief Recreates the swap chain.
			 * @param renderer A reference to the renderer.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreateOnHardware (Graphics::Renderer & renderer) noexcept;

			/**
			 * @brief Returns the swap chain vulkan handle.
			 * @return VkSwapchainKHR
			 */
			[[nodiscard]]
			VkSwapchainKHR
			handle () const noexcept
			{
				return m_handle;
			}

			/**
			 * @brief Returns the swap chain create info.
			 * @return const VkSwapchainCreateInfoKHR &
			 */
			[[nodiscard]]
			const VkSwapchainCreateInfoKHR &
			createInfo () const noexcept
			{
				return m_createInfo;
			}

			/**
			 * @brief Returns the number of sample in use.
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
			 * @brief Returns the number of images in the swap chain.
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t
			imageCount () const noexcept
			{
				return m_imageCount;
			}

			/**
			 * @brief Acquires the next image index available in the swap chain.
			 * @param imageIndex A reference to an unsigned int to store the index.
			 * @return bool
			 */
			bool acquireNextImage (uint32_t & imageIndex) noexcept;

			/**
			 * @brief Submits command buffer to the current rendering image.
			 * @param commandBuffer A reference to a command buffer smart pointer.
			 * @param imageIndex The current rendering image index.
			 * @param waitSemaphores A writable reference to a vector of semaphores.
			 * @return bool
			 */
			bool submitCommandBuffer (const std::shared_ptr< CommandBuffer > & commandBuffer, const uint32_t & imageIndex, std::vector< VkSemaphore > & waitSemaphores) noexcept;

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

		private:

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Libs::Math::CartesianFrame< float > & worldCoordinates, const Libs::Math::Vector< 3, float > & worldVelocity) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::updateProperties() */
			void updateProperties (bool isPerspectiveProjection, float distance, float fovOrNear) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onSourceConnected() */
			void onSourceConnected (AbstractVirtualDevice * sourceDevice) noexcept override;

			/** @copydoc EmEn::AVConsole::AbstractVirtualDevice::onSourceDisconnected() */
			void onSourceDisconnected (AbstractVirtualDevice * sourceDevice) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool onCreate (Graphics::Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void onDestroy () noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::createRenderPass() */
			[[nodiscard]]
			std::shared_ptr< RenderPass > createRenderPass (Graphics::Renderer & renderer) const noexcept override;

			/**
			 * @brief Returns the minimum image count desired in the swap chain.
			 * @param capabilities
			 * @return uint32_t
			 */
			[[nodiscard]]
			uint32_t selectImageCount (const VkSurfaceCapabilitiesKHR & capabilities) noexcept;

			/**
			 * @brief Creates the base swap chain object.
			 * @param oldSwapChain A handle to the previous swap chain. Default none.
			 * @return bool
			 */
			[[nodiscard]]
			bool createBaseSwapChain (VkSwapchainKHR oldSwapChain = VK_NULL_HANDLE) noexcept;

			/**
			 * @brief Destroys the base swap chain object.
			 * @return void
			 */
			void destroyBaseSwapChain () noexcept;

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
			 * @brief Returns the dimensions of the swap chain.
			 * @param capabilities A reference to the surface capabilities structure.
			 * @return VkExtent2D
			 */
			[[nodiscard]]
			VkExtent2D chooseSwapExtent (const VkSurfaceCapabilitiesKHR & capabilities) const noexcept;

			/**
			 * @brief Prepares data to complete the swap chain framebuffer.
			 * @return bool
			 */
			[[nodiscard]]
			bool prepareFrameData () noexcept;

			/**
			 * @brief Returns images created by the swap chain.
			 * @return std::vector< VkImage >
			 */
			[[nodiscard]]
			std::vector< VkImage > retrieveSwapChainImages () noexcept;

			/**
			 * @brief Creates a color buffer.
			 * @param swapChainImage A reference to swap chain image.
			 * @param image A reference to an image smart pointer.
			 * @param imageView A reference to an image view smart pointer.
			 * @param purposeId A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			bool createColorBuffer (const VkImage & swapChainImage, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & purposeId) const noexcept;

			/**
			 * @brief Creates a depth+stencil buffer.
			 * @param device A reference to a graphics device smart pointer.
			 * @param image A reference to an image smart pointer.
			 * @param depthImageView A reference to an image view smart pointer.
			 * @param stencilImageView A reference to an image view smart pointer.
			 * @param purposeId A reference to a string to identify buffers.
			 * @return bool
			 */
			[[nodiscard]]
			bool createDepthStencilBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & depthImageView, std::shared_ptr< ImageView > & stencilImageView, const std::string & purposeId) noexcept override;

			/**
			 * @brief Creates the images and the image views for each swap chain frame.
			 * @return bool
			 */
			[[nodiscard]]
			bool createImageArray () noexcept;

			/**
			 * @brief Creates the framebuffer swap chain frame.
			 * @param renderPass A reference to the render pass smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createFramebufferArray (const std::shared_ptr< RenderPass > & renderPass) noexcept;

			/**
			 * @brief Creates the swap-chain framebuffer.
			 * @param renderer A reference to the renderer.
			 * @return bool
			 */
			bool createFramebuffer (Graphics::Renderer & renderer) noexcept;

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
			 * @brief Creates the synchronization primitives.
			 * @return bool
			 */
			[[nodiscard]]
			bool createSynchronizationPrimitives () noexcept;

			/* Flag names. */
			static constexpr auto ShowInformation{0UL};
			static constexpr auto TripleBufferingEnabled{1UL};
			static constexpr auto VSyncEnabled{2UL};

			Window * m_window;
			VkSwapchainKHR m_handle{VK_NULL_HANDLE};
			VkSwapchainCreateInfoKHR m_createInfo{};
			Status m_status{Status::Uninitialized};
			uint32_t m_sampleCount{1};
			uint32_t m_imageCount{0};
			uint32_t m_currentFrame{0};
			std::vector< Frame > m_frames;
			Graphics::ViewMatrices2DUBO m_viewMatrices;
			std::array< bool, 8 > m_flags{
				false/*ShowInformation*/,
				false/*TripleBufferingEnabled*/,
				false/*VSyncEnabled*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/,
				false/*UNUSED*/
			};
	};
}
