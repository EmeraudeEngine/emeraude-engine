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

/* STL inclusions. */
#include <sstream>

/* Local inclusions. */
#include "Sync/ImageMemoryBarrier.hpp"
#include "Sync/Semaphore.hpp"
#include "Device.hpp"
#include "Queue.hpp"
#include "CommandBuffer.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "StagingBuffer.hpp"

namespace EmEn::Vulkan
{
	using namespace Libs;

	bool
	TransferManager::onInitialize () noexcept
	{
		if ( m_device == nullptr || !m_device->isCreated() )
		{
			Tracer::error(ClassId, "No device set !");

			return false;
		}

		m_transferCommandPool = std::make_shared< CommandPool >(m_device, m_device->getTransferFamilyIndex(), true, false, false);
		m_transferCommandPool->setIdentifier(ClassId, "Transfer", "CommandPool");

		if ( !m_transferCommandPool->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the transfer command pool !");

			m_transferCommandPool.reset();

			return false;
		}

		/* FIXME: Check the idea beyond this with the new queue system. */

		/* Read the queue configuration from the device. */
		if ( !m_device->hasBasicSupport() )
		{
			m_separatedQueues = true;

			m_specificCommandPool = std::make_shared< CommandPool >(m_device, m_device->getGraphicsFamilyIndex(), true, false, false);
			m_specificCommandPool->setIdentifier(ClassId, "Specific", "CommandPool");

			if ( !m_specificCommandPool->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the specific command pool !");

				m_specificCommandPool.reset();

				return false;
			}
		}
		else
		{
			m_specificCommandPool = m_transferCommandPool;
		}

		m_serviceInitialized = true;

		return true;
	}

	bool
	TransferManager::onTerminate () noexcept
	{
		/* [VULKAN-CPU-SYNC] */
		//const std::lock_guard< Device > lock{*m_device};

		m_serviceInitialized = false;

		/* FIXME: Seems unnecessary */
		m_device->waitIdle("Destroying a transfer manager");

		m_specificCommandPool.reset();
		m_transferCommandPool.reset();

		m_stagingBuffers.clear();

		m_device.reset();

		return true;
	}

	std::string
	TransferManager::getStagingBuffersStatistics () const noexcept
	{
		std::stringstream output;

		output << "Allocated staging buffers :" "\n";

		for ( const auto & stagingBuffer : m_stagingBuffers )
		{
			output << " - Buffer @" << stagingBuffer.get() << " size " << stagingBuffer->bytes() << " bytes" "\n";
		}

		return output.str();
	}

	std::shared_ptr< StagingBuffer >
	TransferManager::getAndReserveStagingBuffer (size_t bytes) noexcept
	{
		TraceInfo{ClassId} << this->getStagingBuffersStatistics();

		/* Try to get an existing one with the right size... */
		for ( const auto & stagingBuffer : m_stagingBuffers )
		{
			if ( !stagingBuffer->isFree() )
			{
				continue;
			}

			if ( bytes <= stagingBuffer->bytes() )
			{
				return stagingBuffer;
			}
		}

		/* Try to get a free one for reallocation. */
		for ( const auto & stagingBuffer : m_stagingBuffers )
		{
			if ( !stagingBuffer->isFree() )
			{
				continue;
			}

			if ( bytes > stagingBuffer->bytes() )
			{
				if ( !stagingBuffer->recreateOnHardware(bytes) )
				{
					continue;
				}
			}

			return stagingBuffer;
		}

		/* ... Or create a new one. */
		auto stagingBuffer = std::make_shared< StagingBuffer >(m_device, bytes);
		stagingBuffer->setIdentifier(ClassId, (std::stringstream{} << m_stagingBuffers.size() << "Bytes").str(), "StagingBuffer");

		if ( !stagingBuffer->createOnHardware() )
		{
			TraceError{ClassId} << "Unable to create a new staging buffer of " << bytes << " bytes !";

			return nullptr;
		}

		m_stagingBuffers.emplace_back(stagingBuffer);

		return stagingBuffer;
	}

	bool
	TransferManager::transfer (const StagingBuffer & stagingBuffer, const Buffer & dstBuffer, VkDeviceSize offset) noexcept
	{
		const auto * fence = stagingBuffer.fence();

		if ( !fence->reset() )
		{
			return false;
		}

		if constexpr ( IsDebug )
		{
			const auto endCopyOffset = offset + dstBuffer.bytes();

			if ( endCopyOffset > stagingBuffer.bytes() )
			{
				const auto overflow = endCopyOffset - stagingBuffer.bytes();

				TraceError{ClassId} <<
					"Source buffer overflow with " << overflow << " bytes !" "\n"
					"(offset:" << offset << " + length:" << dstBuffer.bytes() << ") > srcBuffer:" << stagingBuffer.bytes();

				return false;
			}
		}

		const auto commandBuffer = std::make_shared< CommandBuffer >(m_transferCommandPool, true);
		commandBuffer->setIdentifier(ClassId, "ToBuffer", "CommandBuffer");

		if ( !commandBuffer->isCreated() )
		{
			Tracer::error(ClassId, "Unable to create a transfer command buffer !");

			return false;
		}

		if ( !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		commandBuffer->copy(stagingBuffer, dstBuffer, offset, 0, dstBuffer.bytes());

		if ( !commandBuffer->end() )
		{
			return false;
		}

		auto * queue = m_device->getQueue(QueueJob::Transfer, QueuePriority::Medium);

		if ( !queue->submit(commandBuffer, SynchInfo{}.withFence(fence->handle())) )
		{
			Tracer::error(ClassId, "Unable to transfer a buffer !");

			return false;
		}

		return true;
	}

	bool
	TransferManager::transfer (const StagingBuffer & stagingBuffer, Image & dstImage, VkDeviceSize /*offset*/) noexcept
	{
		const auto * fence = stagingBuffer.fence();

		if ( !fence->reset() )
		{
			return false;
		}

		Sync::Semaphore transferCompleteSemaphore{m_device};

		if ( !transferCompleteSemaphore.createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the semaphore for image transfer !");

			return false;
		}

		constexpr VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		const auto baseWidth = dstImage.createInfo().extent.width;
		const auto baseHeight = dstImage.createInfo().extent.height;
		const auto pixelBytes = dstImage.pixelBytes();
		const auto layerCount = dstImage.createInfo().arrayLayers;
		const auto mipLevelCount = dstImage.createInfo().mipLevels;

		/* NOTE: Work on the transfer queue. */
		{
			const auto commandBuffer = std::make_shared< CommandBuffer >(m_transferCommandPool, true);
			commandBuffer->setIdentifier(ClassId, "StagingBufferToImage", "CommandBuffer");

			if ( !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
			{
				return false;
			}

			/* Prepare the image layout to receive data. */
			{
				Sync::ImageMemoryBarrier barrier{
					dstImage,
					VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				};
				barrier.setIdentifier(ClassId, "BaseImage", "ImageMemoryBarrier");

				commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			}

			for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
			{
				const uint32_t layerOffset = layerIndex * (baseWidth * baseHeight * pixelBytes);

				VkBufferImageCopy bufferImageCopy{};
				bufferImageCopy.bufferOffset = layerOffset;
				bufferImageCopy.bufferRowLength = 0;
				bufferImageCopy.bufferImageHeight = 0;
				bufferImageCopy.imageSubresource.aspectMask = aspectMask;
				bufferImageCopy.imageSubresource.mipLevel = 0; /* NOTE: We copy only the first level. */
				bufferImageCopy.imageSubresource.baseArrayLayer = layerIndex;
				bufferImageCopy.imageSubresource.layerCount = 1;
				bufferImageCopy.imageOffset.x = 0;
				bufferImageCopy.imageOffset.y = 0;
				bufferImageCopy.imageOffset.z = 0;
				bufferImageCopy.imageExtent.width = baseWidth;
				bufferImageCopy.imageExtent.height = baseHeight;
				bufferImageCopy.imageExtent.depth = 1;

				vkCmdCopyBufferToImage(
					commandBuffer->handle(),
					stagingBuffer.handle(),
					dstImage.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1, &bufferImageCopy
				);
			}

			if ( mipLevelCount > 1 )
			{
				/* NOTE: Set the base image as a source for the next mip-map level. */
				Sync::ImageMemoryBarrier barrier{
					dstImage,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					aspectMask
				};
				barrier.setIdentifier(ClassId, "PrepareMipMapping", "ImageMemoryBarrier");

				commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
			}

			if ( commandBuffer->end() )
			{
				auto * queue = m_device->getQueue(QueueJob::Transfer, QueuePriority::Medium);

				VkSemaphore semaphoreHandle = transferCompleteSemaphore.handle();

				if ( !queue->submit(commandBuffer, SynchInfo{}.signals({&semaphoreHandle, 1})) )
				{
					Tracer::error(ClassId, "Unable to transfer an image (1/2) !");

					return false;
				}

				dstImage.setCurrentImageLayout(mipLevelCount > 1 ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
			}
			else
			{
				Tracer::error(ClassId, "Unable to finish the command buffer to transfer an image !");

				return false;
			}

			/* FIXME: This causes a VK_ERROR_DEVICE_LOST (smart-pointer gone) */
		}

		/* NOTE: Work on the graphics queue. */
		{
			const auto commandBuffer = std::make_shared< CommandBuffer >(m_specificCommandPool, true);
			commandBuffer->setIdentifier(ClassId, "PrepareImage", "CommandBuffer");

			if ( !commandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
			{
				return false;
			}

			if ( mipLevelCount > 1 )
			{
				for ( uint32_t layerIndex = 0; layerIndex < layerCount; layerIndex++ )
				{
					for ( uint32_t mipLevelIndex = 1; mipLevelIndex < mipLevelCount; mipLevelIndex++ )
					{
						VkImageBlit imageBlit{};

						// Source image, base level or previous mip-map level.
						imageBlit.srcSubresource.aspectMask = aspectMask;
						imageBlit.srcSubresource.mipLevel = mipLevelIndex - 1;
						imageBlit.srcSubresource.baseArrayLayer = layerIndex;
						imageBlit.srcSubresource.layerCount = layerCount;
						imageBlit.srcOffsets[1].x = static_cast< int32_t >(baseWidth >> (mipLevelIndex - 1));
						imageBlit.srcOffsets[1].y = static_cast< int32_t >(baseHeight >> (mipLevelIndex - 1));
						imageBlit.srcOffsets[1].z = 1;

						// Destination mip-map level.
						imageBlit.dstSubresource.aspectMask = aspectMask;
						imageBlit.dstSubresource.mipLevel = mipLevelIndex;
						imageBlit.dstSubresource.baseArrayLayer = layerIndex;
						imageBlit.dstSubresource.layerCount = layerCount;
						imageBlit.dstOffsets[1].x = static_cast< int32_t >(baseWidth >> mipLevelIndex);
						imageBlit.dstOffsets[1].y = static_cast< int32_t >(baseHeight >> mipLevelIndex);
						imageBlit.dstOffsets[1].z  = 1;

						{
							Sync::ImageMemoryBarrier barrier{
								dstImage,
								VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
								VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
								aspectMask
							};
							barrier.targetMipLevel(mipLevelIndex);
							barrier.targetLayer(layerIndex);
							barrier.setIdentifier(ClassId, "MipMapLevelBeforeBlit", "ImageMemoryBarrier");

							commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
						}

						vkCmdBlitImage(
							commandBuffer->handle(),
							dstImage.handle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
							dstImage.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							1, &imageBlit,
							VK_FILTER_LINEAR // VK_FILTER_CUBIC_EXT
						);

						{
							Sync::ImageMemoryBarrier barrier{
								dstImage,
								VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
								VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
								aspectMask
							};
							barrier.targetMipLevel(mipLevelIndex);
							barrier.targetLayer(layerIndex);
							barrier.setIdentifier(ClassId, "MipMapLevelAfterBlit", "ImageMemoryBarrier");

							commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
						}
					}
				}

				/* Prepare the image layout to be used by a fragment shader. */
				{
					/* Prepare the image layout to be used by a fragment shader. */
					Sync::ImageMemoryBarrier barrier{
						dstImage,
						VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
						VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
						aspectMask
					};
					barrier.setIdentifier(ClassId, "FinalImage", "ImageMemoryBarrier");

					commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
				}
			}
			else
			{
				/* Prepare the image layout to be used by a fragment shader. */
				Sync::ImageMemoryBarrier barrier{
					dstImage,
					VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					aspectMask
				};
				barrier.setIdentifier(ClassId, "FinalImage", "ImageMemoryBarrier");

				commandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}

			if ( commandBuffer->end() )
			{
				auto * queue = m_device->getQueue(QueueJob::GraphicsTransfer, QueuePriority::Medium);

				VkSemaphore semaphoreHandle = transferCompleteSemaphore.handle();
				VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

				if ( !queue->submit(commandBuffer, SynchInfo{}.waits({&semaphoreHandle, 1}, {&waitStage, 1}).withFence(fence->handle())) )
				{
					Tracer::error(ClassId, "Unable to transfer an image (2/2) !");

					return false;
				}

				dstImage.setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
			else
			{
				Tracer::error(ClassId, "Unable to finish the command buffer to finalize the image !");

				return false;
			}
		}

		return true;
	}
}
