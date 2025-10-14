/*
 * src/Vulkan/TransferManager.cpp
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

#include "TransferManager.hpp"

/* Local inclusions. */
#include "Sync/ImageMemoryBarrier.hpp"
#include "CommandBuffer.hpp"
#include "Device.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

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
				TraceDebug{ClassId} << "Re-use a buffer transfer operation of " << transferOperation.bytes() << " bytes for " << requiredBytes << " bytes! (A)";

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
					TraceDebug{ClassId} << "Resizing buffer transfer operation (" << transferOperation.bytes() << " bytes -> " << requiredBytes << " bytes) ...";

					if ( !transferOperation.expanseStagingBufferCapacityTo(requiredBytes) )
					{
						continue;
					}
				}
				else
				{
					TraceDebug{ClassId} << "Re-use a buffer transfer operation of " << transferOperation.bytes() << " bytes for " << requiredBytes << " bytes! (B)";
				}

				operation = &transferOperation;

				break;
			}
		}

		/* ... Or create a new one. */
		if ( operation == nullptr )
		{
			TraceDebug{ClassId} << "Creating a new buffer transfer operation of " << requiredBytes << " bytes ...";

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
				TraceDebug{ClassId} << "Re-use an image transfer operation of " << transferOperation.bytes() << " bytes for " << requiredBytes << " bytes! (A)";

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
					TraceDebug{ClassId} << "Resizing image transfer operation (" << transferOperation.bytes() << " bytes -> " << requiredBytes << " bytes) ...";

					if ( !transferOperation.expanseStagingBufferCapacityTo(requiredBytes) )
					{
						continue;
					}
				}
				else
				{
					TraceDebug{ClassId} << "Re-use an image transfer operation of " << transferOperation.bytes() << " bytes for " << requiredBytes << " bytes! (B)";
				}

				operation = &transferOperation;

				break;
			}
		}

		/* ... Or create a new one. */
		if ( operation == nullptr )
		{
			TraceDebug{ClassId} << "Creating an image buffer transfer operation of " << requiredBytes << " bytes ...";

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
