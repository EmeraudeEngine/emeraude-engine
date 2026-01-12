/*
 * src/Vulkan/TransferManager.cpp
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

#include "TransferManager.hpp"

/* Local inclusions. */
#include "Sync/ImageMemoryBarrier.hpp"
#include "Sync/Fence.hpp"
#include "CommandBuffer.hpp"
#include "Device.hpp"
#include "Buffer.hpp"
#include "Image.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;
	using namespace Libs::PixelFactory;

	bool
	TransferManager::onInitialize () noexcept
	{
		if ( m_device == nullptr || !m_device->isCreated() )
		{
			Tracer::error(ClassId, "No valid device set at startup !");

			return false;
		}

		m_transferCommandPool = std::make_shared< CommandPool >(m_device, m_device->getGraphicsTransferFamilyIndex(), true, true, false);
		m_transferCommandPool->setIdentifier(ClassId, "Transfer", "CommandPool");

		if ( !m_transferCommandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the transfer command pool !");

			m_transferCommandPool.reset();

			return false;
		}

		/* Read the queue configuration from the device. */
		if ( m_device->hasBasicSupport() )
		{
			m_imageLayoutTransitionCommandBuffer = std::make_unique< CommandBuffer >(m_transferCommandPool, true);
			m_imageLayoutTransitionCommandBuffer->setIdentifier(ClassId, "ImageLayoutTransition", "CommandBuffer");
		}
		else
		{
			m_graphicsCommandPool = std::make_shared< CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, true, false);
			m_graphicsCommandPool->setIdentifier(ClassId, "Specific", "CommandPool");

			if ( !m_graphicsCommandPool->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the specific command pool !");

				m_graphicsCommandPool.reset();

				return false;
			}

			m_imageLayoutTransitionCommandBuffer = std::make_unique< CommandBuffer >(m_graphicsCommandPool, true);
			m_imageLayoutTransitionCommandBuffer->setIdentifier(ClassId, "ImageLayoutTransition", "CommandBuffer");
		}

		m_imageLayoutTransitionFence = std::make_unique< Sync::Fence >(m_device);
		m_imageLayoutTransitionFence->setIdentifier(ClassId, "ImageLayoutTransition", "Fence");

		if ( !m_imageLayoutTransitionFence->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the image layout transition fence !");

			m_imageLayoutTransitionFence.reset();

			return false;
		}

		return true;
	}

	bool
	TransferManager::onTerminate () noexcept
	{
		m_device->waitIdle("TransferManager::onTerminate()");

		m_imageLayoutTransitionFence.reset();
		m_imageLayoutTransitionCommandBuffer.reset();

		m_bufferTransferOperations.clear();
		m_imageTransferOperations.clear();

		m_graphicsCommandPool.reset();
		m_transferCommandPool.reset();

		m_device.reset();

		return true;
	}

	bool
	TransferManager::transitionImageLayout (Image & image, VkImageAspectFlags aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout) const noexcept
	{
		const std::lock_guard< std::mutex > lock{m_transferOperationsAccess};

		if ( !m_imageLayoutTransitionFence->reset() )
		{
			TraceError{ClassId} << "Unable to reset the image layout transition fence for the image '" << image.identifier() << "' !";

			return false;
		}

		if ( !m_imageLayoutTransitionCommandBuffer->begin() )
		{
			TraceError{ClassId} << "Unable to begin the image layout transition command buffer for the image '" << image.identifier() << "' !";

			return false;
		}

		VkAccessFlags srcAccessMask = VK_IMAGE_ASPECT_NONE;
		VkAccessFlags dstAccessMask = VK_IMAGE_ASPECT_NONE;

		VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_NONE;
		VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_NONE;

		if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL )
		{
			srcAccessMask = VK_IMAGE_ASPECT_NONE;
			dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		{
			srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		{
			srcAccessMask = VK_IMAGE_ASPECT_NONE;
			dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else if ( oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
		{
			srcAccessMask = VK_IMAGE_ASPECT_NONE;
			dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
			TraceError{ClassId} << "Unsupported layout transition for the image '" << image.identifier() << "' !";

			return false;
		}

		const Sync::ImageMemoryBarrier barrier{image, srcAccessMask, dstAccessMask, oldLayout, newLayout, aspectMask};

		m_imageLayoutTransitionCommandBuffer->pipelineBarrier(barrier, sourceStage, destinationStage);

		if ( !m_imageLayoutTransitionCommandBuffer->end() )
		{
			TraceError{ClassId} << "Unable to end the image layout transition command buffer for the image '" << image.identifier() << "' !";

			return false;
		}

		const auto * queue = m_device->getGraphicsQueue(QueuePriority::High);

		if ( !queue->submit(*m_imageLayoutTransitionCommandBuffer, SynchInfo{}.withFence(m_imageLayoutTransitionFence->handle())) )
		{
			TraceError{ClassId} << "Unable to submit the image layout transition command buffer for the image '" << image.identifier() << "' !";

			return false;
		}

		if ( !m_imageLayoutTransitionFence->wait() )
		{
			TraceError{ClassId} << "Unable to wait the image layout transition command fence for the image '" << image.identifier() << "' !";

			return false;
		}

		image.setCurrentImageLayout(newLayout);

		return true;
	}

	bool
	TransferManager::downloadImage (const Image & sourceImage, VkImageLayout currentLayout, VkImageAspectFlags aspectMask, Pixmap< uint8_t > & pixmap) noexcept
	{
		/* [VULKAN-CPU-SYNC] Transfer from GPU (Abusive lock!) */
		const std::lock_guard< std::mutex > lock{m_transferOperationsAccess};

		if ( !this->usable() )
		{
			TraceError{ClassId} << "The transfer manager is not usable !";

			return false;
		}

		const auto & imageCreateInfo = sourceImage.createInfo();
		const auto extent = imageCreateInfo.extent;
		const auto format = imageCreateInfo.format;

		/* Calculate bytes per pixel based on format. */
		uint32_t bytesPerPixel;
		ChannelMode channelMode;

		switch ( format )
		{
			case VK_FORMAT_R8G8B8A8_UNORM:
			case VK_FORMAT_B8G8R8A8_UNORM:
				bytesPerPixel = 4;
				channelMode = ChannelMode::RGBA;
				break;

			case VK_FORMAT_R8G8B8_UNORM:
			case VK_FORMAT_B8G8R8_UNORM:
				bytesPerPixel = 3;
				channelMode = ChannelMode::RGB;
				break;

			case VK_FORMAT_D32_SFLOAT:
			case VK_FORMAT_D32_SFLOAT_S8_UINT:
				bytesPerPixel = 4; /* Depth as float32. */
				channelMode = ChannelMode::Grayscale;
				break;

			case VK_FORMAT_D24_UNORM_S8_UINT:
				bytesPerPixel = 4; /* Depth24 + Stencil8. */
				channelMode = ChannelMode::Grayscale;
				break;

			case VK_FORMAT_S8_UINT:
				bytesPerPixel = 1; /* Stencil only. */
				channelMode = ChannelMode::Grayscale;
				break;

			default:
				/* Default to RGBA8. */
				bytesPerPixel = 4;
				channelMode = ChannelMode::RGBA;
				break;
		}

		const size_t requiredBytes = extent.width * extent.height * bytesPerPixel;

		/* Check if source image supports direct transfer. */
		const bool sourceHasTransferSrc = (imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0;

		/* Get staging buffer from transfer operation. */
		const auto transferOperation = this->getAndReserveImageTransferOperation(requiredBytes);

		if ( transferOperation == nullptr )
		{
			return false;
		}

		auto * stagingBuffer = transferOperation->stagingBuffer();

		if ( stagingBuffer == nullptr )
		{
			return false;
		}

		/* Use graphics command pool to match the graphics queue family. */
		auto commandPool = m_graphicsCommandPool ? m_graphicsCommandPool : m_transferCommandPool;
		auto commandBuffer = std::make_unique< CommandBuffer >(commandPool, true);
		commandBuffer->setIdentifier(ClassId, "ImageDownload", "CommandBuffer");

		if ( !commandBuffer->begin() )
		{
			TraceError{ClassId} << "Unable to begin image download command buffer !";

			return false;
		}

		VkImage finalSourceImage = sourceImage.handle();
		VkImageLayout finalSourceLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

		/* Create intermediate image only if source doesn't have TRANSFER_SRC_BIT. */
		if ( !sourceHasTransferSrc )
		{
			/* Create intermediate image with proper usage flags. */
			auto intermediateImage = std::make_shared< Image >(
				m_device,
				VK_IMAGE_TYPE_2D,
				format,
				extent,
				VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT
			);
			intermediateImage->setIdentifier(ClassId, "ImageDownload", "IntermediateImage");

			if ( !intermediateImage->createOnHardware() )
			{
				TraceError{ClassId} << "Unable to create intermediate image for download !";

				return false;
			}

			/* Transition intermediate image to TRANSFER_DST_OPTIMAL. */
			{
				const Sync::ImageMemoryBarrier barrier{
					*intermediateImage,
					VK_ACCESS_NONE,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					aspectMask
				};
				commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			}

			/* Transition source to GENERAL for blit (compatible with any usage). */
			VkImageLayout srcOriginalLayout = currentLayout;
			VkAccessFlags srcAccessMask = VK_ACCESS_NONE;
			VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

			if ( currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || currentLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
			{
				srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else if ( currentLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
			{
				srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			}

			{
				const Sync::ImageMemoryBarrier barrier{
					sourceImage,
					srcAccessMask,
					VK_ACCESS_TRANSFER_READ_BIT,
					currentLayout,
					VK_IMAGE_LAYOUT_GENERAL,
					aspectMask
				};
				commandBuffer->pipelineBarrier(barrier, sourceStage, VK_PIPELINE_STAGE_TRANSFER_BIT);
			}

			/* Blit from source to intermediate. */
			VkImageBlit blitRegion{};
			blitRegion.srcSubresource.aspectMask = aspectMask;
			blitRegion.srcSubresource.mipLevel = 0;
			blitRegion.srcSubresource.baseArrayLayer = 0;
			blitRegion.srcSubresource.layerCount = 1;
			blitRegion.srcOffsets[0] = {0, 0, 0};
			blitRegion.srcOffsets[1] = {static_cast< int32_t >(extent.width), static_cast< int32_t >(extent.height), static_cast< int32_t >(extent.depth)};
			blitRegion.dstSubresource.aspectMask = aspectMask;
			blitRegion.dstSubresource.mipLevel = 0;
			blitRegion.dstSubresource.baseArrayLayer = 0;
			blitRegion.dstSubresource.layerCount = 1;
			blitRegion.dstOffsets[0] = {0, 0, 0};
			blitRegion.dstOffsets[1] = {static_cast< int32_t >(extent.width), static_cast< int32_t >(extent.height), static_cast< int32_t >(extent.depth)};

			vkCmdBlitImage(
				commandBuffer->handle(),
				sourceImage.handle(),
				VK_IMAGE_LAYOUT_GENERAL,
				intermediateImage->handle(),
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&blitRegion,
				VK_FILTER_NEAREST
			);

			/* Transition source back to original layout. */
			{
				VkAccessFlags dstAccessMask = VK_ACCESS_NONE;
				VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

				if ( srcOriginalLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || srcOriginalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
				{
					dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				}
				else if ( srcOriginalLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
				{
					dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				}

				const Sync::ImageMemoryBarrier barrier{
					sourceImage,
					VK_ACCESS_TRANSFER_READ_BIT,
					dstAccessMask,
					VK_IMAGE_LAYOUT_GENERAL,
					srcOriginalLayout,
					aspectMask
				};
				commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationStage);
			}

			/* Transition intermediate to TRANSFER_SRC_OPTIMAL. */
			{
				const Sync::ImageMemoryBarrier barrier{
					*intermediateImage,
					VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					aspectMask
				};
				commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			}

			finalSourceImage = intermediateImage->handle();
			finalSourceLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		}
		else
		{
			/* Source has TRANSFER_SRC_BIT, transition directly to TRANSFER_SRC_OPTIMAL. */
			if ( currentLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
			{
				VkAccessFlags srcAccessMask = VK_ACCESS_NONE;
				VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

				if ( currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || currentLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
				{
					srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
					sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
				}
				else if ( currentLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
				{
					srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
					sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
				}
				else if ( currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
				{
					srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
					sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
				}

				const Sync::ImageMemoryBarrier barrier{
					sourceImage,
					srcAccessMask,
					VK_ACCESS_TRANSFER_READ_BIT,
					currentLayout,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					aspectMask
				};
				commandBuffer->pipelineBarrier(barrier, sourceStage, VK_PIPELINE_STAGE_TRANSFER_BIT);
			}
		}

		/* Copy final source (either original or intermediate) to staging buffer. */
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = aspectMask;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {0, 0, 0};
		region.imageExtent = extent;

		vkCmdCopyImageToBuffer(
			commandBuffer->handle(),
			finalSourceImage,
			finalSourceLayout,
			stagingBuffer->handle(),
			1,
			&region
		);

		/* Transition source back to original layout if we modified it directly. */
		if ( sourceHasTransferSrc && currentLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL )
		{
			VkAccessFlags dstAccessMask = VK_ACCESS_NONE;
			VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

			if ( currentLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL || currentLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR )
			{
				dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			else if ( currentLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL )
			{
				dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			}
			else if ( currentLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
			{
				dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			}

			const Sync::ImageMemoryBarrier barrier{
				sourceImage,
				VK_ACCESS_TRANSFER_READ_BIT,
				dstAccessMask,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				currentLayout,
				aspectMask
			};
			commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, destinationStage);
		}

		if ( !commandBuffer->end() )
		{
			TraceError{ClassId} << "Unable to end image download command buffer !";

			return false;
		}

		// Create fence for synchronization
		auto downloadFence = std::make_unique< Sync::Fence >(m_device, 0);
		downloadFence->setIdentifier(ClassId, "ImageDownload", "Fence");

		if ( !downloadFence->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create image download fence !";

			return false;
		}

		/* Submit and wait. */
		const auto * queue = m_device->getGraphicsQueue(QueuePriority::High);

		if ( !queue->submit(*commandBuffer, SynchInfo{}.withFence(downloadFence->handle())) )
		{
			TraceError{ClassId} << "Unable to submit image download command buffer !";

			return false;
		}

		if ( !downloadFence->wait() )
		{
			TraceError{ClassId} << "Unable to wait for image download fence !";

			return false;
		}

		/* Map staging buffer and copy to pixmap. */
		auto * pointer = stagingBuffer->mapMemoryAs< uint8_t >();

		if ( pointer == nullptr )
		{
			return false;
		}

		/* Copy data from staging buffer to pixmap. */
		if ( !pixmap.initialize(extent.width, extent.height, channelMode, {pointer, requiredBytes}) )
		{
			return false;
		}

		stagingBuffer->unmapMemory();

		return true;
	}

	BufferTransferOperation *
	TransferManager::getAndReserveBufferTransferOperation (size_t requiredBytes) noexcept
	{
		BufferTransferOperation * operation = nullptr;

		/* Try to get an existing one with the right size... */
		for ( auto & transferOperation : m_bufferTransferOperations )
		{
			if ( !transferOperation.isAvailable() )
			{
				continue;
			}

			if ( requiredBytes <= transferOperation.bytes() )
			{
				//TraceDebug{ClassId} << "Re-use a buffer transfer operation of " << transferOperation.bytes() << " bytes for " << requiredBytes << " bytes! (A)";

				operation = &transferOperation;

				break;
			}
		}

		/* Try to get a free one for reallocation. */
		if ( operation == nullptr )
		{
			for ( auto & transferOperation : m_bufferTransferOperations )
			{
				if ( !transferOperation.isAvailable() )
				{
					continue;
				}

				if ( requiredBytes > transferOperation.bytes() )
				{
					//TraceDebug{ClassId} << "Resizing buffer transfer operation (" << transferOperation.bytes() << " bytes -> " << requiredBytes << " bytes) ...";

					if ( !transferOperation.expanseStagingBufferCapacityTo(requiredBytes) )
					{
						continue;
					}
				}
				/*else
				{
					TraceDebug{ClassId} << "Re-use a buffer transfer operation of " << transferOperation.bytes() << " bytes for " << requiredBytes << " bytes! (B)";
				}*/

				operation = &transferOperation;

				break;
			}
		}

		/* ... Or create a new one. */
		if ( operation == nullptr )
		{
			//TraceDebug{ClassId} << "Creating a new buffer transfer operation of " << requiredBytes << " bytes ...";

			auto & transferOperation = m_bufferTransferOperations.emplace_back();

			if ( !transferOperation.createOnHardware(m_transferCommandPool, requiredBytes) )
			{
				TraceError{ClassId} << "Unable to create a new staging buffer of " << requiredBytes << " bytes !";

				return nullptr;
			}

			operation = &transferOperation;
		}

		if ( !operation->setRequestedForTransfer() )
		{
			TraceError{ClassId} << "Unable to get an available buffer transfer operation !";

			return nullptr;
		}

		return operation;
	}

	ImageTransferOperation *
	TransferManager::getAndReserveImageTransferOperation (size_t requiredBytes) noexcept
	{
		ImageTransferOperation * operation = nullptr;

		/* Try to get an existing one with the right size... */
		for ( auto & transferOperation : m_imageTransferOperations )
		{
			if ( !transferOperation.isAvailable() )
			{
				continue;
			}

			if ( requiredBytes <= transferOperation.bytes() )
			{
				//TraceDebug{ClassId} << "Re-use an image transfer operation of " << transferOperation.bytes() << " bytes for " << requiredBytes << " bytes! (A)";

				operation = &transferOperation;

				break;
			}
		}

		/* Try to get a free one for reallocation. */
		if ( operation == nullptr )
		{
			for ( auto & transferOperation : m_imageTransferOperations )
			{
				if ( !transferOperation.isAvailable() )
				{
					continue;
				}

				if ( requiredBytes > transferOperation.bytes() )
				{
					//TraceDebug{ClassId} << "Resizing image transfer operation (" << transferOperation.bytes() << " bytes -> " << requiredBytes << " bytes) ...";

					if ( !transferOperation.expanseStagingBufferCapacityTo(requiredBytes) )
					{
						continue;
					}
				}
				/*else
				{
					TraceDebug{ClassId} << "Re-use an image transfer operation of " << transferOperation.bytes() << " bytes for " << requiredBytes << " bytes! (B)";
				}*/

				operation = &transferOperation;

				break;
			}
		}

		/* ... Or create a new one. */
		if ( operation == nullptr )
		{
			//TraceDebug{ClassId} << "Creating an image buffer transfer operation of " << requiredBytes << " bytes ...";

			auto & transferOperation = m_imageTransferOperations.emplace_back();

			if ( !transferOperation.createOnHardware(m_transferCommandPool, m_graphicsCommandPool, requiredBytes) )
			{
				TraceError{ClassId} << "Unable to create a new staging buffer of " << requiredBytes << " bytes !";

				return nullptr;
			}

			operation = &transferOperation;
		}

		if ( !operation->setRequestedForTransfer() )
		{
			TraceError{ClassId} << "Unable to get an available image transfer operation !";

			return nullptr;
		}

		return operation;
	}
}
