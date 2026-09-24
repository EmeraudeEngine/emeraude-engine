/*
 * src/Vulkan/ImageTransferOperation.cpp
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

#include "ImageTransferOperation.hpp"

/* STL inclusions. */
#include <algorithm>

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

		/* ⚠️ A 3D image is ONE layer of `depth` slices: the copy covers the whole extent, depth
		 * included. It was `depth = 1` until the cloud shapes needed it — only the first slice of a
		 * 3D image reached the GPU, every other one stayed undefined, and a volume that keeps its
		 * first slice empty (a cloud's margin) sampled as nothing at all, with no error anywhere.
		 * A 2D image has a depth of 1, so its copy is unchanged. */
		const auto & extent = dstImage.createInfo().extent;

		for ( uint32_t layerIndex = 0; layerIndex < dstImage.createInfo().arrayLayers; layerIndex++ )
		{
			const uint32_t layerOffset = layerIndex * ( extent.width * extent.height * extent.depth * dstImage.pixelBytes());

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
			bufferImageCopy.imageExtent.width = extent.width;
			bufferImageCopy.imageExtent.height = extent.height;
			bufferImageCopy.imageExtent.depth = extent.depth;

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
			/* ⚠️ A mip extent never drops below 1 on ANY axis, and a 3D image halves its DEPTH too.
			 * This blit used to write `z = 1` on both ends: a 3D image got its first slice filtered
			 * and every other slice of every level left undefined — sampled through a mip filter,
			 * garbage. The `>>` without a floor was the same defect on a non-square 2D image, whose
			 * short axis reached 0 before the long one finished its chain. */
			const auto & extent = dstImage.createInfo().extent;

			const auto mipExtent = [] (uint32_t size, uint32_t level) {
				return static_cast< int32_t >(std::max(size >> level, 1U));
			};

			for ( uint32_t layerIndex = 0; layerIndex < dstImage.createInfo().arrayLayers; layerIndex++ )
			{
				for ( uint32_t mipLevelIndex = 1; mipLevelIndex < dstImage.createInfo().mipLevels; mipLevelIndex++ )
				{
					VkImageBlit imageBlit{};

					/* Source image, base level or previous mip-map level. */
					imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.srcSubresource.mipLevel = mipLevelIndex - 1;
					imageBlit.srcSubresource.baseArrayLayer = layerIndex;
					imageBlit.srcSubresource.layerCount = 1;
					imageBlit.srcOffsets[1].x = mipExtent(extent.width, mipLevelIndex - 1);
					imageBlit.srcOffsets[1].y = mipExtent(extent.height, mipLevelIndex - 1);
					imageBlit.srcOffsets[1].z = mipExtent(extent.depth, mipLevelIndex - 1);

					/* Destination mip-map level. */
					imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					imageBlit.dstSubresource.mipLevel = mipLevelIndex;
					imageBlit.dstSubresource.baseArrayLayer = layerIndex;
					imageBlit.dstSubresource.layerCount = 1;
					imageBlit.dstOffsets[1].x = mipExtent(extent.width, mipLevelIndex);
					imageBlit.dstOffsets[1].y = mipExtent(extent.height, mipLevelIndex);
					imageBlit.dstOffsets[1].z = mipExtent(extent.depth, mipLevelIndex);

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

	bool
	ImageTransferOperation::transferCompressed (const std::shared_ptr< Device > & device, Image & dstImage, const std::vector< VkBufferImageCopy > & mipRegions) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !m_transferCommandBuffer->isCreated() )
			{
				Tracer::error(ClassId, "The transfer command buffer is not created!");

				return false;
			}
		}

		/* Step 1: Upload all mip levels from staging buffer to image. */
		if ( !m_transferCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		{
			/* Transition entire image to TRANSFER_DST. */
			Sync::ImageMemoryBarrier barrier{
				dstImage,
				VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			};
			barrier.setIdentifier(ClassId, "CompressedPreTransfer", "ImageMemoryBarrier");

			m_transferCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		/* Copy all mip levels in one batch. */
		vkCmdCopyBufferToImage(
			m_transferCommandBuffer->handle(),
			m_stagingBuffer->handle(),
			dstImage.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast< uint32_t >(mipRegions.size()), mipRegions.data()
		);

		if ( m_transferCommandBuffer->end() )
		{
			const auto * queue = device->getGraphicsTransferQueue(QueuePriority::High);

			VkSemaphore semaphoreHandle = m_semaphore->handle();

			if ( !queue->submit(*m_transferCommandBuffer, SynchInfo{}.signals({&semaphoreHandle, 1})) )
			{
				Tracer::error(ClassId, "Unable to transfer compressed image (1/2) !");

				return false;
			}
		}
		else
		{
			Tracer::error(ClassId, "Unable to finish the command buffer for compressed image transfer !");

			return false;
		}

		/* Step 2: Transition to SHADER_READ_ONLY (no mipmap blit). */
		if ( !m_graphicsCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		{
			Sync::ImageMemoryBarrier barrier{
				dstImage,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			};
			barrier.setIdentifier(ClassId, "CompressedFinalImage", "ImageMemoryBarrier");

			m_graphicsCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		if ( m_graphicsCommandBuffer->end() )
		{
			auto * queue = device->getGraphicsQueue(QueuePriority::High);

			VkSemaphore semaphoreHandle = m_semaphore->handle();
			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			if ( !queue->submit(*m_graphicsCommandBuffer, SynchInfo{}.waits({&semaphoreHandle, 1}, {&waitStage, 1}).withFence(m_operationFence->handle())) )
			{
				Tracer::error(ClassId, "Unable to transfer compressed image (2/2) !");

				return false;
			}

			dstImage.setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		else
		{
			Tracer::error(ClassId, "Unable to finish the command buffer for compressed image finalization !");

			return false;
		}

		return true;
	}

	bool
	ImageTransferOperation::transferRegion (const std::shared_ptr< Device > & device, Image & dstImage, const VkBufferImageCopy & region) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( !m_transferCommandBuffer->isCreated() )
			{
				Tracer::error(ClassId, "The transfer command buffer is not created!");

				return false;
			}
		}

		/* NOTE: Preserving the pixels outside the region is only meaningful when there are pixels to
		 * preserve. Any other layout means the image has never been fully uploaded, so barriering
		 * from SHADER_READ_ONLY_OPTIMAL would be a lie to the driver. Refuse instead of guessing. */
		if ( dstImage.currentImageLayout() != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL )
		{
			Tracer::error(ClassId, "A partial image transfer requires a fully uploaded image (SHADER_READ_ONLY_OPTIMAL) !");

			return false;
		}

		/* Step 1: Copy the region into the image, keeping everything else. */
		if ( !m_transferCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		{
			Sync::ImageMemoryBarrier barrier{
				dstImage,
				VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			};
			barrier.setIdentifier(ClassId, "PartialPreTransfer", "ImageMemoryBarrier");

			m_transferCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}

		vkCmdCopyBufferToImage(
			m_transferCommandBuffer->handle(),
			m_stagingBuffer->handle(),
			dstImage.handle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region
		);

		if ( m_transferCommandBuffer->end() )
		{
			const auto * queue = device->getGraphicsTransferQueue(QueuePriority::High);

			VkSemaphore semaphoreHandle = m_semaphore->handle();

			if ( !queue->submit(*m_transferCommandBuffer, SynchInfo{}.signals({&semaphoreHandle, 1})) )
			{
				Tracer::error(ClassId, "Unable to transfer an image region (1/2) !");

				return false;
			}
		}
		else
		{
			Tracer::error(ClassId, "Unable to finish the command buffer for a partial image transfer !");

			return false;
		}

		/* Step 2: Give the image back to the fragment shader. */
		if ( !m_graphicsCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		{
			Sync::ImageMemoryBarrier barrier{
				dstImage,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_IMAGE_ASPECT_COLOR_BIT
			};
			barrier.setIdentifier(ClassId, "PartialFinalImage", "ImageMemoryBarrier");

			m_graphicsCommandBuffer->pipelineBarrier(barrier, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
		}

		if ( m_graphicsCommandBuffer->end() )
		{
			auto * queue = device->getGraphicsQueue(QueuePriority::High);

			VkSemaphore semaphoreHandle = m_semaphore->handle();
			VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

			if ( !queue->submit(*m_graphicsCommandBuffer, SynchInfo{}.waits({&semaphoreHandle, 1}, {&waitStage, 1}).withFence(m_operationFence->handle())) )
			{
				Tracer::error(ClassId, "Unable to transfer an image region (2/2) !");

				return false;
			}

			dstImage.setCurrentImageLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
		else
		{
			Tracer::error(ClassId, "Unable to finish the command buffer for a partial image finalization !");

			return false;
		}

		return true;
	}

}
