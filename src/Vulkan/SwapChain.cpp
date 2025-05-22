/*
 * src/Vulkan/SwapChain.cpp
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

#include "SwapChain.hpp"

/* STL inclusions. */
#include <cstddef>
#include <limits>
#include <mutex>

/* Local inclusions. */
#include "Graphics/Renderer.hpp"
#include "Sync/Fence.hpp"
#include "Sync/Semaphore.hpp"
#include "Instance.hpp"
#include "Device.hpp"
#include "Queue.hpp"
#include "Framebuffer.hpp"
#include "RenderPass.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Utility.hpp"
#include "Window.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace EmEn::Libs;
	using namespace EmEn::Libs::Math;
	using namespace Graphics;

	SwapChain::SwapChain (const std::shared_ptr< Device > & device, Settings & settings, Window & window) noexcept
		: AbstractDeviceDependentObject(device),
		Abstract(ClassId, {}, {}, RenderTargetType::View, AVConsole::ConnexionType::Input, false),
		m_window(&window)
	{
		m_window->surface()->update(device);

		/* NOTE: Check for multisampling. */
		{
			const auto sampleCount = settings.get< uint32_t >(VideoFramebufferSamplesKey, DefaultVideoFramebufferSamples);

			if ( sampleCount > 1 )
			{
				m_sampleCount = device->findSampleCount(sampleCount);
			}
		}

		m_flags[ShowInformation] = settings.get< bool >(VkShowInformationKey, DefaultVkShowInformation);
		m_flags[VSyncEnabled] = settings.get< bool >(VideoEnableVSyncKey, DefaultVideoEnableVSync);
		m_flags[TripleBufferingEnabled] = settings.get< bool >(VideoEnableTripleBufferingKey, DefaultVideoEnableTripleBuffering);
	}

	bool
	SwapChain::createBaseSwapChain (VkSwapchainKHR oldSwapChain) noexcept
	{
		const auto & capabilities = m_window->surface()->capabilities();

		const auto surfaceFormat = this->chooseSurfaceFormat();

		m_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = 0;
		m_createInfo.surface = m_window->surface()->handle();
		m_createInfo.minImageCount = this->selectImageCount(capabilities);
		m_createInfo.imageFormat = surfaceFormat.format;
		m_createInfo.imageColorSpace = surfaceFormat.colorSpace;
		m_createInfo.imageExtent = this->chooseSwapExtent(capabilities);
		m_createInfo.imageArrayLayers = 1;
		m_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		//auto index = device->getFamilyQueueIndex(VK_QUEUE_GRAPHICS_BIT);

		//QueueFamilyIndices indices = this->findQueueFamilies(m_device->physicalDeviceHandle());

		//uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		//if ( indices.graphicsFamily != indices.presentFamily )
		//{
		//	m_createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		//	m_createInfo.queueFamilyIndexCount = 2;
		//	m_createInfo.pQueueFamilyIndices = queueFamilyIndices;
		//}
		//else
		{
			m_createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; /* Performance! */
			m_createInfo.queueFamilyIndexCount = 0; /* Optional */
			m_createInfo.pQueueFamilyIndices = nullptr; /* Optional */
		}
		m_createInfo.preTransform = capabilities.currentTransform; /* NOTE: No transformation. Check "supportedTransforms" in "capabilities" */
		m_createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		m_createInfo.presentMode = this->choosePresentMode();
		m_createInfo.clipped = VK_TRUE;
		m_createInfo.oldSwapchain = oldSwapChain;

		const auto result = vkCreateSwapchainKHR(
			this->device()->handle(),
			&m_createInfo,
			nullptr,
			&m_handle
		);

		/* NOTE: Destroy the previous swap chain if exists. */
		if ( m_createInfo.oldSwapchain != VK_NULL_HANDLE )
		{
			vkDestroySwapchainKHR(
				this->device()->handle(),
				m_createInfo.oldSwapchain,
				nullptr
			);
		}

		if ( result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to create the swap chain : " << vkResultToCString(result) << " !";

			return false;
		}

		this->setExtent(m_createInfo.imageExtent.width, m_createInfo.imageExtent.height);

		return true;
	}

	void
	SwapChain::destroyBaseSwapChain () noexcept
	{
		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroySwapchainKHR(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}
	}

	bool
	SwapChain::onCreate (Renderer & renderer) noexcept
	{
		if ( !this->hasDevice() || m_window == nullptr || m_window->surface() == nullptr )
		{
			Tracer::fatal(ClassId, "No device, window or surface to create the swap chain !");

			return false;
		}

		m_status = Status::UnderConstruction;

		if ( !this->createBaseSwapChain() )
		{
			Tracer::error(ClassId, "Unable to create the base of the swap chain !");

			m_status = Status::Failure;

			return false;
		}

		if ( !this->prepareFrameData() )
		{
			Tracer::error(ClassId, "Unable to prepare data to complete the swap chain !");

			m_status = Status::Failure;

			return false;
		}

		if ( !this->createFramebuffer(renderer) )
		{
			Tracer::error(ClassId, "Unable to complete the framebuffer !");

			m_status = Status::Failure;

			return false;
		}

		if ( !this->createSynchronizationPrimitives() )
		{
			Tracer::error(ClassId, "Unable to create synchronization primitives !");

			m_status = Status::Failure;

			return false;
		}

		this->setCreated();

		m_status = Status::Ready;

		return true;
	}

	void
	SwapChain::onDestroy () noexcept
	{
		if ( !this->hasDevice() )
		{
			TraceError{ClassId} << "No device to destroy the swap chain " << m_handle << " (" << this->identifier() << ") !";

			return;
		}

		this->device()->waitIdle("Destroying the swap chain");

		m_status = Status::Uninitialized;

		this->destroyFramebuffer();

		this->destroyBaseSwapChain();

		this->setDestroyed();
	}

	bool
	SwapChain::recreateOnHardware (Renderer & renderer) noexcept
	{
		this->resetFramebuffer();

		m_status = Status::UnderConstruction;

		if ( !this->createBaseSwapChain(m_handle) )
		{
			Tracer::error(ClassId, "Unable to recreate the base of the swap chain !");

			return false;
		}

		if ( !this->createFramebuffer(renderer) )
		{
			Tracer::error(ClassId, "Unable to complete the framebuffer !");

			return false;
		}

		if ( m_flags[ShowInformation] )
		{
			TraceSuccess{ClassId} << "The swap chain " << m_handle << " (" << this->identifier() << ") is successfully recreated !";
		}

		m_status = Status::Ready;

		return true;
	}

	void
	SwapChain::resetFramebuffer () noexcept
	{
		m_status = Status::Uninitialized;

		for ( const auto & frame : m_frames )
		{
			/* Clear the framebuffer. */
			frame.framebuffer->destroyFromHardware();

			/* Clear the color buffer. */
			frame.colorImageView->destroyFromHardware();
			frame.colorImage->destroyFromHardware();

			/* Clear the depth+stencil buffer. */
			frame.depthImageView->destroyFromHardware();
			frame.depthStencilImage->destroyFromHardware();

			/* Reset the image fence. */
			frame.inFlightFence->destroyFromHardware();
			frame.inFlightFence->createOnHardware();
		}

		m_currentFrame = 0;
	}

	void
	SwapChain::destroyFramebuffer () noexcept
	{
		m_frames.clear();
	}

	uint32_t
	SwapChain::selectImageCount (const VkSurfaceCapabilitiesKHR & capabilities) noexcept
	{
		/* NOTE: Only one image is possible. */
		if ( capabilities.minImageCount == 1 && capabilities.minImageCount == capabilities.maxImageCount )
		{
			Tracer::error(ClassId, "The swap chain can only use 1 image. Disabling double buffering and V-Sync !");

			m_flags[TripleBufferingEnabled] = false;
			m_flags[VSyncEnabled] = false;

			return 1;
		}

		/* NOTE: It looks like the system enforces the triple-buffering. */
		if ( capabilities.minImageCount == 3 )
		{
			m_flags[TripleBufferingEnabled] = true;
		}

		if ( m_flags[TripleBufferingEnabled] && ( capabilities.maxImageCount == 0 || capabilities.maxImageCount >= 3 ) )
		{
			return 3;
		}

		return capabilities.minImageCount;
	}

	VkExtent2D
	SwapChain::chooseSwapExtent (const VkSurfaceCapabilitiesKHR & capabilities) const noexcept
	{
		const auto framebufferSize = m_window->getFramebufferSize();

		TraceDebug{ClassId} <<
			"Vulkan minimum extent detected : " << capabilities.minImageExtent.width << 'X' << capabilities.minImageExtent.height << "\n"
			"Vulkan maximum extent detected : " << capabilities.maxImageExtent.width << 'X' << capabilities.maxImageExtent.height << "\n"
			"Vulkan current extent detected : " << capabilities.currentExtent.width << 'X' << capabilities.currentExtent.height << "\n"
			"GLFW framebuffer : " << framebufferSize.at(0) << 'X' << framebufferSize.at(1);

		return {
			std::clamp(framebufferSize[0], capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp(framebufferSize[1], capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

	VkSurfaceFormatKHR
	SwapChain::chooseSurfaceFormat () const noexcept
	{
		const auto & formats = m_window->surface()->formats();

		/* NOTE: Check for 24 bits sRGB. */
		const auto formatIt = std::ranges::find_if(formats, [] (auto item) {
			return item.format == VK_FORMAT_B8G8R8A8_SRGB && item.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		});

		if ( formatIt == formats.cend() )
		{
			return formats.at(0);
		}

		return *formatIt;
	}

	bool
	SwapChain::prepareFrameData () noexcept
	{
		/* NOTE: Will set the image count for the swap chain. */
		const auto result = vkGetSwapchainImagesKHR(this->device()->handle(), m_handle, &m_imageCount, nullptr);

		if ( result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to get image count from the swap chain : " << vkResultToCString(result) << " !";

			return false;
		}

		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} << "The swap chain is using " << m_imageCount << " images.";
		}

		m_frames.resize(m_imageCount);

		return true;
	}

	std::vector< VkImage >
	SwapChain::retrieveSwapChainImages () noexcept
	{
		std::vector< VkImage > imageHandles{m_imageCount, VK_NULL_HANDLE};

		const auto result = vkGetSwapchainImagesKHR(
			this->device()->handle(),
			m_handle,
			&m_imageCount, imageHandles.data()
		);

		if ( result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to get images from the swap chain : " << vkResultToCString(result) << " !";

			return {};
		}

		return imageHandles;
	}

	bool
	SwapChain::createImageArray () noexcept
	{
		const auto swapChainImages = this->retrieveSwapChainImages();

		if ( swapChainImages.empty() )
		{
			return false;
		}

		/* Create image and image views to create the render pass. */
		for ( size_t imageIndex = 0; imageIndex < m_imageCount; ++imageIndex )
		{
			auto & frame = m_frames[imageIndex];

			/* Color buffer. */
			{
				const auto purposeId = (std::stringstream{} << "SwapChain-ColorBuffer" << imageIndex).str();

				if ( !this->createColorBuffer(swapChainImages[imageIndex], frame.colorImage, frame.colorImageView, purposeId) )
				{
					TraceFatal{ClassId} << "Unable to create the color buffer #" << imageIndex << " !";

					return false;
				}
			}

			/* Depth/Stencil buffer. */
			{
				const auto purposeId = (std::stringstream{} << "SwapChain-DepthBuffer" << imageIndex).str();

				if ( !this->createDepthStencilBuffer(this->device(), frame.depthStencilImage, frame.depthImageView, frame.stencilImageView, purposeId) )
				{
					TraceFatal{ClassId} << "Unable to create the depth buffer #" << imageIndex << " !";

					return false;
				}
			}
		}

		return true;
	}

	bool
	SwapChain::createFramebufferArray (const std::shared_ptr< RenderPass > & renderPass) noexcept
	{
		/* Create the frame buffer for each set of images using the same render pass. */
		for ( size_t imageIndex = 0; imageIndex < m_imageCount; ++imageIndex )
		{
			auto & frame = m_frames[imageIndex];

			/* Prepare the framebuffer for attachments. */
			frame.framebuffer = std::make_unique< Framebuffer >(renderPass, this->extent());
			frame.framebuffer->setIdentifier((std::stringstream{} << "SwapChain-Frame" << imageIndex << "-Framebuffer").str());

			/* Color buffer. */
			frame.framebuffer->addAttachment(frame.colorImageView->handle());

			/* Depth/Stencil buffer. */
			frame.framebuffer->addAttachment(frame.depthImageView->handle());

			if ( !frame.framebuffer->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create a framebuffer #" << imageIndex << " !";

				return false;
			}
		}

		return true;
	}

	std::shared_ptr< RenderPass >
	SwapChain::createRenderPass (Renderer & renderer) const noexcept
	{
		auto renderPass = renderer.getRenderPass(ClassId, 0);

		if ( !renderPass->isCreated() )
		{
			/* Prepare a subpass for the render pass. */
			RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

			/* Color buffer. */
			{
				const auto & colorBufferCreateInfo = m_frames.front().colorImage->createInfo();

				renderPass->addAttachmentDescription(VkAttachmentDescription{
					.flags = 0,
					.format = colorBufferCreateInfo.format,
					.samples = colorBufferCreateInfo.samples,
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
			{
				const auto & depthStencilBufferCreateInfo = m_frames.front().depthStencilImage->createInfo();

				renderPass->addAttachmentDescription(VkAttachmentDescription{
					.flags = 0,
					.format = depthStencilBufferCreateInfo.format,
					.samples = depthStencilBufferCreateInfo.samples,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
				});

				subPass.setDepthStencilAttachment(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			}

			renderPass->addSubPass(subPass);

			if ( !renderPass->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create a render pass !");

				return nullptr;
			}
		}

		return renderPass;
	}

	bool
	SwapChain::createFramebuffer (Renderer & renderer) noexcept
	{
		if ( m_imageCount == 0 )
		{
			Tracer::error(ClassId, "No image count to create the swap chain !");

			return false;
		}

		if ( !this->createImageArray() )
		{
			Tracer::error(ClassId, "Unable to create the swap chain images !");

			return false;
		}

		/* Create the render pass base on the first set of images (this is the swap chain, all images are technically the same) */
		const auto renderPass = this->createRenderPass(renderer);

		if ( !this->createFramebufferArray(renderPass) )
		{
			Tracer::error(ClassId, "Unable to create the swap chain framebuffer !");

			return false;
		}

		return true;
	}

	bool
	SwapChain::createColorBuffer (const VkImage & swapChainImage, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & purposeId) const noexcept
	{
		/* NOTE: Create an image from existing data from the swap chain. */
		image = Image::createFromSwapChain(this->device(), swapChainImage, m_createInfo);
		image->setIdentifier(purposeId + "-Image");

		if ( swapChainImage != image->handle() )
		{
			TraceFatal{ClassId} << "Unable to create image '" << purposeId << "' !";

			return false;
		}

		const auto & imageCreateInfo = image->createInfo();

		imageView = std::make_shared< ImageView >(
			image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = imageCreateInfo.mipLevels,
				.baseArrayLayer = 0,
				.layerCount = imageCreateInfo.arrayLayers
			}
		);
		imageView->setIdentifier(purposeId + "-ImageView");

		if ( !imageView->createOnHardware() )
		{
			TraceFatal{ClassId} << "Unable to create image view '" << purposeId << "' !";

			return false;
		}

		return true;
	}

	bool
	SwapChain::createDepthStencilBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & depthImageView, std::shared_ptr< ImageView > & stencilImageView, const std::string & purposeId) noexcept
	{
		image = std::make_shared< Image >(
			device,
			VK_IMAGE_TYPE_2D,
			Instance::findDepthStencilFormat(device, this->precisions()),
			this->extent(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			0, // flags
			1, // Image mip levels
			1, //m_createInfo.imageArrayLayers, // Image array layers
			VK_SAMPLE_COUNT_1_BIT, // Image multi sampling
			VK_IMAGE_TILING_OPTIMAL // Image tiling
		);
		image->setIdentifier(purposeId + "-Image");

		if ( !image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create image '" << purposeId << "' !";

			return false;
		}

		const auto & imageCreateInfo = image->createInfo();

		depthImageView = std::make_shared< ImageView >(
			image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = imageCreateInfo.mipLevels,
				.baseArrayLayer = 0,
				.layerCount = imageCreateInfo.arrayLayers
			}
		);
		depthImageView->setIdentifier(purposeId + "Depth-ImageView");

		if ( !depthImageView->createOnHardware() )
		{
			TraceFatal{ClassId} << "Unable to create image view '" << purposeId << "' !";

			return false;
		}

		stencilImageView = std::make_shared< ImageView >(
			image,
			VK_IMAGE_VIEW_TYPE_2D,
			VkImageSubresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
				.baseMipLevel = 0,
				.levelCount = imageCreateInfo.mipLevels,
				.baseArrayLayer = 0,
				.layerCount = imageCreateInfo.arrayLayers
			}
		);
		stencilImageView->setIdentifier(purposeId + "Stencil-ImageView");

		if ( !stencilImageView->createOnHardware() )
		{
			TraceFatal{ClassId} << "Unable to create image view '" << purposeId << "' !";

			return false;
		}

		return true;
	}

	bool
	SwapChain::createSynchronizationPrimitives () noexcept
	{
		for ( size_t imageIndex = 0; imageIndex < m_imageCount; ++imageIndex )
		{
			auto & frame = m_frames[imageIndex];

			frame.imageAvailableSemaphore = std::make_unique< Sync::Semaphore >(this->device());
			frame.imageAvailableSemaphore->setIdentifier((std::stringstream{} << "SwapChain-ImageAvailable" << imageIndex << "-Semaphore").str());

			if ( !frame.imageAvailableSemaphore->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create a semaphore #" << imageIndex << " for image available !";

				return false;
			}

			frame.renderFinishedSemaphore = std::make_unique< Sync::Semaphore >(this->device());
			frame.renderFinishedSemaphore->setIdentifier((std::stringstream{} << "SwapChain-RenderFinished" << imageIndex << "-Semaphore").str());

			if ( !frame.renderFinishedSemaphore->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create a semaphore #" << imageIndex << " for image finished !";

				return false;
			}

			frame.inFlightFence = std::make_unique< Sync::Fence >(this->device());
			frame.inFlightFence->setIdentifier((std::stringstream{} << "SwapChain-ImageInFlight" << imageIndex << "-Fence").str());

			if ( !frame.inFlightFence->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create a fence #" << imageIndex << " for in-flight !";

				return false;
			}
		}

		return true;
	}

	bool
	SwapChain::acquireNextImage (uint32_t & imageIndex) noexcept
	{
		if ( m_status != Status::Ready )
		{
			return false;
		}

		const auto & currentFrame = m_frames.at(m_currentFrame);

		if ( !currentFrame.inFlightFence->wait() )
		{
			Tracer::error(ClassId, "Something wrong happens while waiting the fence !");

			return false;
		}

		const auto result = vkAcquireNextImageKHR(
			this->device()->handle(),
			m_handle,
			std::numeric_limits< uint64_t >::max(),
			currentFrame.imageAvailableSemaphore->handle(),
			VK_NULL_HANDLE,
			&imageIndex
		);

		switch ( result )
		{
			case VK_SUCCESS :
				return true;

			case VK_TIMEOUT :
				TraceWarning{ClassId} << "VK_TIMEOUT @ acquisition for image #" << imageIndex << " !";

				return false;

			case VK_ERROR_OUT_OF_DATE_KHR :
			case VK_SUBOPTIMAL_KHR :
				TraceError{ClassId} << vkResultToCString(result) << " @ acquisition for image #" << imageIndex << " !";

				m_status = Status::Degraded;

				return false;

			default:
				TraceError{ClassId} << "Error from the swap chain : " << vkResultToCString(result) << " !";

				return false;
		}
	}

	bool
	SwapChain::submitCommandBuffer (const std::shared_ptr< CommandBuffer > & commandBuffer, const uint32_t & imageIndex, std::vector< VkSemaphore > & waitSemaphores) noexcept
	{
		const std::lock_guard< std::mutex > deviceAccessLockGuard{this->device()->deviceAccessLock()};

		if ( m_status != Status::Ready )
		{
			return false;
		}

		if ( !m_frames.at(imageIndex).inFlightFence->wait() )
		{
			TraceError{ClassId} << "Something wrong happens while waiting the fence for image #" << imageIndex << " !";

			return false;
		}

		const auto & currentFrame = m_frames.at(m_currentFrame);

		if ( !currentFrame.inFlightFence->reset() )
		{
			TraceError{ClassId} << " Unable to reset in-flight fence for image " << m_currentFrame << " !" "\n";

			return false;
		}

		const auto * graphicsQueue = this->device()->getQueue(QueueJob::Presentation, QueuePriority::High);

		const auto signalSemaphore = currentFrame.renderFinishedSemaphore->handle();

		waitSemaphores.emplace_back(currentFrame.imageAvailableSemaphore->handle());

		if ( !graphicsQueue->submit(commandBuffer, waitSemaphores, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, signalSemaphore, currentFrame.inFlightFence->handle()) )
		{
			return false;
		}

		const auto * presentationQueue = this->device()->getQueue(QueueJob::Presentation, QueuePriority::High);

		bool swapChainRecreationNeeded = false;

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &signalSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_handle;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		if ( !presentationQueue->present(&presentInfo, swapChainRecreationNeeded) )
		{
			if ( swapChainRecreationNeeded )
			{
				m_status = Status::Degraded;
			}

			return false;
		}

		/* TODO: Check if this is correct to skip the current frame index, when swap-chain has failed to present the image. */
		m_currentFrame = (m_currentFrame + 1) % m_imageCount;

		return true;
	}

	VkPresentModeKHR
	SwapChain::choosePresentMode () const noexcept
	{
		const auto & presentModes = m_window->surface()->presentModes();

		if ( m_flags[ShowInformation] )
		{
			std::stringstream info;
			info << "Present modes available :" "\n";

			for ( const auto presentMode : presentModes )
			{
				switch ( presentMode )
				{
					// NOTE: Tearing.
					case VK_PRESENT_MODE_IMMEDIATE_KHR :
						info << " - VK_PRESENT_MODE_IMMEDIATE_KHR" "\n";
						break;

					// NOTE: No tearing.
					case VK_PRESENT_MODE_MAILBOX_KHR :
						info << " - VK_PRESENT_MODE_MAILBOX_KHR" "\n";
						break;

					// NOTE: Always available, no tearing.
					case VK_PRESENT_MODE_FIFO_KHR :
						info << " - VK_PRESENT_MODE_FIFO_KHR" "\n";
						break;

					// NOTE: Tearing on timeout.
					case VK_PRESENT_MODE_FIFO_RELAXED_KHR :
						info << " - VK_PRESENT_MODE_FIFO_RELAXED_KHR" "\n";
						break;

					case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR :
						info << " - VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR" "\n";
						break;

					case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR :
						info << " - VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR" "\n";
						break;

					default:
						info << " - UNKNOWN MODE !" "\n";
						break;
				}
			}

			TraceInfo{ClassId} << info.str();
		}

		if ( m_flags[VSyncEnabled] )
		{
			if ( std::ranges::find(presentModes, VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.cend() )
			{
				if ( m_flags[ShowInformation] )
				{
					TraceInfo{ClassId} << "The swap chain will use MAILBOX as presentation mode.";
				}

				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}
		else if ( m_flags[TripleBufferingEnabled] )
		{
			if ( std::ranges::find(presentModes, VK_PRESENT_MODE_FIFO_RELAXED_KHR) != presentModes.cend() )
			{
				if ( m_flags[ShowInformation] )
				{
					TraceInfo{ClassId} << "The swap chain will use FIFO_RELAXED as presentation mode.";
				}

				return VK_PRESENT_MODE_FIFO_RELAXED_KHR;
			}
		}
		else
		{
			if ( std::ranges::find(presentModes, VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.cend() )
			{
				if ( m_flags[ShowInformation] )
				{
					TraceInfo{ClassId} << "The swap chain will use IMMEDIATE as presentation mode.";
				}

				return VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}

		if ( m_flags[ShowInformation] )
		{
			TraceInfo{ClassId} << "The swap chain will use FIFO as presentation mode.";
		}

		/* Default presentation mode (Always available). */
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	void
	SwapChain::onSourceConnected (AbstractVirtualDevice * /*sourceDevice*/) noexcept
	{
		m_viewMatrices.create(*Renderer::instance(), this->id());
	}

	void
	SwapChain::onSourceDisconnected (AbstractVirtualDevice * /*sourceDevice*/) noexcept
	{
		m_viewMatrices.destroy();
	}

	void
	SwapChain::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
	}

	void
	SwapChain::updateProperties (bool isPerspectiveProjection, float distance, float fovOrNear) noexcept
	{
		const auto & extent = this->extent();
		const auto width = static_cast< float >(extent.width);
		const auto height = static_cast< float >(extent.height);

		if ( isPerspectiveProjection )
		{
			m_viewMatrices.updatePerspectiveViewProperties(width, height, distance, fovOrNear);
		}
		else
		{
			m_viewMatrices.updateOrthographicViewProperties(width, height, distance, fovOrNear);
		}
	}
}
