/*
 * src/Vulkan/SwapChain.cpp
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

#include "SwapChain.hpp"

/* Application configuration. */
#include "emeraude_config.hpp"

/* STL inclusions. */
#if !IS_MACOS
#include <format>
#endif

/* Local inclusions. */
#include "Libs/PixelFactory/Processor.hpp"
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
	using namespace Libs::PixelFactory;
	using namespace Graphics;

	SwapChain::SwapChain (Renderer & renderer, Settings & settings, bool showInformation) noexcept
		: AbstractDeviceDependentObject{renderer.device()},
		Abstract{
			ClassId,
			{renderer.device(), settings},
			{},
			settings.getOrSetDefault< float >(GraphicsViewDistanceKey, DefaultGraphicsViewDistance),
			RenderTargetType::View,
			Scenes::AVConsole::ConnexionType::Input,
			false,
			false
		},
		m_renderer{renderer},
		m_showInformation{showInformation},
		m_tripleBufferingEnabled{settings.getOrSetDefault< bool >(VideoEnableTripleBufferingKey, DefaultVideoEnableTripleBuffering)},
		m_VSyncEnabled{settings.getOrSetDefault< bool >(VideoEnableVSyncKey, DefaultVideoEnableVSync)},
		m_sRGBEnabled{settings.getOrSetDefault< bool >(VideoEnableSRGBKey, DefaultEnableSRGB)}
	{
		m_renderer.window().surface()->update(renderer.device());
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
		m_createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT; /* USAGE_TRANSFER_SRC enable the screenshot capabilities. FIXME: check for performances. */
		m_createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; /* NOTE: Graphics and presentation (99.9%) are from the same family. */
		m_createInfo.queueFamilyIndexCount = 0;
		m_createInfo.pQueueFamilyIndices = nullptr;
		m_createInfo.preTransform = capabilities.currentTransform; /* NOTE: No transformation. Check "supportedTransforms" in "capabilities" */
		m_createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		m_createInfo.presentMode = this->choosePresentMode();
		m_createInfo.clipped = VK_TRUE;
		m_createInfo.oldSwapchain = oldSwapChain;

		VkResult result = vkCreateSwapchainKHR(this->device()->handle(), &m_createInfo, nullptr, &m_handle);

		/* NOTE: Destroy the previous swap chain if exists. */
		if ( m_createInfo.oldSwapchain != VK_NULL_HANDLE )
		{
			vkDestroySwapchainKHR(this->device()->handle(), m_createInfo.oldSwapchain, nullptr);
		}

		if ( result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to create the swap-chain : " << vkResultToCString(result) << " !";

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
		auto & window = renderer.window();

		if ( !this->hasDevice() || window.surface() == nullptr )
		{
			Tracer::fatal(ClassId, "No device or window surface to create the swap-chain !");

			return false;
		}

		TraceDebug{ClassId} << "Application swap-chain creation." "\n" << this->precisions() << '\n';

		m_status = Status::UnderConstruction;

		if ( !this->createBaseSwapChain(window) )
		{
			Tracer::error(ClassId, "Unable to create the base of the swap-chain !");

			m_status = Status::Failure;

			return false;
		}

		if ( !this->prepareFrameData() )
		{
			Tracer::error(ClassId, "Unable to prepare data to complete the swap-chain !");

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
			TraceError{ClassId} << "No device to destroy the swap-chain " << m_handle << " (" << this->identifier() << ") !";

			return;
		}

		m_status = Status::Uninitialized;

		this->destroyFramebuffer();

		this->destroyBaseSwapChain();

		this->setDestroyed();
	}

	bool
	SwapChain::recreate () noexcept
	{
		/* The old framebuffer must be throw away. */
		this->resetFramebuffer();

		/* Prepare a new swap-chain. */
		m_status = Status::UnderConstruction;

		/* The base swap-chain needs to re-analyze the system surface. */
		if ( !this->createBaseSwapChain(m_renderer.window(), m_handle) )
		{
			Tracer::error(ClassId, "Unable to recreate the base of the swap-chain !");

			return false;
		}

		/* Now we are fine to rebuild the new framebuffer. */
		if ( !this->createFramebuffer() )
		{
			Tracer::error(ClassId, "Unable to complete the framebuffer !");

			return false;
		}

		/* This will rework the view-related matrices. */
		this->updateViewProperties();

		m_status = Status::Ready;

		return true;
	}

	bool
	SwapChain::fullRecreate (bool useNativeCode) noexcept
	{
		/* Destroy the old framebuffer. */
		this->resetFramebuffer();

		m_status = Status::UnderConstruction;

		/* Destroy the current swap-chain completely. */
		if ( m_handle != VK_NULL_HANDLE )
		{
			vkDestroySwapchainKHR(this->device()->handle(), m_handle, nullptr);

			m_handle = VK_NULL_HANDLE;
		}

		/* Get the window reference. */
		auto & window = m_renderer.window();

		/* Destroy and recreate the Vulkan surface using native Win32 API. */
		if ( !window.recreateSurface(useNativeCode) )
		{
			Tracer::error(ClassId, "Unable to recreate the Vulkan surface !");

			return false;
		}

		/* Update surface capabilities with the new surface. */
		if ( !window.surface()->update(this->device()) )
		{
			Tracer::error(ClassId, "Unable to update the new surface properties !");

			return false;
		}

		/* Create a brand new swap-chain (no oldSwapchain). */
		if ( !this->createBaseSwapChain(window, VK_NULL_HANDLE) )
		{
			Tracer::error(ClassId, "Unable to create the new swap-chain !");

			return false;
		}

		/* Rebuild the framebuffer. */
		if ( !this->createFramebuffer() )
		{
			Tracer::error(ClassId, "Unable to complete the framebuffer !");

			return false;
		}

		/* Update view-related matrices. */
		this->updateViewProperties();

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
			if ( frame.framebuffer != nullptr )
			{
				frame.framebuffer->destroyFromHardware();
			}

			/* Clear the MSAA specific resources. */
			if ( this->isMultisamplingEnabled() )
			{
				/* Clear MSAA color buffer. */
				if ( frame.MSAAColorImageView != nullptr )
				{
					frame.MSAAColorImageView->destroyFromHardware();
				}

				if ( frame.MSAAColorImage != nullptr )
				{
					frame.MSAAColorImage->destroyFromHardware();
				}

				/* Clear MSAA depth+stencil buffer. */
				if ( frame.MSAADepthImageView != nullptr )
				{
					frame.MSAADepthImageView->destroyFromHardware();
				}

				if ( frame.MSAAStencilImageView != nullptr )
				{
					frame.MSAAStencilImageView->destroyFromHardware();
				}

				if ( frame.MSAADepthStencilImage != nullptr )
				{
					frame.MSAADepthStencilImage->destroyFromHardware();
				}
			}

			/* Clear the color buffer. */
			if ( frame.colorImageView != nullptr )
			{
				frame.colorImageView->destroyFromHardware();
			}

			if ( frame.colorImage != nullptr )
			{
				frame.colorImage->destroyFromHardware();
			}

			/* Clear the depth+stencil buffer. */
			if ( frame.stencilImageView != nullptr )
			{
				frame.stencilImageView->destroyFromHardware();
			}

			if ( frame.depthImageView != nullptr )
			{
				frame.depthImageView->destroyFromHardware();
			}

			if ( frame.depthStencilImage != nullptr )
			{
				frame.depthStencilImage->destroyFromHardware();
			}
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
			Tracer::error(ClassId, "The swap-chain can only use 1 image. Disabling double buffering and V-Sync !");

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

		/*TraceDebug{ClassId} <<
			"Vulkan minimum extent detected : " << capabilities.minImageExtent.width << 'X' << capabilities.minImageExtent.height << "\n"
			"Vulkan maximum extent detected : " << capabilities.maxImageExtent.width << 'X' << capabilities.maxImageExtent.height << "\n"
			"Vulkan current extent detected : " << capabilities.currentExtent.width << 'X' << capabilities.currentExtent.height << "\n"
			"GLFW framebuffer : " << framebufferSize.at(0) << 'X' << framebufferSize.at(1);*/

		return {
			std::clamp(framebufferSize[0], capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp(framebufferSize[1], capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

	VkSurfaceFormatKHR
	SwapChain::chooseSurfaceFormat () const noexcept
	{
		const auto & formats = m_renderer.window().surface()->formats();

		/* NOTE: Two modes are available:
		 * - SRGB (m_sRGBEnabled = true): For native 3D rendering with linear lighting.
		 *   The GPU automatically converts linear→sRGB on write.
		 * - UNORM (m_sRGBEnabled = false): For pre-gamma-corrected content (like CEF overlays).
		 *   Values are stored as-is without automatic conversion. */
		const VkFormat targetFormat = m_sRGBEnabled ? VK_FORMAT_B8G8R8A8_SRGB : VK_FORMAT_B8G8R8A8_UNORM;
		const char * formatName = m_sRGBEnabled ? "SRGB" : "UNORM";

		const auto formatIt = std::ranges::find_if(formats, [targetFormat] (auto item) {
			return item.format == targetFormat && item.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		});

		if ( formatIt == formats.cend() )
		{
			const auto & fallback = formats.at(0);

			TraceWarning{ClassId} <<
				"The " << formatName << " surface format (VK_FORMAT_B8G8R8A8_" << formatName << ") is not available! "
				"Falling back to format " << fallback.format << " with color space " << fallback.colorSpace << ". "
				"This may cause incorrect color rendering.";

			return fallback;
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} << "The swap-chain will use " << formatName << " surface format (VK_FORMAT_B8G8R8A8_" << formatName << ").";
		}

		return *formatIt;
	}

	bool
	SwapChain::prepareFrameData () noexcept
	{
		/* NOTE: Will set the image count for the swap-chain. */
		if ( const auto result = vkGetSwapchainImagesKHR(this->device()->handle(), m_handle, &m_imageCount, nullptr); result != VK_SUCCESS )
		{
			TraceFatal{ClassId} << "Unable to get image count from the swap-chain : " << vkResultToCString(result) << " !";

			return false;
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} << "The swap-chain will use " << m_imageCount << " images.";
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
			TraceFatal{ClassId} << "Unable to get images from the swap-chain : " << vkResultToCString(result) << " !";

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

			/* Create the frame N color buffer (swap-chain image, used as resolve target when MSAA is enabled). */
			if ( !this->createColorBuffer(swapChainImages[imageIndex], frame.colorImage, frame.colorImageView, identifier) )
			{
				TraceFatal{ClassId} << "Unable to create the color buffer #" << imageIndex << " !";

				return false;
			}

			/* Create MSAA color buffer if multisampling is enabled. */
			if ( this->isMultisamplingEnabled() )
			{
				const auto msaaIdentifier = identifier + "MSAAColorBuffer";

				/* Create MSAA color image. */
				frame.MSAAColorImage = std::make_shared< Image >(
					this->device(),
					VK_IMAGE_TYPE_2D,
					m_createInfo.imageFormat,
					VkExtent3D{this->extent().width, this->extent().height, 1},
					VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
					0,
					1,
					1,
					Device::getSampleCountFlag(this->precisions().samples())
				);
				frame.MSAAColorImage->setIdentifier(ClassId, msaaIdentifier, "Image");

				if ( !frame.MSAAColorImage->createOnHardware() )
				{
					TraceFatal{ClassId} << "Unable to create MSAA color image #" << imageIndex << " !";

					return false;
				}

				/* Create MSAA color image view. */
				const auto & imageCreateInfo = frame.MSAAColorImage->createInfo();

				frame.MSAAColorImageView = std::make_shared< ImageView >(
					frame.MSAAColorImage,
					VK_IMAGE_VIEW_TYPE_2D,
					VkImageSubresourceRange{
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = imageCreateInfo.mipLevels,
						.baseArrayLayer = 0,
						.layerCount = imageCreateInfo.arrayLayers
					}
				);
				frame.MSAAColorImageView->setIdentifier(ClassId, msaaIdentifier, "ImageView");

				if ( !frame.MSAAColorImageView->createOnHardware() )
				{
					TraceFatal{ClassId} << "Unable to create MSAA color image view #" << imageIndex << " !";

					return false;
				}
			}

			if ( !requestDepthStencilBuffer )
			{
				continue;
			}

			/* Create the frame N depth/stencil buffer (used as resolve target when MSAA is enabled). */
			if ( !this->createDepthStencilBuffer(this->device(), frame.depthStencilImage, frame.depthImageView, frame.stencilImageView, identifier) )
			{
				TraceFatal{ClassId} << "Unable to create the depth buffer #" << imageIndex << " !";

				return false;
			}

			/* Create MSAA depth/stencil buffer if multisampling is enabled. */
			if ( this->isMultisamplingEnabled() )
			{
				const auto msaaIdentifier = identifier + "MSAADepthStencilBuffer";

				/* Create MSAA depth/stencil image. */
				frame.MSAADepthStencilImage = std::make_shared< Image >(
					this->device(),
					VK_IMAGE_TYPE_2D,
					Instance::findDepthStencilFormat(this->device(), this->precisions()),
					VkExtent3D{this->extent().width, this->extent().height, 1},
					VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
					0,
					1,
					1,
					Device::getSampleCountFlag(this->precisions().samples())
				);
				frame.MSAADepthStencilImage->setIdentifier(ClassId, msaaIdentifier, "Image");

				if ( !frame.MSAADepthStencilImage->createOnHardware() )
				{
					TraceFatal{ClassId} << "Unable to create MSAA depth/stencil image #" << imageIndex << " !";

					return false;
				}

				/* Create MSAA depth image view. */
				const auto & imageCreateInfo = frame.MSAADepthStencilImage->createInfo();

				if ( this->precisions().depthBits() > 0 )
				{
					frame.MSAADepthImageView = std::make_shared< ImageView >(
						frame.MSAADepthStencilImage,
						VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT,
							.baseMipLevel = 0,
							.levelCount = imageCreateInfo.mipLevels,
							.baseArrayLayer = 0,
							.layerCount = imageCreateInfo.arrayLayers
						}
					);
					frame.MSAADepthImageView->setIdentifier(ClassId, msaaIdentifier + "Depth", "ImageView");

					if ( !frame.MSAADepthImageView->createOnHardware() )
					{
						TraceFatal{ClassId} << "Unable to create MSAA depth image view #" << imageIndex << " !";

						return false;
					}
				}

				/* Create MSAA stencil image view if requested. */
				if ( this->precisions().stencilBits() > 0 )
				{
					frame.MSAAStencilImageView = std::make_shared< ImageView >(
						frame.MSAADepthStencilImage,
						VK_IMAGE_VIEW_TYPE_2D,
						VkImageSubresourceRange{
							.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT,
							.baseMipLevel = 0,
							.levelCount = imageCreateInfo.mipLevels,
							.baseArrayLayer = 0,
							.layerCount = imageCreateInfo.arrayLayers
						}
					);
					frame.MSAAStencilImageView->setIdentifier(ClassId, msaaIdentifier + "Stencil", "ImageView");

					if ( !frame.MSAAStencilImageView->createOnHardware() )
					{
						TraceFatal{ClassId} << "Unable to create MSAA stencil image view #" << imageIndex << " !";

						return false;
					}
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
#if IS_MACOS
			frame.framebuffer->setIdentifier(ClassId, (std::stringstream{} << "Frame" << imageIndex).str(), "Framebuffer");
#else
			frame.framebuffer->setIdentifier(ClassId, std::format("Frame{}", imageIndex), "Framebuffer");
#endif

			if ( this->isMultisamplingEnabled() )
			{
				/* MSAA mode: 4 attachments in order. */

				/* Attachment 0: MSAA Color buffer. */
				frame.framebuffer->addAttachment(frame.MSAAColorImageView->handle());

				/* Attachment 1: MSAA Depth/Stencil buffer. */
				frame.framebuffer->addAttachment(frame.MSAADepthImageView->handle());

				/* Attachment 2: Color Resolve buffer (swap-chain image). */
				frame.framebuffer->addAttachment(frame.colorImageView->handle());

				/* Attachment 3: Depth/Stencil Resolve buffer. */
				frame.framebuffer->addAttachment(frame.depthImageView->handle());
			}
			else
			{
				/* Standard mode: 2 attachments. */

				/* Attachment 0: Color buffer (swap-chain image). */
				frame.framebuffer->addAttachment(frame.colorImageView->handle());

				/* Attachment 1: Depth/Stencil buffer. */
				frame.framebuffer->addAttachment(frame.depthImageView->handle());
			}

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
		/* Create a new RenderPass for this swap chain. */
		auto renderPass = std::make_shared< RenderPass >(renderer.device(), 0);
		renderPass->setIdentifier(ClassId, "SwapChain", "RenderPass");

		/* Prepare a subpass for the render pass. */
		RenderSubPass subPass{VK_PIPELINE_BIND_POINT_GRAPHICS, 0};

		if ( this->isMultisamplingEnabled() )
		{
			/* MSAA rendering: render to MSAA attachments, then resolve to swap-chain images. */

			/* Attachment 0: MSAA Color buffer (multisampled). */
			{
				const auto & msaaColorBufferCreateInfo = m_frames.front().MSAAColorImage->createInfo();

				renderPass->addAttachmentDescription(VkAttachmentDescription{
					.flags = 0,
					.format = msaaColorBufferCreateInfo.format,
					.samples = msaaColorBufferCreateInfo.samples,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, /* Don't need to store MSAA buffer, only the resolved one. */
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
				});

				subPass.addColorAttachment(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}

			/* Attachment 1: MSAA Depth/Stencil buffer (multisampled). */
			{
				const auto & msaaDepthStencilBufferCreateInfo = m_frames.front().MSAADepthStencilImage->createInfo();

				renderPass->addAttachmentDescription(VkAttachmentDescription{
					.flags = 0,
					.format = msaaDepthStencilBufferCreateInfo.format,
					.samples = msaaDepthStencilBufferCreateInfo.samples,
					.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
					.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, /* Don't need to store MSAA buffer. */
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
				});

				subPass.setDepthStencilAttachment(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
			}

			/* Attachment 2: Color Resolve buffer (swap-chain image, single sample). */
			{
				const auto & colorBufferCreateInfo = m_frames.front().colorImage->createInfo();

				renderPass->addAttachmentDescription(VkAttachmentDescription{
					.flags = 0,
					.format = colorBufferCreateInfo.format,
					.samples = colorBufferCreateInfo.samples,
					.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE, /* We don't care about the previous content. */
					.storeOp = VK_ATTACHMENT_STORE_OP_STORE, /* Store the resolved result. */
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR /* Ready for presentation. */
				});

				subPass.addResolveAttachment(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
			}

			/* Attachment 3: Depth/Stencil Resolve buffer (single sample) - Optional but included for completeness. */
			{
				const auto & depthStencilBufferCreateInfo = m_frames.front().depthStencilImage->createInfo();

				renderPass->addAttachmentDescription(VkAttachmentDescription{
					.flags = 0,
					.format = depthStencilBufferCreateInfo.format,
					.samples = depthStencilBufferCreateInfo.samples,
					.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE, /* Usually don't need the resolved depth. */
					.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
					.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
					.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
					.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
				});
			}
		}
		else
		{
			/* Standard rendering without MSAA. */

			/* Attachment 0: Color buffer (swap-chain image). */
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

			/* Attachment 1: Depth/Stencil buffer. */
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
		}

		renderPass->addSubPass(subPass);

		if ( !renderPass->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create a render pass !");

			return nullptr;
		}

		return renderPass;
	}

	bool
	SwapChain::createFramebuffer () noexcept
	{
		if ( m_imageCount == 0 )
		{
			Tracer::error(ClassId, "No image count to create the swap-chain !");

			return false;
		}

		if ( !this->createImageArray() )
		{
			Tracer::error(ClassId, "Unable to create the swap-chain images !");

			return false;
		}

		/* Create the render pass base on the first set of images (this is the swap-chain, all images are technically the same) */
		if ( !this->createFramebufferArray(this->createRenderPass(m_renderer)) )
		{
			Tracer::error(ClassId, "Unable to create the swap-chain framebuffer !");

			return false;
		}

		return true;
	}

	bool
	SwapChain::createColorBuffer (const VkImage & swapChainImage, std::shared_ptr< Image > & image, std::shared_ptr< ImageView > & imageView, const std::string & identifier) const noexcept
	{
		const auto instanceId = identifier + "ColorBuffer";

		/* NOTE: Create an image from existing data from the swap-chain. */
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
		image = std::make_shared< Vulkan::Image >(
			device,
			VK_IMAGE_TYPE_2D,
			Instance::findDepthStencilFormat(device, this->precisions()),
			this->extent(),
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
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
			/* NOTE: These codes below are considered as a success. */
			case VK_SUCCESS :
				return m_acquiredImageIndex;

			case VK_NOT_READY :
				Tracer::warning(ClassId, "The swap-chain is not ready!");

				m_status = Status::Uninitialized;

				return std::nullopt;

			case VK_SUBOPTIMAL_KHR :
				Tracer::debug(ClassId, "vkAcquireNextImageKHR() detected the swap-chain is 'sub-optimal'! [SWAP-CHAIN-RECREATION-PLANNED]");

				m_status = Status::Degraded;

				return m_acquiredImageIndex;

			case VK_TIMEOUT :
				TraceWarning{ClassId} << "The acquisition of the next image was canceled by the " << timeout << " ns timeout!";

				return std::nullopt;

			/* NOTE: These codes below are considered as an error. */
			case VK_ERROR_OUT_OF_DATE_KHR :
				Tracer::debug(ClassId, "vkAcquireNextImageKHR() detected the swap-chain is 'out of date'! [SWAP-CHAIN-RECREATION-PLANNED]");

				m_status = Status::Degraded;

				return std::nullopt;

			case VK_ERROR_DEVICE_LOST :
			case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT :
			case VK_ERROR_OUT_OF_DEVICE_MEMORY :
			case VK_ERROR_OUT_OF_HOST_MEMORY :
			case VK_ERROR_SURFACE_LOST_KHR :
			case VK_ERROR_UNKNOWN :
			case VK_ERROR_VALIDATION_FAILED_EXT :
			default:
				TraceError{ClassId} << "Error from the swap-chain : " << vkResultToCString(result) << " !";

				m_status = Status::Failure;

				return std::nullopt;
		}
	}

	void
	SwapChain::present (const uint32_t & imageIndex, const Queue * queue, VkSemaphore renderFinishedSemaphore) noexcept
	{
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pNext = nullptr;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &m_handle;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		queue->present(&presentInfo, m_status);
	}

	VkPresentModeKHR
	SwapChain::choosePresentMode () const noexcept
	{
		/*
		 * Present Mode Selection Strategy (Windows, Linux, macOS)
		 *
		 * Available modes and their characteristics:
		 * ┌─────────────────────┬─────────┬──────────┬─────────┬──────────────────────────────────┐
		 * │ Mode				│ VSync   │ Blocking │ Tearing │ Notes							│
		 * ├─────────────────────┼─────────┼──────────┼─────────┼──────────────────────────────────┤
		 * │ IMMEDIATE		   │ No	  │ No	   │ Yes	 │ Lowest latency, may tear		 │
		 * │ MAILBOX			 │ Yes	 │ No	   │ No	  │ Triple-buffer, best for games	│
		 * │ FIFO				│ Yes	 │ Yes	  │ No	  │ Always available, classic vsync  │
		 * │ FIFO_RELAXED		│ Partial │ Partial  │ If late │ Vsync but allows late present	│
		 * └─────────────────────┴─────────┴──────────┴─────────┴──────────────────────────────────┘
		 *
		 * Selection matrix based on user preferences:
		 * ┌───────────┬────────────────┬────────────────────────────────────────────────────────┐
		 * │ VSync	 │ Triple-Buffer  │ Priority order										 │
		 * ├───────────┼────────────────┼────────────────────────────────────────────────────────┤
		 * │ ON		│ ON			 │ MAILBOX > FIFO_RELAXED > FIFO						  │
		 * │ ON		│ OFF			│ FIFO (standard double-buffered vsync)				  │
		 * │ OFF	   │ ON			 │ IMMEDIATE > MAILBOX > FIFO_RELAXED > FIFO			  │
		 * │ OFF	   │ OFF			│ IMMEDIATE > FIFO_RELAXED > FIFO						│
		 * └───────────┴────────────────┴────────────────────────────────────────────────────────┘
		 *
		 * Platform notes:
		 * - Windows:  MAILBOX widely supported on modern GPUs.
		 * - Linux:	MAILBOX often unavailable (Mesa drivers). FIFO_RELAXED is a good fallback.
		 * - macOS:	Limited mode support through MoltenVK, FIFO typically used.
		 */

		const auto & presentModes = m_renderer.window().surface()->presentModes();

		/* Helper lambda to check mode availability. */
		auto isAvailable = [&presentModes](VkPresentModeKHR mode) -> bool {
			return std::ranges::find(presentModes, mode) != presentModes.cend();
		};

		/* Helper lambda to get mode name for logging. */
		auto getModeName = [](VkPresentModeKHR mode) -> const char * {
			switch ( mode )
			{
				case VK_PRESENT_MODE_IMMEDIATE_KHR :
					return "IMMEDIATE";

				case VK_PRESENT_MODE_MAILBOX_KHR :
					return "MAILBOX";

				case VK_PRESENT_MODE_FIFO_KHR :
					return "FIFO";

				case VK_PRESENT_MODE_FIFO_RELAXED_KHR :
					return "FIFO_RELAXED";

				case VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR :
					return "SHARED_DEMAND_REFRESH";

				case VK_PRESENT_MODE_SHARED_CONTINUOUS_REFRESH_KHR :
					return "SHARED_CONTINUOUS_REFRESH";

				default :
					return "UNKNOWN";
			}
		};

		/* Log available modes. */
		if ( m_showInformation )
		{
			std::stringstream info;
			info << "Present modes available:" "\n";

			for ( const auto presentMode : presentModes )
			{
				info << " - VK_PRESENT_MODE_" << getModeName(presentMode) << "_KHR" "\n";
			}

			TraceInfo{ClassId} << info.str();
		}

		/* Select optimal present mode based on user preferences. */
		VkPresentModeKHR selectedMode = VK_PRESENT_MODE_FIFO_KHR;
		auto selectionReason = "default fallback (always available)";

		if ( m_VSyncEnabled )
		{
			if ( m_tripleBufferingEnabled )
			{
				/* VSync ON + Triple-buffer ON:
				 * User wants smooth vsync without input lag.
				 * MAILBOX is ideal: presents at vsync but doesn't block if ahead.
				 * FIFO_RELAXED is acceptable: allows tearing only if late, avoiding latency buildup. */
				if ( isAvailable(VK_PRESENT_MODE_MAILBOX_KHR) )
				{
					selectedMode = VK_PRESENT_MODE_MAILBOX_KHR;
					selectionReason = "vsync + triple-buffer (optimal)";
				}
				else if ( isAvailable(VK_PRESENT_MODE_FIFO_RELAXED_KHR) )
				{
					selectedMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
					selectionReason = "vsync + triple-buffer (MAILBOX unavailable)";
				}
				else
				{
					selectionReason = "vsync + triple-buffer (only FIFO available)";
				}
			}
			else
			{
				/* VSync ON + Triple-buffer OFF:
				 * User wants classic double-buffered vsync.
				 * FIFO is the standard choice. */
				selectionReason = "vsync without triple-buffer (classic double-buffered)";
			}
		}
		else
		{
			if ( m_tripleBufferingEnabled )
			{
				/* VSync OFF + Triple-buffer ON:
				 * User wants lowest latency but with triple-buffer to smooth frame times.
				 * IMMEDIATE is best for latency.
				 * MAILBOX provides smooth frame pacing with vsync (acceptable compromise).
				 * FIFO_RELAXED allows uncapped presentation when ahead. */
				if ( isAvailable(VK_PRESENT_MODE_IMMEDIATE_KHR) )
				{
					selectedMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
					selectionReason = "no vsync + triple-buffer (lowest latency)";
				}
				else if ( isAvailable(VK_PRESENT_MODE_MAILBOX_KHR) )
				{
					selectedMode = VK_PRESENT_MODE_MAILBOX_KHR;
					selectionReason = "no vsync + triple-buffer (IMMEDIATE unavailable)";
				}
				else if ( isAvailable(VK_PRESENT_MODE_FIFO_RELAXED_KHR) )
				{
					selectedMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
					selectionReason = "no vsync + triple-buffer (IMMEDIATE/MAILBOX unavailable)";
				}
				else
				{
					selectionReason = "no vsync + triple-buffer (only FIFO available, forced vsync)";
				}
			}
			else
			{
				/* VSync OFF + Triple-buffer OFF:
				 * User wants absolute minimum latency, accepts tearing.
				 * IMMEDIATE is the only true "no vsync" mode.
				 * FIFO_RELAXED is acceptable as it allows late presentation. */
				if ( isAvailable(VK_PRESENT_MODE_IMMEDIATE_KHR) )
				{
					selectedMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
					selectionReason = "no vsync, no triple-buffer (lowest latency)";
				}
				else if ( isAvailable(VK_PRESENT_MODE_FIFO_RELAXED_KHR) )
				{
					selectedMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
					selectionReason = "no vsync, no triple-buffer (IMMEDIATE unavailable)";
				}
				else
				{
					selectionReason = "no vsync, no triple-buffer (only FIFO available, forced vsync)";
				}
			}
		}

		if ( m_showInformation )
		{
			TraceInfo{ClassId} <<
				"Present mode selected: VK_PRESENT_MODE_" << getModeName(selectedMode) << "_KHR "
				"[" << selectionReason << "]";
		}

		return selectedMode;
	}

	void
	SwapChain::updateVideoDeviceProperties (float fovOrNear, float distanceOrFar, bool isOrthographicProjection) noexcept
	{
		m_isPerspectiveProjection = !isOrthographicProjection;

		this->updateViewRangesProperties(fovOrNear, distanceOrFar);
	}

	void
	SwapChain::updateViewRangesProperties (float fovOrNear, float distanceOrFar) noexcept
	{
		m_fovOrNear = fovOrNear;
		m_distanceOrFar = distanceOrFar;

		this->updateViewProperties();
	}

	void
	SwapChain::updateViewProperties () noexcept
	{
		const auto & extent = this->extent();
		const auto width = static_cast< float >(extent.width);
		const auto height = static_cast< float >(extent.height);

		if ( m_isPerspectiveProjection )
		{
			m_viewMatrices.updatePerspectiveViewProperties(width, height, m_fovOrNear, m_distanceOrFar);
		}
		else
		{
			m_viewMatrices.updateOrthographicViewProperties(width, height, m_fovOrNear, m_distanceOrFar);
		}

		this->setViewDistance(m_distanceOrFar);
	}

	void
	SwapChain::updateDeviceFromCoordinates (const CartesianFrame< float > & worldCoordinates, const Vector< 3, float > & worldVelocity) noexcept
	{
		m_worldCoordinates = worldCoordinates;
		m_viewMatrices.updateViewCoordinates(worldCoordinates, worldVelocity);
	}

	void
	SwapChain::onInputDeviceConnected (EngineContext & engineContext, AbstractVirtualDevice & /*sourceDevice*/) noexcept
	{
		if ( !m_viewMatrices.create(engineContext.graphicsRenderer, this->id()) )
		{
			Tracer::error(ClassId, "Unable to create the view matrices on source device connexion !");
		}
	}

	void
	SwapChain::onInputDeviceDisconnected (EngineContext & /*engineContext*/, AbstractVirtualDevice & /*sourceDevice*/) noexcept
	{
		m_viewMatrices.destroy();
	}

	std::array< Pixmap< uint8_t >, 3 >
	SwapChain::capture (TransferManager & transferManager, uint32_t layerIndex, bool keepAlpha, bool withDepthBuffer, bool withStencilBuffer) const noexcept
	{
		std::array< Pixmap< uint8_t >, 3 > result;

		/* SwapChain has only single-layer images (not cubemaps or arrays). */
		if ( layerIndex > 0 )
		{
			TraceWarning{ClassId} << "SwapChain does not support layered images. Layer " << layerIndex << " requested, using layer 0 instead.";
		}

		if ( m_acquiredImageIndex >= m_frames.size() )
		{
			TraceError{ClassId} << "Invalid acquired image index for capture!";

			return result;
		}

		const auto & frame = m_frames[m_acquiredImageIndex];

		/* Capture color buffer. */
		if ( frame.colorImage )
		{
			if ( !transferManager.downloadImage(*frame.colorImage, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_ASPECT_COLOR_BIT, result[0]) )
			{
				TraceError{ClassId} << "Failed to capture color buffer!";

				return result;
			}

			if ( !keepAlpha )
			{
				result[0] = Processor< uint8_t >::toRGB(result[0]);
			}
			result[0] = Processor< uint8_t >::swapChannels(result[0], false);
		}

		/* Capture depth buffer if requested and available. */
		if ( withDepthBuffer && frame.depthImageView )
		{
			if ( !transferManager.downloadImage(*frame.depthStencilImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT, result[1]) )
			{
				TraceWarning{ClassId} << "Failed to capture depth buffer!";
			}
		}

		/* Capture stencil buffer if requested and available. */
		if ( withStencilBuffer && frame.stencilImageView )
		{
			if ( !transferManager.downloadImage(*frame.depthStencilImage, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_STENCIL_BIT, result[2]) )
			{
				TraceWarning{ClassId} << "Failed to capture stencil buffer!";
			}
		}

		return result;
	}
}
