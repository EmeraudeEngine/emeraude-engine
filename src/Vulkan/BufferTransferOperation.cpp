/*
 * src/Vulkan/BufferTransferOperation.cpp
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

#include "BufferTransferOperation.hpp"

namespace EmEn::Vulkan
{
	bool
	BufferTransferOperation::createOnHardware (const std::shared_ptr< CommandPool > & commandPool, size_t initialReservedBytes) noexcept
	{
		auto device = commandPool->device();

		/* Create the staging buffer. */
		m_stagingBuffer = std::make_unique< Buffer >(device, 0, static_cast< VkDeviceSize >(initialReservedBytes), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);
		m_stagingBuffer->setIdentifier(ClassId, "StagingBuffer", "Buffer");

		if ( !m_stagingBuffer->createOnHardware() )
		{
			Tracer::error(ClassId, "Unable to create the stage buffer!");

			return false;
		}

		/* Create command buffers. */
		m_transferCommandBuffer = std::make_unique< CommandBuffer >(commandPool, true);
		m_transferCommandBuffer->setIdentifier(ClassId, "BufferTransfer", "CommandBuffer");

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
		m_stagingBuffer.reset();
		m_transferCommandBuffer.reset();
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

		if ( !m_transferCommandBuffer->end() )
		{
			return false;
		}

		/* NOTE: Get a pure transfer queue or the transfer queue for graphics. */
		const auto * queue = device->getGraphicsTransferQueue(QueuePriority::High);

		if ( !queue->submit(*m_transferCommandBuffer, SynchInfo{}.withFence(m_operationFence->handle())) )
		{
			return false;
		}

		return true;
	}
}
