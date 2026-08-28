/*
 * src/Vulkan/SwapChain.hpp
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
#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/* Local inclusions for inheritances. */
#include "AbstractDeviceDependentObject.hpp"
#include "Graphics/RenderTarget/Abstract.hpp"

/* Local inclusions for usages. */
#include "Graphics/ViewMatrices2DUBO.hpp"
#include "Graphics/ViewMatrices3DUBO.hpp"
#include "StaticVector.hpp"
#include "Vulkan/CommandBuffer.hpp"
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/Types.hpp"
#include "Window.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief The vulkan swap-chain class.
	 * @extends EmEn::Vulkan::AbstractDeviceDependentObject This object needs a device.
	 * @extends EmEn::Graphics::RenderTarget::Abstract This is a render target.
	 */
	class EMEN_API SwapChain final : public AbstractDeviceDependentObject, public Graphics::RenderTarget::Abstract
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanSwapChain"};

			/**
			 * @brief Constructs a swap-chain.
			 * @param renderer A reference to the graphics renderer.
			 * @param settings A reference to the settings.
			 * @param showInformation Enable log output.
			 */
			SwapChain (Graphics::Renderer & renderer, Settings & settings, bool showInformation) noexcept;

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

			/**
			 * @copydoc EmEn::Graphics::RenderTarget::Abstract::frameRegionCount()
			 * @note ⚠️ The swap chain is created BEFORE Renderer::createRenderingSystem(), so
			 * framesInFlight() is still zero for it — its own image count is the number the renderer
			 * will be built from a moment later. Valid only after onCreate(), which is exactly when
			 * createRenderTarget() asks.
			 */
			[[nodiscard]]
			uint32_t
			frameRegionCount (Graphics::Renderer & /*renderer*/) const noexcept override
			{
				return this->imageCount();
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

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::videoType() const noexcept */
			[[nodiscard]]
			Scenes::AVConsole::VideoType
			videoType () const noexcept override
			{
				return Scenes::AVConsole::VideoType::View;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::setViewDistance() */
			void
			setViewDistance (float meters) noexcept override
			{
				m_distanceOrFar = meters;
				this->updateViewProperties();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::viewDistance() */
			[[nodiscard]]
			float
			viewDistance () const noexcept override
			{
				return m_viewMatrices.farPlane();
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::updateViewRangesProperties() noexcept */
			void updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateNearestObjectDistance() */
			void updateNearestObjectDistance (float distance) noexcept override;

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
				return this->isCreated() && m_status == SwapChainStatus::Ready;
			}

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::capture() */
			bool capture (TransferManager & transferManager, uint32_t layerIndex, bool keepAlpha, bool withDepthBuffer, bool withStencilBuffer, std::array< Base::PixelFactory::Pixmap< uint8_t >, 3 > & result) const noexcept override;

			/**
			 * @brief Returns the current frame's color image.
			 * @return std::shared_ptr< Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Image >
			currentColorImage () const noexcept
			{
				return m_frames[m_acquiredImageIndex].colorImage;
			}

			/**
			 * @brief Returns the current frame's depth/stencil image (single-sample).
			 * @return std::shared_ptr< Image >
			 */
			[[nodiscard]]
			std::shared_ptr< Image >
			currentDepthStencilImage () const noexcept
			{
				return m_frames[m_acquiredImageIndex].depthStencilImage;
			}

			/**
			 * @brief Returns the depth/stencil format used by the swap chain.
			 * @return VkFormat
			 */
			[[nodiscard]]
			VkFormat depthStencilFormat () const noexcept;

			/**
			 * @brief Returns the post-process framebuffer for the current frame.
			 * @note This framebuffer uses LOAD_OP_LOAD to preserve existing content.
			 * @return const Framebuffer *
			 */
			[[nodiscard]]
			const Framebuffer *
			postProcessFramebuffer () const noexcept override
			{
				return m_frames[m_acquiredImageIndex].postProcessFramebuffer.get();
			}

			/**
			 * @brief Returns the offscreen-composite framebuffer for the current frame.
			 * @note Used by the internal-target render path, where the scene is rendered
			 * offscreen and the swap chain only receives the composite (post-process quad,
			 * gizmos, overlay). Its render pass starts from UNDEFINED and clears both
			 * attachments, so no prior layout-establishing pass is needed. Its pipelines are
			 * shared with the post-process pass (compatible: same formats and sample counts).
			 * @return const Framebuffer *
			 */
			[[nodiscard]]
			const Framebuffer *
			offscreenCompositeFramebuffer () const noexcept
			{
				return m_frames[m_acquiredImageIndex].offscreenCompositeFramebuffer.get();
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
			 * @brief Returns whether the multisampling is enabled and effective.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isMultisamplingEnabled () const noexcept
			{
				return this->precisions().samples() > 1;
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
			 * @brief Sets the swap-chain degraded.
			 * @return void
			 */
			void
			setDegraded () noexcept
			{
				m_status = SwapChainStatus::Degraded;
			}

			/**
			 * @brief Returns the current status of the swap-chain.
			 * @return Status
			 */
			[[nodiscard]]
			SwapChainStatus
			status () const noexcept
			{
				return m_status;
			}

			/**
			 * @brief Recreates the swap-chain when a resize occurs or properties change.
			 * @return bool
			 */
			[[nodiscard]]
			bool recreate () noexcept;

			/**
			 * @brief Fully recreates the swap-chain by destroying and recreating the Vulkan surface.
			 * @note This is a workaround for Windows where vkCreateSwapchainKHR() can deadlock
			 * during interactive window resize. By destroying the surface completely, we avoid
			 * the problematic swap-chain transition on the same surface.
			 * @param useNativeCode Use the native code to build the surface.
			 * @return bool
			 */
			[[nodiscard]]
			bool fullRecreate (bool useNativeCode) noexcept;

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
			 * @param imageIndex The image index returned by acquireNextImage().
			 * @param queue The graphics queue to present the image.
			 * @param presentSemaphore A semaphore handle to wait the signal for a finished render.
			 * @warning This semaphore MUST belong to @a imageIndex and to no other image
			 * (Renderer::m_presentSemaphores). No fence observes the completion of a present, so
			 * the sole proof that this semaphore is free again is the re-acquisition of the image
			 * it was presented with. A semaphore indexed by frame in flight instead gets
			 * re-signaled while a present still waits on it, because acquireNextImage() returns
			 * indices in an arbitrary order: VUID-vkQueueSubmit-pSignalSemaphores-00067.
			 * @return void
			 */
			void present (const uint32_t & imageIndex, const Queue * queue, VkSemaphore presentSemaphore) noexcept;

		private:

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateVideoDeviceProperties() */
			void updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept override;

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::getWorldCoordinates() */
			[[nodiscard]]
			Base::Math::CartesianFrame< float >
			getWorldCoordinates () const noexcept override
			{
				return m_worldCoordinates;
			}

			/** @copydoc EmEn::Scenes::AVConsole::AbstractVirtualDevice::updateDeviceFromCoordinates() */
			void updateDeviceFromCoordinates (const Base::Math::CartesianFrame< float > & worldCoordinates, const Base::Math::Vector< 3, float > & worldVelocity) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onCreate() */
			[[nodiscard]]
			bool onCreate (Graphics::Renderer & renderer) noexcept override;

			/** @copydoc EmEn::Graphics::RenderTarget::Abstract::onDestroy() */
			void onDestroy () noexcept override;

			/**
			 * @copydoc EmEn::Graphics::RenderTarget::Abstract::writeCombinedImageSampler(const Vulkan::DescriptorSet &, uint32_t) const
			 * @note Intentionally left in private methods because this is not a texture.
			 */
			[[nodiscard]]
			bool
			writeCombinedImageSampler (const Vulkan::DescriptorSet & /*descriptorSet*/, uint32_t /*bindingIndex*/) const noexcept override
			{
				return false;
			}

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
			 * @brief Creates a post-process render pass with LOAD_OP_LOAD.
			 * @param renderer A reference to the renderer.
			 * @return std::shared_ptr< RenderPass >
			 */
			[[nodiscard]]
			std::shared_ptr< RenderPass > createPostProcessRenderPass (Graphics::Renderer & renderer) const noexcept;

			/**
			 * @brief Creates the post-process framebuffer array.
			 * @param renderPass A reference to the render pass smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createPostProcessFramebufferArray (const std::shared_ptr< RenderPass > & renderPass) noexcept;

			/**
			 * @brief Creates the offscreen-composite render pass (initialLayout UNDEFINED, LOAD_OP_CLEAR).
			 * @note Single swap-chain pass of the internal-target render path: it performs the
			 * UNDEFINED -> attachment layout transitions itself (carrying the acquire-semaphore
			 * dependency) and presents, so the former empty layout-establishing pass is not needed.
			 * Compatible with the post-process render pass (same attachments), so pipelines are shared.
			 * @param renderer A reference to the renderer.
			 * @return std::shared_ptr< RenderPass >
			 */
			[[nodiscard]]
			std::shared_ptr< RenderPass > createOffscreenCompositeRenderPass (Graphics::Renderer & renderer) const noexcept;

			/**
			 * @brief Creates the offscreen-composite framebuffer array.
			 * @param renderPass A reference to the render pass smart pointer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createOffscreenCompositeFramebufferArray (const std::shared_ptr< RenderPass > & renderPass) noexcept;

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
			struct EMEN_API Frame
			{
				/* Framebuffer configuration holder. */
				std::unique_ptr< Framebuffer > framebuffer;
				/* Post-process framebuffer (LOAD_OP_LOAD, for overlay/post-processing after grab pass). */
				std::unique_ptr< Framebuffer > postProcessFramebuffer;
				/* Offscreen-composite framebuffer (initialLayout UNDEFINED, LOAD_OP_CLEAR): the
				 * single swap-chain pass of the internal-target render path, replacing the former
				 * empty layout-establishing pass + LOAD pass sequence. */
				std::unique_ptr< Framebuffer > offscreenCompositeFramebuffer;
				/* MSAA Color buffer (multisampled) */
				std::shared_ptr< Image > MSAAColorImage;
				std::shared_ptr< ImageView > MSAAColorImageView;
				/* MSAA Depth+Stencil buffers (multisampled) */
				std::shared_ptr< Image > MSAADepthStencilImage;
				std::shared_ptr< ImageView > MSAADepthImageView;
				std::shared_ptr< ImageView > MSAAStencilImageView;
				/* Color buffer (resolve target, swapchain image) */
				std::shared_ptr< Image > colorImage;
				std::shared_ptr< ImageView > colorImageView;
				/* Depth+Stencil buffers (resolve target) */
				std::shared_ptr< Image > depthStencilImage;
				std::shared_ptr< ImageView > depthImageView;
				std::shared_ptr< ImageView > stencilImageView;
			};

			Graphics::Renderer & m_renderer;
			VkSwapchainKHR m_handle{VK_NULL_HANDLE};
			VkSwapchainCreateInfoKHR m_createInfo{};
			std::atomic<SwapChainStatus> m_status{SwapChainStatus::Uninitialized};
			uint32_t m_imageCount{0};
			uint32_t m_acquiredImageIndex{0};
			Base::StaticVector< Frame, 5 > m_frames;
			Graphics::ViewMatrices2DUBO m_viewMatrices;
			Base::Math::CartesianFrame< float > m_worldCoordinates;
			float m_fovOrNear{0.0F};
			float m_distanceOrFar{0.0F};
			bool m_showInformation{false};
			bool m_isPerspectiveProjection{false};
			bool m_tripleBufferingEnabled{false};
			bool m_VSyncEnabled{false};
			bool m_sRGBEnabled{false};
	};
}
