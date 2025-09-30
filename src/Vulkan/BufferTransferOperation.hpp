/*
 * src/Vulkan/BufferTransferOperation.hpp
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

#pragma once

/* Local inclusions for usages. */
#include "Sync/Fence.hpp"
#include "Buffer.hpp"
#include "CommandBuffer.hpp"

namespace EmEn::Vulkan
{
	/**
	 * @brief This class is responsible for sending a buffer on the GPU.
	 */
	class BufferTransferOperation final
	{
		public:

			/** @brief Class identifier. */
			static constexpr auto ClassId{"VulkanBufferTransferOperation"};

			/**
			 * @brief Constructs a buffer transfer operation using one command pool.
			 */
			BufferTransferOperation () noexcept = default;

			/**
			 * @brief Copy constructor.
			 * @param copy A reference to the copied instance.
			 */
			BufferTransferOperation (const BufferTransferOperation & copy) noexcept = delete;

			/**
			 * @brief Move constructor.
			 * @param copy A reference to the copied instance.
			 */
			BufferTransferOperation (BufferTransferOperation && copy) noexcept = default;

			/**
			 * @brief Copy assignment.
			 * @param copy A reference to the copied instance.
			 * @return BufferTransferOperation &
			 */
			BufferTransferOperation & operator= (const BufferTransferOperation & copy) noexcept = delete;

			/**
			 * @brief Move assignment.
			 * @param copy A reference to the copied instance.
			 * @return BufferTransferOperation &
			 */
			BufferTransferOperation & operator= (BufferTransferOperation && copy) noexcept = default;

			/**
			 * @brief Destructs the buffer transfer operation.
			 */
			~BufferTransferOperation ()
			{
				this->destroyFromHardware();
			}

			/**
			 * @brief Creates the staging buffer and synchronization primitives on the device.
			 * @param commandPool A reference to the command pool smart-point for transfer.
			 * @param initialReservedBytes The reserved bytes for the initial staging buffer.
			 * @return bool
			 */
			[[nodiscard]]
			bool createOnHardware (const std::shared_ptr< CommandPool > & commandPool, size_t initialReservedBytes) noexcept;

			/**
			 * @brief Destroys the staging buffer and synchronization primitives from the device.
			 * @return void
			 */
			void destroyFromHardware () noexcept;

			/**
			 * @brief Transfers a buffer from the CPU to the GPU.
			 * @param device A reference to the device smart-pointer.
			 * @param dstBuffer A reference to the destination buffer (GPU side).
			 * @param offset The offset in the staging buffer where the date to copy starts. Default 0.
			 * @return bool
			 */
			[[nodiscard]]
			bool transfer (const std::shared_ptr< Device > & device, Buffer & dstBuffer, VkDeviceSize offset = 0) const noexcept;

			/**
			 * @brief Returns whether the buffer transfer operation is valid for usage.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isCreated () const
			{
				if ( m_stagingBuffer == nullptr )
				{
					return false;
				}

				if ( m_transferCommandBuffer == nullptr )
				{
					return false;
				}

				if ( m_operationFence == nullptr )
				{
					return false;
				}

				return true;
			}

			/**
			 * @brief Returns if this transfer operation is available for a new transfer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			isAvailable () const noexcept
			{
				if ( m_operationFence == nullptr )
				{
					return false;
				}

				return m_operationFence->getStatus() == Sync::FenceStatus::Ready;
			}

			/**
			 * @brie Declares the operation in use for a new transfer.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			setRequestedForTransfer () const noexcept
			{
				if ( m_operationFence == nullptr )
				{
					return false;
				}

				return m_operationFence->reset();
			}

			/**
			 * @brief Returns the access of the staging buffer to write data.
			 * @return Buffer *
			 */
			[[nodiscard]]
			Buffer *
			stagingBuffer () const noexcept
			{
				return m_stagingBuffer.get();
			}

			/**
			 * @brief Returns the staging buffer capacity.
			 * @return size_t
			 */
			[[nodiscard]]
			size_t
			bytes () const noexcept
			{
				if ( m_stagingBuffer == nullptr )
				{
					return 0;
				}

				return m_stagingBuffer->bytes();
			}

			/**
			 * @brief Resizes the staging buffer to a new capacity.
			 * @param bytes New bytes size.
			 * @return bool
			 */
			[[nodiscard]]
			bool
			expanseStagingBufferCapacityTo (size_t bytes) const noexcept
			{
				if ( m_stagingBuffer == nullptr )
				{
					return false;
				}

				return m_stagingBuffer->recreateOnHardware(bytes);
			}

		private:

			std::unique_ptr< Buffer > m_stagingBuffer;
			std::unique_ptr< CommandBuffer > m_transferCommandBuffer;
			std::unique_ptr< Sync::Fence > m_operationFence;
	};
}
