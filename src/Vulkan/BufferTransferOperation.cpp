/*
 * src/Vulkan/BufferTransferOperation.cpp
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

#include "BufferTransferOperation.hpp"

/* Project configuration. */
#include "emeraude_config.hpp"

namespace EmEn::Vulkan
{
	bool
	BufferTransferOperation::createOnHardware (const std::shared_ptr< CommandPool > & transferCommandPool, const std::shared_ptr< CommandPool > & graphicsCommandPool, size_t initialReservedBytes) noexcept
	{
		auto device = transferCommandPool->device();

		/* Create the staging buffer. */
		m_stagingBuffer = std::make_unique< Buffer >(device, 0, static_cast< VkDeviceSize >(initialReservedBytes), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
		m_stagingBuffer->setIdentifier(ClassId, "StagingBuffer", "Buffer");

		if ( !m_stagingBuffer->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the stage buffer!");

			return false;
		}

		/* Create command buffers. */
		m_transferCommandBuffer = std::make_unique< CommandBuffer >(transferCommandPool, true);
		m_transferCommandBuffer->setIdentifier(ClassId, "BufferTransfer", "CommandBuffer");

		/* Queue family ownership transfer: the release recorded on the transfer queue is paired
		 * with its acquire on the graphics queue, HERE, in the same operation. A release is a
		 * ONE-SHOT token — leaving the acquire to "whatever reads the buffer first on the
		 * graphics queue" meant the second reader acquired into the void (VUID-vkQueueSubmit-
		 * pSubmits-02207, two instances of a skeletal mesh building their own BLAS from the same
		 * vertex buffer). Pairing it here makes the invariant statable: once uploaded, a buffer
		 * belongs to the GRAPHICS family, period.
		 * NOTE: On a single-queue-family device (Device::hasBasicSupport()) there is no graphics
		 * command pool AND no ownership transfer to perform — both conditions travel together. */
		if ( graphicsCommandPool != nullptr && device->getGraphicsTransferFamilyIndex() != device->getGraphicsFamilyIndex() )
		{
			m_graphicsCommandBuffer = std::make_unique< CommandBuffer >(graphicsCommandPool, true);
			m_graphicsCommandBuffer->setIdentifier(ClassId, "BufferOwnershipAcquire", "CommandBuffer");

			m_semaphore = std::make_unique< Sync::Semaphore >(device);
			m_semaphore->setIdentifier(ClassId, "BufferTransferSemaphore", "Semaphore");

			if ( !m_semaphore->createOnHardware() )
			{
				Tracer::error(ClassId, "Unable to create the buffer transfer semaphore!");

				return false;
			}
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

		return true;
	}

	void
	BufferTransferOperation::destroyFromHardware () noexcept
	{
		m_semaphore.reset();
		m_stagingBuffer.reset();
		m_transferCommandBuffer.reset();
		m_graphicsCommandBuffer.reset();
		m_operationFence.reset();
	}

	bool
	BufferTransferOperation::transfer (const std::shared_ptr< Device > & device, Buffer & dstBuffer, VkDeviceSize offset) const noexcept
	{
		if constexpr ( IsDebug )
		{
			if ( const auto endCopyOffset = offset + dstBuffer.bytes(); endCopyOffset > m_stagingBuffer->bytes() )
			{
				const auto overflow = endCopyOffset - m_stagingBuffer->bytes();

				TraceError{ClassId} <<
					"Source buffer overflow with " << overflow << " bytes !" "\n"
					"(offset:" << offset << " + length:" << dstBuffer.bytes() << ") > srcBuffer:" << m_stagingBuffer->bytes();

				return false;
			}

			if ( !m_transferCommandBuffer->isCreated() )
			{
				Tracer::error(ClassId, "The transfer command buffer is not created!");

				return false;
			}
		}

		if ( !m_transferCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		m_transferCommandBuffer->copy(*m_stagingBuffer, dstBuffer, offset, 0, dstBuffer.bytes());

		/* Single queue family, or no ownership to transfer: one submission, done. */
		if ( m_graphicsCommandBuffer == nullptr )
		{
			if ( !m_transferCommandBuffer->end() )
			{
				return false;
			}

			/* NOTE: Get a pure transfer queue or the transfer queue for graphics. */
			const auto * transferQueue = device->getGraphicsTransferQueue(QueuePriority::High);

			return transferQueue->submit(*m_transferCommandBuffer, SynchInfo{}.withFence(m_operationFence->handle()));
		}

		/* Two families: release the buffer on the transfer queue, acquire it on the graphics
		 * queue, both HERE. Same two-step shape as ImageTransferOperation (transfer submission
		 * signals the semaphore, graphics submission waits on it and signals the operation
		 * fence — so the operation is only reusable once BOTH command buffers are idle). */
		const auto srcFamily = device->getGraphicsTransferFamilyIndex();
		const auto dstFamily = device->getGraphicsFamilyIndex();

		VkBufferMemoryBarrier ownershipBarrier{};
		ownershipBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		ownershipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		ownershipBarrier.dstAccessMask = 0;
		ownershipBarrier.srcQueueFamilyIndex = srcFamily;
		ownershipBarrier.dstQueueFamilyIndex = dstFamily;
		ownershipBarrier.buffer = dstBuffer.handle();
		ownershipBarrier.offset = 0;
		ownershipBarrier.size = VK_WHOLE_SIZE;

		vkCmdPipelineBarrier(
			m_transferCommandBuffer->handle(),
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			0,
			0, nullptr,
			1, &ownershipBarrier,
			0, nullptr
		);

		if ( !m_transferCommandBuffer->end() )
		{
			return false;
		}

		if ( !m_graphicsCommandBuffer->begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) )
		{
			return false;
		}

		/* The acquire half MUST mirror the release: same buffer, same range, same families.
		 * The destination access mask covers every way the graphics family reads an uploaded
		 * buffer — vertex/index input, shaders, and acceleration structure builds. */
		ownershipBarrier.srcAccessMask = 0;
		ownershipBarrier.dstAccessMask =
			VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
			VK_ACCESS_INDEX_READ_BIT |
			VK_ACCESS_UNIFORM_READ_BIT |
			VK_ACCESS_SHADER_READ_BIT |
			VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

		vkCmdPipelineBarrier(
			m_graphicsCommandBuffer->handle(),
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_VERTEX_INPUT_BIT | VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT | VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
			0,
			0, nullptr,
			1, &ownershipBarrier,
			0, nullptr
		);

		if ( !m_graphicsCommandBuffer->end() )
		{
			return false;
		}

		VkSemaphore semaphoreHandle = m_semaphore->handle();

		{
			/* NOTE: Get a pure transfer queue or the transfer queue for graphics. */
			const auto * transferQueue = device->getGraphicsTransferQueue(QueuePriority::High);

			if ( !transferQueue->submit(*m_transferCommandBuffer, SynchInfo{}.signals({&semaphoreHandle, 1})) )
			{
				Tracer::error(ClassId, "Unable to transfer a buffer (1/2) !");

				return false;
			}
		}

		VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

		const auto * graphicsQueue = device->getGraphicsQueue(QueuePriority::High);

		if ( !graphicsQueue->submit(*m_graphicsCommandBuffer, SynchInfo{}.waits({&semaphoreHandle, 1}, {&waitStage, 1}).withFence(m_operationFence->handle())) )
		{
			Tracer::error(ClassId, "Unable to acquire the buffer ownership on the graphics queue (2/2) !");

			return false;
		}

		return true;
	}
}
