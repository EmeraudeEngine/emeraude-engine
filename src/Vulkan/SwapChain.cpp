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

/* Application configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#if !IS_MACOS
#include <format>
#endif

/* Local inclusions. */
#include "Vulkan/Framebuffer.hpp"
#include "Vulkan/ImageView.hpp"
#include "Vulkan/Device.hpp"
#include "Vulkan/Utility.hpp"
#include "Graphics/Renderer.hpp"
#include "Settings.hpp"
#include "Tracer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;
	using namespace Libs::Math;
	using namespace Graphics;

	SwapChain::SwapChain (const std::shared_ptr< Device > & device, Renderer & renderer, Settings & settings) noexcept
		: AbstractDeviceDependentObject{device},
		Abstract(ClassId, {}, {}, RenderTargetType::View, AVConsole::ConnexionType::Input, false),
		m_renderer{renderer}
	{
		m_renderer.window().surface()->update(device);

		/* NOTE: Check for multisampling. */
		{
			const auto sampleCount = settings.getOrSetDefault< uint32_t >(VideoFramebufferSamplesKey, DefaultVideoFramebufferSamples);

			if ( sampleCount > 1 )
			{
				m_sampleCount = device->findSampleCount(sampleCount);
			}
		}

		m_showInformation = settings.getOrSetDefault< bool >(VkShowInformationKey, DefaultVkShowInformation);
		m_tripleBufferingEnabled = settings.getOrSetDefault< bool >(VideoEnableTripleBufferingKey, DefaultVideoEnableTripleBuffering);
		m_VSyncEnabled = settings.getOrSetDefault< bool >(VideoEnableVSyncKey, DefaultVideoEnableVSync);
	}

	bool
	SwapChain::createBaseSwapChain (const Window & window, VkSwapchainKHR oldSwapChain) noexcept
	{
		const auto * surface = window.surface();
		const auto surfaceFormat = this->chooseSurfaceFormat();
		const auto & capabilities = surface->capabilities();

		m_createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		m_createInfo.pNext = nullptr;
		m_createInfo.flags = 0;
		m_createInfo.surface = surface->handle();
		m_createInfo.minImageCount = this->selectImageCount(capabilities);
		m_createInfo.imageFormat = surfaceFormat.format;
		m_createInfo.imageColorSpace = surfaceFormat.colorSpace;
		m_createInfo.imageExtent = this->chooseSwapExtent(capabilities);
		m_createInfo.imageArrayLayers = 1;
		m_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		m_createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; /* NOTE: Graphics and presentation (99.9%) are from the same family. */
		m_createInfo.queueFamilyIndexCount = 0;
		m_createInfo.pQueueFamilyIndices = nullptr;
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
		const auto & window = renderer.window();

		if ( !this->hasDevice() || window.surface() == nullptr )
		{
			Tracer::fatal(ClassId, "No device or window surface to create the swap chain !");

			return false;
		}

		TraceDebug{ClassId} << "Application swap-chain creation." "\n" << this->precisions() << '\n';

		m_status = Status::UnderConstruction;

		if ( !this->createBaseSwapChain(window) )
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

		if ( !this->createFramebuffer() )
		{
			Tracer::error(ClassId, "Unable to complete the framebuffer !");

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

		m_status = Status::Uninitialized;

		this->destroyFramebuffer();

		this->destroyBaseSwapChain();

		this->setDestroyed();
	}

	bool
	SwapChain::recreateOnHardware () noexcept
	{
		this->resetFramebuffer();

		m_status = Status::UnderConstruction;

		if ( !this->createBaseSwapChain(m_renderer.window(), m_handle) )
		{
			Tracer::error(ClassId, "Unable to recreate the base of the swap chain !");

			return false;
		}

		if ( !this->createFramebuffer() )
		{
			Tracer::error(ClassId, "Unable to complete the framebuffer !");

			return false;
		}

		this->updateProperties();

		if ( m_showInformation )
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
		}
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

			m_tripleBufferingEnabled = false;
			m_VSyncEnabled = false;

			return 1;
		}

		/* NOTE: It looks like the system enforces the triple-buffering. */
		if ( capabilities.minImageCount == 3 )
		{
			m_tripleBufferingEnabled = true;
		}

		if ( m_tripleBufferingEnabled && ( capabilities.maxImageCount == 0 || capabilities.maxImageCount >= 3 ) )
		{
			return 3;
		}

		return capabilities.minImageCount;
	}

	VkExtent2D
	SwapChain::chooseSwapExtent (const VkSurfaceCapabilitiesKHR & capabilities) const noexcept
	{
		const auto framebufferSize = m_renderer.window().getFramebufferSize();

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
		const auto & formats = m_renderer.window().surface()->formats();

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

		if ( m_showInformation )
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

		if ( const auto result = vkGetSwapchainImagesKHR(this->device()->handle(), m_handle, &m_imageCount, imageHandles.data()); result != VK_SUCCESS )
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

		const bool requestDepthStencilBuffer = this->precisions().depthBits() > 0 || this->precisions().stencilBits() > 0;

		/* Create image and image views to create the render pass. */
		for ( size_t imageIndex = 0; imageIndex < m_imageCount; ++imageIndex )
		{
			auto & frame = m_frames[imageIndex];

#if IS_MACOS
			const auto identifier = (std::stringstream{} << "Frame" << imageIndex).str();
#else
			const auto identifier = std::format("Frame{}", imageIndex);
#endif

			/* Create the frame N color buffer, actually only the image view. */
			if ( !this->createColorBuffer(swapChainImages[imageIndex], frame.colorImage, frame.colorImageView, identifier) )
			{
				TraceFatal{ClassId} << "Unable to create the color buffer #" << imageIndex << " !";

				return false;
			}

			if ( !requestDepthStencilBuffer )
			{
				continue;
			}

			/* Create the frame N depth/stencil buffer. */
			if ( !this->createDepthStencilBuffer(this->device(), frame.depthStencilImage, frame.depthImageView, frame.stencilImageView, identifier) )
			{
				TraceFatal{ClassId} << "Unable to create the depth buffer #" << imageIndex << " !";

				return false;
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
#if IS_MACOS
			frame.framebuffer->setIdentifier(ClassId, (std::stringstream{} << "Frame" << imageIndex).str(), "Framebuffer");
#else
			frame.framebuffer->setIdentifier(ClassId, std::format("Frame{}", imageIndex), "Framebuffer");
#endif

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
					.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
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
	SwapChain::createFramebuffer () noexcept
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
		if ( !this->createFramebufferArray(this->createRenderPass(m_renderer)) )
		{
			Tracer::error(ClassId, "Unable to create the swap chain framebuffer !");

			return false;
		}

		return true;
	}

	bool
	SwapChain::createColorBuffer (const VkImage & swapChainImage, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & identifier) const noexcept
	{
		const auto instanceId = identifier + "ColorBuffer";

		/* NOTE: Create an image from existing data from the swap chain. */
		image = Image::createFromSwapChain(this->device(), swapChainImage, m_createInfo);
		image->setIdentifier(ClassId, instanceId, "Image");

		if ( swapChainImage != image->handle() )
		{
			TraceFatal{ClassId} << "Unable to create image '" << instanceId << "' !";

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
		imageView->setIdentifier(ClassId, instanceId, "ImageView");

		if ( !imageView->createOnHardware() )
		{
			TraceFatal{ClassId} << "Unable to create image view '" << instanceId << "' !";

			return false;
		}

		return true;
	}

	bool
	SwapChain::createDepthStencilBuffer (const std::shared_ptr< Device > & device, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & depthImageView, std::shared_ptr< ImageView > & stencilImageView, const std::string & identifier) noexcept
	{
		/* Create the depth/stencil buffer. */
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
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_TILING_OPTIMAL
		);
		image->setIdentifier(ClassId, identifier + "DepthStencilBuffer", "Image");

		if ( !image->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create image '" << identifier << "DepthStencilBuffer' !";

			return false;
		}

		const auto & imageCreateInfo = image->createInfo();

		if ( this->precisions().depthBits() > 0 )
		{
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
			depthImageView->setIdentifier(ClassId, identifier + "DepthBuffer", "ImageView");

			if ( !depthImageView->createOnHardware() )
			{
				TraceFatal{ClassId} << "Unable to create image view '" << identifier << "DepthBuffer' !";

				return false;
			}
		}

		if ( this->precisions().stencilBits() > 0 )
		{
			stencilImageView = std::make_shared< ImageView >(
				image,
				VK_IMAGE_VIEW_TYPE_2D,
				VkImageSubresourceRange{
					.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
					.baseMipLevel = 0,
					.levelCount = imageCreateInfo.mipLevels,
					.baseArrayLayer = 0,
					.layerCount = imageCreateInfo.arrayLayers
				}
			);
			stencilImageView->setIdentifier(ClassId, identifier + "StencilBuffer", "ImageView");

			if ( !stencilImageView->createOnHardware() )
			{
				TraceFatal{ClassId} << "Unable to create image view '" << identifier << "StencilBuffer' !";

				return false;
			}
		}

		return true;
	}

	std::optional< uint32_t >
	SwapChain::acquireNextImage (const Sync::Semaphore * imageAvailableSemaphore, uint64_t timeout) noexcept
	{
		if ( m_status != Status::Ready )
		{
			return std::nullopt;
		}

		const auto result = vkAcquireNextImageKHR(
			this->device()->handle(),
			m_handle,
			timeout,
			imageAvailableSemaphore->handle(),
			VK_NULL_HANDLE,
			&m_acquiredImageIndex
		);

		switch ( result )
		{
			case VK_SUCCESS :
				return m_acquiredImageIndex;

			case VK_TIMEOUT :
				TraceWarning{ClassId} << "VK_TIMEOUT at image acquisition!";

				return std::nullopt;

			case VK_ERROR_OUT_OF_DATE_KHR :
			case VK_SUBOPTIMAL_KHR :
				TraceError{ClassId} << vkResultToCString(result) << " at image acquisition!";

				m_status = Status::Degraded;

				return std::nullopt;

			default:
				TraceError{ClassId} << "Error from the swap chain : " << vkResultToCString(result) << " !";

				return std::nullopt;
		}
	}

	bool
	SwapChain::submitCommandBuffer (const std::shared_ptr< CommandBuffer > & commandBuffer, const uint32_t & imageIndex, const StaticVector< VkSemaphore, 16 > & callerWaitSemaphores, const Sync::Semaphore * renderFinishedSemaphore, const Sync::Fence * inFlightFence) noexcept
	{
		if ( m_status != Status::Ready )
		{
			return false;
		}

		auto signalSemaphoreHandle = renderFinishedSemaphore->handle();

		/* Submit */
		{
			const StaticVector< VkPipelineStageFlags, 16 > waitStages(callerWaitSemaphores.size(), VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

			const auto submitted = this->device()
				->getGraphicsQueue(QueuePriority::High)
				->submit(
					*commandBuffer,
					SynchInfo{}
						.waits(callerWaitSemaphores, waitStages)
						.signals({&signalSemaphoreHandle, 1})
						.withFence(inFlightFence->handle())
				);

			if ( !submitted )
			{
				return false;
			}
		}

		/* Present */
		{
			VkPresentInfoKHR presentInfo{};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.pNext = nullptr;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &signalSemaphoreHandle;
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &m_handle;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;

			bool swapChainRecreationNeeded = false;

			if ( !this->device()->getGraphicsQueue(QueuePriority::High)->present(&presentInfo, swapChainRecreationNeeded) )
			{
				if ( swapChainRecreationNeeded )
				{
					m_status = Status::Degraded;
				}

				return false;
			}
		}

		return true;
	}

	VkPresentModeKHR
	SwapChain::choosePresentMode () const noexcept
	{
		const auto & presentModes = m_renderer.window().surface()->presentModes();

		if ( m_showInformation )
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

		if ( m_VSyncEnabled )
		{
			if ( std::ranges::find(presentModes, VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.cend() )
			{
				if ( m_showInformation )
				{
					TraceInfo{ClassId} << "The swap chain will use MAILBOX as presentation mode.";
				}

				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}
		else if ( m_tripleBufferingEnabled )
		{
			if ( std::ranges::find(presentModes, VK_PRESENT_MODE_FIFO_RELAXED_KHR) != presentModes.cend() )
			{
				if ( m_showInformation )
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
				if ( m_showInformation )
				{
					TraceInfo{ClassId} << "The swap chain will use IMMEDIATE as presentation mode.";
				}

				return VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} << "The swap chain will use FIFO as presentation mode.";
		}

		/* Default presentation mode (Always available). */
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	void
	SwapChain::onInputDeviceConnected (AVConsole::AVManagers & managers, AbstractVirtualDevice * /*sourceDevice*/) noexcept
	{
		if ( !m_viewMatrices.create(managers.graphicsRenderer, this->id()) )
		{
			Tracer::error(ClassId, "Unable to create the view matrices on source device connexion !");
		}
	}

	void
	SwapChain::onInputDeviceDisconnected (AVConsole::AVManagers & /*managers*/, AbstractVirtualDevice * /*sourceDevice*/) noexcept
	{
		m_viewMatrices.destroy();
	}

	void
	SwapChain::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
	}

	void
	SwapChain::updateProperties () noexcept
	{
		const auto & extent = this->extent();
		const auto width = static_cast< float >(extent.width);
		const auto height = static_cast< float >(extent.height);

		if ( m_isPerspectiveProjection )
		{
			m_viewMatrices.updatePerspectiveViewProperties(width, height, m_distance, m_fovOrNear);
		}
		else
		{
			m_viewMatrices.updateOrthographicViewProperties(width, height, m_distance, m_fovOrNear);
		}
	}
}
