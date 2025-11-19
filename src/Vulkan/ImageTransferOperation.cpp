/*
 * src/Vulkan/ImageTransferOperation.cpp
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

#include "ImageTransferOperation.hpp"

/* Local inclusions. */
#include "Sync/ImageMemoryBarrier.hpp"

namespace EmEn::Vulkan
{
	bool
	ImageTransferOperation::createOnHardware (const std::shared_ptr< CommandPool > & transferCommandPool, const std::shared_ptr< CommandPool > & graphicsCommandPool, size_t initialReservedBytes) noexcept
	{
		auto device = transferCommandPool->device();

		/* Create the staging buffer. */
		m_stagingBuffer = std::make_unique< Buffer >(device, 0, static_cast< VkDeviceSize >(initialReservedBytes), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);
		m_stagingBuffer->setIdentifier(ClassId, "StagingBuffer", "Buffer");

		if ( !m_stagingBuffer->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the stage buffer!");

			return false;
		}

		/* Create command buffers. */
		m_transferCommandBuffer = std::make_unique< CommandBuffer >(transferCommandPool, true);
		m_transferCommandBuffer->setIdentifier(ClassId, "ImageTransfer", "CommandBuffer");

		if ( graphicsCommandPool == nullptr )
		{
			m_graphicsCommandBuffer = std::make_unique< CommandBuffer >(transferCommandPool, true);
			m_graphicsCommandBuffer->setIdentifier(ClassId, "GraphicsImageTransition", "CommandBuffer");
		}
		else
		{
			m_graphicsCommandBuffer = std::make_unique< CommandBuffer >(graphicsCommandPool, true);
			m_graphicsCommandBuffer->setIdentifier(ClassId, "GraphicsImageTransition", "CommandBuffer");
		}

		/* Create the operation fence.
		 * Here the fence controls the availability when choosing a transfer operation. */
		m_operationFence = std::make_unique< Sync::Fence >(device, VK_FENCE_CREATE_SIGNALED_BIT);
		m_operationFence->setIdentifier(ClassId, "OperationCompletion", "Fence");

		if ( !m_operationFence->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the transfer operation fence!");

			return false;
		}

		/* Create the operation semaphore. */
		m_semaphore = std::make_unique< Sync::Semaphore >(device);
		m_semaphore->setIdentifier(ClassId, "ImageTransferSemaphore", "Semaphore");

		if ( !m_semaphore->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the image transfer semaphore!");

			return false;
		}

		return true;
	}

	void
	ImageTransferOperation::destroyFromHardware () noexcept
	{
		m_semaphore.reset();
		m_stagingBuffer.reset();
		m_transferCommandBuffer.reset();
		m_graphicsCommandBuffer.reset();
		m_operationFence.reset();
	}

	bool
	ImageTransferOperation::transfer (const std::shared_ptr< Device > & device, Image & dstImage, size_t offset) const noexcept
	{
		if ( !this->transferToGPU(device, dstImage, offset) )
		{
			Tracer::error(ClassId, "The first step of image transfer failed!");

			return false;
		}

		if ( !this->finalizeForGPU(device, dstImage) )
		{
			return false;
		}

		return true;
	}

	bool
	ImageTransferOperation::transferToGPU (const std::shared_ptr< Device > & device, Image & dstImage, VkDeviceSize /*offset*/) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !m_transferCommandBuffer->isCreated() )
			{
				Tracer::error(ClassId, "The transfer command buffer is not created!");

				return false;
			}
		}

		/* NOTE: Work on the transfer queue. */
		if ( !m_transferCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
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

			m_transferCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		for ( uint32_t layerIndex = 0; layerIndex < dstImage.createInfo().arrayLayers; layerIndex++ )
		{
			const uint32_t layerOffset = layerIndex * ( dstImage.createInfo().extent.width * dstImage.createInfo().extent.height * dstImage.pixelBytes());

			VkBufferImageCopy bufferImageCopy{};
			bufferImageCopy.bufferOffset = layerOffset;
			bufferImageCopy.bufferRowLength = 0;
			bufferImageCopy.bufferImageHeight = 0;
			bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferImageCopy.imageSubresource.mipLevel = 0; /* NOTE: We copy only the first level. */
			bufferImageCopy.imageSubresource.baseArrayLayer = layerIndex;
			bufferImageCopy.imageSubresource.layerCount = 1;
			bufferImageCopy.imageOffset.x = 0;
			bufferImageCopy.imageOffset.y = 0;
			bufferImageCopy.imageOffset.z = 0;
			bufferImageCopy.imageExtent.width = dstImage.createInfo().extent.width;
			bufferImageCopy.imageExtent.height = dstImage.createInfo().extent.height;
			bufferImageCopy.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(
				m_transferCommandBuffer->handle(),
				m_stagingBuffer->handle(),
				dstImage.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &bufferImageCopy
			);
		}

		if ( dstImage.createInfo().mipLevels > 1 )
		{
			/* NOTE: Set the base image as a source for the next mip-map level. */
			Sync::ImageMemoryBarrier barrier{
				dstImage,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			};
			barrier.setIdentifier(ClassId, "PrepareMipMapping", "ImageMemoryBarrier");

			m_transferCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		if ( m_transferCommandBuffer->end() )
		{
			const auto * queue = device->getGraphicsTransferQueue(QueuePriority::High);

			VkSemaphore semaphoreHandle = m_semaphore->handle();

			if ( !queue->submit(*m_transferCommandBuffer, SynchInfo{}.signals({&semaphoreHandle, 1})) )
			{
				Tracer::error(ClassId, "Unable to transfer an image (1/2) !");

				return false;
			}

			dstImage.setCurrentImageLayout(dstImage.createInfo().mipLevels > 1 ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		}
		else
		{
			Tracer::error(ClassId, "Unable to finish the command buffer to transfer an image !");

			return false;
		}

		return true;
	}

	bool
	ImageTransferOperation::finalizeForGPU (const std::shared_ptr< Device > & device, Image & dstImage) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !m_graphicsCommandBuffer->isCreated() )
			{
				Tracer::error(ClassId, "The transfer command buffer is not created!");

				return false;
			}
		}

		if ( !m_graphicsCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		if ( dstImage.createInfo().mipLevels > 1 )
		{
			for ( uint32_t layerIndex = 0; layerIndex < dstImage.createInfo().arrayLayers; layerIndex++ )
			{
				for ( uint32_t mipLevelIndex = 1; mipLevelIndex < dstImage.createInfo().mipLevels; mipLevelIndex++ )
				{
					VkImageBlit imageBlit{};

					/* Source image, base level or previous mip-map level. */
					imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.srcSubresource.mipLevel = mipLevelIndex - 1;
					imageBlit.srcSubresource.baseArrayLayer = layerIndex;
					imageBlit.srcSubresource.layerCount = dstImage.createInfo().arrayLayers;
					imageBlit.srcOffsets[1].x = static_cast< int32_t >(dstImage.createInfo().extent.width >> (mipLevelIndex - 1));
					imageBlit.srcOffsets[1].y = static_cast< int32_t >(dstImage.createInfo().extent.height >> (mipLevelIndex - 1));
					imageBlit.srcOffsets[1].z = 1;

					/* Destination mip-map level. */
					imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.dstSubresource.mipLevel = mipLevelIndex;
					imageBlit.dstSubresource.baseArrayLayer = layerIndex;
					imageBlit.dstSubresource.layerCount = dstImage.createInfo().arrayLayers;
					imageBlit.dstOffsets[1].x = static_cast< int32_t >(dstImage.createInfo().extent.width >> mipLevelIndex);
					imageBlit.dstOffsets[1].y = static_cast< int32_t >(dstImage.createInfo().extent.height >> mipLevelIndex);
					imageBlit.dstOffsets[1].z  = 1;

					{
						Sync::ImageMemoryBarrier barrier{
							dstImage,
							VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
							VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
							VK_IMAGE_ASPECT_COLOR_BIT
						};
						barrier.targetMipLevel(mipLevelIndex);
						barrier.targetLayer(layerIndex);
						barrier.setIdentifier(ClassId, "MipMapLevelBeforeBlit", "ImageMemoryBarrier");

						m_graphicsCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
					}

					vkCmdBlitImage(
						m_graphicsCommandBuffer->handle(),
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
							VK_IMAGE_ASPECT_COLOR_BIT
						};
						barrier.targetMipLevel(mipLevelIndex);
						barrier.targetLayer(layerIndex);
						barrier.setIdentifier(ClassId, "MipMapLevelAfterBlit", "ImageMemoryBarrier");

						m_graphicsCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
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
					VK_IMAGE_ASPECT_COLOR_BIT
				};
				barrier.setIdentifier(ClassId, "FinalImage", "ImageMemoryBarrier");

				m_graphicsCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
			}
		}
		else
		{
			/* Prepare the image layout to be used by a fragment shader. */
			Sync::ImageMemoryBarrier barrier{
				dstImage,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			};
			barrier.setIdentifier(ClassId, "FinalImage", "ImageMemoryBarrier");

			m_graphicsCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		if ( m_graphicsCommandBuffer->end() )
		{
			auto * queue = device->getGraphicsQueue(QueuePriority::High);

			VkSemaphore semaphoreHandle = m_semaphore->handle();
			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			if ( !queue->submit(*m_graphicsCommandBuffer, SynchInfo{}.waits({&semaphoreHandle, 1}, {&waitStage, 1}).withFence(m_operationFence->handle())) )
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

		return true;
	}
}
